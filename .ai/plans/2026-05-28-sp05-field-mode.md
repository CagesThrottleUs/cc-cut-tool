# SP-05: Field Mode — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use `subagent-driven-development` (recommended) or `executing-plans` to implement task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement `FieldProcessor` with split/select/process/run methods and wire `main.cpp` so the tool processes `-f` mode end-to-end.

**Architecture:** `FieldProcessor` owns `CutOptions`. Two pure static helpers (`split_fields` × 2, `select_fields`) are independently testable. `process_line` composes them and writes to `std::ostream&`. `run` iterates files, calling `process_line` per line. All indexing uses range-for or `span.subspan().front()` to avoid operator[] suppressions.

**Tech Stack:** C++23, GoogleTest, CMake 4.0, Boost.Iostreams (already linked)

**Spec:** `.ai/specs/2026-05-28-sp05-field-mode-design.md` (SPEC-5)

**Key assumptions (surface before coding):**
- `split_fields("", delim)` → one empty-string field (not zero fields) per REQ-002
- Positions beyond `fields.size()` in `select_fields` → empty `string_view` per REQ-004
- `opts_.delim.has_value()` is always true from CLI (SP-03 sets `some('\t')` by default)

**File map:**
```
Created:
  include/cut/field_processor.hpp
  src/field_processor.cpp
  tests/cut/field_processor_test.cpp

Modified:
  CMakeLists.txt
  src/main.cpp
```

---

### Task 1: CMakeLists wiring + placeholders

**Files:**
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Add field_processor.cpp to SRC_FILES**

```cmake
set(SRC_FILES
  src/list_parser.cpp
  src/arg_parser.cpp
  src/stdin_source.cpp
  src/buffered_file_source.cpp
  src/mmap_file_source.cpp
  src/make_file_source.cpp
  src/field_processor.cpp
)
```

- [ ] **Step 2: Add test file to test executable**

```cmake
add_executable(${TEST_PROJECT_NAME}
  tests/cut/mode_test.cpp
  tests/cut/list_test.cpp
  tests/cut/options_test.cpp
  tests/cut/file_source_test.cpp
  tests/cut/list_parser_test.cpp
  tests/cut/arg_parser_test.cpp
  tests/cut/file_interface_test.cpp
  tests/cut/field_processor_test.cpp
  ${SRC_FILES}
)
```

- [ ] **Step 3: Create placeholder files**

```bash
printf '// placeholder\n' > src/field_processor.cpp
printf '// placeholder\n' > tests/cut/field_processor_test.cpp
```

- [ ] **Step 4: Verify CMake configures**

```bash
cd /Users/laksh/Documents/PersonalCode/cc-cut-tool && cmake --preset clang-debug 2>&1 | tail -3
```

Expected: `Build files have been written to: ...clang-debug`

- [ ] **Step 5: Commit**

```bash
git add CMakeLists.txt src/field_processor.cpp tests/cut/field_processor_test.cpp
git commit -m "build(sp05): add field_processor to CMake sources and test target"
```

Body:
```
field_processor.cpp must be in SRC_FILES before implementing so both
the main binary and the test binary link against the same object. The
placeholder satisfies CMake 4.0 configure-time source existence.
```

---

### Task 2: Write failing test file (RED)

**Files:**
- Modify: `tests/cut/field_processor_test.cpp`

- [ ] **Step 1: Write the complete test file**

