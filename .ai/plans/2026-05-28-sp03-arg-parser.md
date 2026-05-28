# SP-03: Arg Parser — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use `subagent-driven-development` (recommended) or `executing-plans` to implement task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement `cc_cut::parse_args()` — a sequential pipeline of named helpers that converts raw `argc`/`argv` into a typed `ParseResult` with coreutils-compatible error messages.

**Architecture:** Five helpers (`detect_mode`, `extract_list_spec`, `parse_mode_properties`, `collect_files`, plus a private `format_error`) orchestrated by `parse_args`. A CMake `configure_file` generates `config.hpp` from project metadata. All symbols in `namespace cc_cut`. Commit order per component: `test:` (red) → `feat:` (impl with docs).

**Tech Stack:** C++23, GoogleTest, CMake 4.0, Ninja, Clang

**Spec:** `.ai/specs/2026-05-28-sp03-arg-parser-design.md` (SPEC-3)

**File map:**
```
Created:
  include/cut/config.hpp.in      ← CMake template → config.hpp
  include/cut/parse_result.hpp   ← ParseResult struct
  include/cut/arg_parser.hpp     ← declare parse_args + helpers
  src/arg_parser.cpp             ← implement all functions
  tests/cut/arg_parser_test.cpp  ← all TCs REQ-001..010

Modified:
  CMakeLists.txt                 ← configure_file, include dirs,
                                    arg_parser.cpp in SRC_FILES,
                                    test file in test target
```

**Key assumption (surface explicitly):** `extract_list_spec` receives `index = 2` (argv[1] is the mode flag already consumed by detect_mode). All tests of the helper must match this convention.

---

### Task 1: CMake configure_file + config.hpp

**Files:**
- Create: `include/cut/config.hpp.in`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Create config.hpp.in**

```cpp
// include/cut/config.hpp.in
#pragma once
#include <string_view>

namespace cc_cut::config {

inline constexpr std::string_view program_name = "@PROJECT_NAME@";
inline constexpr std::string_view version      = "@PROJECT_VERSION@";
inline constexpr std::string_view help_hint    =
    "Try '@PROJECT_NAME@ --help' for more information.";

}  // namespace cc_cut::config
```

- [ ] **Step 2: Add configure_file and include-dir to CMakeLists.txt**

After the `find_package(Boost REQUIRED)` line, add:

```cmake
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/include/cut/config.hpp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/include/cut/config.hpp"
    @ONLY
)
```

In BOTH `target_include_directories` blocks (main executable AND test executable), add:
```cmake
  ${CMAKE_CURRENT_BINARY_DIR}/include
```

So both blocks look like:
```cmake
target_include_directories(${PROJECT_NAME} PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_BINARY_DIR}/include
)
```

- [ ] **Step 3: Verify config.hpp is generated**

```bash
cd /Users/laksh/Documents/PersonalCode/cc-cut-tool && \
  cmake --preset clang-debug 2>&1 | tail -3 && \
  cat out/build/clang-debug/include/cut/config.hpp | grep program_name
```

Expected output contains: `inline constexpr std::string_view program_name = "cc-cut-tool";`

- [ ] **Step 4: Commit**

```bash
git add include/cut/config.hpp.in CMakeLists.txt
git commit -m "build(sp03): add CMake configure_file for config.hpp

config.hpp.in must be generated at build time so program_name and
version come from a single source of truth in CMakeLists.txt rather
than being duplicated as string literals across the codebase."
```

---

### Task 2: ParseResult struct — test → impl

**Files:**
- Create: `include/cut/parse_result.hpp`
- Modify: `tests/cut/arg_parser_test.cpp` (placeholder → real tests for REQ-001)
- Modify: `CMakeLists.txt` (add arg_parser_test.cpp + placeholder src/arg_parser.cpp)

- [ ] **Step 1: Wire CMakeLists for new files**

Add `src/arg_parser.cpp` to `SRC_FILES`:
```cmake
set(SRC_FILES
  src/list_parser.cpp
  src/arg_parser.cpp
)
```

Add `tests/cut/arg_parser_test.cpp` to test executable sources:
```cmake
add_executable(${TEST_PROJECT_NAME}
  tests/cut/mode_test.cpp
  tests/cut/list_test.cpp
  tests/cut/options_test.cpp
  tests/cut/file_source_test.cpp
  tests/cut/list_parser_test.cpp
  tests/cut/arg_parser_test.cpp
  ${SRC_FILES}
)
```

Create placeholder files:
```bash
printf '// placeholder\n' > src/arg_parser.cpp
printf '// placeholder\n' > tests/cut/arg_parser_test.cpp
```

Verify configure succeeds:
```bash
cmake --preset clang-debug 2>&1 | tail -3
```

