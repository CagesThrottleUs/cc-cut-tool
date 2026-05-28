# SP-04: File Interface — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use `subagent-driven-development` (recommended) or `executing-plans` to implement task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add `mmap_threshold`, `next_line`, and three concrete `FileSource` implementations plus a factory so the processing pipeline can read stdin, small files, and large files transparently.

**Architecture:** `next_line` lives as a `protected static` on `FileSource` so all subclasses inherit it without duplication. `StdinSource` redirects `std::cin.rdbuf()` for testability. `BufferedFileSource` and `MmapFileSource` take a `std::filesystem::path` in their constructor. `make_file_source` is the only call site that creates concrete instances.

**Tech Stack:** C++23, Boost.Iostreams, GoogleTest, CMake 4.0, Ninja, Clang, ASan

**Spec:** `.ai/specs/2026-05-28-sp04-file-interface-design.md` (SPEC-4)

**Assumption (surface explicitly):** `std::cin.rdbuf(other_buf)` replaces the underlying buffer for `StdinSource::load()` tests — this is the standard in-process stdin redirect pattern and does not require OS-level dup2.

**File map:**
```
Modified:
  include/cut/file_source.hpp    ← add mmap_threshold + next_line
  CMakeLists.txt                 ← Boost::iostreams component + new sources

Created:
  include/cut/stdin_source.hpp
  include/cut/buffered_file_source.hpp
  include/cut/mmap_file_source.hpp
  include/cut/make_file_source.hpp
  src/stdin_source.cpp
  src/buffered_file_source.cpp
  src/mmap_file_source.cpp
  src/make_file_source.cpp
  tests/cut/file_interface_test.cpp
```

---

### Task 1: Update file_source.hpp — threshold + next_line

**Files:**
- Modify: `include/cut/file_source.hpp`

- [ ] **Step 1: Add mmap_threshold and next_line to file_source.hpp**

Read the current file first, then add after the existing `#include` block and before `namespace cc_cut {`:

```cpp
#pragma once
#include <cstddef>
#include <optional>
#include <string_view>

namespace cc_cut {

// spec_id: SPEC-4  req_id: REQ-001
inline constexpr std::size_t mmap_threshold = 100ULL * 1024ULL * 1024ULL;

// spec_id: SPEC-4  req_id: REQ-004
/// Abstract source for line-oriented reading of a single input.
/// ...existing class doc...
class FileSource {
public:
    FileSource()                                     = default;
    FileSource(const FileSource&)                    = delete;
    auto operator=(const FileSource&) -> FileSource& = delete;
    FileSource(FileSource&&)                         = delete;
    auto operator=(FileSource&&)      -> FileSource& = delete;

    // spec_id: SPEC-1  req_id: REQ-004
    virtual void load()                                   = 0;
    virtual auto getline() -> std::optional<std::string_view> = 0;
    virtual ~FileSource()                                 = default;

protected:
    // spec_id: SPEC-4  req_id: REQ-002
    /// Returns the next line from buffer without its trailing \n.
    /// \r before \n is included in the returned view — no CRLF stripping.
    /// Returns nullopt when cursor >= buffer.size().
    static auto next_line(std::string_view buffer, std::size_t& cursor)
        -> std::optional<std::string_view> {
      if (cursor >= buffer.size()) {
        return std::nullopt;
      }
      const auto pos = buffer.find('\n', cursor);
      if (pos == std::string_view::npos) {
        const auto result = buffer.substr(cursor);
        cursor = buffer.size();
        return result;
      }
      const auto result = buffer.substr(cursor, pos - cursor);
      cursor = pos + 1;
      return result;
    }
};

}  // namespace cc_cut
```

- [ ] **Step 2: Build — verify no new errors**

```bash
cmake --build --preset clang-debug 2>&1 | grep -E "error:|warning:" | head -5
```

Expected: no output (zero errors, zero warnings)

- [ ] **Step 3: Run existing tests — verify 86/86 still pass**

