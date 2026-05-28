# SP-02: List Parser — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use `subagent-driven-development` (recommended) or `executing-plans` to implement task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement `cc_cut::parse_list()` — a pure function converting a cut list string into a `CutList` — and retroactively wrap all SP-01 types in `namespace cc_cut`.

**Architecture:** One refactor task (namespace wrap SP-01), one CMake wiring task, then TDD cycles for the list parser: test file → header declaration → implementation. Two file-static helpers (`tokenize`, `apply_token`) keep `parse_list` readable. `parse_pos_int` shared within `list_parser.cpp`.

**Tech Stack:** C++23, GoogleTest, CMake 4.0, Ninja, Clang

**Spec:** `.ai/specs/2026-05-28-sp02-list-parser-design.md` (SPEC-2)

**Commit order per logical unit:** `refactor:` → `build:` → `test:` (red) → `feat:` (header) → `feat:` (impl)

**File map:**
```
Modified:
  include/cut/mode.hpp          ← add namespace cc_cut{}
  include/cut/list.hpp          ← add namespace cc_cut{}
  include/cut/options.hpp       ← add namespace cc_cut{}
  include/cut/file_source.hpp   ← add namespace cc_cut{}
  tests/cut/mode_test.cpp       ← add using namespace cc_cut;
  tests/cut/list_test.cpp       ← add using namespace cc_cut;
  tests/cut/options_test.cpp    ← add using namespace cc_cut;
  tests/cut/file_source_test.cpp ← add using namespace cc_cut;
  CMakeLists.txt                ← add list_parser.cpp + test file

Created:
  include/cut/list_parser.hpp   ← declare cc_cut::parse_list
  src/list_parser.cpp           ← implement parse_list
  tests/cut/list_parser_test.cpp ← all 37 TCs (SPEC-2)
```

---

### Task 1: Namespace wrap SP-01 (REQ-011)

**Files:**
- Modify: `include/cut/mode.hpp`
- Modify: `include/cut/list.hpp`
- Modify: `include/cut/options.hpp`
- Modify: `include/cut/file_source.hpp`
- Modify: `tests/cut/mode_test.cpp`
- Modify: `tests/cut/list_test.cpp`
- Modify: `tests/cut/options_test.cpp`
- Modify: `tests/cut/file_source_test.cpp`

Headers and tests must change together — wrapping headers without updating tests causes compile failures.

- [ ] **Step 1: Update include/cut/mode.hpp**

```cpp
#pragma once
#include <cstdint>

namespace cc_cut {

// spec_id: SPEC-1  req_id: REQ-001
/// Operating mode for the cut tool, selected by the first CLI flag.
///
/// Exactly one mode is active per invocation. Modes are mutually exclusive.
///
/// @code
///   cc_cut::CutMode m = cc_cut::CutMode::FIELD;  // selected by -f
/// @endcode
enum class CutMode : uint8_t {
    BYTE,       ///< Byte-position mode (-b). Selects raw bytes from each line.
    CHARACTER,  ///< UTF-8 codepoint mode (-c). Selects Unicode characters.
    FIELD       ///< Field mode (-f). Splits lines on a delimiter character.
};

}  // namespace cc_cut
```

- [ ] **Step 2: Update include/cut/list.hpp**

```cpp
#pragma once
#include <optional>
#include <set>

namespace cc_cut {

// spec_id: SPEC-1  req_id: REQ-002
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
///   cc_cut::CutList list;
///   list.indices = {0, 2};  // select positions 0 and 2
///   list.open_from = 4;     // also select position 4 to end-of-line
/// @endcode
struct CutList {
    std::set<int>      indices;   ///< 0-based positions to select (finite portion).
    std::optional<int> open_from; ///< If set, select from this 0-based index to EOL.
};

}  // namespace cc_cut
```

- [ ] **Step 3: Update include/cut/options.hpp**

