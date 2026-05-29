// tests/cut/byte_processor_test.cpp
// spec_id: SPEC-6  validates_req: REQ-001,REQ-002,REQ-003,REQ-004,REQ-005,REQ-006
#include "cut/byte_processor.hpp"
#include "cut/field_processor.hpp"

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

#include "cut/list.hpp"
#include "cut/options.hpp"
#include "cut/processor.hpp"

using cc_cut::ByteProcessor;
using cc_cut::CutList;
using cc_cut::CutMode;
using cc_cut::CutOptions;
using cc_cut::Processor;

// ---------------------------------------------------------------------------
// REQ-006: Processor abstract base
// ---------------------------------------------------------------------------

// spec_id: SPEC-6  validates_req: REQ-006  tc: TC-REQ006-01
static_assert(std::is_abstract_v<cc_cut::Processor>,
              "TC-REQ006-01: Processor must be abstract (pure virtual run)");

// ---------------------------------------------------------------------------
// REQ-001: ByteProcessor class
// ---------------------------------------------------------------------------

// spec_id: SPEC-6  validates_req: REQ-001  tc: TC-REQ001-02
static_assert(std::is_constructible_v<ByteProcessor, CutOptions>,
              "TC-REQ001-02: ByteProcessor must be constructible from CutOptions");

// spec_id: SPEC-6  validates_req: REQ-001  tc: TC-REQ001-03
static_assert(std::is_base_of_v<Processor, ByteProcessor>,
              "TC-REQ001-03: ByteProcessor must inherit Processor");

// spec_id: SPEC-6  validates_req: REQ-006  tc: TC-REQ006-02
static_assert(!std::is_abstract_v<ByteProcessor>,
              "TC-REQ006-02: ByteProcessor must be concrete (overrides run)");

// spec_id: SPEC-6  validates_req: REQ-006  tc: TC-REQ006-07
static_assert(std::is_base_of_v<Processor, ByteProcessor>,
              "TC-REQ006-07: ByteProcessor is_base_of Processor");

// spec_id: SPEC-6  validates_req: REQ-006  tc: TC-REQ006-06
static_assert(std::is_base_of_v<Processor, cc_cut::FieldProcessor>,
              "TC-REQ006-06: FieldProcessor is_base_of Processor");

// spec_id: SPEC-6  validates_req: REQ-001  tc: TC-REQ001-01
TEST(ByteProcessorTest, ConstructsFromCutOptions) {
  CutOptions opts;
  EXPECT_NO_THROW(ByteProcessor{opts});
}

// ---------------------------------------------------------------------------
// REQ-002: select_bytes — raw byte selection
// ---------------------------------------------------------------------------

namespace {

// Helper: build a CutList with only indices (no open_from)
auto make_list(std::initializer_list<int> idxs) -> cc_cut::CutList {
  cc_cut::CutList list;
  list.indices = std::set<int>{idxs};
  return list;
}

// Helper: build a CutList with open_from only
auto make_open_list(int from) -> cc_cut::CutList {
  cc_cut::CutList list;
  list.open_from = from;
  return list;
}

}  // namespace

// spec_id: SPEC-6  validates_req: REQ-002  tc: TC-REQ002-01
TEST(SelectBytesTest, SingleByte) {
  EXPECT_EQ(ByteProcessor::select_bytes("hello", make_list({0})), "h");
}

// spec_id: SPEC-6  validates_req: REQ-002  tc: TC-REQ002-02
TEST(SelectBytesTest, NonContiguousBytes) {
  EXPECT_EQ(ByteProcessor::select_bytes("hello", make_list({0, 1, 4})), "heo");
}

// spec_id: SPEC-6  validates_req: REQ-002  tc: TC-REQ002-03
TEST(SelectBytesTest, OpenRange) {
  EXPECT_EQ(ByteProcessor::select_bytes("hello", make_open_list(3)), "lo");
}