Expected: `Build files have been written to: ...clang-debug`

- [ ] **Step 2: Write failing test for ParseResult (REQ-001)**

```cpp
// tests/cut/arg_parser_test.cpp
// spec_id: SPEC-3  validates_req: REQ-001,REQ-002,REQ-003,REQ-004,REQ-005,REQ-006,REQ-007,REQ-008,REQ-009,REQ-010
#include "cut/arg_parser.hpp"
#include "cut/config.hpp"
#include "cut/parse_result.hpp"

#include <gtest/gtest.h>

#include <string>
#include <vector>

using cc_cut::CutMode;
using cc_cut::CutOptions;
using cc_cut::ParseResult;
using cc_cut::collect_files;
using cc_cut::detect_mode;
using cc_cut::extract_list_spec;
using cc_cut::parse_args;
using cc_cut::parse_mode_properties;

// ---------------------------------------------------------------------------
// Helper: build a char** argv from vector<string> (strings stay alive)
// ---------------------------------------------------------------------------
struct ArgvHolder {
  std::vector<std::string> storage;
  std::vector<char*>       argv_vec;

  explicit ArgvHolder(std::vector<std::string> args) : storage(std::move(args)) {
    for (auto& s : storage) argv_vec.push_back(s.data());
    argv_vec.push_back(nullptr);
  }

  int    argc() const { return static_cast<int>(storage.size()); }
  char** argv() { return argv_vec.data(); }
};

// ---------------------------------------------------------------------------
// REQ-001: ParseResult struct
// ---------------------------------------------------------------------------

// spec_id: SPEC-3  validates_req: REQ-001  tc: TC-REQ001-01
TEST(ParseResultTest, DefaultInit) {
  ParseResult result;
  EXPECT_EQ(result.opts.mode, CutMode::FIELD);
  EXPECT_TRUE(result.files.empty());
  EXPECT_FALSE(result.help_requested);
}

// spec_id: SPEC-3  validates_req: REQ-001  tc: TC-REQ001-02
TEST(ParseResultTest, FilesAssignment) {
  ParseResult result;
  result.files = {"a.txt", "b.txt"};
  EXPECT_EQ(result.files.size(), 2U);
}

// spec_id: SPEC-3  validates_req: REQ-001  tc: TC-REQ001-03
TEST(ParseResultTest, HelpRequestedFlag) {
  ParseResult result;
  result.help_requested = true;
  EXPECT_TRUE(result.help_requested);
}

// ---------------------------------------------------------------------------
// REQ-002: config.hpp constants
// ---------------------------------------------------------------------------

// spec_id: SPEC-3  validates_req: REQ-002  tc: TC-REQ002-01
TEST(ConfigTest, ProgramName) {
  EXPECT_EQ(cc_cut::config::program_name, std::string_view{"cc-cut-tool"});
}

// spec_id: SPEC-3  validates_req: REQ-002  tc: TC-REQ002-02
TEST(ConfigTest, HelpHintFormat) {
  EXPECT_TRUE(cc_cut::config::help_hint.ends_with("for more information."));
}

// ---------------------------------------------------------------------------
// REQ-004: detect_mode
// ---------------------------------------------------------------------------

// spec_id: SPEC-3  validates_req: REQ-004  tc: TC-REQ004-01
TEST(DetectModeTest, ByteMode) {
  auto result = detect_mode("-b");
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, CutMode::BYTE);
}

// spec_id: SPEC-3  validates_req: REQ-004  tc: TC-REQ004-02
TEST(DetectModeTest, CharModeAttached) {
  auto result = detect_mode("-c3-5");
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, CutMode::CHARACTER);
}

// spec_id: SPEC-3  validates_req: REQ-004  tc: TC-REQ004-03
TEST(DetectModeTest, FieldMode) {
  auto result = detect_mode("-f");
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, CutMode::FIELD);
}

// spec_id: SPEC-3  validates_req: REQ-004  tc: TC-REQ004-04
TEST(DetectModeTest, UnknownFlag) {
  auto result = detect_mode("-x");
  ASSERT_FALSE(result.has_value());
  EXPECT_NE(result.error().find("invalid option"), std::string::npos);
  EXPECT_NE(result.error().find("'x'"), std::string::npos);
}

// spec_id: SPEC-3  validates_req: REQ-004  tc: TC-REQ004-05
TEST(DetectModeTest, LoneDash) {
  auto result = detect_mode("-");
  ASSERT_FALSE(result.has_value());
  EXPECT_NE(result.error().find("invalid option"), std::string::npos);
}

// spec_id: SPEC-3  validates_req: REQ-004  tc: TC-REQ004-06
TEST(DetectModeTest, EmptyString) {
  auto result = detect_mode("");
  ASSERT_FALSE(result.has_value());
  EXPECT_NE(result.error().find("invalid option"), std::string::npos);
}

// ---------------------------------------------------------------------------
// REQ-005: extract_list_spec
// ---------------------------------------------------------------------------

// spec_id: SPEC-3  validates_req: REQ-005  tc: TC-REQ005-01
TEST(ExtractListSpecTest, AttachedList) {
  ArgvHolder holder{{"cut", "-f1,3", "file.txt"}};
  int index = 2;
  auto result = extract_list_spec("-f1,3", holder.argc(), holder.argv(), index);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, "1,3");
  EXPECT_EQ(index, 2);  // unchanged
}

// spec_id: SPEC-3  validates_req: REQ-005  tc: TC-REQ005-02
TEST(ExtractListSpecTest, SeparateList) {
  ArgvHolder holder{{"cut", "-f", "1,3", "file.txt"}};
  int index = 2;
  auto result = extract_list_spec("-f", holder.argc(), holder.argv(), index);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, "1,3");
  EXPECT_EQ(index, 3);  // advanced by 1
}

// spec_id: SPEC-3  validates_req: REQ-005  tc: TC-REQ005-03
TEST(ExtractListSpecTest, MissingListArgForF) {
  ArgvHolder holder{{"cut", "-f"}};
  int index = 2;
  auto result = extract_list_spec("-f", holder.argc(), holder.argv(), index);
  ASSERT_FALSE(result.has_value());
  EXPECT_NE(result.error().find("option requires an argument"), std::string::npos);
  EXPECT_NE(result.error().find("'f'"), std::string::npos);
}

// spec_id: SPEC-3  validates_req: REQ-005  tc: TC-REQ005-04
TEST(ExtractListSpecTest, MissingListArgForB) {
  ArgvHolder holder{{"cut", "-b"}};
  int index = 2;
  auto result = extract_list_spec("-b", holder.argc(), holder.argv(), index);
  ASSERT_FALSE(result.has_value());
  EXPECT_NE(result.error().find("option requires an argument"), std::string::npos);
  EXPECT_NE(result.error().find("'b'"), std::string::npos);
}

// ---------------------------------------------------------------------------
// REQ-006: parse_mode_properties
// ---------------------------------------------------------------------------

// spec_id: SPEC-3  validates_req: REQ-006  tc: TC-REQ006-01
TEST(ParseModePropertiesTest, ByteNoSplit) {
  ArgvHolder holder{{"cut", "-b1", "-n", "file.txt"}};
  CutOptions opts;
  opts.mode = CutMode::BYTE;
  int index = 0;
  // Reuse argv starting at position 0 of "-n", "file.txt"
  ArgvHolder sub{{"cut", "-n", "file.txt"}};
  int sub_index = 1;
  auto result = parse_mode_properties(sub.argc(), sub.argv(), sub_index, opts);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(opts.no_split);
  EXPECT_EQ(sub_index, 2);
}

// spec_id: SPEC-3  validates_req: REQ-006  tc: TC-REQ006-02
TEST(ParseModePropertiesTest, FieldDelimSeparate) {
  ArgvHolder sub{{"cut", "-d", ",", "file.txt"}};
  CutOptions opts;
  opts.mode = CutMode::FIELD;
  int index = 1;
  auto result = parse_mode_properties(sub.argc(), sub.argv(), index, opts);
  ASSERT_TRUE(result.has_value());
  ASSERT_TRUE(opts.delim.has_value());
  EXPECT_EQ(*opts.delim, ',');
  EXPECT_EQ(index, 3);
}

// spec_id: SPEC-3  validates_req: REQ-006  tc: TC-REQ006-03
TEST(ParseModePropertiesTest, FieldDelimAttached) {
  ArgvHolder sub{{"cut", "-d,", "file.txt"}};
  CutOptions opts;
  opts.mode = CutMode::FIELD;
  int index = 1;
  auto result = parse_mode_properties(sub.argc(), sub.argv(), index, opts);
  ASSERT_TRUE(result.has_value());
  ASSERT_TRUE(opts.delim.has_value());
  EXPECT_EQ(*opts.delim, ',');
  EXPECT_EQ(index, 2);
}

// spec_id: SPEC-3  validates_req: REQ-006  tc: TC-REQ006-04
TEST(ParseModePropertiesTest, FieldDelimMultiChar) {
  ArgvHolder sub{{"cut", "-d", ",,"}};
  CutOptions opts;
  opts.mode = CutMode::FIELD;
  int index = 1;
  auto result = parse_mode_properties(sub.argc(), sub.argv(), index, opts);
  ASSERT_FALSE(result.has_value());
  EXPECT_NE(result.error().find("the delimiter must be a single character"),
            std::string::npos);
}

// spec_id: SPEC-3  validates_req: REQ-006  tc: TC-REQ006-05
TEST(ParseModePropertiesTest, FieldSuppress) {
  ArgvHolder sub{{"cut", "-s", "file.txt"}};
  CutOptions opts;
  opts.mode = CutMode::FIELD;
  int index = 1;
  auto result = parse_mode_properties(sub.argc(), sub.argv(), index, opts);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(opts.suppress);
  EXPECT_EQ(index, 2);
}

// spec_id: SPEC-3  validates_req: REQ-006  tc: TC-REQ006-07
TEST(ParseModePropertiesTest, FieldDelimAndSuppress) {
  ArgvHolder sub{{"cut", "-d", ",", "-s", "file.txt"}};
  CutOptions opts;
  opts.mode = CutMode::FIELD;
  int index = 1;
  auto result = parse_mode_properties(sub.argc(), sub.argv(), index, opts);
  ASSERT_TRUE(result.has_value());
  ASSERT_TRUE(opts.delim.has_value());
  EXPECT_EQ(*opts.delim, ',');
  EXPECT_TRUE(opts.suppress);
  EXPECT_EQ(index, 4);
}

// spec_id: SPEC-3  validates_req: REQ-006  tc: TC-REQ006-08
TEST(ParseModePropertiesTest, ModeIrrelevantFlagNotConsumed) {
  ArgvHolder sub{{"cut", "-s", "file.txt"}};
  CutOptions opts;
  opts.mode = CutMode::BYTE;  // -s is not a BYTE property
  int index = 1;
  auto result = parse_mode_properties(sub.argc(), sub.argv(), index, opts);
  ASSERT_TRUE(result.has_value());
  EXPECT_FALSE(opts.suppress);
  EXPECT_EQ(index, 1);  // unchanged — -s treated as file arg start
}

// spec_id: SPEC-3  validates_req: REQ-006  tc: TC-REQ006-09
TEST(ParseModePropertiesTest, FieldDelimMissingArg) {
  ArgvHolder sub{{"cut", "-d"}};
  CutOptions opts;
  opts.mode = CutMode::FIELD;
  int index = 1;
  auto result = parse_mode_properties(sub.argc(), sub.argv(), index, opts);
  ASSERT_FALSE(result.has_value());
  EXPECT_NE(result.error().find("option requires an argument"), std::string::npos);
}

// spec_id: SPEC-3  validates_req: REQ-006  tc: TC-REQ006-06
TEST(ParseModePropertiesTest, CharModeNoOp) {
  ArgvHolder sub{{"cut", "file.txt"}};
  CutOptions opts;
  opts.mode = CutMode::CHARACTER;
  int index = 1;
  auto result = parse_mode_properties(sub.argc(), sub.argv(), index, opts);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(index, 1);
}

// ---------------------------------------------------------------------------
// REQ-007: collect_files
// ---------------------------------------------------------------------------

// spec_id: SPEC-3  validates_req: REQ-007  tc: TC-REQ007-01
TEST(CollectFilesTest, TwoDistinctFiles) {
  ArgvHolder holder{{"cut", "-f1", "a.txt", "b.txt"}};
  auto files = collect_files(holder.argc(), holder.argv(), 2);
  ASSERT_EQ(files.size(), 2U);
  EXPECT_EQ(files[0], "a.txt");
  EXPECT_EQ(files[1], "b.txt");
}

// spec_id: SPEC-3  validates_req: REQ-007  tc: TC-REQ007-02
TEST(CollectFilesTest, DeduplicatesFirstOccurrence) {
  ArgvHolder holder{{"cut", "-f1", "a.txt", "-", "b.txt", "-", "a.txt"}};
  auto files = collect_files(holder.argc(), holder.argv(), 2);
  ASSERT_EQ(files.size(), 3U);
  EXPECT_EQ(files[0], "a.txt");
  EXPECT_EQ(files[1], "-");
  EXPECT_EQ(files[2], "b.txt");
}

// spec_id: SPEC-3  validates_req: REQ-007  tc: TC-REQ007-03
TEST(CollectFilesTest, NoFiles) {
  ArgvHolder holder{{"cut", "-f1"}};
  auto files = collect_files(holder.argc(), holder.argv(), 2);
  EXPECT_TRUE(files.empty());
}

// spec_id: SPEC-3  validates_req: REQ-007  tc: TC-REQ007-04
TEST(CollectFilesTest, StdinDedup) {
  ArgvHolder holder{{"cut", "-f1", "-", "-"}};
  auto files = collect_files(holder.argc(), holder.argv(), 2);
  ASSERT_EQ(files.size(), 1U);
  EXPECT_EQ(files[0], "-");
}

// ---------------------------------------------------------------------------
// REQ-003: parse_args end-to-end
// ---------------------------------------------------------------------------

// spec_id: SPEC-3  validates_req: REQ-003  tc: TC-REQ003-01
TEST(ParseArgsTest, NoArgs) {
  ArgvHolder holder{{"cut"}};
  auto result = parse_args(holder.argc(), holder.argv());
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(),
    "cc-cut-tool: you must specify a list of bytes, characters, or fields\n"
    "Try 'cc-cut-tool --help' for more information.");
}

// spec_id: SPEC-3  validates_req: REQ-003  tc: TC-REQ003-02
TEST(ParseArgsTest, FieldModeSeparate) {
  ArgvHolder holder{{"cut", "-f", "1"}};
  auto result = parse_args(holder.argc(), holder.argv());
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->opts.mode, CutMode::FIELD);
  EXPECT_EQ(result->opts.list.indices, (std::set<int>{0}));
  EXPECT_TRUE(result->files.empty());
}

// spec_id: SPEC-3  validates_req: REQ-003  tc: TC-REQ003-03
TEST(ParseArgsTest, FilesCollected) {
  ArgvHolder holder{{"cut", "-f1", "a.txt", "b.txt"}};
  auto result = parse_args(holder.argc(), holder.argv());
  ASSERT_TRUE(result.has_value());
  ASSERT_EQ(result->files.size(), 2U);
  EXPECT_EQ(result->files[0], "a.txt");
  EXPECT_EQ(result->files[1], "b.txt");
}

// ---------------------------------------------------------------------------
// REQ-008: error message format
// ---------------------------------------------------------------------------

// spec_id: SPEC-3  validates_req: REQ-008  tc: TC-REQ008-01
TEST(ErrorFormatTest, StartsWithProgramName) {
  ArgvHolder holder{{"cut"}};
  auto result = parse_args(holder.argc(), holder.argv());
  ASSERT_FALSE(result.has_value());
  EXPECT_TRUE(result.error().starts_with("cc-cut-tool: "));
}

// spec_id: SPEC-3  validates_req: REQ-008  tc: TC-REQ008-02
TEST(ErrorFormatTest, EndsWithHelpHint) {
  ArgvHolder holder{{"cut"}};
  auto result = parse_args(holder.argc(), holder.argv());
  ASSERT_FALSE(result.has_value());
  EXPECT_TRUE(result.error().ends_with("Try 'cc-cut-tool --help' for more information."));
}

// spec_id: SPEC-3  validates_req: REQ-008  tc: TC-REQ008-03
TEST(ErrorFormatTest, ByteModeZeroPosition) {
  ArgvHolder holder{{"cut", "-b", "0"}};
  auto result = parse_args(holder.argc(), holder.argv());
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(),
    "cc-cut-tool: byte/character positions are numbered from 1\n"
    "Try 'cc-cut-tool --help' for more information.");
}

// spec_id: SPEC-3  validates_req: REQ-008  tc: TC-REQ008-04
TEST(ErrorFormatTest, FieldModeZeroPosition) {
  ArgvHolder holder{{"cut", "-f", "0"}};
  auto result = parse_args(holder.argc(), holder.argv());
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(),
    "cc-cut-tool: fields are numbered from 1\n"
    "Try 'cc-cut-tool --help' for more information.");
}

// spec_id: SPEC-3  validates_req: REQ-008  tc: TC-REQ008-05
TEST(ErrorFormatTest, CharModeZeroPosition) {
  ArgvHolder holder{{"cut", "-c", "0"}};
  auto result = parse_args(holder.argc(), holder.argv());
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(),
    "cc-cut-tool: byte/character positions are numbered from 1\n"
    "Try 'cc-cut-tool --help' for more information.");
}

// ---------------------------------------------------------------------------
// REQ-009: no mode specified
// ---------------------------------------------------------------------------

// spec_id: SPEC-3  validates_req: REQ-009  tc: TC-REQ009-01
TEST(NoModeTest, NoArguments) {
  ArgvHolder holder{{"cut"}};
  auto result = parse_args(holder.argc(), holder.argv());
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(),
    "cc-cut-tool: you must specify a list of bytes, characters, or fields\n"
    "Try 'cc-cut-tool --help' for more information.");
}

// spec_id: SPEC-3  validates_req: REQ-009  tc: TC-REQ009-02
TEST(NoModeTest, FileAsFirstArg) {
  ArgvHolder holder{{"cut", "file.txt"}};
  auto result = parse_args(holder.argc(), holder.argv());
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(),
    "cc-cut-tool: you must specify a list of bytes, characters, or fields\n"
    "Try 'cc-cut-tool --help' for more information.");
}

// spec_id: SPEC-3  validates_req: REQ-009  tc: TC-REQ009-03
TEST(NoModeTest, DoubleDash) {
  ArgvHolder holder{{"cut", "--"}};
  auto result = parse_args(holder.argc(), holder.argv());
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(),
    "cc-cut-tool: you must specify a list of bytes, characters, or fields\n"
    "Try 'cc-cut-tool --help' for more information.");
}

// ---------------------------------------------------------------------------
// REQ-010: --help flag
// ---------------------------------------------------------------------------

// spec_id: SPEC-3  validates_req: REQ-010  tc: TC-REQ010-01
TEST(HelpFlagTest, HelpReturnsSuccess) {
  ArgvHolder holder{{"cut", "--help"}};
  auto result = parse_args(holder.argc(), holder.argv());
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->help_requested);
}

// spec_id: SPEC-3  validates_req: REQ-010  tc: TC-REQ010-02
TEST(HelpFlagTest, HelpWinsOverOtherArgs) {
  ArgvHolder holder{{"cut", "--help", "-f1"}};
  auto result = parse_args(holder.argc(), holder.argv());
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->help_requested);
}

// TC-REQ010-03: stdout check for "Usage:" — done via manual smoke test
// (GoogleTest cannot easily capture stdout in cross-platform way without
// redirecting; verified by running: cut --help | head -1)
```