```cpp
#pragma once
#include <optional>
#include "cut/list.hpp"
#include "cut/mode.hpp"

namespace cc_cut {

// spec_id: SPEC-1  req_id: REQ-003
/// Aggregated, validated options for a single cut invocation.
///
/// Populated by the argument parser (SP-03) from argv. A
/// default-constructed CutOptions is valid for field mode reading
/// stdin with whitespace splitting and no flags.
///
/// @code
///   cc_cut::CutOptions opts;
///   opts.mode  = cc_cut::CutMode::FIELD;
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

}  // namespace cc_cut
```

- [ ] **Step 4: Update include/cut/file_source.hpp**

```cpp
#pragma once
#include <optional>
#include <string_view>

namespace cc_cut {

// spec_id: SPEC-1  req_id: REQ-004
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
///       a side effect, matching std::istream convention.
///
/// @code
///   std::unique_ptr<cc_cut::FileSource> src = make_file_source("data.tsv");
///   src->load();
///   while (auto line = src->getline()) {
///       process(*line);
///   }
/// @endcode
class FileSource {
public:
    FileSource()                           = default;
    FileSource(const FileSource&)                    = delete;
    auto operator=(const FileSource&) -> FileSource& = delete;
    FileSource(FileSource&&)                         = delete;
    auto operator=(FileSource&&)      -> FileSource& = delete;

    // spec_id: SPEC-1  req_id: REQ-004
    /// Reads the entire input into an internal buffer.
    ///
    /// Must be called exactly once before getline(). Calling more
    /// than once is undefined behaviour.
    ///
    /// @throws std::ios_base::failure If the underlying source cannot be
    ///         opened or read (e.g. file missing, permission denied, I/O
    ///         error). Implementations must not silently swallow errors.
    virtual void load() = 0;

    // spec_id: SPEC-1  req_id: REQ-004
    /// Returns the next line without its trailing newline character.
    ///
    /// @return A string_view into the internal buffer for the next
    ///         line, or nullopt when all lines are consumed.
    /// @pre    load() has been called.
    /// @warning The returned string_view aliases the internal buffer.
    ///          It is invalidated when load() is called again or when
    ///          the FileSource is destroyed. Storing it past either
    ///          event is undefined behaviour.
    virtual auto getline() -> std::optional<std::string_view> = 0;

    virtual ~FileSource() = default;
};

}  // namespace cc_cut
```

- [ ] **Step 5: Add `using namespace cc_cut;` to tests/cut/mode_test.cpp**

Insert after the last `#include` line:

```cpp
// spec_id: SPEC-1  validates_req: REQ-001
#include <gtest/gtest.h>
#include "cut/mode.hpp"

using namespace cc_cut;

// TC-REQ001-01, TC-REQ001-02
static_assert(CutMode::BYTE != CutMode::CHARACTER);
static_assert(CutMode::CHARACTER != CutMode::FIELD);

// TC-REQ001-03
static_assert(sizeof(CutMode) == 1);

// spec_id: SPEC-1  validates_req: REQ-001
TEST(CutModeTest, DistinctValues) {
    EXPECT_NE(CutMode::BYTE, CutMode::CHARACTER);
    EXPECT_NE(CutMode::CHARACTER, CutMode::FIELD);
}
```

- [ ] **Step 6: Add `using namespace cc_cut;` to tests/cut/list_test.cpp**

```cpp
// spec_id: SPEC-1  validates_req: REQ-002
#include <gtest/gtest.h>
#include "cut/list.hpp"

using namespace cc_cut;

// spec_id: SPEC-1  validates_req: REQ-002  tc: TC-REQ002-01
TEST(CutListTest, DefaultInit) {
    CutList list;
    EXPECT_EQ(list.open_from, std::nullopt);
    EXPECT_TRUE(list.indices.empty());
}

// spec_id: SPEC-1  validates_req: REQ-002  tc: TC-REQ002-02
TEST(CutListTest, SetOpenFrom) {
    CutList list;
    list.open_from = 2;
    EXPECT_TRUE(list.open_from.has_value());
    EXPECT_EQ(list.open_from.value(), 2);
}

// spec_id: SPEC-1  validates_req: REQ-002  tc: TC-REQ002-03
TEST(CutListTest, InsertIndices) {
    CutList list;
    list.indices.insert({0, 2, 4});
    EXPECT_EQ(list.indices.size(), 3U);
}
```

