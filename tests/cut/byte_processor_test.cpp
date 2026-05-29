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
