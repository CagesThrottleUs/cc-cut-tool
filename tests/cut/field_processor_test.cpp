// spec_id: SPEC-5  validates_req: REQ-001,REQ-002,REQ-003,REQ-004,REQ-005,REQ-006,REQ-007
#include "cut/field_processor.hpp"
#include "cut/processor.hpp"

#include <gtest/gtest.h>

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
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

// spec_id: SPEC-6  validates_req: REQ-006  tc: TC-REQ006-05
static_assert(std::is_base_of_v<cc_cut::Processor, cc_cut::FieldProcessor>,
              "TC-REQ006-05: FieldProcessor must inherit Processor");

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
  const auto result =
      FieldProcessor::select_fields(make_fields({"a", "b", "c"}), make_list({0}));
  ASSERT_EQ(result.size(), 1U);
  EXPECT_EQ(result[0], "a");
}

// spec_id: SPEC-5  validates_req: REQ-004  tc: TC-REQ004-02
TEST(SelectFieldsTest, TwoIndices) {
  const auto result =
      FieldProcessor::select_fields(make_fields({"a", "b", "c"}), make_list({0, 2}));
  ASSERT_EQ(result.size(), 2U);
  EXPECT_EQ(result[0], "a");
  EXPECT_EQ(result[1], "c");
}

// spec_id: SPEC-5  validates_req: REQ-004  tc: TC-REQ004-03
TEST(SelectFieldsTest, OpenFrom) {
  const auto result =
      FieldProcessor::select_fields(make_fields({"a", "b", "c"}), make_list({}, 1));
  ASSERT_EQ(result.size(), 2U);
  EXPECT_EQ(result[0], "b");
  EXPECT_EQ(result[1], "c");
}

// spec_id: SPEC-5  validates_req: REQ-004  tc: TC-REQ004-04
TEST(SelectFieldsTest, MissingFieldBecomesEmpty) {
  const auto result =
      FieldProcessor::select_fields(make_fields({"a"}), make_list({0, 1}));
  ASSERT_EQ(result.size(), 2U);
  EXPECT_EQ(result[0], "a");
  EXPECT_EQ(result[1], "");
}

// spec_id: SPEC-5  validates_req: REQ-004  tc: TC-REQ004-05
TEST(SelectFieldsTest, IndicesAndOpenFromMerged) {
  const auto result =
      FieldProcessor::select_fields(make_fields({"a", "b", "c"}), make_list({0}, 1));
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
  FieldProcessor{make_opts_field(',', {0, 2})}.process_line("a,b,c", out);
  EXPECT_EQ(out.str(), "a,c\n");
}

// spec_id: SPEC-5  validates_req: REQ-005  tc: TC-REQ005-03
TEST(ProcessLineTest, SuppressNoDelimiter) {
  std::ostringstream out;
  FieldProcessor{make_opts_field(',', {0}, std::nullopt, true)}.process_line("hello",
                                                                              out);
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
  FieldProcessor{make_opts_field(',', {0, 3})}.process_line("a,b", out);
  EXPECT_EQ(out.str(), "a,\n");
}

// ---------------------------------------------------------------------------
// REQ-006: run — file loop
// ---------------------------------------------------------------------------

namespace {

auto make_temp_file_sp05(const std::string& content) -> std::filesystem::path {
  const auto path = std::filesystem::temp_directory_path() / "sp05_test.tmp";
  std::ofstream ofs{path};
  ofs << content;
  return path;
}

}  // namespace

// spec_id: SPEC-5  validates_req: REQ-006  tc: TC-REQ006-01
TEST(RunTest, ValidFileReturnsZero) {
  const auto path = make_temp_file_sp05("a,b\n");
  std::ostringstream out;
  std::ostringstream err;
  const int ret =
      FieldProcessor{make_opts_field(',', {0})}.run(out, {path.string()}, err);
  EXPECT_EQ(ret, 0);
  EXPECT_EQ(out.str(), "a\n");
  std::filesystem::remove(path);
}

// spec_id: SPEC-5  validates_req: REQ-006  tc: TC-REQ006-02
TEST(RunTest, NonExistentFileReturnsOne) {
  std::ostringstream out;
  std::ostringstream err;
  const int ret = FieldProcessor{make_opts_field(',', {0})}.run(
      out, {"/tmp/sp05_no_such_xyz.txt"}, err);
  EXPECT_EQ(ret, 1);
  EXPECT_NE(err.str().find("/tmp/sp05_no_such_xyz.txt"), std::string::npos);
}

// spec_id: SPEC-5  validates_req: REQ-006  tc: TC-REQ006-03
TEST(RunTest, ContinuesAfterOneError) {
  const auto path = make_temp_file_sp05("x,y\n");
  std::ostringstream out;
  std::ostringstream err;
  const int ret = FieldProcessor{make_opts_field(',', {0})}.run(
      out, {"/tmp/sp05_no_such_xyz.txt", path.string()}, err);
  EXPECT_EQ(ret, 1);
  EXPECT_EQ(out.str(), "x\n");
  std::filesystem::remove(path);
}

// spec_id: SPEC-5  validates_req: REQ-006  tc: TC-REQ006-04
TEST(RunTest, EmptyFilesListReadStdin) {
  std::istringstream input_ss{"a,b\n"};
  auto* const old_buf = std::cin.rdbuf(input_ss.rdbuf());
  std::ostringstream out;
  std::ostringstream err;
  const int ret = FieldProcessor{make_opts_field(',', {0})}.run(out, {}, err);
  std::cin.rdbuf(old_buf);
  EXPECT_EQ(ret, 0);
  EXPECT_EQ(out.str(), "a\n");
}
