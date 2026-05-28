// spec_id: SPEC-2  validates_req: REQ-001,REQ-002,REQ-003,REQ-004,REQ-005,REQ-006,REQ-007,REQ-008,REQ-009,REQ-010,REQ-011
#include <gtest/gtest.h>
#include <set>
#include <type_traits>
#include "cut/list_parser.hpp"

using namespace cc_cut;

// ---- REQ-011: namespace cc_cut ----

// TC-REQ011-01: cc_cut::CutMode::FIELD is of type cc_cut::CutMode
static_assert(
    std::is_same_v<decltype(cc_cut::CutMode::FIELD), cc_cut::CutMode>,
    "TC-REQ011-01: CutMode must be in namespace cc_cut");

// TC-REQ011-02: CutMode::FIELD without qualifier must not compile.
// Verified by code review — using namespace cc_cut; is added to all
// test files; unqualified access works only because of that directive.

// ---- REQ-001: function signature ----

// spec_id: SPEC-2  validates_req: REQ-001  tc: TC-REQ001-01
TEST(ParseListTest, ReturnsValueForValidInput) {
    auto result = parse_list("1");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->indices, (std::set<int>{0}));
}

// spec_id: SPEC-2  validates_req: REQ-001  tc: TC-REQ001-02
TEST(ParseListTest, ReturnsErrorForEmptyInput) {
    auto result = parse_list("");
    EXPECT_FALSE(result.has_value());
}

// spec_id: SPEC-2  validates_req: REQ-001  tc: TC-REQ001-03
TEST(ParseListTest, PureFunctionIdenticalResults) {
    auto r1 = parse_list("1");
    auto r2 = parse_list("1");
    ASSERT_TRUE(r1.has_value());
    ASSERT_TRUE(r2.has_value());
    EXPECT_EQ(r1->indices, r2->indices);
    EXPECT_EQ(r1->open_from, r2->open_from);
}

// ---- REQ-002: comma tokenization ----

// spec_id: SPEC-2  validates_req: REQ-002  tc: TC-REQ002-01
TEST(ParseListTest, CommaMode_ThreeTokens) {
    auto result = parse_list("1,3,5");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->indices, (std::set<int>{0, 2, 4}));
    EXPECT_EQ(result->open_from, std::nullopt);
}

// spec_id: SPEC-2  validates_req: REQ-002  tc: TC-REQ002-04
TEST(ParseListTest, CommaMode_WhitespaceStripped) {
    auto result = parse_list("1, 3, 5");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->indices, (std::set<int>{0, 2, 4}));
}

// spec_id: SPEC-2  validates_req: REQ-002  tc: TC-REQ002-02
TEST(ParseListTest, CommaMode_EmptyTokenBetweenCommas) {
    auto result = parse_list("1,,3");
    ASSERT_FALSE(result.has_value());
    EXPECT_NE(result.error().find("invalid field value"), std::string::npos);
}

// spec_id: SPEC-2  validates_req: REQ-002  tc: TC-REQ002-03
TEST(ParseListTest, CommaMode_LeadingComma) {
    auto result = parse_list(",1");
    ASSERT_FALSE(result.has_value());
    EXPECT_NE(result.error().find("invalid field value"), std::string::npos);
}

// ---- REQ-003: whitespace tokenization ----

// spec_id: SPEC-2  validates_req: REQ-003  tc: TC-REQ003-01
TEST(ParseListTest, WhitespaceMode_SpaceSeparated) {
    auto result = parse_list("1 3 5");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->indices, (std::set<int>{0, 2, 4}));
    EXPECT_EQ(result->open_from, std::nullopt);
}

// spec_id: SPEC-2  validates_req: REQ-003  tc: TC-REQ003-02
TEST(ParseListTest, WhitespaceMode_LeadingTrailingSpaces) {
    auto result = parse_list("  1  3  ");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->indices, (std::set<int>{0, 2}));
}

// spec_id: SPEC-2  validates_req: REQ-003  tc: TC-REQ003-03
TEST(ParseListTest, WhitespaceMode_TabSeparated) {
    auto result = parse_list("1\t3");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->indices, (std::set<int>{0, 2}));
}

// ---- REQ-004: plain number token ----

// spec_id: SPEC-2  validates_req: REQ-004  tc: TC-REQ004-01
TEST(ParseListTest, PlainNumber_Three) {
    auto result = parse_list("3");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->indices, (std::set<int>{2}));
    EXPECT_EQ(result->open_from, std::nullopt);
}

// spec_id: SPEC-2  validates_req: REQ-004  tc: TC-REQ004-02
TEST(ParseListTest, PlainNumber_One) {
    auto result = parse_list("1");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->indices, (std::set<int>{0}));
}

// spec_id: SPEC-2  validates_req: REQ-004  tc: TC-REQ004-03
TEST(ParseListTest, PlainNumber_Duplicate) {
    auto result = parse_list("1 1");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->indices.size(), 1U);
    EXPECT_EQ(result->indices, (std::set<int>{0}));
}

// ---- REQ-005: range token N-M ----

// spec_id: SPEC-2  validates_req: REQ-005  tc: TC-REQ005-01
TEST(ParseListTest, Range_TwoToFive) {
    auto result = parse_list("2-5");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->indices, (std::set<int>{1, 2, 3, 4}));
    EXPECT_EQ(result->open_from, std::nullopt);
}

