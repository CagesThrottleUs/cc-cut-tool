// spec_id: SPEC-1  validates_req: REQ-003
#include "cut/options.hpp"

#include <gtest/gtest.h>

// spec_id: SPEC-2  validates_req: REQ-011
using cc_cut::CutMode;
using cc_cut::CutOptions;

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