- [ ] **Step 3: Build — expect error (missing parse_result.hpp)**

```bash
cmake --build --preset clang-debug 2>&1 | grep "fatal error" | head -3
```

Expected: `fatal error: 'cut/arg_parser.hpp' file not found` (or similar)

- [ ] **Step 4: Commit test file**

```bash
git add tests/cut/arg_parser_test.cpp CMakeLists.txt
git commit -m "test(sp03): add arg parser unit tests"
```

- [ ] **Step 5: Create parse_result.hpp**

```cpp
// include/cut/parse_result.hpp
#pragma once
#include "cut/options.hpp"

#include <string>
#include <vector>

namespace cc_cut {

// spec_id: SPEC-3  req_id: REQ-001
/// Aggregated result of parsing CLI arguments.
///
/// `help_requested == true` signals the caller to exit 0 after
/// printing help — parse_args has already written to stdout.
///
/// @code
///   auto result = cc_cut::parse_args(argc, argv);
///   if (result && result->help_requested) { return 0; }
/// @endcode
struct ParseResult {
  CutOptions              opts;
  std::vector<std::string> files;
  bool                    help_requested = false;
};

}  // namespace cc_cut
```

- [ ] **Step 6: Build — expect next missing header**

```bash
cmake --build --preset clang-debug 2>&1 | grep "fatal error" | head -3
```

