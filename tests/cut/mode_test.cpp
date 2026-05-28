// spec_id: SPEC-1  validates_req: REQ-001
#include "cut/mode.hpp"

#include <gtest/gtest.h>
#include <type_traits>

// spec_id: SPEC-2  validates_req: REQ-011
using cc_cut::CutMode;

// TC-REQ011-01: CutMode is in namespace cc_cut
static_assert(std::is_same_v<decltype(cc_cut::CutMode::FIELD), cc_cut::CutMode>,
              "TC-REQ011-01: CutMode must be in namespace cc_cut");

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