```bash
ctest --test-dir out/build/clang-debug --output-on-failure --exclude-regex "integ_"
```

Expected: `100% tests passed, 0 tests failed out of 86`

- [ ] **Step 4: Commit**

```bash
git add include/cut/file_source.hpp
git commit -m "feat(sp04): add mmap_threshold and next_line to FileSource"
```

Body (WHY — multi-file commit requires it? Single file here so body optional. But non-trivial):
```
next_line on the base class eliminates duplication across three
concrete implementations that all need the same \n-split logic.
mmap_threshold co-located with the interface that uses it so callers
and tests can reference it without including an unrelated config file.
```

---

### Task 2: CMakeLists wiring — Boost::iostreams + new source slots

**Files:**
- Modify: `CMakeLists.txt`
- Create: `src/stdin_source.cpp`, `src/buffered_file_source.cpp`,
          `src/mmap_file_source.cpp`, `src/make_file_source.cpp`,
          `tests/cut/file_interface_test.cpp` (all placeholders)

- [ ] **Step 1: Update find_package to include Boost iostreams component**

Replace:
```cmake
find_package(Boost REQUIRED)
```
with:
```cmake
find_package(Boost REQUIRED COMPONENTS iostreams)
```

- [ ] **Step 2: Add new .cpp files to SRC_FILES**

```cmake
set(SRC_FILES
  src/list_parser.cpp
  src/arg_parser.cpp
  src/stdin_source.cpp
  src/buffered_file_source.cpp
  src/mmap_file_source.cpp
  src/make_file_source.cpp
)
```

- [ ] **Step 3: Add test file to test executable sources**

```cmake
add_executable(${TEST_PROJECT_NAME}
  tests/cut/mode_test.cpp
  tests/cut/list_test.cpp
  tests/cut/options_test.cpp
  tests/cut/file_source_test.cpp
  tests/cut/list_parser_test.cpp
  tests/cut/arg_parser_test.cpp
  tests/cut/file_interface_test.cpp
  ${SRC_FILES}
)
```

- [ ] **Step 4: Link Boost::iostreams to both targets**

```cmake
target_link_libraries(${PROJECT_NAME} PRIVATE
  Boost::iostreams
)

target_link_libraries(${TEST_PROJECT_NAME} PRIVATE
  GTest::gtest_main
  Boost::iostreams
)
```

- [ ] **Step 5: Create placeholder files**

```bash
for f in stdin_source buffered_file_source mmap_file_source make_file_source; do
  printf '// placeholder\n' > src/${f}.cpp
done
printf '// placeholder\n' > tests/cut/file_interface_test.cpp
```

- [ ] **Step 6: Reconfigure and verify build**

```bash
rm -rf out/build/clang-debug && cmake --preset clang-debug 2>&1 | tail -3 && cmake --build --preset clang-debug 2>&1 | grep "error:" | head -5
```

Expected: configure succeeds; build may produce linker errors only if Boost not linked yet — confirm `Boost::iostreams` found.

- [ ] **Step 7: Commit**

```bash
git add CMakeLists.txt \
        src/stdin_source.cpp src/buffered_file_source.cpp \
        src/mmap_file_source.cpp src/make_file_source.cpp \
        tests/cut/file_interface_test.cpp
```

```
build(sp04): wire Boost::iostreams and new FileSource source slots

Boost.Iostreams was declared but not linked since SP-01 — linking it
now enables MmapFileSource without further CMake changes. Placeholder
source files satisfy CMake 4.0's configure-time source existence
requirement.
```

---

### Task 3: Write failing tests (RED)

**Files:**
- Modify: `tests/cut/file_interface_test.cpp`

- [ ] **Step 1: Write the complete test file**