- [ ] **Step 7: Add `using namespace cc_cut;` to tests/cut/options_test.cpp**

```cpp
// spec_id: SPEC-1  validates_req: REQ-003
#include <gtest/gtest.h>
#include "cut/options.hpp"

using namespace cc_cut;

// spec_id: SPEC-1  validates_req: REQ-003  tc: TC-REQ003-01
TEST(CutOptionsTest, DefaultInit) {
    CutOptions opts;
    EXPECT_EQ(opts.mode, CutMode::FIELD);
    EXPECT_EQ(opts.delim, std::nullopt);
    EXPECT_FALSE(opts.suppress);
    EXPECT_FALSE(opts.no_split);
}

// spec_id: SPEC-1  validates_req: REQ-003  tc: TC-REQ003-02
TEST(CutOptionsTest, SetDelim) {
    CutOptions opts;
    opts.delim = ',';
    EXPECT_TRUE(opts.delim.has_value());
    EXPECT_EQ(opts.delim.value(), ',');
}

// spec_id: SPEC-1  validates_req: REQ-003  tc: TC-REQ003-03
TEST(CutOptionsTest, SetSuppress) {
    CutOptions opts;
    opts.suppress = true;
    EXPECT_TRUE(opts.suppress);
}
```

- [ ] **Step 8: Add `using namespace cc_cut;` to tests/cut/file_source_test.cpp**

```cpp
// spec_id: SPEC-1  validates_req: REQ-004
#include <gtest/gtest.h>
#include <memory>
#include <type_traits>
#include "cut/file_source.hpp"

using namespace cc_cut;

// TC-REQ004-01: FileSource must be abstract.
static_assert(std::is_abstract_v<FileSource>,
              "TC-REQ004-01: FileSource must be abstract");

class StubFileSource : public FileSource {
public:
    void load() override {}
    auto getline() -> std::optional<std::string_view> override { return std::nullopt; }
};

// spec_id: SPEC-1  validates_req: REQ-004  tc: TC-REQ004-02
TEST(FileSourceTest, ConcreteSubclassInstantiable) {
    StubFileSource stub;
    (void)stub;
}

// spec_id: SPEC-1  validates_req: REQ-004  tc: TC-REQ004-03
TEST(FileSourceTest, GetlineReturnsNullopt) {
    StubFileSource stub;
    stub.load();
    EXPECT_EQ(stub.getline(), std::nullopt);
}

// spec_id: SPEC-1  validates_req: REQ-004  tc: TC-REQ004-04
TEST(FileSourceTest, VirtualDestructorViaBasePtr) {
    std::unique_ptr<FileSource> p = std::make_unique<StubFileSource>();
    EXPECT_EQ(p->getline(), std::nullopt);
}
```

- [ ] **Step 9: Build and run all 10 existing tests — verify still green**

```bash
cmake --build --preset clang-debug && \
  ctest --test-dir out/build/clang-debug --output-on-failure --exclude-regex "integ_"
```

Expected: `100% tests passed, 0 tests failed out of 10`

- [ ] **Step 10: Commit**

```bash
git add include/cut/mode.hpp include/cut/list.hpp \
        include/cut/options.hpp include/cut/file_source.hpp \
        tests/cut/mode_test.cpp tests/cut/list_test.cpp \
        tests/cut/options_test.cpp tests/cut/file_source_test.cpp
```

Commit message:
```
refactor(cut): wrap all public types in namespace cc_cut

Without a namespace all cut types pollute the global namespace and
risk clashing with system or third-party headers. cc_cut scopes the
project's public API and is enforced by SPEC-2 REQ-011. All 10
existing unit tests pass after adding using namespace cc_cut to each
test file.
```

---

### Task 2: CMakeLists wiring

**Files:**
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Add list_parser.cpp to SRC_FILES and test file to test target**

Replace:
```cmake
set(SRC_FILES
)
```
with:
```cmake
set(SRC_FILES
  src/list_parser.cpp
)
```

