# SP-01: Foundation — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use `subagent-driven-development` (recommended) or `executing-plans` to implement task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Create four type-definition headers and wire CMake so all SP-01 unit tests compile and pass.

**Architecture:** Headers-only; no logic. One test file per header (SRP). Dependency order: `mode.hpp` → `list.hpp` → `options.hpp` (depends on both). `file_source.hpp` has no inter-header deps. Each header follows: test commit → impl commit → docs commit.

**Tech Stack:** C++23, GoogleTest, CMake 4.0, Ninja, Clang

**Spec:** `.ai/specs/2026-05-28-sp01-foundation-design.md` (SPEC-1)

**File map:**
```
include/cut/
  mode.hpp          ← CutMode enum
  list.hpp          ← CutList struct
  options.hpp       ← CutOptions struct
  file_source.hpp   ← FileSource interface

tests/cut/
  mode_test.cpp
  list_test.cpp
  options_test.cpp
  file_source_test.cpp
```

**Commit order per header:** `test:` → `feat:` → `docs:` (never batched together)

---

### Task 1: Wire CMakeLists — Boost stub + test target skeleton

**Files:**
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Add `find_package(Boost REQUIRED)` under `# Find Packages`**

Replace:
```cmake
# ----------------------------- Find Packages -----------------------------

```
with:
```cmake
# ----------------------------- Find Packages -----------------------------

find_package(Boost REQUIRED)
```

- [ ] **Step 2: Replace test executable block with sources + GTest link**

Replace:
```cmake
add_executable(${TEST_PROJECT_NAME} 
  ${SRC_FILES}
)

target_include_directories(${TEST_PROJECT_NAME} PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)
```
with:
```cmake
add_executable(${TEST_PROJECT_NAME}
  tests/cut/mode_test.cpp
  tests/cut/list_test.cpp
  tests/cut/options_test.cpp
  tests/cut/file_source_test.cpp
  ${SRC_FILES}
)

target_include_directories(${TEST_PROJECT_NAME} PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(${TEST_PROJECT_NAME} PRIVATE
  GTest::gtest_main
)
```

- [ ] **Step 3: Create directories**

```bash
mkdir -p include/cut tests/cut
```

- [ ] **Step 4: Verify CMake configures**

```bash
cmake --preset clang-debug
```

Expected: exits 0. Build fails — test files don't exist yet. That is correct.

- [ ] **Step 5: Commit**

```bash
git add CMakeLists.txt
git commit -m "build(sp01): wire test target with GTest and Boost stub

Test executable wired with GTest::gtest_main and four test source
slots. Boost find_package declared — not linked until SP-04."
```

---

### Task 2: CutMode — test → impl → docs (REQ-001)

**Files:**
- Create: `tests/cut/mode_test.cpp`
- Create: `include/cut/mode.hpp`

- [ ] **Step 1: Write failing test**

```cpp
// tests/cut/mode_test.cpp
#include <gtest/gtest.h>
#include "cut/mode.hpp"

// TC-REQ001-01, TC-REQ001-02
static_assert(CutMode::BYTE != CutMode::CHARACTER);
static_assert(CutMode::CHARACTER != CutMode::FIELD);

// TC-REQ001-03
static_assert(sizeof(CutMode) == 1);

TEST(CutModeTest, DistinctValues) {
    EXPECT_NE(CutMode::BYTE, CutMode::CHARACTER);
    EXPECT_NE(CutMode::CHARACTER, CutMode::FIELD);
}
```

- [ ] **Step 2: Build — verify red**

```bash
cmake --build --preset clang-debug 2>&1 | head -5
```

Expected: error containing `'cut/mode.hpp' file not found`

- [ ] **Step 3: Commit test**

```bash
git add tests/cut/mode_test.cpp
git commit -m "test(sp01): add CutMode unit tests

TC-REQ001-01..03: static_asserts for distinct values and uint8_t
underlying type. Red — mode.hpp does not exist yet."
```