```cpp
// spec_id: SPEC-5  validates_req: REQ-001,REQ-002,REQ-003,REQ-004,REQ-005,REQ-006,REQ-007
#include "cut/field_processor.hpp"

#include <gtest/gtest.h>

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

using cc_cut::CutList;
using cc_cut::CutMode;
using cc_cut::CutOptions;
using cc_cut::FieldProcessor;

// ---------------------------------------------------------------------------
// REQ-001: FieldProcessor class
// ---------------------------------------------------------------------------

// spec_id: SPEC-5  validates_req: REQ-001  tc: TC-REQ001-01
TEST(FieldProcessorTest, ConstructsFromCutOptions) {
  CutOptions opts;
  EXPECT_NO_THROW(FieldProcessor{opts});
}

// spec_id: SPEC-5  validates_req: REQ-001  tc: TC-REQ001-02
static_assert(std::is_constructible_v<FieldProcessor, CutOptions>,
              "TC-REQ001-02: FieldProcessor must be constructible from CutOptions");

// ---------------------------------------------------------------------------
// REQ-002: split_fields — exact delimiter
// ---------------------------------------------------------------------------

// spec_id: SPEC-5  validates_req: REQ-002  tc: TC-REQ002-01
TEST(SplitFieldsExactTest, ThreeTokens) {
  const auto fields = FieldProcessor::split_fields("a,b,c", ',');
  ASSERT_EQ(fields.size(), 3U);
  EXPECT_EQ(fields[0], "a");
  EXPECT_EQ(fields[1], "b");
  EXPECT_EQ(fields[2], "c");
}

// spec_id: SPEC-5  validates_req: REQ-002  tc: TC-REQ002-02
TEST(SplitFieldsExactTest, EmptyMiddleField) {
  const auto fields = FieldProcessor::split_fields("a,,c", ',');
  ASSERT_EQ(fields.size(), 3U);
  EXPECT_EQ(fields[1], "");
}

// spec_id: SPEC-5  validates_req: REQ-002  tc: TC-REQ002-03
TEST(SplitFieldsExactTest, LeadingDelimiter) {
  const auto fields = FieldProcessor::split_fields(",a", ',');
  ASSERT_EQ(fields.size(), 2U);
  EXPECT_EQ(fields[0], "");
}

// spec_id: SPEC-5  validates_req: REQ-002  tc: TC-REQ002-04
TEST(SplitFieldsExactTest, EmptyString) {
  const auto fields = FieldProcessor::split_fields("", ',');
  ASSERT_EQ(fields.size(), 1U);
  EXPECT_EQ(fields[0], "");
}

// spec_id: SPEC-5  validates_req: REQ-002  tc: TC-REQ002-05
TEST(SplitFieldsExactTest, NoDelimiter) {
  const auto fields = FieldProcessor::split_fields("no-delim", ',');
  ASSERT_EQ(fields.size(), 1U);
  EXPECT_EQ(fields[0], "no-delim");
}

// ---------------------------------------------------------------------------
// REQ-003: split_fields — whitespace delimiter
// ---------------------------------------------------------------------------

// spec_id: SPEC-5  validates_req: REQ-003  tc: TC-REQ003-01
TEST(SplitFieldsWhitespaceTest, ThreeTokens) {
  const auto fields = FieldProcessor::split_fields("a b c");
  ASSERT_EQ(fields.size(), 3U);
}

// spec_id: SPEC-5  validates_req: REQ-003  tc: TC-REQ003-02
TEST(SplitFieldsWhitespaceTest, LeadingTrailingStripped) {
  const auto fields = FieldProcessor::split_fields("  a  b  ");
  ASSERT_EQ(fields.size(), 2U);
  EXPECT_EQ(fields[0], "a");
}

// spec_id: SPEC-5  validates_req: REQ-003  tc: TC-REQ003-03
TEST(SplitFieldsWhitespaceTest, TabSeparated) {
  const auto fields = FieldProcessor::split_fields("a\tb");
  ASSERT_EQ(fields.size(), 2U);
}

// spec_id: SPEC-5  validates_req: REQ-003  tc: TC-REQ003-04
TEST(SplitFieldsWhitespaceTest, EmptyString) {
  EXPECT_EQ(FieldProcessor::split_fields("").size(), 0U);
}

// spec_id: SPEC-5  validates_req: REQ-003  tc: TC-REQ003-05
TEST(SplitFieldsWhitespaceTest, WhitespaceOnly) {
  EXPECT_EQ(FieldProcessor::split_fields("   ").size(), 0U);
}

// ---------------------------------------------------------------------------
// REQ-004: select_fields
// ---------------------------------------------------------------------------

namespace {
auto make_fields(std::initializer_list<const char*> strs)
    -> std::vector<std::string_view> {
  return {strs.begin(), strs.end()};
}
auto make_list(std::initializer_list<int> idx,
               std::optional<int> open = std::nullopt) -> CutList {
  CutList list;
  list.indices = std::set<int>{idx.begin(), idx.end()};
  list.open_from = open;
  return list;
}
}  // namespace

// spec_id: SPEC-5  validates_req: REQ-004  tc: TC-REQ004-01
TEST(SelectFieldsTest, SingleIndex) {
  const auto result = FieldProcessor::select_fields(make_fields({"a","b","c"}),
                                                    make_list({0}));
  ASSERT_EQ(result.size(), 1U);
  EXPECT_EQ(result[0], "a");
}

// spec_id: SPEC-5  validates_req: REQ-004  tc: TC-REQ004-02
TEST(SelectFieldsTest, TwoIndices) {
  const auto result = FieldProcessor::select_fields(make_fields({"a","b","c"}),
                                                    make_list({0,2}));
  ASSERT_EQ(result.size(), 2U);
  EXPECT_EQ(result[0], "a");
  EXPECT_EQ(result[1], "c");
}

// spec_id: SPEC-5  validates_req: REQ-004  tc: TC-REQ004-03
TEST(SelectFieldsTest, OpenFrom) {
  const auto result = FieldProcessor::select_fields(make_fields({"a","b","c"}),
                                                    make_list({}, 1));
  ASSERT_EQ(result.size(), 2U);
  EXPECT_EQ(result[0], "b");
  EXPECT_EQ(result[1], "c");
}

// spec_id: SPEC-5  validates_req: REQ-004  tc: TC-REQ004-04
TEST(SelectFieldsTest, MissingFieldBecomesEmpty) {
  const auto result = FieldProcessor::select_fields(make_fields({"a"}),
                                                    make_list({0,1}));
  ASSERT_EQ(result.size(), 2U);
  EXPECT_EQ(result[0], "a");
  EXPECT_EQ(result[1], "");
}

// spec_id: SPEC-5  validates_req: REQ-004  tc: TC-REQ004-05
TEST(SelectFieldsTest, IndicesAndOpenFromMerged) {
  const auto result = FieldProcessor::select_fields(make_fields({"a","b","c"}),
                                                    make_list({0}, 1));
  ASSERT_EQ(result.size(), 3U);
  EXPECT_EQ(result[0], "a");
  EXPECT_EQ(result[1], "b");
  EXPECT_EQ(result[2], "c");
}

// ---------------------------------------------------------------------------
// REQ-005: process_line
// ---------------------------------------------------------------------------

namespace {
auto make_opts_field(char delim, std::initializer_list<int> idx,
                     std::optional<int> open = std::nullopt,
                     bool suppress = false) -> CutOptions {
  CutOptions opts;
  opts.mode = CutMode::FIELD;
  opts.delim = delim;
  opts.suppress = suppress;
  opts.list.indices = std::set<int>{idx.begin(), idx.end()};
  opts.list.open_from = open;
  return opts;
}
}  // namespace

// spec_id: SPEC-5  validates_req: REQ-005  tc: TC-REQ005-01
TEST(ProcessLineTest, SingleFieldSelected) {
  std::ostringstream out;
  FieldProcessor{make_opts_field(',', {1})}.process_line("a,b,c", out);
  EXPECT_EQ(out.str(), "b\n");
}

// spec_id: SPEC-5  validates_req: REQ-005  tc: TC-REQ005-02
TEST(ProcessLineTest, TwoFieldsJoinedWithDelim) {
  std::ostringstream out;
  FieldProcessor{make_opts_field(',', {0,2})}.process_line("a,b,c", out);
  EXPECT_EQ(out.str(), "a,c\n");
}

// spec_id: SPEC-5  validates_req: REQ-005  tc: TC-REQ005-03
TEST(ProcessLineTest, SuppressNoDelimiter) {
  std::ostringstream out;
  FieldProcessor{make_opts_field(',', {0}, std::nullopt, true)}.process_line("hello", out);
  EXPECT_EQ(out.str(), "");
}

// spec_id: SPEC-5  validates_req: REQ-005  tc: TC-REQ005-04
TEST(ProcessLineTest, PassThroughNoDelimiter) {
  std::ostringstream out;
  FieldProcessor{make_opts_field(',', {0})}.process_line("hello", out);
  EXPECT_EQ(out.str(), "hello\n");
}

// spec_id: SPEC-5  validates_req: REQ-005  tc: TC-REQ005-05
TEST(ProcessLineTest, OpenEndRange) {
  std::ostringstream out;
  FieldProcessor{make_opts_field(',', {}, 1)}.process_line("a,b,c", out);
  EXPECT_EQ(out.str(), "b,c\n");
}

// spec_id: SPEC-5  validates_req: REQ-005  tc: TC-REQ005-06
TEST(ProcessLineTest, MissingFieldEmptyInOutput) {
  std::ostringstream out;
  FieldProcessor{make_opts_field(',', {0,3})}.process_line("a,b", out);
  EXPECT_EQ(out.str(), "a,\n");
}

// ---------------------------------------------------------------------------
// REQ-006: run — file loop
// ---------------------------------------------------------------------------

namespace {
auto make_temp_file(const std::string& content) -> std::filesystem::path {
  const auto path =
      std::filesystem::temp_directory_path() / "sp05_test.tmp";
  std::ofstream ofs{path};
  ofs << content;
  return path;
}
}  // namespace

// spec_id: SPEC-5  validates_req: REQ-006  tc: TC-REQ006-01
TEST(RunTest, ValidFileReturnsZero) {
  const auto path = make_temp_file("a,b\n");
  std::ostringstream out;
  std::ostringstream err;
  const int rc = FieldProcessor{make_opts_field(',', {0})}.run(
      {path.string()}, out, err);
  EXPECT_EQ(rc, 0);
  EXPECT_EQ(out.str(), "a\n");
  std::filesystem::remove(path);
}

// spec_id: SPEC-5  validates_req: REQ-006  tc: TC-REQ006-02
TEST(RunTest, NonExistentFileReturnsOne) {
  std::ostringstream out;
  std::ostringstream err;
  const int rc = FieldProcessor{make_opts_field(',', {0})}.run(
      {"/tmp/sp05_no_such.txt"}, out, err);
  EXPECT_EQ(rc, 1);
  EXPECT_NE(err.str().find("/tmp/sp05_no_such.txt"), std::string::npos);
}

// spec_id: SPEC-5  validates_req: REQ-006  tc: TC-REQ006-03
TEST(RunTest, ContinuesAfterOneError) {
  const auto path = make_temp_file("x,y\n");
  std::ostringstream out;
  std::ostringstream err;
  const int rc = FieldProcessor{make_opts_field(',', {0})}.run(
      {"/tmp/sp05_no_such.txt", path.string()}, out, err);
  EXPECT_EQ(rc, 1);       // error occurred
  EXPECT_EQ(out.str(), "x\n");  // second file still processed
  std::filesystem::remove(path);
}

// spec_id: SPEC-5  validates_req: REQ-006  tc: TC-REQ006-04
TEST(RunTest, EmptyFilesListReadStdin) {
  std::istringstream input_ss{"a,b\n"};
  const auto old_buf = std::cin.rdbuf(input_ss.rdbuf());
  std::ostringstream out;
  std::ostringstream err;
  const int rc = FieldProcessor{make_opts_field(',', {0})}.run({}, out, err);
  std::cin.rdbuf(old_buf);
  EXPECT_EQ(rc, 0);
  EXPECT_EQ(out.str(), "a\n");
}
```