Add `tests/cut/list_parser_test.cpp` to the test executable sources:
```cmake
add_executable(${TEST_PROJECT_NAME}
  tests/cut/mode_test.cpp
  tests/cut/list_test.cpp
  tests/cut/options_test.cpp
  tests/cut/file_source_test.cpp
  tests/cut/list_parser_test.cpp
  ${SRC_FILES}
)
```

- [ ] **Step 2: Create placeholder test file so CMake configures**

```bash
printf '// placeholder\n' > tests/cut/list_parser_test.cpp
```

- [ ] **Step 3: Verify CMake configures and build fails gracefully**

```bash
cmake --preset clang-debug && cmake --build --preset clang-debug 2>&1 | tail -5
```

Expected: configure succeeds; build will fail only after test file is filled.

- [ ] **Step 4: Commit**

```bash
git add CMakeLists.txt tests/cut/list_parser_test.cpp
```

Commit message:
```
build(sp02): add list_parser to CMake sources and test target

list_parser.cpp added to SRC_FILES so it links into both the main
binary and test binary. Placeholder test file satisfies CMake 4.0
configure-time source existence requirement.
```

---

### Task 3: Write failing test file (RED)

**Files:**
- Modify: `tests/cut/list_parser_test.cpp`

- [ ] **Step 1: Write the full test file**