Expected: `fatal error: 'cut/arg_parser.hpp' file not found`

- [ ] **Step 7: Commit parse_result.hpp**

```bash
git add include/cut/parse_result.hpp
git commit -m "feat(sp03): add ParseResult struct"
```

---

### Task 3: Declare arg_parser.hpp

**Files:**
- Create: `include/cut/arg_parser.hpp`

- [ ] **Step 1: Write the header**

```cpp
// include/cut/arg_parser.hpp
#pragma once
#include "cut/options.hpp"
#include "cut/parse_result.hpp"

#include <expected>
#include <string>
#include <string_view>
#include <vector>

namespace cc_cut {

// spec_id: SPEC-3  req_id: REQ-003,REQ-010
/// Parse CLI arguments into a typed ParseResult.
///
/// Orchestrates detect_mode, extract_list_spec, parse_list,
/// parse_mode_properties, and collect_files in order. Returns
/// the first error encountered. Checks for --help before mode
/// detection; prints help and sets help_requested on match.
///
/// @param argc  Argument count (including program name at argv[0]).
/// @param argv  Null-terminated argument vector.
/// @return      ParseResult on success; error string on failure.
///              Error string format: "cc-cut-tool: <msg>\nTry '...'".
auto parse_args(int argc, char** argv) -> std::expected<ParseResult, std::string>;

// spec_id: SPEC-3  req_id: REQ-004
/// Identify the cut mode from the first CLI flag.
///
/// @param flag  The first argument (e.g. "-f", "-b3-5").
/// @return      CutMode on "-b"/"-c"/"-f"; error on anything else.
auto detect_mode(std::string_view flag) -> std::expected<CutMode, std::string>;

// spec_id: SPEC-3  req_id: REQ-005
/// Extract the list specification string from argv.
///
/// If `flag.size() > 2` (attached form, e.g. "-f1,3"), returns
/// `flag.substr(2)` and leaves `index` unchanged. Otherwise consumes
/// `argv[index]` as the list and increments `index` by 1.
///
/// @param flag   The mode flag (e.g. "-f" or "-f1,3") — argv[1].
/// @param argc   Total argument count.
/// @param argv   Argument vector.
/// @param index  Position of first arg after the flag (starts at 2).
/// @return       List spec string on success; error on missing arg.
/// @pre          index >= 2 (flag has been consumed as argv[1]).
auto extract_list_spec(std::string_view flag, int argc, char** argv, int& index)
    -> std::expected<std::string_view, std::string>;

// spec_id: SPEC-3  req_id: REQ-006
/// Consume zero or more mode-specific flags from argv[index..].
///
/// Advances `index` for each consumed flag. Stops when the argument
/// at argv[index] is not a recognised mode property; that argument
/// is left for collect_files to treat as a file path.
///
/// @param argc   Total argument count.
/// @param argv   Argument vector.
/// @param index  Current parse position; modified in-place.
/// @param opts   Options struct updated in-place.
/// @return       void on success; error if a property value is invalid.
auto parse_mode_properties(int argc, char** argv, int& index, CutOptions& opts)
    -> std::expected<void, std::string>;

// spec_id: SPEC-3  req_id: REQ-007
/// Collect remaining argv elements as file paths (first-occurrence dedup).
///
/// @param argc   Total argument count.
/// @param argv   Argument vector.
/// @param index  First position to collect from.
/// @return       Deduplicated file paths in first-occurrence order.
auto collect_files(int argc, char** argv, int index) -> std::vector<std::string>;

}  // namespace cc_cut
```