```cpp
// spec_id: SPEC-4  validates_req: REQ-001,REQ-002,REQ-003,REQ-004,REQ-005,REQ-006,REQ-007
#include "cut/buffered_file_source.hpp"
#include "cut/file_source.hpp"
#include "cut/make_file_source.hpp"
#include "cut/mmap_file_source.hpp"
#include "cut/stdin_source.hpp"

#include <gtest/gtest.h>

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <type_traits>

using cc_cut::BufferedFileSource;
using cc_cut::FileSource;
using cc_cut::MmapFileSource;
using cc_cut::StdinSource;
using cc_cut::make_file_source;
using cc_cut::mmap_threshold;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
namespace {

// Write content to a temp file; return path. Caller must delete.
auto make_temp_file(const std::string& content) -> std::filesystem::path {
  const auto path = std::filesystem::temp_directory_path() /
                    std::filesystem::path{"sp04_test_XXXXXX"};
  // Use a deterministic name per test; tests run sequentially.
  const auto p = std::filesystem::temp_directory_path() / "sp04_test.tmp";
  std::ofstream ofs{p};
  ofs << content;
  return p;
}

}  // namespace

// ---------------------------------------------------------------------------
// REQ-001: mmap_threshold constant
// ---------------------------------------------------------------------------

// spec_id: SPEC-4  validates_req: REQ-001  tc: TC-REQ001-01
static_assert(mmap_threshold > 0,
              "TC-REQ001-01: mmap_threshold must be positive");

// spec_id: SPEC-4  validates_req: REQ-001  tc: TC-REQ001-02
static_assert(std::is_same_v<decltype(mmap_threshold), const std::size_t>,
              "TC-REQ001-02: mmap_threshold must be std::size_t");

// ---------------------------------------------------------------------------
// REQ-002: next_line logic (tested via StdinSource — inherits protected method)
// ---------------------------------------------------------------------------

// Helper subclass to expose next_line for direct testing
namespace {
struct NextLineTester : cc_cut::FileSource {
  void load() override {}
  auto getline() -> std::optional<std::string_view> override {
    return std::nullopt;
  }
  static auto test_next_line(std::string_view buf, std::size_t& cur)
      -> std::optional<std::string_view> {
    return next_line(buf, cur);
  }
};
}  // namespace

// spec_id: SPEC-4  validates_req: REQ-002  tc: TC-REQ002-01
TEST(NextLineTest, BasicSplit) {
  std::size_t cur = 0;
  auto result = NextLineTester::test_next_line("a\nb\n", cur);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, "a");
  EXPECT_EQ(cur, 2U);
}

// spec_id: SPEC-4  validates_req: REQ-002  tc: TC-REQ002-02
TEST(NextLineTest, SecondLine) {
  std::size_t cur = 2;
  auto result = NextLineTester::test_next_line("a\nb\n", cur);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, "b");
  EXPECT_EQ(cur, 4U);
}

// spec_id: SPEC-4  validates_req: REQ-002  tc: TC-REQ002-03
TEST(NextLineTest, AtEnd) {
  std::size_t cur = 4;
  auto result = NextLineTester::test_next_line("a\nb\n", cur);
  EXPECT_FALSE(result.has_value());
}

// spec_id: SPEC-4  validates_req: REQ-002  tc: TC-REQ002-04
TEST(NextLineTest, NoTrailingNewline) {
  std::size_t cur = 2;
  auto result = NextLineTester::test_next_line("a\nb", cur);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, "b");
  EXPECT_EQ(cur, 3U);
}

// spec_id: SPEC-4  validates_req: REQ-002  tc: TC-REQ002-05
TEST(NextLineTest, CrlfIncluded) {
  std::size_t cur = 0;
  auto result = NextLineTester::test_next_line("a\r\nb", cur);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, "a\r");
  EXPECT_EQ(cur, 3U);
}

// spec_id: SPEC-4  validates_req: REQ-002  tc: TC-REQ002-06
TEST(NextLineTest, EmptyBuffer) {
  std::size_t cur = 0;
  auto result = NextLineTester::test_next_line("", cur);
  EXPECT_FALSE(result.has_value());
}

// spec_id: SPEC-4  validates_req: REQ-002  tc: TC-REQ002-07
TEST(NextLineTest, BlankLine) {
  std::size_t cur = 0;
  auto result = NextLineTester::test_next_line("\n", cur);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, "");
  EXPECT_EQ(cur, 1U);
}

// ---------------------------------------------------------------------------
// REQ-003: StdinSource
// ---------------------------------------------------------------------------

// spec_id: SPEC-4  validates_req: REQ-003  tc: TC-REQ003-01
TEST(StdinSourceTest, TwoLinesWithTrailingNewline) {
  std::istringstream ss{"line1\nline2\n"};
  const auto old_buf = std::cin.rdbuf(ss.rdbuf());
  StdinSource src;
  src.load();
  std::cin.rdbuf(old_buf);
  EXPECT_EQ(src.getline(), "line1");
  EXPECT_EQ(src.getline(), "line2");
  EXPECT_FALSE(src.getline().has_value());
}

// spec_id: SPEC-4  validates_req: REQ-003  tc: TC-REQ003-02
TEST(StdinSourceTest, NoTrailingNewline) {
  std::istringstream ss{"line1\nline2"};
  const auto old_buf = std::cin.rdbuf(ss.rdbuf());
  StdinSource src;
  src.load();
  std::cin.rdbuf(old_buf);
  EXPECT_EQ(src.getline(), "line1");
  EXPECT_EQ(src.getline(), "line2");
  EXPECT_FALSE(src.getline().has_value());
}

// spec_id: SPEC-4  validates_req: REQ-003  tc: TC-REQ003-03
TEST(StdinSourceTest, EmptyStdin) {
  std::istringstream ss{""};
  const auto old_buf = std::cin.rdbuf(ss.rdbuf());
  StdinSource src;
  src.load();
  std::cin.rdbuf(old_buf);
  EXPECT_FALSE(src.getline().has_value());
}

// spec_id: SPEC-4  validates_req: REQ-003  tc: TC-REQ003-04
TEST(StdinSourceTest, CrlfPreserved) {
  std::istringstream ss{"a\r\nb"};
  const auto old_buf = std::cin.rdbuf(ss.rdbuf());
  StdinSource src;
  src.load();
  std::cin.rdbuf(old_buf);
  EXPECT_EQ(src.getline(), "a\r");
  EXPECT_EQ(src.getline(), "b");
}

// ---------------------------------------------------------------------------
// REQ-004: BufferedFileSource
// ---------------------------------------------------------------------------

// spec_id: SPEC-4  validates_req: REQ-004  tc: TC-REQ004-01
TEST(BufferedFileSourceTest, TwoLines) {
  const auto path = make_temp_file("a\nb\n");
  BufferedFileSource src{path};
  src.load();
  EXPECT_EQ(src.getline(), "a");
  EXPECT_EQ(src.getline(), "b");
  EXPECT_FALSE(src.getline().has_value());
  std::filesystem::remove(path);
}

// spec_id: SPEC-4  validates_req: REQ-004  tc: TC-REQ004-02
TEST(BufferedFileSourceTest, NoTrailingNewline) {
  const auto path = make_temp_file("a\nb");
  BufferedFileSource src{path};
  src.load();
  EXPECT_EQ(src.getline(), "a");
  EXPECT_EQ(src.getline(), "b");
  EXPECT_FALSE(src.getline().has_value());
  std::filesystem::remove(path);
}

// spec_id: SPEC-4  validates_req: REQ-004  tc: TC-REQ004-03
TEST(BufferedFileSourceTest, NonExistentThrows) {
  BufferedFileSource src{"/tmp/sp04_definitely_does_not_exist_12345.txt"};
  EXPECT_THROW(src.load(), std::ios_base::failure);
}

// spec_id: SPEC-4  validates_req: REQ-004  tc: TC-REQ004-04
TEST(BufferedFileSourceTest, EmptyFile) {
  const auto path = make_temp_file("");
  BufferedFileSource src{path};
  src.load();
  EXPECT_FALSE(src.getline().has_value());
  std::filesystem::remove(path);
}

// ---------------------------------------------------------------------------
// REQ-005: MmapFileSource
// ---------------------------------------------------------------------------

// spec_id: SPEC-4  validates_req: REQ-005  tc: TC-REQ005-01
TEST(MmapFileSourceTest, TwoLines) {
  const auto path = make_temp_file("a\nb\n");
  MmapFileSource src{path};
  src.load();
  EXPECT_EQ(src.getline(), "a");
  EXPECT_EQ(src.getline(), "b");
  EXPECT_FALSE(src.getline().has_value());
  std::filesystem::remove(path);
}

// spec_id: SPEC-4  validates_req: REQ-005  tc: TC-REQ005-02
TEST(MmapFileSourceTest, NoTrailingNewline) {
  const auto path = make_temp_file("a\nb");
  MmapFileSource src{path};
  src.load();
  EXPECT_EQ(src.getline(), "a");
  EXPECT_EQ(src.getline(), "b");
  EXPECT_FALSE(src.getline().has_value());
  std::filesystem::remove(path);
}

// spec_id: SPEC-4  validates_req: REQ-005  tc: TC-REQ005-03
TEST(MmapFileSourceTest, NonExistentThrows) {
  MmapFileSource src{"/tmp/sp04_definitely_does_not_exist_12345.txt"};
  EXPECT_THROW(src.load(), std::ios_base::failure);
}

// spec_id: SPEC-4  validates_req: REQ-005  tc: TC-REQ005-04
TEST(MmapFileSourceTest, EmptyFile) {
  const auto path = make_temp_file("");
  MmapFileSource src{path};
  src.load();
  EXPECT_FALSE(src.getline().has_value());
  std::filesystem::remove(path);
}

// ---------------------------------------------------------------------------
// REQ-006 + REQ-007: make_file_source factory
// ---------------------------------------------------------------------------

// spec_id: SPEC-4  validates_req: REQ-006  tc: TC-REQ006-01
TEST(MakeFileSourceTest, StdinPath) {
  auto result = make_file_source("-");
  ASSERT_TRUE(result.has_value());
  EXPECT_NE(result->get(), nullptr);
  EXPECT_NE(dynamic_cast<StdinSource*>(result->get()), nullptr);
}

// spec_id: SPEC-4  validates_req: REQ-006  tc: TC-REQ006-02
TEST(MakeFileSourceTest, SmallFilePath) {
  const auto path = make_temp_file("hello\n");
  auto result = make_file_source(path.string());
  ASSERT_TRUE(result.has_value());
  EXPECT_NE(dynamic_cast<BufferedFileSource*>(result->get()), nullptr);
  std::filesystem::remove(path);
}

// spec_id: SPEC-4  validates_req: REQ-006  tc: TC-REQ006-03
TEST(MakeFileSourceTest, NonExistentReturnsError) {
  auto result = make_file_source("/tmp/sp04_no_such_file_xyz.txt");
  ASSERT_FALSE(result.has_value());
  EXPECT_NE(result.error().find("/tmp/sp04_no_such_file_xyz.txt"),
            std::string::npos);
}

// spec_id: SPEC-4  validates_req: REQ-006  tc: TC-REQ006-04
TEST(MakeFileSourceTest, FactoryDoesNotCallLoad) {
  // Factory returns source in pre-load state — load() can be called after.
  std::istringstream ss{"hello\n"};
  const auto old_buf = std::cin.rdbuf(ss.rdbuf());
  auto result = make_file_source("-");
  ASSERT_TRUE(result.has_value());
  result->get()->load();  // must succeed (not double-loaded)
  std::cin.rdbuf(old_buf);
  EXPECT_EQ(result->get()->getline(), "hello");
}

// spec_id: SPEC-4  validates_req: REQ-007  tc: TC-REQ007-01
TEST(MakeFileSourceTest, ErrorStartsWithProgramName) {
  auto result = make_file_source("/no/such/file");
  ASSERT_FALSE(result.has_value());
  EXPECT_TRUE(result.error().starts_with("cc-cut-tool: "));
}

// spec_id: SPEC-4  validates_req: REQ-007  tc: TC-REQ007-02
TEST(MakeFileSourceTest, ErrorContainsPath) {
  auto result = make_file_source("/no/such/file");
  ASSERT_FALSE(result.has_value());
  EXPECT_NE(result.error().find("/no/such/file"), std::string::npos);
}

// spec_id: SPEC-4  validates_req: REQ-007  tc: TC-REQ007-03
TEST(MakeFileSourceTest, ErrorNoTrailingNewline) {
  auto result = make_file_source("/no/such/file");
  ASSERT_FALSE(result.has_value());
  EXPECT_NE(result.error().back(), '\n');
}
```

