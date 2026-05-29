// tests/cut/char_processor_test.cpp
// spec_id: SPEC-7
#include "cut/char_processor.hpp"
#include "cut/mode.hpp"
#include "cut/processor.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <type_traits>

using cc_cut::CharProcessor;

namespace {

auto make_list(std::initializer_list<int> idxs) -> cc_cut::CutList {
  cc_cut::CutList list;
  list.indices = std::set<int>{idxs};
  return list;
}

auto make_open_list(int from) -> cc_cut::CutList {
  cc_cut::CutList list;
  list.open_from = from;
  return list;
}

}  // namespace

// ---------------------------------------------------------------------------
// REQ-001: CharProcessor class structure
// ---------------------------------------------------------------------------

// spec_id: SPEC-7  validates_req: REQ-001  tc: TC-REQ001-01
static_assert(!std::is_abstract_v<cc_cut::CharProcessor>,
              "CharProcessor must be concrete");

// spec_id: SPEC-7  validates_req: REQ-001  tc: TC-REQ001-02
static_assert(std::is_base_of_v<cc_cut::Processor, cc_cut::CharProcessor>,
              "CharProcessor must inherit Processor");

// spec_id: SPEC-7  validates_req: REQ-001  tc: TC-REQ001-03
static_assert(std::is_abstract_v<cc_cut::Processor>,
              "Processor base must remain abstract");

TEST(CharProcessorStructureTest, StaticAssertsCompile) {
  // static_asserts above enforce class structure at compile time.
  SUCCEED();
}

// ---------------------------------------------------------------------------
// REQ-002: select_chars — codepoint-index selection
// ---------------------------------------------------------------------------

// spec_id: SPEC-7  validates_req: REQ-002  tc: TC-REQ002-01
TEST(SelectCharsTest, SingleAsciiCp) {
  EXPECT_EQ(CharProcessor::select_chars("hello", make_list({0})), "h");
}

// spec_id: SPEC-7  validates_req: REQ-002  tc: TC-REQ002-02
TEST(SelectCharsTest, NonContiguousAsciiCp) {
  EXPECT_EQ(CharProcessor::select_chars("hello", make_list({0, 1, 4})), "heo");
}

// spec_id: SPEC-7  validates_req: REQ-002  tc: TC-REQ002-03
TEST(SelectCharsTest, OpenRange) {
  EXPECT_EQ(CharProcessor::select_chars("hello", make_open_list(3)), "lo");
}

// spec_id: SPEC-7  validates_req: REQ-002  tc: TC-REQ002-04
TEST(SelectCharsTest, OutOfRangeCpSkipped) {
  EXPECT_EQ(CharProcessor::select_chars("hello", make_list({9})), "");
}

// spec_id: SPEC-7  validates_req: REQ-002  tc: TC-REQ002-05
TEST(SelectCharsTest, EmptyLineReturnsEmpty) {
  EXPECT_EQ(CharProcessor::select_chars("", make_list({0})), "");
}

// spec_id: SPEC-7  validates_req: REQ-002  tc: TC-REQ002-06
TEST(SelectCharsTest, TwoByteCodepointSelected) {
  // "\xC3\xA9" = é (U+00E9): lead at codepoint 0 → full 2 bytes emitted
  EXPECT_EQ(CharProcessor::select_chars("\xC3\xA9", make_list({0})), "\xC3\xA9");
}

// spec_id: SPEC-7  validates_req: REQ-002  tc: TC-REQ002-07
TEST(SelectCharsTest, ThreeByteCodepointSelected) {
  // "\xE2\x82\xAC" = € (U+20AC): codepoint 0 → full 3 bytes emitted
  EXPECT_EQ(CharProcessor::select_chars("\xE2\x82\xAC", make_list({0})),
            "\xE2\x82\xAC");
}

// spec_id: SPEC-7  validates_req: REQ-002  tc: TC-REQ002-08
TEST(SelectCharsTest, FourByteCodepointSelected) {
  // "\xF0\x9F\x98\x80" = 😀 (U+1F600): codepoint 0 → full 4 bytes emitted
  EXPECT_EQ(CharProcessor::select_chars("\xF0\x9F\x98\x80", make_list({0})),
            "\xF0\x9F\x98\x80");
}

// spec_id: SPEC-7  validates_req: REQ-002  tc: TC-REQ002-09
TEST(SelectCharsTest, MultibyteAtCpIndex1) {
  // "h\xC3\xA9llo" = "héllo"; codepoints: h=0, é=1, l=2, l=3, o=4
  EXPECT_EQ(CharProcessor::select_chars("h\xC3\xA9llo", make_list({1})),
            "\xC3\xA9");
}

// spec_id: SPEC-7  validates_req: REQ-002  tc: TC-REQ002-10
TEST(SelectCharsTest, MultibyteExcludedFromResult) {
  // select {0,2}: h(0) and l(2); é(1) excluded → "hl"
  EXPECT_EQ(CharProcessor::select_chars("h\xC3\xA9llo", make_list({0, 2})), "hl");
}

// spec_id: SPEC-7  validates_req: REQ-002  tc: TC-REQ002-11
TEST(SelectCharsTest, IndexBeyondCpCount) {
  // "\xC3\xA9" has 1 codepoint; index 1 is out of range → ""
  EXPECT_EQ(CharProcessor::select_chars("\xC3\xA9", make_list({1})), "");
}

