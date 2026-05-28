#include <gtest/gtest.h>
#include "cut/mode.hpp"

// TC-REQ001-01, TC-REQ001-02
static_assert(CutMode::BYTE != CutMode::CHARACTER);
static_assert(CutMode::CHARACTER != CutMode::FIELD);

// TC-REQ001-03
static_assert(sizeof(CutMode) == 1);

TEST(CutModeTest, DistinctValues) {
    EXPECT_NE(CutMode::BYTE, CutMode::CHARACTER);
    EXPECT_NE(CutMode::CHARACTER, CutMode::FIELD);
}