- [ ] **Step 2: Build — expect linker error (undefined symbols)**

```bash
cmake --build --preset clang-debug 2>&1 | grep "error:" | head -5
```

Expected: linker error about `cc_cut::parse_args` undefined symbol

- [ ] **Step 3: Commit header**

```bash
git add include/cut/arg_parser.hpp
git commit -m "feat(sp03): declare arg parser helpers in arg_parser.hpp"
```

---

### Task 4: Implement arg_parser.cpp (GREEN)

**Files:**
- Modify: `src/arg_parser.cpp`

- [ ] **Step 1: Write the implementation**

```cpp
// src/arg_parser.cpp
#include "cut/arg_parser.hpp"

#include "cut/config.hpp"
#include "cut/list_parser.hpp"

#include <algorithm>
#include <format>
#include <iostream>
#include <string>
#include <unordered_set>
#include <vector>

namespace cc_cut {

namespace {

auto format_error(std::string_view msg) -> std::string {
  return std::format("{}: {}\n{}", config::program_name, msg, config::help_hint);
}

auto translate_list_error(std::string_view list_err, CutMode mode) -> std::string {
  if (list_err == "values may not include zero") {
    if (mode == CutMode::FIELD) {
      return format_error("fields are numbered from 1");
    }
    return format_error("byte/character positions are numbered from 1");
  }
  return format_error(list_err);
}

auto print_help() -> void {
  std::cout << std::format(
      "Usage: {0} -b list [-n] [file ...]\n"
      "       {0} -c list [file ...]\n"
      "       {0} -f list [-d delim] [-s] [file ...]\n\n"
      "  -b list  Cut by byte positions\n"
      "  -c list  Cut by character positions (UTF-8)\n"
      "  -f list  Cut by fields\n"
      "  -d delim Field delimiter (default: tab)\n"
      "  -n       Do not split multi-byte characters (byte mode)\n"
      "  -s       Suppress lines with no delimiter (field mode)\n"
      "  --help   Show this help\n",
      config::program_name);
}

}  // anonymous namespace

// NOLINTNEXTLINE(misc-use-internal-linkage)
auto detect_mode(std::string_view flag) -> std::expected<CutMode, std::string> {
  if (flag.size() >= 2 && flag[0] == '-') {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
    switch (flag[1]) {
      case 'b': return CutMode::BYTE;
      case 'c': return CutMode::CHARACTER;
      case 'f': return CutMode::FIELD;
      default:
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
        return std::unexpected(
            format_error(std::format("invalid option -- '{}'", flag[1])));
    }
  }
  return std::unexpected(format_error("invalid option"));
}

// NOLINTNEXTLINE(misc-use-internal-linkage)
auto extract_list_spec(std::string_view flag, int argc, char** argv, int& index)
    -> std::expected<std::string_view, std::string>
{
  if (flag.size() > 2) {
    return flag.substr(2);
  }
  if (index >= argc) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
    return std::unexpected(
        format_error(std::format("option requires an argument -- '{}'", flag[1])));
  }
  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
  return std::string_view{argv[index++]};
}

// NOLINTNEXTLINE(misc-use-internal-linkage)
auto parse_mode_properties(int argc, char** argv, int& index, CutOptions& opts)
    -> std::expected<void, std::string>
{
  while (index < argc) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    const std::string_view arg{argv[index]};

    if (opts.mode == CutMode::BYTE && arg == "-n") {
      opts.no_split = true;
      ++index;
    } else if (opts.mode == CutMode::FIELD && arg == "-s") {
      opts.suppress = true;
      ++index;
    } else if (opts.mode == CutMode::FIELD && arg.size() >= 2 &&
               arg[0] == '-' && arg[1] == 'd') {
      if (arg.size() > 2) {
        // -d, (attached)
        auto delim_str = arg.substr(2);
        if (delim_str.size() != 1) {
          return std::unexpected(
              format_error("the delimiter must be a single character"));
        }
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
        opts.delim = delim_str[0];
        ++index;
      } else {
        // -d , (separate)
        if (index + 1 >= argc) {
          return std::unexpected(
              format_error("option requires an argument -- 'd'"));
        }
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
        const std::string_view delim_str{argv[index + 1]};
        if (delim_str.size() != 1) {
          return std::unexpected(
              format_error("the delimiter must be a single character"));
        }
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
        opts.delim = delim_str[0];
        index += 2;
      }
    } else {
      break;
    }
  }
  return {};
}

// NOLINTNEXTLINE(misc-use-internal-linkage)
auto collect_files(int argc, char** argv, int index) -> std::vector<std::string> {
  std::vector<std::string> files;
  std::unordered_set<std::string> seen;
  for (int i = index; i < argc; ++i) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    std::string path{argv[i]};
    if (seen.insert(path).second) {
      files.push_back(std::move(path));
    }
  }
  return files;
}

// NOLINTNEXTLINE(misc-use-internal-linkage)
auto parse_args(int argc, char** argv) -> std::expected<ParseResult, std::string> {
  // Check --help before anything else
  for (int i = 1; i < argc; ++i) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    if (std::string_view{argv[i]} == "--help") {
      print_help();
      ParseResult help_result;
      help_result.help_requested = true;
      return help_result;
    }
  }

  // Require a mode flag
  static constexpr std::string_view no_mode_err =
      "you must specify a list of bytes, characters, or fields";

  if (argc < 2) {
    return std::unexpected(format_error(no_mode_err));
  }

  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
  const std::string_view first{argv[1]};

  // Reject anything that doesn't look like -b/-c/-f
  if (first.size() < 2 || first[0] != '-' || first == "--") {
    return std::unexpected(format_error(no_mode_err));
  }

  // Unknown flag (-x) → propagate detect_mode's "invalid option" error.
  // Non-flag and "--" are already caught above.
  auto mode_result = detect_mode(first);
  if (!mode_result) {
    return std::unexpected(mode_result.error());
  }

  ParseResult result;
  result.opts.mode = *mode_result;

  int index = 2;

  auto spec_result = extract_list_spec(first, argc, argv, index);
  if (!spec_result) return std::unexpected(spec_result.error());

  auto list_result = parse_list(*spec_result);
  if (!list_result) {
    return std::unexpected(translate_list_error(list_result.error(), *mode_result));
  }
  result.opts.list = *list_result;

  auto props_result = parse_mode_properties(argc, argv, index, result.opts);
  if (!props_result) return std::unexpected(props_result.error());

  result.files = collect_files(argc, argv, index);

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

Expected: all tests pass. Count will be 44 (existing) + new arg_parser tests.

- [ ] **Step 4: Commit implementation**

```bash
git add src/arg_parser.cpp
git commit -m "feat(sp03): implement parse_args and helpers