- [ ] **Step 2: Build — verify RED (missing headers)**

```bash
cmake --build --preset clang-debug 2>&1 | grep "fatal error" | head -3
```

Expected: `fatal error: 'cut/stdin_source.hpp' file not found`

- [ ] **Step 3: Commit test file**

```bash
git add tests/cut/file_interface_test.cpp
git commit -m "test(sp04): add file interface unit tests"
```

---

### Task 4: StdinSource — header + impl

**Files:**
- Create: `include/cut/stdin_source.hpp`
- Create: `src/stdin_source.cpp`

- [ ] **Step 1: Write stdin_source.hpp**

```cpp
// include/cut/stdin_source.hpp
#pragma once
#include "cut/file_source.hpp"

#include <optional>
#include <string>
#include <string_view>

namespace cc_cut {

// spec_id: SPEC-4  req_id: REQ-003
/// Reads all bytes from std::cin into memory on load().
/// getline() returns successive lines without their trailing \n.
class StdinSource : public FileSource {
 public:
  void load() override;
  auto getline() -> std::optional<std::string_view> override;

 private:
  std::string   buffer_;
  std::size_t   cursor_{0};
};

}  // namespace cc_cut
```

- [ ] **Step 2: Write stdin_source.cpp**

```cpp
// src/stdin_source.cpp
#include "cut/stdin_source.hpp"

#include <iostream>
#include <iterator>

namespace cc_cut {

void StdinSource::load() {
  buffer_.assign(std::istreambuf_iterator<char>{std::cin},
                 std::istreambuf_iterator<char>{});
  cursor_ = 0;
}

auto StdinSource::getline() -> std::optional<std::string_view> {
  return next_line(buffer_, cursor_);
}

}  // namespace cc_cut
```