- [ ] **Step 4: Create mode.hpp (no docs yet)**

```cpp
#pragma once
#include <cstdint>

enum class CutMode : uint8_t { BYTE, CHARACTER, FIELD };
```

- [ ] **Step 5: Build and run — verify green**

```bash
cmake --build --preset clang-debug && \
  cd out/build/clang-debug && ctest --output-on-failure -R CutMode
```

Expected: `Test #1: CutModeTest.DistinctValues ... Passed`, `100% tests passed`

- [ ] **Step 6: Commit implementation**

```bash
git add include/cut/mode.hpp
git commit -m "feat(sp01): add CutMode enum

REQ-001: enum class CutMode : uint8_t {BYTE, CHARACTER, FIELD}."
```

- [ ] **Step 7: Add Doxygen documentation to mode.hpp**

Replace the contents of `include/cut/mode.hpp` with:

```cpp
#pragma once
#include <cstdint>

/// Operating mode for the cut tool, selected by the first CLI flag.
///
/// Exactly one mode is active per invocation. Modes are mutually exclusive.
///
/// @code
///   CutMode m = CutMode::FIELD;  // selected by -f
/// @endcode
enum class CutMode : uint8_t {
    BYTE,       ///< Byte-position mode (-b). Selects raw bytes from each line.
    CHARACTER,  ///< UTF-8 codepoint mode (-c). Selects Unicode characters.
    FIELD       ///< Field mode (-f). Splits lines on a delimiter character.
};
```

- [ ] **Step 8: Build — verify docs compile clean**

```bash
cmake --build --preset clang-debug
```

Expected: exits 0, zero warnings

- [ ] **Step 9: Commit docs**

```bash
git add include/cut/mode.hpp
git commit -m "docs(sp01): document CutMode public interface

Doxygen comment on enum and each enumerator explaining CLI flag
mapping and mutual-exclusion invariant."
```

---

### Task 3: CutList — test → impl → docs (REQ-002)

**Files:**
- Create: `tests/cut/list_test.cpp`
- Create: `include/cut/list.hpp`

- [ ] **Step 1: Write failing test**

```cpp
// tests/cut/list_test.cpp
#include <gtest/gtest.h>
#include "cut/list.hpp"

// TC-REQ002-01
TEST(CutListTest, DefaultInit) {
    CutList list;
    EXPECT_EQ(list.open_from, std::nullopt);
    EXPECT_TRUE(list.indices.empty());
}

// TC-REQ002-02
TEST(CutListTest, SetOpenFrom) {
    CutList list;
    list.open_from = 2;
    EXPECT_TRUE(list.open_from.has_value());
    EXPECT_EQ(list.open_from.value(), 2);
}

// TC-REQ002-03
TEST(CutListTest, InsertIndices) {
    CutList list;
    list.indices.insert({0, 2, 4});
    EXPECT_EQ(list.indices.size(), 3u);
}
```

- [ ] **Step 2: Build — verify red**

```bash
cmake --build --preset clang-debug 2>&1 | head -5
```

Expected: error containing `'cut/list.hpp' file not found`

- [ ] **Step 3: Commit test**

```bash
git add tests/cut/list_test.cpp
git commit -m "test(sp01): add CutList unit tests

TC-REQ002-01..03: default init, open_from set/query, indices insert.
Red — list.hpp does not exist yet."
```

- [ ] **Step 4: Create list.hpp (no docs yet)**

```cpp
#pragma once
#include <optional>
#include <set>

struct CutList {
    std::set<int>      indices;
    std::optional<int> open_from;
};
```

- [ ] **Step 5: Build and run — verify green**

```bash
cmake --build --preset clang-debug && \
  cd out/build/clang-debug && ctest --output-on-failure -R CutList
```

Expected: all 3 CutList tests pass

- [ ] **Step 6: Commit implementation**