// spec_id: SPEC-6  validates_req: REQ-002  tc: TC-REQ002-04
TEST(SelectBytesTest, OutOfBoundsPositionSkipped) {
  EXPECT_EQ(ByteProcessor::select_bytes("hello", make_list({9})), "");
}

// spec_id: SPEC-6  validates_req: REQ-002  tc: TC-REQ002-05
TEST(SelectBytesTest, EmptyLineReturnsEmpty) {
  EXPECT_EQ(ByteProcessor::select_bytes("", make_list({0})), "");
}

// spec_id: SPEC-6  validates_req: REQ-002  tc: TC-REQ002-06
TEST(SelectBytesTest, AllBytesSelected) {
  EXPECT_EQ(ByteProcessor::select_bytes("abc", make_list({0, 1, 2})), "abc");
}

// ---------------------------------------------------------------------------
// REQ-003: select_bytes_no_split — UTF-8-boundary-aware selection
// ---------------------------------------------------------------------------

// spec_id: SPEC-6  validates_req: REQ-003  tc: TC-REQ003-01
TEST(SelectBytesNoSplitTest, PureAsciiMatchesSelectBytes) {
  const auto list = make_list({0, 1, 4});
  EXPECT_EQ(ByteProcessor::select_bytes_no_split("hello", list),
            ByteProcessor::select_bytes("hello", list));
}

// spec_id: SPEC-6  validates_req: REQ-003  tc: TC-REQ003-02
TEST(SelectBytesNoSplitTest, TwoByteCharLeadSelected) {
  // "\xC3\xA9" = é; lead at byte 0 → full char included
  EXPECT_EQ(ByteProcessor::select_bytes_no_split("\xC3\xA9", make_list({0})),
            "\xC3\xA9");
}

// spec_id: SPEC-6  validates_req: REQ-003  tc: TC-REQ003-03
TEST(SelectBytesNoSplitTest, TwoByteCharLeadNotSelected) {
  // "\xC3\xA9" = é; lead at byte 0 not in {1} → char excluded
  EXPECT_EQ(ByteProcessor::select_bytes_no_split("\xC3\xA9", make_list({1})),
            "");
}

// spec_id: SPEC-6  validates_req: REQ-003  tc: TC-REQ003-04
TEST(SelectBytesNoSplitTest, ThreeByteCharIncluded) {
  // "\xE2\x82\xAC" = €; lead at byte 0 selected → full 3-byte char
  EXPECT_EQ(
      ByteProcessor::select_bytes_no_split("\xE2\x82\xAC", make_list({0})),
      "\xE2\x82\xAC");
}

// spec_id: SPEC-6  validates_req: REQ-003  tc: TC-REQ003-05
TEST(SelectBytesNoSplitTest, InvalidLeadByteIncludedAs1Byte) {
  // "\x80" is an invalid lead (continuation byte); treated as 1-byte char
  EXPECT_EQ(ByteProcessor::select_bytes_no_split("\x80", make_list({0})),
            "\x80");
}

// spec_id: SPEC-6  validates_req: REQ-003  tc: TC-REQ003-06
TEST(SelectBytesNoSplitTest, InvalidLeadByteExcluded) {
  cc_cut::CutList empty;
  EXPECT_EQ(ByteProcessor::select_bytes_no_split("\x80", empty), "");
}

// spec_id: SPEC-6  validates_req: REQ-003  tc: TC-REQ003-07
TEST(SelectBytesNoSplitTest, MixedSelectByLeads) {
  // "a\xC3\xA9b" = {0x61, 0xC3, 0xA9, 0x62} — 3 chars: 'a'(0), 'é'(1), 'b'(3)
  // Leads at 0 and 1 selected → "a" + "é"
  EXPECT_EQ(
      ByteProcessor::select_bytes_no_split("a\xC3\xA9""b", make_list({0, 1})),
      "a\xC3\xA9");
}