- [ ] **Step 3: Build and run StdinSource tests**

```bash
cmake --build --preset clang-debug 2>&1 | grep "error:" | head -5 && \
ctest --test-dir out/build/clang-debug --output-on-failure -R StdinSource
```

Expected: `StdinSourceTest.*` tests pass (4 tests)

- [ ] **Step 4: Commit**

```bash
git add include/cut/stdin_source.hpp src/stdin_source.cpp
git commit -m "feat(sp04): add StdinSource implementation"
```

---

### Task 5: BufferedFileSource — header + impl

**Files:**
- Create: `include/cut/buffered_file_source.hpp`
- Create: `src/buffered_file_source.cpp`

- [ ] **Step 1: Write buffered_file_source.hpp**

```cpp
// include/cut/buffered_file_source.hpp
#pragma once
#include "cut/file_source.hpp"

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace cc_cut {

// spec_id: SPEC-4  req_id: REQ-004
/// Reads a file < mmap_threshold entirely into memory on load().
/// Throws std::ios_base::failure if the file cannot be opened or read.
class BufferedFileSource : public FileSource {
 public:
  explicit BufferedFileSource(std::filesystem::path path);
  void load() override;
  auto getline() -> std::optional<std::string_view> override;

 private:
  std::filesystem::path path_;
  std::string           buffer_;
  std::size_t           cursor_{0};
};

}  // namespace cc_cut
```