// spec_id: SPEC-2  validates_req: REQ-005  tc: TC-REQ005-02
TEST(ParseListTest, Range_SingleElement) {
    auto result = parse_list("3-3");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->indices, (std::set<int>{2}));
}

// spec_id: SPEC-2  validates_req: REQ-005  tc: TC-REQ005-03
TEST(ParseListTest, Range_Decreasing) {
    auto result = parse_list("5-3");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), "invalid decreasing range");
}

// spec_id: SPEC-2  validates_req: REQ-005  tc: TC-REQ005-04
TEST(ParseListTest, Range_Combined) {
    auto result = parse_list("1,3-5,7");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->indices, (std::set<int>{0, 2, 3, 4, 6}));
    EXPECT_EQ(result->open_from, std::nullopt);
}

// spec_id: SPEC-2  validates_req: REQ-005  tc: TC-REQ005-05
TEST(ParseListTest, Range_ZeroEndpoint) {
    auto result = parse_list("3-0");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), "values may not include zero");
}

// ---- REQ-006: open-start token -M ----

// spec_id: SPEC-2  validates_req: REQ-006  tc: TC-REQ006-01
TEST(ParseListTest, OpenStart_DashFour) {
    auto result = parse_list("-4");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->indices, (std::set<int>{0, 1, 2, 3}));
    EXPECT_EQ(result->open_from, std::nullopt);
}

// spec_id: SPEC-2  validates_req: REQ-006  tc: TC-REQ006-02
TEST(ParseListTest, OpenStart_DashOne) {
    auto result = parse_list("-1");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->indices, (std::set<int>{0}));
}

// ---- REQ-007: open-end token N- ----

// spec_id: SPEC-2  validates_req: REQ-007  tc: TC-REQ007-01
TEST(ParseListTest, OpenEnd_ThreeDash) {
    auto result = parse_list("3-");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->open_from, 2);
    EXPECT_TRUE(result->indices.empty());
}

// spec_id: SPEC-2  validates_req: REQ-007  tc: TC-REQ007-02
TEST(ParseListTest, OpenEnd_OneDash) {
    auto result = parse_list("1-");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->open_from, 0);
}

// spec_id: SPEC-2  validates_req: REQ-007  tc: TC-REQ007-03
TEST(ParseListTest, OpenEnd_MultipleSelectsMinimum) {
    auto result = parse_list("3-,5-");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->open_from, 2);
}

// spec_id: SPEC-2  validates_req: REQ-007  tc: TC-REQ007-04
TEST(ParseListTest, OpenEnd_WithFiniteIndices) {
    auto result = parse_list("1,3-");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->indices, (std::set<int>{0}));
    EXPECT_EQ(result->open_from, 2);
}

// ---- REQ-008: zero position error ----

// spec_id: SPEC-2  validates_req: REQ-008  tc: TC-REQ008-01
TEST(ParseListTest, Zero_PlainZero) {
    auto result = parse_list("0");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), "values may not include zero");
}

// spec_id: SPEC-2  validates_req: REQ-008  tc: TC-REQ008-02
TEST(ParseListTest, Zero_RangeStartsAtZero) {
    auto result = parse_list("0-3");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), "values may not include zero");
}

// spec_id: SPEC-2  validates_req: REQ-008  tc: TC-REQ008-03
TEST(ParseListTest, Zero_OpenStartZero) {
    auto result = parse_list("-0");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), "values may not include zero");
}

// ---- REQ-009: invalid token error ----

// spec_id: SPEC-2  validates_req: REQ-009  tc: TC-REQ009-01
TEST(ParseListTest, Invalid_LoneDash) {
    auto result = parse_list("-");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), "invalid range with no endpoint: -");
}

// spec_id: SPEC-2  validates_req: REQ-009  tc: TC-REQ009-02
TEST(ParseListTest, Invalid_AlphaToken) {
    auto result = parse_list("a");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), "invalid field value: a");
}

// spec_id: SPEC-2  validates_req: REQ-009  tc: TC-REQ009-03
TEST(ParseListTest, Invalid_AlphaSuffix) {
    auto result = parse_list("1a");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), "invalid field value: 1a");
}

// spec_id: SPEC-2  validates_req: REQ-009  tc: TC-REQ009-04
TEST(ParseListTest, Invalid_MultipleDashes) {
    auto result = parse_list("1-2-3");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), "invalid field value: 1-2-3");
}

// spec_id: SPEC-2  validates_req: REQ-009  tc: TC-REQ009-05
TEST(ParseListTest, Invalid_DoubleDashPrefix) {
    auto result = parse_list("--3");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), "invalid field value: --3");
}

// ---- REQ-010: empty input error ----

// spec_id: SPEC-2  validates_req: REQ-010  tc: TC-REQ010-01
TEST(ParseListTest, Empty_EmptyString) {
    auto result = parse_list("");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), "missing list specification");
}

// spec_id: SPEC-2  validates_req: REQ-010  tc: TC-REQ010-02
TEST(ParseListTest, Empty_WhitespaceOnly) {
    auto result = parse_list("   ");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), "missing list specification");
}

// ---- REQ-011: tc: TC-REQ011-03 — SP-01 tests pass under cc_cut namespace ----
// Verified implicitly: this file and the four SP-01 test files compile and
// pass with using namespace cc_cut; — confirmed by ctest run in Task 5.