```bash
git add include/cut/list.hpp
git commit -m "feat(sp01): add CutList struct

REQ-002: set<int> indices and optional<int> open_from replace magic
-1 sentinel per design-principles Explicit-over-Implicit ruling."
```

- [ ] **Step 7: Add Doxygen documentation to list.hpp**

Replace contents of `include/cut/list.hpp` with:

```cpp
#pragma once
#include <optional>
#include <set>

/// Parsed selection list from a -b, -c, or -f argument.
///
/// A selection is finite (closed) or open-ended:
/// - Finite: `indices` holds all 0-based positions; `open_from` is nullopt.
/// - Open-ended: `open_from` holds the start index; all positions from
///   that index to end-of-line are implicitly selected in addition to
///   any positions in `indices` below it.
///
/// @invariant All values in `indices` are >= 0.
/// @invariant If `open_from` has a value, it is >= 0.
///
/// @code
///   CutList list;
///   list.indices = {0, 2};  // select positions 0 and 2
///   list.open_from = 4;     // also select position 4 to end-of-line
/// @endcode
struct CutList {
    std::set<int>      indices;   ///< 0-based positions to select (finite portion).
    std::optional<int> open_from; ///< If set, select from this 0-based index to EOL.
};
```

- [ ] **Step 8: Build — verify docs compile clean**

```bash
cmake --build --preset clang-debug
```

Expected: exits 0, zero warnings

- [ ] **Step 9: Commit docs**

```bash
git add include/cut/list.hpp
git commit -m "docs(sp01): document CutList public interface

Doxygen comment explains finite vs open-ended selection semantics
and the invariants on indices and open_from."
```

---

### Task 4: CutOptions — test → impl → docs (REQ-003)

**Files:**
- Create: `tests/cut/options_test.cpp`
- Create: `include/cut/options.hpp`

- [ ] **Step 1: Write failing test**

```cpp
// tests/cut/options_test.cpp
#include <gtest/gtest.h>
#include "cut/options.hpp"

// TC-REQ003-01
TEST(CutOptionsTest, DefaultInit) {
    CutOptions opts;
    EXPECT_EQ(opts.mode, CutMode::FIELD);
    EXPECT_EQ(opts.delim, std::nullopt);
    EXPECT_FALSE(opts.suppress);
    EXPECT_FALSE(opts.no_split);
}

// TC-REQ003-02
TEST(CutOptionsTest, SetDelim) {
    CutOptions opts;
    opts.delim = ',';
    EXPECT_TRUE(opts.delim.has_value());
    EXPECT_EQ(opts.delim.value(), ',');
}

// TC-REQ003-03
TEST(CutOptionsTest, SetSuppress) {
    CutOptions opts;
    opts.suppress = true;
    EXPECT_TRUE(opts.suppress);
}
```

- [ ] **Step 2: Build — verify red**

```bash
cmake --build --preset clang-debug 2>&1 | head -5
```

Expected: error containing `'cut/options.hpp' file not found`

- [ ] **Step 3: Commit test**

```bash
git add tests/cut/options_test.cpp
git commit -m "test(sp01): add CutOptions unit tests

TC-REQ003-01..03: default init, delim set/query, suppress flag.
Red — options.hpp does not exist yet."
```

- [ ] **Step 4: Create options.hpp (no docs yet)**

```cpp
#pragma once
#include <optional>
#include "cut/list.hpp"
#include "cut/mode.hpp"

struct CutOptions {
    CutMode             mode     = CutMode::FIELD;
    CutList             list;
    std::optional<char> delim;
    bool                suppress = false;
    bool                no_split = false;
};
```

- [ ] **Step 5: Build and run — verify green**

```bash
cmake --build --preset clang-debug && \
  cd out/build/clang-debug && ctest --output-on-failure -R CutOptions
```

Expected: all 3 CutOptions tests pass

- [ ] **Step 6: Commit implementation**