// spec_id: SPEC-6  validates_req: REQ-003  tc: TC-REQ003-08
TEST(SelectBytesNoSplitTest, ContinuationByteInListDoesNotIncludeChar) {
  // "a\xC3\xA9b": byte 2 is continuation of 'é' whose lead at 1 is not in {0,2}
  // → only 'a' (lead at 0) is included
  EXPECT_EQ(
      ByteProcessor::select_bytes_no_split("a\xC3\xA9""b", make_list({0, 2})),
      "a");
}

// spec_id: SPEC-6  validates_req: REQ-003  tc: TC-REQ003-09
TEST(SelectBytesNoSplitTest, TruncatedSequenceIncludedAs1Byte) {
  // "\xC3" alone is a truncated 2-byte sequence; treated as 1-byte char per ASM-002
  EXPECT_EQ(ByteProcessor::select_bytes_no_split("\xC3", make_list({0})), "\xC3");
}

// spec_id: SPEC-6  validates_req: REQ-003  tc: TC-REQ003-10
TEST(SelectBytesNoSplitTest, TruncatedSequenceExcluded) {
  cc_cut::CutList empty;
  EXPECT_EQ(ByteProcessor::select_bytes_no_split("\xC3", empty), "");
}

// ---------------------------------------------------------------------------
// REQ-004: process_line
// ---------------------------------------------------------------------------

namespace {

auto make_opts(std::initializer_list<int> idxs,
               bool no_split = false) -> cc_cut::CutOptions {
  cc_cut::CutOptions opts;
  opts.mode = cc_cut::CutMode::BYTE;
  opts.list = make_list(idxs);
  opts.no_split = no_split;
  return opts;
}

}  // namespace

// spec_id: SPEC-6  validates_req: REQ-004  tc: TC-REQ004-01
TEST(ProcessLineTest, SingleByteNoSplit) {
  std::ostringstream out;
  ByteProcessor{make_opts({0})}.process_line("hello", out);
  EXPECT_EQ(out.str(), "h\n");
}

// spec_id: SPEC-6  validates_req: REQ-004  tc: TC-REQ004-02
TEST(ProcessLineTest, NonContiguousBytesNoSplit) {
  std::ostringstream out;
  ByteProcessor{make_opts({0, 1, 4})}.process_line("hello", out);
  EXPECT_EQ(out.str(), "heo\n");
}

// spec_id: SPEC-6  validates_req: REQ-004  tc: TC-REQ004-03
TEST(ProcessLineTest, MultibyteCharNoSplitEnabled) {
  std::ostringstream out;
  ByteProcessor{make_opts({0}, /*no_split=*/true)}.process_line("\xC3\xA9", out);
  EXPECT_EQ(out.str(), "\xC3\xA9\n");
}

// spec_id: SPEC-6  validates_req: REQ-004  tc: TC-REQ004-04
TEST(ProcessLineTest, MultibyteCharLeadNotSelectedNoSplitEnabled) {
  std::ostringstream out;
  ByteProcessor{make_opts({1}, /*no_split=*/true)}.process_line("\xC3\xA9", out);
  EXPECT_EQ(out.str(), "\n");
}

// spec_id: SPEC-6  validates_req: REQ-004  tc: TC-REQ004-05
TEST(ProcessLineTest, EmptyLineProducesNewline) {
  std::ostringstream out;
  cc_cut::CutOptions opts;
  opts.mode = cc_cut::CutMode::BYTE;
  ByteProcessor{opts}.process_line("", out);
  EXPECT_EQ(out.str(), "\n");
}

// ---------------------------------------------------------------------------
// REQ-005: run — file loop
// ---------------------------------------------------------------------------

// spec_id: SPEC-6  validates_req: REQ-005  tc: TC-REQ005-01
TEST(ByteProcessorRunTest, ValidFileReturns0) {
  const auto tmp =
      std::filesystem::temp_directory_path() / "bp_run_test_valid.txt";
  {
    std::ofstream ofs(tmp);
    ofs << "hello\n";
  }

  std::ostringstream out;
  std::ostringstream err;
  const int ret = ByteProcessor{make_opts({0})}.run(out, {tmp.string()}, err);
  std::filesystem::remove(tmp);

  EXPECT_EQ(ret, 0);
  EXPECT_EQ(out.str(), "h\n");
  EXPECT_TRUE(err.str().empty());
}