- [ ] **Step 2: Write buffered_file_source.cpp**

```cpp
// src/buffered_file_source.cpp
#include "cut/buffered_file_source.hpp"

#include <fstream>
#include <iterator>

namespace cc_cut {

BufferedFileSource::BufferedFileSource(std::filesystem::path path)
    : path_(std::move(path)) {}

void BufferedFileSource::load() {
  std::ifstream ifs;
  ifs.exceptions(std::ifstream::failbit | std::ifstream::badbit);
  ifs.open(path_);
  buffer_.assign(std::istreambuf_iterator<char>{ifs},
                 std::istreambuf_iterator<char>{});
  cursor_ = 0;
}

auto BufferedFileSource::getline() -> std::optional<std::string_view> {
  return next_line(buffer_, cursor_);
}

}  // namespace cc_cut
```

- [ ] **Step 3: Build and run BufferedFileSource tests**

```bash
cmake --build --preset clang-debug 2>&1 | grep "error:" | head -5 && \
ctest --test-dir out/build/clang-debug --output-on-failure -R BufferedFileSource
```

Expected: 4 `BufferedFileSourceTest.*` tests pass

- [ ] **Step 4: Commit**

```bash
git add include/cut/buffered_file_source.hpp src/buffered_file_source.cpp
git commit -m "feat(sp04): add BufferedFileSource implementation"
```

