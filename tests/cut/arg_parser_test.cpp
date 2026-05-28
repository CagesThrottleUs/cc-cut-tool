// spec_id: SPEC-3  validates_req:
// REQ-001,REQ-002,REQ-003,REQ-004,REQ-005,REQ-006,REQ-007,REQ-008,REQ-009,REQ-010
#include "cut/arg_parser.hpp"

#include <gtest/gtest.h>

#include <set>
#include <string>
#include <vector>

#include "cut/config.hpp"
#include "cut/parse_result.hpp"

using cc_cut::collect_files;
using cc_cut::CutMode;
using cc_cut::CutOptions;
using cc_cut::detect_mode;
using cc_cut::extract_list_spec;
using cc_cut::parse_args;
using cc_cut::parse_mode_properties;
using cc_cut::ParseResult;

// ---------------------------------------------------------------------------
// Helper: build a char** argv from vector<string> (strings stay alive)
// ---------------------------------------------------------------------------
namespace {

struct ArgvHolder {
 private:
  std::vector<std::string> storage;
  std::vector<char*>       argv_vec;

 public:
  explicit ArgvHolder(std::vector<std::string> args)
      : storage(std::move(args)) {
    for (auto& str : storage) {
      argv_vec.push_back(str.data());
    }
    argv_vec.push_back(nullptr);
  }

  [[nodiscard]] auto argc() const -> int {
    return static_cast<int>(storage.size());
  }
  auto argv() -> char** { return argv_vec.data(); }
};

}  // namespace

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
  EXPECT_EQ(index, 2);
}

// spec_id: SPEC-3  validates_req: REQ-005  tc: TC-REQ005-02
TEST(ExtractListSpecTest, SeparateList) {
  ArgvHolder holder{{"cut", "-f", "1,3", "file.txt"}};
  int index = 2;
  auto result = extract_list_spec("-f", holder.argc(), holder.argv(), index);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, "1,3");
  EXPECT_EQ(index, 3);
}

// spec_id: SPEC-3  validates_req: REQ-005  tc: TC-REQ005-03
TEST(ExtractListSpecTest, MissingListArgForF) {
  ArgvHolder holder{{"cut", "-f"}};
  int index = 2;
  auto result = extract_list_spec("-f", holder.argc(), holder.argv(), index);
  ASSERT_FALSE(result.has_value());
  EXPECT_NE(result.error().find("option requires an argument"),
            std::string::npos);
  EXPECT_NE(result.error().find("'f'"), std::string::npos);
}

// spec_id: SPEC-3  validates_req: REQ-005  tc: TC-REQ005-04
TEST(ExtractListSpecTest, MissingListArgForB) {
  ArgvHolder holder{{"cut", "-b"}};
  int index = 2;
  auto result = extract_list_spec("-b", holder.argc(), holder.argv(), index);
  ASSERT_FALSE(result.has_value());
  EXPECT_NE(result.error().find("option requires an argument"),
            std::string::npos);
  EXPECT_NE(result.error().find("'b'"), std::string::npos);
}

// ---------------------------------------------------------------------------
// REQ-006: parse_mode_properties
// ---------------------------------------------------------------------------

// spec_id: SPEC-3  validates_req: REQ-006  tc: TC-REQ006-01
TEST(ParseModePropertiesTest, ByteNoSplit) {
  ArgvHolder sub{{"cut", "-n", "file.txt"}};
  CutOptions opts;
  opts.mode = CutMode::BYTE;
  int index = 1;
  auto result = parse_mode_properties(sub.argc(), sub.argv(), index, opts);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(opts.no_split);
  EXPECT_EQ(index, 2);
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
  opts.mode = CutMode::BYTE;
  int index = 1;
  auto result = parse_mode_properties(sub.argc(), sub.argv(), index, opts);
  ASSERT_TRUE(result.has_value());
  EXPECT_FALSE(opts.suppress);
  EXPECT_EQ(index, 1);
}

// spec_id: SPEC-3  validates_req: REQ-006  tc: TC-REQ006-09
TEST(ParseModePropertiesTest, FieldDelimMissingArg) {
  ArgvHolder sub{{"cut", "-d"}};
  CutOptions opts;
  opts.mode = CutMode::FIELD;
  int index = 1;
  auto result = parse_mode_properties(sub.argc(), sub.argv(), index, opts);
  ASSERT_FALSE(result.has_value());
  EXPECT_NE(result.error().find("option requires an argument"),
            std::string::npos);
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
TEST(CollectFilesTest, RemovesDuplicatePaths) {
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
TEST(CollectFilesTest, StdinDeduplicated) {
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
  EXPECT_EQ(
      result.error(),
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
  EXPECT_TRUE(result.error().ends_with(
      "Try 'cc-cut-tool --help' for more information."));
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
  EXPECT_EQ(
      result.error(),
      "cc-cut-tool: you must specify a list of bytes, characters, or fields\n"
      "Try 'cc-cut-tool --help' for more information.");
}

// spec_id: SPEC-3  validates_req: REQ-009  tc: TC-REQ009-02
TEST(NoModeTest, FileAsFirstArg) {
  ArgvHolder holder{{"cut", "file.txt"}};
  auto result = parse_args(holder.argc(), holder.argv());
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(
      result.error(),
      "cc-cut-tool: you must specify a list of bytes, characters, or fields\n"
      "Try 'cc-cut-tool --help' for more information.");
}

// spec_id: SPEC-3  validates_req: REQ-009  tc: TC-REQ009-03
TEST(NoModeTest, DoubleDash) {
  ArgvHolder holder{{"cut", "--"}};
  auto result = parse_args(holder.argc(), holder.argv());
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(
      result.error(),
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