- [ ] **Step 2: Build — verify RED (missing header)**

```bash
cmake --build --preset clang-debug 2>&1 | grep "fatal error" | head -3
```

Expected: `fatal error: 'cut/field_processor.hpp' file not found`

- [ ] **Step 3: Commit test file**

```bash
git add tests/cut/field_processor_test.cpp
git commit -m "test(sp05): add field processor unit tests"
```

---

### Task 3: Declare field_processor.hpp

**Files:**
- Create: `include/cut/field_processor.hpp`

- [ ] **Step 1: Write the header**

```cpp
// include/cut/field_processor.hpp
#pragma once
#include "cut/list.hpp"
#include "cut/options.hpp"

#include <iosfwd>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace cc_cut {

// spec_id: SPEC-5  req_id: REQ-001,REQ-002,REQ-003,REQ-004,REQ-005,REQ-006
/// Processes cut's field mode (-f) for a given set of CutOptions.
///
/// Static helpers split_fields and select_fields are pure and independently
/// testable. process_line and run perform I/O through std::ostream references.
///
/// @note opts.delim is expected to have a value for all CLI-reachable paths
///       in SP-05; whitespace-mode output (delim=nullopt) is out of scope.
class FieldProcessor {
 public:
  explicit FieldProcessor(CutOptions opts);

  // spec_id: SPEC-5  req_id: REQ-002
  /// Splits line on every occurrence of delim. Empty fields are preserved.
  /// An empty line produces exactly one empty-string field.
  static auto split_fields(std::string_view line, char delim)
      -> std::vector<std::string_view>;

  // spec_id: SPEC-5  req_id: REQ-003
  /// Splits line on runs of ASCII space/tab. Leading/trailing whitespace and
  /// consecutive whitespace produce no empty fields.
  static auto split_fields(std::string_view line)
      -> std::vector<std::string_view>;

  // spec_id: SPEC-5  req_id: REQ-004
  /// Returns fields selected by list in ascending position order.
  /// Positions beyond fields.size() produce empty string_view entries.
  static auto select_fields(const std::vector<std::string_view>& fields,
                             const CutList& list)
      -> std::vector<std::string_view>;

  // spec_id: SPEC-5  req_id: REQ-005
  /// Processes one line: splits, selects, joins with delimiter, writes to out.
  /// If no delimiter found and suppress=true: writes nothing.
  /// If no delimiter found and suppress=false: writes line unchanged + '\n'.
  void process_line(std::string_view line, std::ostream& out);

  // spec_id: SPEC-5  req_id: REQ-006
  /// Processes all files (or stdin if files is empty).
  /// Continues past individual file errors; writes errors to err.
  /// Returns 0 on full success, 1 if any file error occurred.
  auto run(const std::vector<std::string>& files, std::ostream& out,
           std::ostream& err) -> int;

 private:
  CutOptions opts_;
};

}  // namespace cc_cut
```