```cpp
// spec_id: SPEC-2  validates_req: REQ-001,REQ-002,REQ-003,REQ-004,REQ-005,REQ-006,REQ-007,REQ-008,REQ-009,REQ-010,REQ-011
#include <gtest/gtest.h>
#include <set>
#include <type_traits>
#include "cut/list_parser.hpp"

using namespace cc_cut;

// ---- REQ-011: namespace cc_cut ----

// TC-REQ011-01: cc_cut::CutMode::FIELD is of type cc_cut::CutMode
static_assert(
    std::is_same_v<decltype(cc_cut::CutMode::FIELD), cc_cut::CutMode>,
    "TC-REQ011-01: CutMode must be in namespace cc_cut");

// TC-REQ011-02: CutMode::FIELD without qualifier must not compile.
// Verified by code review — using namespace cc_cut; is added to all
// test files; unqualified access works only because of that directive.

// ---- REQ-001: function signature ----

// spec_id: SPEC-2  validates_req: REQ-001  tc: TC-REQ001-01
TEST(ParseListTest, ReturnsValueForValidInput) {
    auto result = parse_list("1");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->indices, (std::set<int>{0}));
}

// spec_id: SPEC-2  validates_req: REQ-001  tc: TC-REQ001-02
TEST(ParseListTest, ReturnsErrorForEmptyInput) {
    auto result = parse_list("");
    EXPECT_FALSE(result.has_value());
}

// spec_id: SPEC-2  validates_req: REQ-001  tc: TC-REQ001-03
TEST(ParseListTest, PureFunctionIdenticalResults) {
    auto r1 = parse_list("1");
    auto r2 = parse_list("1");
    ASSERT_TRUE(r1.has_value());
    ASSERT_TRUE(r2.has_value());
    EXPECT_EQ(r1->indices, r2->indices);
    EXPECT_EQ(r1->open_from, r2->open_from);
}

// ---- REQ-002: comma tokenization ----

// spec_id: SPEC-2  validates_req: REQ-002  tc: TC-REQ002-01
TEST(ParseListTest, CommaMode_ThreeTokens) {
    auto result = parse_list("1,3,5");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->indices, (std::set<int>{0, 2, 4}));
    EXPECT_EQ(result->open_from, std::nullopt);
}

// spec_id: SPEC-2  validates_req: REQ-002  tc: TC-REQ002-04
TEST(ParseListTest, CommaMode_WhitespaceStripped) {
    auto result = parse_list("1, 3, 5");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->indices, (std::set<int>{0, 2, 4}));
}

// spec_id: SPEC-2  validates_req: REQ-002  tc: TC-REQ002-02
TEST(ParseListTest, CommaMode_EmptyTokenBetweenCommas) {
    auto result = parse_list("1,,3");
    ASSERT_FALSE(result.has_value());
    EXPECT_NE(result.error().find("invalid field value"), std::string::npos);
}

// spec_id: SPEC-2  validates_req: REQ-002  tc: TC-REQ002-03
TEST(ParseListTest, CommaMode_LeadingComma) {
    auto result = parse_list(",1");
    ASSERT_FALSE(result.has_value());
    EXPECT_NE(result.error().find("invalid field value"), std::string::npos);
}

// ---- REQ-003: whitespace tokenization ----

// spec_id: SPEC-2  validates_req: REQ-003  tc: TC-REQ003-01
TEST(ParseListTest, WhitespaceMode_SpaceSeparated) {
    auto result = parse_list("1 3 5");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->indices, (std::set<int>{0, 2, 4}));
    EXPECT_EQ(result->open_from, std::nullopt);
}

// spec_id: SPEC-2  validates_req: REQ-003  tc: TC-REQ003-02
TEST(ParseListTest, WhitespaceMode_LeadingTrailingSpaces) {
    auto result = parse_list("  1  3  ");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->indices, (std::set<int>{0, 2}));
}

// spec_id: SPEC-2  validates_req: REQ-003  tc: TC-REQ003-03
TEST(ParseListTest, WhitespaceMode_TabSeparated) {
    auto result = parse_list("1\t3");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->indices, (std::set<int>{0, 2}));
}

// ---- REQ-004: plain number token ----

// spec_id: SPEC-2  validates_req: REQ-004  tc: TC-REQ004-01
TEST(ParseListTest, PlainNumber_Three) {
    auto result = parse_list("3");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->indices, (std::set<int>{2}));
    EXPECT_EQ(result->open_from, std::nullopt);
}

// spec_id: SPEC-2  validates_req: REQ-004  tc: TC-REQ004-02
TEST(ParseListTest, PlainNumber_One) {
    auto result = parse_list("1");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->indices, (std::set<int>{0}));
}

// spec_id: SPEC-2  validates_req: REQ-004  tc: TC-REQ004-03
TEST(ParseListTest, PlainNumber_Duplicate) {
    auto result = parse_list("1 1");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->indices.size(), 1U);
    EXPECT_EQ(result->indices, (std::set<int>{0}));
}

// ---- REQ-005: range token N-M ----

// spec_id: SPEC-2  validates_req: REQ-005  tc: TC-REQ005-01
TEST(ParseListTest, Range_TwoToFive) {
    auto result = parse_list("2-5");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->indices, (std::set<int>{1, 2, 3, 4}));
    EXPECT_EQ(result->open_from, std::nullopt);
}

// spec_id: SPEC-2  validates_req: REQ-005  tc: TC-REQ005-02
TEST(ParseListTest, Range_SingleElement) {
    auto result = parse_list("3-3");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->indices, (std::set<int>{2}));
}

// spec_id: SPEC-2  validates_req: REQ-005  tc: TC-REQ005-03
TEST(ParseListTest, Range_Decreasing) {
    auto result = parse_list("5-3");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), "invalid decreasing range");
}

// spec_id: SPEC-2  validates_req: REQ-005  tc: TC-REQ005-04
TEST(ParseListTest, Range_Combined) {
    auto result = parse_list("1,3-5,7");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->indices, (std::set<int>{0, 2, 3, 4, 6}));
    EXPECT_EQ(result->open_from, std::nullopt);
}

// spec_id: SPEC-2  validates_req: REQ-005  tc: TC-REQ005-05
TEST(ParseListTest, Range_ZeroEndpoint) {
    auto result = parse_list("3-0");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), "values may not include zero");
}

// ---- REQ-006: open-start token -M ----

// spec_id: SPEC-2  validates_req: REQ-006  tc: TC-REQ006-01
TEST(ParseListTest, OpenStart_DashFour) {
    auto result = parse_list("-4");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->indices, (std::set<int>{0, 1, 2, 3}));
    EXPECT_EQ(result->open_from, std::nullopt);
}

// spec_id: SPEC-2  validates_req: REQ-006  tc: TC-REQ006-02
TEST(ParseListTest, OpenStart_DashOne) {
    auto result = parse_list("-1");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->indices, (std::set<int>{0}));
}

// ---- REQ-007: open-end token N- ----

// spec_id: SPEC-2  validates_req: REQ-007  tc: TC-REQ007-01
TEST(ParseListTest, OpenEnd_ThreeDash) {
    auto result = parse_list("3-");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->open_from, 2);
    EXPECT_TRUE(result->indices.empty());
}

// spec_id: SPEC-2  validates_req: REQ-007  tc: TC-REQ007-02
TEST(ParseListTest, OpenEnd_OneDash) {
    auto result = parse_list("1-");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->open_from, 0);
}

// spec_id: SPEC-2  validates_req: REQ-007  tc: TC-REQ007-03
TEST(ParseListTest, OpenEnd_MultipleSelectsMinimum) {
    auto result = parse_list("3-,5-");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->open_from, 2);
}

// spec_id: SPEC-2  validates_req: REQ-007  tc: TC-REQ007-04
TEST(ParseListTest, OpenEnd_WithFiniteIndices) {
    auto result = parse_list("1,3-");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->indices, (std::set<int>{0}));
    EXPECT_EQ(result->open_from, 2);
}

// ---- REQ-008: zero position error ----

// spec_id: SPEC-2  validates_req: REQ-008  tc: TC-REQ008-01
TEST(ParseListTest, Zero_PlainZero) {
    auto result = parse_list("0");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), "values may not include zero");
}

// spec_id: SPEC-2  validates_req: REQ-008  tc: TC-REQ008-02
TEST(ParseListTest, Zero_RangeStartsAtZero) {
    auto result = parse_list("0-3");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), "values may not include zero");
}

// spec_id: SPEC-2  validates_req: REQ-008  tc: TC-REQ008-03
TEST(ParseListTest, Zero_OpenStartZero) {
    auto result = parse_list("-0");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), "values may not include zero");
}

// ---- REQ-009: invalid token error ----

// spec_id: SPEC-2  validates_req: REQ-009  tc: TC-REQ009-01
TEST(ParseListTest, Invalid_LoneDash) {
    auto result = parse_list("-");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), "invalid range with no endpoint: -");
}

// spec_id: SPEC-2  validates_req: REQ-009  tc: TC-REQ009-02
TEST(ParseListTest, Invalid_AlphaToken) {
    auto result = parse_list("a");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), "invalid field value: a");
}

// spec_id: SPEC-2  validates_req: REQ-009  tc: TC-REQ009-03
TEST(ParseListTest, Invalid_AlphaSuffix) {
    auto result = parse_list("1a");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), "invalid field value: 1a");
}

// spec_id: SPEC-2  validates_req: REQ-009  tc: TC-REQ009-04
TEST(ParseListTest, Invalid_MultipleDashes) {
    auto result = parse_list("1-2-3");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), "invalid field value: 1-2-3");
}

// spec_id: SPEC-2  validates_req: REQ-009  tc: TC-REQ009-05
TEST(ParseListTest, Invalid_DoubleDashPrefix) {
    auto result = parse_list("--3");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), "invalid field value: --3");
}

// ---- REQ-010: empty input error ----

// spec_id: SPEC-2  validates_req: REQ-010  tc: TC-REQ010-01
TEST(ParseListTest, Empty_EmptyString) {
    auto result = parse_list("");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), "missing list specification");
}

// spec_id: SPEC-2  validates_req: REQ-010  tc: TC-REQ010-02
TEST(ParseListTest, Empty_WhitespaceOnly) {
    auto result = parse_list("   ");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), "missing list specification");
}

// ---- REQ-011: tc: TC-REQ011-03 — SP-01 tests pass under cc_cut namespace ----
// Verified implicitly: this file and the four SP-01 test files compile and
// pass with using namespace cc_cut; — confirmed by ctest run in Task 4.
```