```bash
git add include/cut/options.hpp
git commit -m "feat(sp01): add CutOptions aggregate struct

REQ-003: mode, list, optional<char> delim, suppress, no_split with
correct defaults. Includes mode.hpp and list.hpp."
```

- [ ] **Step 7: Add Doxygen documentation to options.hpp**

Replace contents of `include/cut/options.hpp` with:

```cpp
#pragma once
#include <optional>
#include "cut/list.hpp"
#include "cut/mode.hpp"

/// Aggregated, validated options for a single cut invocation.
///
/// Populated by the argument parser (SP-03) from argv. A
/// default-constructed CutOptions is valid for field mode reading
/// stdin with whitespace splitting and no flags.
///
/// @code
///   CutOptions opts;
///   opts.mode  = CutMode::FIELD;
///   opts.delim = ',';              // CSV input
///   opts.list.indices = {0, 2};   // first and third fields
/// @endcode
struct CutOptions {
    CutMode             mode     = CutMode::FIELD; ///< Active cut mode.
    CutList             list;                      ///< Field/byte/char selection list.
    /// Delimiter for FIELD mode.
    /// nullopt = split on contiguous whitespace (default).
    /// some(c) = split exactly on character c.
    std::optional<char> delim;
    bool suppress = false; ///< -s: skip lines that contain no delimiter.
    bool no_split = false; ///< -n: do not split multibyte chars (BYTE mode only).
};
```

- [ ] **Step 8: Build — verify docs compile clean**

```bash
cmake --build --preset clang-debug
```

Expected: exits 0, zero warnings

- [ ] **Step 9: Commit docs**

```bash
git add include/cut/options.hpp
git commit -m "docs(sp01): document CutOptions public interface

Doxygen comment on struct and each field including the nullopt vs
some(c) semantics for delim."
```

---

### Task 5: FileSource — test → impl → docs (REQ-004)

**Files:**
- Create: `tests/cut/file_source_test.cpp`
- Create: `include/cut/file_source.hpp`

- [ ] **Step 1: Write failing test**

```cpp
// tests/cut/file_source_test.cpp
#include <gtest/gtest.h>
#include <memory>
#include "cut/file_source.hpp"

// TC-REQ004-01: FileSource is pure virtual.
// The line below must NOT compile — documented here as evidence:
//   FileSource fs;  // must not compile

class StubFileSource : public FileSource {
public:
    void load() override {}
    std::optional<std::string_view> getline() override { return std::nullopt; }
};

// TC-REQ004-02
TEST(FileSourceTest, ConcreteSubclassInstantiable) {
    StubFileSource stub;
    (void)stub;
}

// TC-REQ004-03
TEST(FileSourceTest, GetlineReturnsNullopt) {
    StubFileSource stub;
    stub.load();
    EXPECT_EQ(stub.getline(), std::nullopt);
}

// TC-REQ004-04: delete via base pointer verifies virtual destructor
TEST(FileSourceTest, VirtualDestructorViaBasePtr) {
    std::unique_ptr<FileSource> p = std::make_unique<StubFileSource>();
    EXPECT_EQ(p->getline(), std::nullopt);
    // unique_ptr calls ~FileSource() (virtual) on scope exit
}
```

- [ ] **Step 2: Build — verify red**

```bash
cmake --build --preset clang-debug 2>&1 | head -5
```

Expected: error containing `'cut/file_source.hpp' file not found`

- [ ] **Step 3: Commit test**

```bash
git add tests/cut/file_source_test.cpp
git commit -m "test(sp01): add FileSource interface unit tests

TC-REQ004-02..04: concrete subclass, getline nullopt, virtual dtor
via base pointer. Red — file_source.hpp does not exist yet."
```

- [ ] **Step 4: Create file_source.hpp (no docs yet)**

```cpp
#pragma once
#include <optional>
#include <string_view>

class FileSource {
public:
    virtual void                            load()    = 0;
    virtual std::optional<std::string_view> getline() = 0;
    virtual ~FileSource()                             = default;
};
```

