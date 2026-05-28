#include <gtest/gtest.h>
#include "cut/options.hpp"

// TC-REQ003-01
TEST(CutOptionsTest, DefaultInit) {
    CutOptions opts;
    EXPECT_EQ(opts.mode, CutMode::FIELD);
    EXPECT_EQ(opts.delim, std::nullopt);
    EXPECT_FALSE(opts.suppress);
    EXPECT_FALSE(opts.no_split);
}

// TC-REQ003-02
TEST(CutOptionsTest, SetDelim) {
    CutOptions opts;
    opts.delim = ',';
    EXPECT_TRUE(opts.delim.has_value());
    EXPECT_EQ(opts.delim.value(), ',');
}

// TC-REQ003-03
TEST(CutOptionsTest, SetSuppress) {
    CutOptions opts;
    opts.suppress = true;
    EXPECT_TRUE(opts.suppress);
}