- [ ] **Step 2: Build — verify error shifts to linker**

```bash
cmake --build --preset clang-debug 2>&1 | grep "error:" | head -3
```

Expected: linker error about undefined `cc_cut::FieldProcessor` symbols

- [ ] **Step 3: Commit header**

```bash
git add include/cut/field_processor.hpp
git commit -m "feat(sp05): declare FieldProcessor in field_processor.hpp"
```

---

### Task 4: Implement field_processor.cpp (GREEN)

**Files:**
- Modify: `src/field_processor.cpp`

- [ ] **Step 1: Write the implementation**

```cpp
// src/field_processor.cpp
#include "cut/field_processor.hpp"

#include "cut/config.hpp"
#include "cut/list.hpp"
#include "cut/make_file_source.hpp"
#include "cut/options.hpp"

#include <climits>
#include <cstddef>
#include <ios>
#include <iosfwd>
#include <ostream>
#include <set>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace cc_cut {

FieldProcessor::FieldProcessor(CutOptions opts) : opts_(std::move(opts)) {}

auto FieldProcessor::split_fields(std::string_view line, char delim)
    -> std::vector<std::string_view> {
  std::vector<std::string_view> fields;
  std::size_t start = 0;
  while (true) {
    const auto pos = line.find(delim, start);
    if (pos == std::string_view::npos) {
      fields.push_back(line.substr(start));
      break;
    }
    fields.push_back(line.substr(start, pos - start));
    start = pos + 1;
  }
  return fields;
}

auto FieldProcessor::split_fields(std::string_view line)
    -> std::vector<std::string_view> {
  std::vector<std::string_view> fields;
  std::size_t pos = 0;
  while (pos < line.size()) {
    const auto first = line.find_first_not_of(" \t", pos);
    if (first == std::string_view::npos) { break; }
    const auto end = line.find_first_of(" \t", first);
    if (end == std::string_view::npos) {
      fields.push_back(line.substr(first));
      break;
    }
    fields.push_back(line.substr(first, end - first));
    pos = end;
  }
  return fields;
}

auto FieldProcessor::select_fields(const std::vector<std::string_view>& fields,
                                   const CutList& list)
    -> std::vector<std::string_view> {
  if (list.indices.empty() && !list.open_from.has_value()) {
    return {};
  }

  // Upper bound of positions to consider
  int max_pos = -1;
  if (!list.indices.empty()) {
    max_pos = *list.indices.rbegin();
  }
  if (list.open_from.has_value()) {
    max_pos = std::max(max_pos, static_cast<int>(fields.size()) - 1);
  }
  if (max_pos < 0) { return {}; }

  const int open_start = list.open_from.value_or(max_pos + 1);
  const auto fspan = std::span<const std::string_view>{fields};

  std::vector<std::string_view> result;
  for (int pos = 0; pos <= max_pos; ++pos) {
    if (!list.indices.contains(pos) && pos < open_start) { continue; }
    const auto idx = static_cast<std::size_t>(pos);
    result.push_back(idx < fspan.size() ? fspan.subspan(idx).front()
                                        : std::string_view{});
  }
  return result;
}

void FieldProcessor::process_line(std::string_view line, std::ostream& out) {
  // Check whether the line contains the delimiter
  const bool has_delim = opts_.delim.has_value()
      ? line.contains(opts_.delim.value())
      : line.find_first_of(" \t") != std::string_view::npos;

  if (!has_delim) {
    if (!opts_.suppress) { out << line << '\n'; }
    return;
  }

  const auto fields = opts_.delim.has_value()
      ? split_fields(line, opts_.delim.value())
      : split_fields(line);

  const auto selected = select_fields(fields, opts_.list);

  const char join_char = opts_.delim.value_or(' ');
  bool first = true;
  for (const auto& field : selected) {
    if (!first) { out << join_char; }
    out << field;
    first = false;
  }
  out << '\n';
}

auto FieldProcessor::run(const std::vector<std::string>& files,
                         std::ostream& out, std::ostream& err) -> int {
  int exit_code = 0;

  const auto process_source = [&](const std::string& path) {
    auto source_result = make_file_source(path);
    if (!source_result) {
      err << source_result.error() << '\n';
      exit_code = 1;
      return;
    }
    try {
      (*source_result)->load();
    } catch (const std::ios_base::failure& ex) {
      err << config::program_name << ": " << path << ": " << ex.what() << '\n';
      exit_code = 1;
      return;
    }
    while (auto line = (*source_result)->getline()) {
      process_line(*line, out);
    }
  };

  if (files.empty()) {
    process_source("-");
  } else {
    for (const auto& path : files) {
      process_source(path);
    }
  }

  return exit_code;
}

}  // namespace cc_cut
```