- [ ] **Step 5: Build and run all tests — verify green**

```bash
cmake --build --preset clang-debug && \
  cd out/build/clang-debug && ctest --output-on-failure
```

Expected: all 10 tests pass, 0 failed

- [ ] **Step 6: Commit implementation**

```bash
git add include/cut/file_source.hpp
git commit -m "feat(sp01): add FileSource abstract interface

REQ-004: pure virtual load()/getline() with virtual destructor.
All 10 SP-01 unit tests pass."
```

- [ ] **Step 7: Add Doxygen documentation to file_source.hpp**

Replace contents of `include/cut/file_source.hpp` with:

```cpp
#pragma once
#include <optional>
#include <string_view>

/// Abstract source for line-oriented reading of a single input.
///
/// Implementations provide stdin (always buffered in memory) or
/// file-backed reading (buffered <100 MB, memory-mapped >=100 MB
/// via Boost.Iostreams — see SP-04).
///
/// Usage contract:
/// 1. Call load() exactly once to populate the internal buffer.
/// 2. Call getline() repeatedly until it returns nullopt.
///
/// @note getline() advances an internal cursor — it is a query with
///       a side effect, matching std::istream convention (SPEC-1 REQ-004).
///
/// @code
///   std::unique_ptr<FileSource> src = make_file_source("data.tsv");
///   src->load();
///   while (auto line = src->getline()) {
///       process(*line);
///   }
/// @endcode
class FileSource {
public:
    /// Reads the entire input into an internal buffer.
    ///
    /// Must be called exactly once before getline(). Calling more
    /// than once is undefined behaviour.
    virtual void load() = 0;

    /// Returns the next line without its trailing newline character.
    ///
    /// @return A string_view into the internal buffer for the next
    ///         line, or nullopt when all lines are consumed.
    /// @pre    load() has been called.
    /// @note   The returned string_view is valid until load() is
    ///         called again or the FileSource is destroyed.
    virtual std::optional<std::string_view> getline() = 0;

    virtual ~FileSource() = default;
};
```

- [ ] **Step 8: Build — verify docs compile clean**

```bash
cmake --build --preset clang-debug
```

Expected: exits 0, zero warnings

- [ ] **Step 9: Commit docs**

```bash
git add include/cut/file_source.hpp
git commit -m "docs(sp01): document FileSource public interface

Doxygen comment on class and each virtual method: load() contract,
getline() return semantics, pre-conditions, and string_view lifetime."
```

---

### Task 6: Verify REQ-005 and REQ-006

**Files:** none modified

- [ ] **Step 1: Verify no Boost dylib linked (TC-REQ005-02)**

```bash
otool -L out/build/clang-debug/cc-cut-tool | grep -i boost \
  && echo "FAIL: boost found" || echo "PASS: no boost linked"
```

Expected: `PASS: no boost linked`

- [ ] **Step 2: Run clang-tidy on all new headers (TC-REQ006-02)**

```bash
clang-tidy -p out/build/clang-debug \
  include/cut/mode.hpp \
  include/cut/list.hpp \
  include/cut/options.hpp \
  include/cut/file_source.hpp \
  -- -std=c++23 -I include 2>&1 \
  | grep -E "error:|warning:" \
  && echo "FAIL: tidy issues" || echo "PASS: zero tidy issues"
```

Expected: `PASS: zero tidy issues`

---

## Spec Coverage

| REQ | Task |
|-----|------|
| REQ-001 | Task 2 (mode_test.cpp + mode.hpp + docs) |
| REQ-002 | Task 3 (list_test.cpp + list.hpp + docs) |
| REQ-003 | Task 4 (options_test.cpp + options.hpp + docs) |
| REQ-004 | Task 5 (file_source_test.cpp + file_source.hpp + docs) |
| REQ-005 | Task 1 (CMakeLists) + Task 6 Step 1 |
| REQ-006 | Task 5 Step 5 (clean build) + Task 6 Step 2 |
