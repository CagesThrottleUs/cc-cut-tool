#include <gtest/gtest.h>
#include "cut/list.hpp"

// TC-REQ002-01
TEST(CutListTest, DefaultInit) {
    CutList list;
    EXPECT_EQ(list.open_from, std::nullopt);
    EXPECT_TRUE(list.indices.empty());
}

// TC-REQ002-02
TEST(CutListTest, SetOpenFrom) {
    CutList list;
    list.open_from = 2;
    EXPECT_TRUE(list.open_from.has_value());
    EXPECT_EQ(list.open_from.value(), 2);
}

// TC-REQ002-03
TEST(CutListTest, InsertIndices) {
    CutList list;
    list.indices.insert({0, 2, 4});
    EXPECT_EQ(list.indices.size(), 3U);
}