parse_args has no mode-detection knowledge — it delegates to helpers
and translates parse_list's generic zero-position error into
mode-specific messages so the CLI error matches coreutils output."
```

---

## Spec Coverage

| REQ | Task |
|-----|------|
| REQ-001 (ParseResult) | Task 2 (parse_result.hpp + tests) |
| REQ-002 (config.hpp) | Task 1 (config.hpp.in + CMake) + Task 2 (TC-REQ002-*) |
| REQ-003 (parse_args) | Task 4 (arg_parser.cpp) + Task 2 (TC-REQ003-*) |
| REQ-004 (detect_mode) | Task 3+4 (arg_parser.hpp + .cpp) + Task 2 (TC-REQ004-*) |
| REQ-005 (extract_list_spec) | Task 3+4 + Task 2 (TC-REQ005-*) |
| REQ-006 (parse_mode_properties) | Task 3+4 + Task 2 (TC-REQ006-*) |
| REQ-007 (collect_files) | Task 3+4 + Task 2 (TC-REQ007-*) |
| REQ-008 (error format) | Task 4 (format_error helper) + Task 2 (TC-REQ008-*) |
| REQ-009 (no mode) | Task 4 (parse_args guard) + Task 2 (TC-REQ009-*) |
| REQ-010 (--help) | Task 4 (print_help + help_requested) + Task 2 (TC-REQ010-*) |