- [ ] **Step 2: Verify build fails (missing header)**

```bash
cmake --build --preset clang-debug 2>&1 | grep "fatal error" | head -3
```

Expected: `fatal error: 'cut/list_parser.hpp' file not found`

- [ ] **Step 3: Commit test file**

```bash
git add tests/cut/list_parser_test.cpp
git commit -m "test(sp02): add list parser unit tests"
```

---

### Task 4: Create list_parser.hpp (still RED — no impl)

**Files:**
- Create: `include/cut/list_parser.hpp`

- [ ] **Step 1: Write the header**

```cpp
// include/cut/list_parser.hpp
#pragma once
#include "cut/list.hpp"
#include <expected>
#include <string>
#include <string_view>

namespace cc_cut {

// spec_id: SPEC-2  req_id: REQ-001
/// Parses a cut list specification string into a CutList.
///
/// Tokenizes on comma when `list_arg` contains a comma; otherwise on
/// contiguous whitespace. Each token is classified as a plain 1-based
/// position, an N-M range, a -M open-start, or an N- open-end.
///
/// @param list_arg  Raw list string from a -b, -c, or -f argument
///                  (e.g. "1,3-5,7-" or "1 3 5").
/// @return          CutList on success; error string on the first
///                  invalid token encountered.
///
/// @throws          Never throws. All errors returned via std::unexpected.
///
/// @code
///   auto result = cc_cut::parse_list("1,3-5,7-");
///   if (result) { use(*result); }
///   else        { std::cerr << result.error() << '\n'; }
/// @endcode
auto parse_list(std::string_view list_arg)
    -> std::expected<CutList, std::string>;

}  // namespace cc_cut
```