---

### Task 6: MmapFileSource — header + impl

**Files:**
- Create: `include/cut/mmap_file_source.hpp`
- Create: `src/mmap_file_source.cpp`

- [ ] **Step 1: Write mmap_file_source.hpp**

```cpp
// include/cut/mmap_file_source.hpp
#pragma once
#include "cut/file_source.hpp"

#include <boost/iostreams/device/mapped_file.hpp>
#include <filesystem>
#include <optional>
#include <string_view>

namespace cc_cut {

// spec_id: SPEC-4  req_id: REQ-005
/// Memory-maps a file >= mmap_threshold via Boost.Iostreams on load().
/// getline() returns zero-copy string_view slices into the mapped region.
/// Throws std::ios_base::failure if the file cannot be mapped.
class MmapFileSource : public FileSource {
 public:
  explicit MmapFileSource(std::filesystem::path path);
  void load() override;
  auto getline() -> std::optional<std::string_view> override;

 private:
  std::filesystem::path                    path_;
  boost::iostreams::mapped_file_source     map_;
  std::string_view                         buffer_;
  std::size_t                              cursor_{0};
};

}  // namespace cc_cut
```

- [ ] **Step 2: Write mmap_file_source.cpp**

```cpp
// src/mmap_file_source.cpp
#include "cut/mmap_file_source.hpp"

#include <ios>
#include <stdexcept>

namespace cc_cut {

MmapFileSource::MmapFileSource(std::filesystem::path path)
    : path_(std::move(path)) {}

void MmapFileSource::load() {
  // mmap of a zero-byte file is undefined on most OSes; short-circuit.
  if (std::filesystem::file_size(path_) == 0) {
    buffer_ = std::string_view{};
    cursor_ = 0;
    return;
  }
  try {
    map_.open(path_.string());
  } catch (const std::exception& e) {
    throw std::ios_base::failure(e.what());
  }
  if (!map_.is_open()) {
    throw std::ios_base::failure("failed to map: " + path_.string());
  }
  buffer_ = std::string_view{map_.data(), map_.size()};
  cursor_ = 0;
}

auto MmapFileSource::getline() -> std::optional<std::string_view> {
  return next_line(buffer_, cursor_);
}

}  // namespace cc_cut
```

- [ ] **Step 3: Build and run MmapFileSource tests**

```bash
cmake --build --preset clang-debug 2>&1 | grep "error:" | head -5 && \
ctest --test-dir out/build/clang-debug --output-on-failure -R MmapFileSource
```

Expected: 4 `MmapFileSourceTest.*` tests pass

- [ ] **Step 4: Commit**

```bash
git add include/cut/mmap_file_source.hpp src/mmap_file_source.cpp
git commit -m "feat(sp04): add MmapFileSource implementation"
```

---

### Task 7: make_file_source factory — header + impl

**Files:**
- Create: `include/cut/make_file_source.hpp`
- Create: `src/make_file_source.cpp`