// ---------------------------------------------------------------------------
// REQ-003: Invalid UTF-8 bytes treated as single codepoints
// ---------------------------------------------------------------------------

// spec_id: SPEC-7  validates_req: REQ-003  tc: TC-REQ003-01
TEST(SelectCharsInvalidUtf8Test, InvalidLeadByteSelected) {
  EXPECT_EQ(CharProcessor::select_chars("\x80", make_list({0})), "\x80");
}

// spec_id: SPEC-7  validates_req: REQ-003  tc: TC-REQ003-02
TEST(SelectCharsInvalidUtf8Test, InvalidLeadByteNotSelected) {
  EXPECT_EQ(CharProcessor::select_chars("\x80", make_list({})), "");
}

// spec_id: SPEC-7  validates_req: REQ-003  tc: TC-REQ003-03
TEST(SelectCharsInvalidUtf8Test, ValidCpAfterInvalidIsIndex1) {
  // '\x80' = invalid lead (codepoint index 0); 'a' = valid (codepoint index 1)
  EXPECT_EQ(CharProcessor::select_chars("\x80\x61", make_list({1})), "a");
}

// spec_id: SPEC-7  validates_req: REQ-003  tc: TC-REQ003-04
TEST(SelectCharsInvalidUtf8Test, TruncatedSequenceAtEol) {
  // "\xC3" alone = truncated 2-byte lead; treated as 1 codepoint at index 0
  EXPECT_EQ(CharProcessor::select_chars("\xC3", make_list({0})), "\xC3");
}

// ---------------------------------------------------------------------------
// REQ-004: process_line writes selected codepoints + newline
// ---------------------------------------------------------------------------

// spec_id: SPEC-7  validates_req: REQ-004  tc: TC-REQ004-01
TEST(ProcessLineTest, SelectFirstCpWritesWithNewline) {
  cc_cut::CutOptions opts;
  opts.mode = cc_cut::CutMode::CHARACTER;
  opts.list = make_list({0});
  CharProcessor proc{opts};
  std::ostringstream oss;
  proc.process_line("hello", oss);
  EXPECT_EQ(oss.str(), "h\n");
}

// spec_id: SPEC-7  validates_req: REQ-004  tc: TC-REQ004-02
TEST(ProcessLineTest, EmptyLineWritesJustNewline) {
  cc_cut::CutOptions opts;
  opts.mode = cc_cut::CutMode::CHARACTER;
  opts.list = make_list({0});
  CharProcessor proc{opts};
  std::ostringstream oss;
  proc.process_line("", oss);
  EXPECT_EQ(oss.str(), "\n");
}

// ---------------------------------------------------------------------------
// REQ-005: run() processes files, accumulates errors
// ---------------------------------------------------------------------------

// spec_id: SPEC-7  validates_req: REQ-005  tc: TC-REQ005-01
TEST(RunTest, EmptyFilesReadsStdin) {
  cc_cut::CutOptions opts;
  opts.mode = cc_cut::CutMode::CHARACTER;
  opts.list = make_list({0});
  CharProcessor proc{opts};
  std::istringstream fake_stdin{"ab\n"};
  auto* const old_buf = std::cin.rdbuf(fake_stdin.rdbuf());
  std::ostringstream out;
  std::ostringstream err;
  const int ret = proc.run(out, {}, err);
  std::cin.rdbuf(old_buf);
  EXPECT_EQ(ret, 0);
  EXPECT_EQ(out.str(), "a\n");
}

// spec_id: SPEC-7  validates_req: REQ-005  tc: TC-REQ005-02
TEST(RunTest, NonExistentFileReturns1) {
  cc_cut::CutOptions opts;
  opts.mode = cc_cut::CutMode::CHARACTER;
  opts.list = make_list({0});
  CharProcessor proc{opts};
  std::ostringstream out;
  std::ostringstream err;
  const int ret = proc.run(out, {"/no/such/sp07_missing.txt"}, err);
  EXPECT_EQ(ret, 1);
  EXPECT_TRUE(err.str().starts_with("cc-cut-tool: "));
}

// spec_id: SPEC-7  validates_req: REQ-005  tc: TC-REQ005-03
TEST(RunTest, ErrorMessageContainsPath) {
  cc_cut::CutOptions opts;
  opts.mode = cc_cut::CutMode::CHARACTER;
  opts.list = make_list({0});
  CharProcessor proc{opts};
  std::ostringstream out;
  std::ostringstream err;
  proc.run(out, {"/no/such/sp07_path_check.txt"}, err);
  EXPECT_NE(err.str().find("/no/such/sp07_path_check.txt"), std::string::npos);
}

// spec_id: SPEC-7  validates_req: REQ-005  tc: TC-REQ005-04
TEST(RunTest, ContinuesAfterMissingFile) {
  const auto tmp =
      std::filesystem::temp_directory_path() / "sp07_continue_test.tmp";
  { std::ofstream{tmp} << "ab\n"; }

  cc_cut::CutOptions opts;
  opts.mode = cc_cut::CutMode::CHARACTER;
  opts.list = make_list({0});
  CharProcessor proc{opts};
  std::ostringstream out;
  std::ostringstream err;
  const int ret = proc.run(out, {"/no/such/sp07_missing.txt", tmp.string()}, err);
  std::filesystem::remove(tmp);
  EXPECT_EQ(ret, 1);
  EXPECT_EQ(out.str(), "a\n");
}