- [ ] **Step 2: Verify build still fails (undefined symbol)**

```bash
cmake --build --preset clang-debug 2>&1 | grep "error:" | head -3
```

Expected: linker error — `undefined symbol: cc_cut::parse_list`

- [ ] **Step 3: Commit header**

```bash
git add include/cut/list_parser.hpp
git commit -m "feat(sp02): declare parse_list in list_parser.hpp"
```

---

### Task 5: Implement list_parser.cpp (GREEN)

**Files:**
- Create: `src/list_parser.cpp`

- [ ] **Step 1: Write the implementation**

```cpp
// src/list_parser.cpp
#include "cut/list_parser.hpp"
#include <charconv>
#include <format>
#include <string>
#include <vector>

namespace cc_cut {

namespace {

auto parse_pos_int(std::string_view s) -> std::optional<int> {
    if (s.empty()) return std::nullopt;
    int val = 0;
    auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), val);
    if (ec != std::errc{} || ptr != s.data() + s.size()) return std::nullopt;
    return val;
}

auto tokenize(std::string_view list_arg) -> std::vector<std::string> {
    std::vector<std::string> tokens;

    if (list_arg.find(',') != std::string_view::npos) {
        std::string_view rem = list_arg;
        while (true) {
            auto comma = rem.find(',');
            std::string_view part =
                (comma == std::string_view::npos) ? rem : rem.substr(0, comma);

            auto first = part.find_first_not_of(" \t");
            if (first == std::string_view::npos) {
                tokens.emplace_back("");
            } else {
                auto last = part.find_last_not_of(" \t");
                tokens.emplace_back(part.substr(first, last - first + 1));
            }

            if (comma == std::string_view::npos) break;
            rem = rem.substr(comma + 1);
        }
    } else {
        std::string_view rem = list_arg;
        while (!rem.empty()) {
            auto first = rem.find_first_not_of(" \t");
            if (first == std::string_view::npos) break;
            rem = rem.substr(first);
            auto end = rem.find_first_of(" \t");
            if (end == std::string_view::npos) {
                tokens.emplace_back(rem);
                break;
            }
            tokens.emplace_back(rem.substr(0, end));
            rem = rem.substr(end);
        }
    }

    return tokens;
}

// Zero-position check precedes decreasing-range check per SPEC-2 REQ-005.
auto apply_token(std::string_view token, CutList& result)
    -> std::expected<void, std::string>
{
    if (token.empty())
        return std::unexpected(std::format("invalid field value: {}", token));

    auto dash = token.find('-');

    if (dash == std::string_view::npos) {
        auto n = parse_pos_int(token);
        if (!n) return std::unexpected(std::format("invalid field value: {}", token));
        if (*n == 0) return std::unexpected("values may not include zero");
        result.indices.insert(*n - 1);
        return {};
    }

    if (dash == 0) {
        auto rest = token.substr(1);
        if (rest.empty()) return std::unexpected("invalid range with no endpoint: -");
        auto m = parse_pos_int(rest);
        if (!m) return std::unexpected(std::format("invalid field value: {}", token));
        if (*m == 0) return std::unexpected("values may not include zero");
        for (int i = 0; i < *m; ++i) result.indices.insert(i);
        return {};
    }

    auto left  = token.substr(0, dash);
    auto right = token.substr(dash + 1);

    auto n = parse_pos_int(left);
    if (!n) return std::unexpected(std::format("invalid field value: {}", token));
    if (*n == 0) return std::unexpected("values may not include zero");

    if (right.empty()) {
        int open = *n - 1;
        if (!result.open_from.has_value() || open < *result.open_from)
            result.open_from = open;
        return {};
    }

    auto m = parse_pos_int(right);
    if (!m) return std::unexpected(std::format("invalid field value: {}", token));
    if (*m == 0) return std::unexpected("values may not include zero");
    if (*n > *m) return std::unexpected("invalid decreasing range");
    for (int i = *n - 1; i < *m; ++i) result.indices.insert(i);
    return {};
}

}  // anonymous namespace

auto parse_list(std::string_view list_arg)
    -> std::expected<CutList, std::string>
{
    auto tokens = tokenize(list_arg);
    if (tokens.empty()) return std::unexpected("missing list specification");

    CutList result;
    for (const auto& token : tokens) {
        auto outcome = apply_token(token, result);
        if (!outcome) return std::unexpected(outcome.error());
    }
    return result;
}

}  // namespace cc_cut
```