- [ ] **Step 2: Build — expect zero errors**

```bash
cmake --build --preset clang-debug 2>&1 | grep -E "error:|warning:" | head -5
```

Expected: no output

- [ ] **Step 3: Run field_processor tests**

```bash
ctest --test-dir out/build/clang-debug --output-on-failure \
  -R "FieldProcessor|SplitFields|SelectFields|ProcessLine|RunTest" 2>&1 | tail -5
```

Expected: all pass

- [ ] **Step 4: Run clang-tidy**

```bash
clang-tidy -p out/build/clang-debug src/field_processor.cpp 2>&1 \
  | grep -E "error:|warning:" | head -10 && echo "TIDY DONE"
```

Expected: `TIDY DONE` with zero warnings

- [ ] **Step 5: Commit**

```bash
git add src/field_processor.cpp include/cut/field_processor.hpp
git commit -m "feat(sp05): implement FieldProcessor field mode"
```

Body:
```
Without field processing, the tool accepted valid CLI arguments but
produced no output. FieldProcessor closes the gap between parse_args
(SP-03) and file reading (SP-04) by splitting, selecting, and writing
fields per the CutOptions contract.
```

---

### Task 5: Wire main.cpp

**Files:**
- Modify: `src/main.cpp`

- [ ] **Step 1: Write the new main.cpp**

