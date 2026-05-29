// tests/cut/byte_processor_test.cpp
// spec_id: SPEC-6  validates_req: REQ-001,REQ-002,REQ-003,REQ-004,REQ-005,REQ-006
#include "cut/byte_processor.hpp"

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