// spec_id: SPEC-6  validates_req: REQ-005  tc: TC-REQ005-02
TEST(ByteProcessorRunTest, NonExistentFileReturns1) {
  std::ostringstream out;
  std::ostringstream err;
  const int ret =
      ByteProcessor{make_opts({0})}.run(out, {"/no/such/file.txt"}, err);

  EXPECT_EQ(ret, 1);
  EXPECT_FALSE(err.str().empty());
  EXPECT_NE(err.str().find("/no/such/file.txt"), std::string::npos);
  EXPECT_NE(err.str().find("cc-cut-tool:"), std::string::npos);
}

// spec_id: SPEC-6  validates_req: REQ-005  tc: TC-REQ005-03
TEST(ByteProcessorRunTest, FirstValidSecondMissingContinues) {
  const auto tmp =
      std::filesystem::temp_directory_path() / "bp_run_test_first.txt";
  {
    std::ofstream ofs(tmp);
    ofs << "hello\n";
  }

  std::ostringstream out;
  std::ostringstream err;
  const int ret = ByteProcessor{make_opts({0})}.run(
      out, {tmp.string(), "/no/such/file.txt"}, err);
  std::filesystem::remove(tmp);

  EXPECT_EQ(ret, 1);
  EXPECT_EQ(out.str(), "h\n");
  EXPECT_NE(err.str().find("/no/such/file.txt"), std::string::npos);
}

// spec_id: SPEC-6  validates_req: REQ-005  tc: TC-REQ005-04
TEST(ByteProcessorRunTest, EmptyFileListReadsStdin) {
  std::istringstream fake_stdin("hello\n");
  auto* old_buf = std::cin.rdbuf(fake_stdin.rdbuf());

  std::ostringstream out;
  std::ostringstream err;
  const int ret = ByteProcessor{make_opts({0})}.run(out, {}, err);

  std::cin.rdbuf(old_buf);

  EXPECT_EQ(ret, 0);
  EXPECT_EQ(out.str(), "h\n");
}

// ---------------------------------------------------------------------------
// REQ-006: make_processor factory
// ---------------------------------------------------------------------------

// spec_id: SPEC-6  validates_req: REQ-006  tc: TC-REQ006-03
TEST(MakeProcessorTest, ByteModeReturnsNonNullByteProcessor) {
  cc_cut::CutOptions opts;
  opts.mode = cc_cut::CutMode::BYTE;
  auto result = cc_cut::make_processor(opts);
  ASSERT_TRUE(result.has_value());
  ASSERT_NE(result->get(), nullptr);
  EXPECT_NE(dynamic_cast<ByteProcessor*>(result->get()), nullptr);
}

// spec_id: SPEC-6  validates_req: REQ-006  tc: TC-REQ006-04
TEST(MakeProcessorTest, FieldModeReturnsNonNullFieldProcessor) {
  cc_cut::CutOptions opts;
  opts.mode = cc_cut::CutMode::FIELD;
  auto result = cc_cut::make_processor(opts);
  ASSERT_TRUE(result.has_value());
  ASSERT_NE(result->get(), nullptr);
  EXPECT_NE(dynamic_cast<cc_cut::FieldProcessor*>(result->get()), nullptr);
}

// spec_id: SPEC-6  validates_req: REQ-006  tc: TC-REQ006-05
TEST(MakeProcessorTest, CharacterModeReturnsError) {
  cc_cut::CutOptions opts;
  opts.mode = cc_cut::CutMode::CHARACTER;
  auto result = cc_cut::make_processor(opts);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(),
            "cc-cut-tool: character mode not yet implemented");
}