```cpp
// src/main.cpp
#include "cut/arg_parser.hpp"
#include "cut/field_processor.hpp"
#include "cut/mode.hpp"

#include <iostream>

// spec_id: SPEC-5  req_id: REQ-007
auto main(int argc, char** argv) -> int {
  auto result = cc_cut::parse_args(argc, argv);
  if (!result) {
    std::cerr << result.error() << '\n';
    return 1;
  }
  if (result->help_requested) {
    return 0;
  }
  if (result->opts.mode != cc_cut::CutMode::FIELD) {
    std::cerr << "cc-cut-tool: byte and character modes not yet implemented\n";
    return 1;
  }
  return cc_cut::FieldProcessor{result->opts}.run(result->files, std::cout,
                                                   std::cerr);
}
```

- [ ] **Step 2: Build and run all tests**

```bash
cmake --build --preset clang-debug 2>&1 | grep -E "error:|warning:" | head -5 && \
ctest --test-dir out/build/clang-debug --output-on-failure --exclude-regex "integ_" 2>&1 | tail -4
```

Expected: zero errors, all tests pass (112 existing + new field_processor tests)

- [ ] **Step 3: Smoke test the binary (TC-REQ007-01)**

```bash
echo "a,b,c" | out/build/clang-debug/cc-cut-tool -f2 -d, ; echo "exit: $?"
```