- [ ] **Step 2: Build — expect success**

```bash
cmake --build --preset clang-debug 2>&1 | grep -E "error:|warning:" | head -5
```

Expected: no output (zero errors, zero warnings)

- [ ] **Step 3: Run all unit tests**

```bash
ctest --test-dir out/build/clang-debug --output-on-failure --exclude-regex "integ_"
```

Expected: all tests pass (10 SP-01 + 34 SP-02 = 44 total), 0 failed
Note: TC-REQ011-01 runs as static_assert at compile time (not counted by ctest)

- [ ] **Step 4: Commit implementation**

```bash
git add src/list_parser.cpp
git commit -m "feat(sp02): implement parse_list"
```

Body (required — multi-file change with logic):
```
Tokenization and token classification kept as anonymous-namespace
helpers so parse_list stays readable and each concern is testable
in isolation. Zero-position check applied before decreasing-range
check per SPEC-2 REQ-005 ordering requirement.
```

---

## Spec Coverage

| REQ | Task |
|-----|------|
| REQ-001 (signature) | Task 4 (header) + Task 3 (TC-REQ001-*) |
| REQ-002 (comma tokenize) | Task 5 (tokenize) + Task 3 (TC-REQ002-*) |
| REQ-003 (whitespace tokenize) | Task 5 (tokenize) + Task 3 (TC-REQ003-*) |
| REQ-004 (plain number) | Task 5 (apply_token) + Task 3 (TC-REQ004-*) |
| REQ-005 (N-M range) | Task 5 (apply_token) + Task 3 (TC-REQ005-*) |
| REQ-006 (open-start -M) | Task 5 (apply_token) + Task 3 (TC-REQ006-*) |
| REQ-007 (open-end N-) | Task 5 (apply_token) + Task 3 (TC-REQ007-*) |
| REQ-008 (zero error) | Task 5 (apply_token) + Task 3 (TC-REQ008-*) |
| REQ-009 (invalid token) | Task 5 (apply_token) + Task 3 (TC-REQ009-*) |
| REQ-010 (empty input) | Task 5 (parse_list entry) + Task 3 (TC-REQ010-*) |
| REQ-011 (namespace) | Task 1 (headers + tests) + Task 3 (TC-REQ011-*) |