- [ ] **Step 1: Write make_file_source.hpp**

```cpp
// include/cut/make_file_source.hpp
#pragma once
#include "cut/file_source.hpp"

#include <expected>
#include <memory>
#include <string>
#include <string_view>

namespace cc_cut {

// spec_id: SPEC-4  req_id: REQ-006,REQ-007
/// Creates a FileSource appropriate for path.
/// "-"  → StdinSource
/// file < mmap_threshold → BufferedFileSource
/// file >= mmap_threshold → MmapFileSource
/// Returns error string ("cc-cut-tool: <path>: <reason>") on failure.
/// Does NOT call load() — caller must call load() after receiving the source.
///
/// @param path  File path or "-" for stdin.
/// @return      Unique ptr to FileSource on success; error string on failure.
/// @throws      Never throws.
auto make_file_source(std::string_view path)
    -> std::expected<std::unique_ptr<FileSource>, std::string>;

}  // namespace cc_cut
```

- [ ] **Step 2: Write make_file_source.cpp**

```cpp
// src/make_file_source.cpp
#include "cut/make_file_source.hpp"

#include "cut/buffered_file_source.hpp"
#include "cut/config.hpp"
#include "cut/mmap_file_source.hpp"
#include "cut/stdin_source.hpp"

#include <expected>
#include <filesystem>
#include <format>
#include <memory>
#include <string>
#include <string_view>

namespace cc_cut {

auto make_file_source(std::string_view path)
    -> std::expected<std::unique_ptr<FileSource>, std::string> {
  if (path == "-") {
    return std::make_unique<StdinSource>();
  }
  try {
    const std::filesystem::path fpath{path};
    const auto size = std::filesystem::file_size(fpath);
    if (size >= mmap_threshold) {
      return std::make_unique<MmapFileSource>(fpath);
    }
    return std::make_unique<BufferedFileSource>(fpath);
  } catch (const std::filesystem::filesystem_error& ex) {
    return std::unexpected(std::format(
        "{}: {}: {}", config::program_name, path, ex.code().message()));
  }
}

}  // namespace cc_cut
```

- [ ] **Step 3: Build — expect success; run all tests**

```bash
cmake --build --preset clang-debug 2>&1 | grep -E "error:|warning:" | head -5 && \
ctest --test-dir out/build/clang-debug --output-on-failure --exclude-regex "integ_" 2>&1 | tail -4
```

Expected: zero errors, zero warnings; ALL tests pass (86 existing + new file_interface tests)

- [ ] **Step 4: Run clang-tidy on new files**

```bash
clang-tidy -p out/build/clang-debug \
  src/stdin_source.cpp \
  src/buffered_file_source.cpp \
  src/mmap_file_source.cpp \
  src/make_file_source.cpp 2>&1 | grep -E "error:|warning:" | head -10 && echo "TIDY DONE"
```

Expected: `TIDY DONE` with zero warnings

- [ ] **Step 5: Commit**

```bash
git add include/cut/make_file_source.hpp src/make_file_source.cpp
git commit -m "feat(sp04): add make_file_source factory"
```

Body:
```
No typed dispatch existed between file paths and FileSource
implementations — every caller would need to check size and choose
a class manually. Centralising the threshold check here ensures the
100 MiB boundary is enforced in exactly one place.
```

---

## Spec Coverage

| REQ | Task |
|-----|------|
| REQ-001 (mmap_threshold) | Task 1 + static_asserts in Task 3 |
| REQ-002 (next_line) | Task 1 + NextLineTester tests in Task 3 |
| REQ-003 (StdinSource) | Task 4 + StdinSource tests in Task 3 |
| REQ-004 (BufferedFileSource) | Task 5 + tests in Task 3 |
| REQ-005 (MmapFileSource) | Task 6 + tests in Task 3 |
| REQ-006 (make_file_source) | Task 7 + tests in Task 3 |
| REQ-007 (error format) | Task 7 + TC-REQ007-01..03 in Task 3 |