Expected: `b` printed, `exit: 0`

- [ ] **Step 4: Smoke test --help (TC-REQ007-02)**

```bash
out/build/clang-debug/cc-cut-tool --help ; echo "exit: $?"
```

Expected: usage printed to stdout, `exit: 0`

- [ ] **Step 5: Smoke test not-yet-implemented (TC-REQ007-04)**

```bash
out/build/clang-debug/cc-cut-tool -b1 /dev/null 2>&1 ; echo "exit: $?"
```

Expected: `cc-cut-tool: byte and character modes not yet implemented`, `exit: 1`

- [ ] **Step 6: Commit**

```bash
git add src/main.cpp
git commit -m "feat(sp05): wire main.cpp to FieldProcessor"
```

Body:
```
main.cpp was a stub that printed "Hello". Wiring it to parse_args and
FieldProcessor::run makes the binary usable for the first time — field
mode cuts are now functional. Byte and character modes return exit 1
with an explicit message until SP-06 and SP-07 implement them.
```

---

## Spec Coverage

| REQ | Task |
|-----|------|
| REQ-001 (FieldProcessor class) | Task 3 (header) + Task 2 (TC-REQ001-*) |
| REQ-002 (split_fields exact) | Task 4 (impl) + Task 2 (TC-REQ002-*) |
| REQ-003 (split_fields whitespace) | Task 4 (impl) + Task 2 (TC-REQ003-*) |
| REQ-004 (select_fields) | Task 4 (impl) + Task 2 (TC-REQ004-*) |
| REQ-005 (process_line) | Task 4 (impl) + Task 2 (TC-REQ005-*) |
| REQ-006 (run loop) | Task 4 (impl) + Task 2 (TC-REQ006-*) |
| REQ-007 (main.cpp wiring) | Task 5 (main.cpp) + smoke tests |
