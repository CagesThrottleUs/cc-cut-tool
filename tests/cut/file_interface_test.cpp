// spec_id: SPEC-4  validates_req:
// REQ-001,REQ-002,REQ-003,REQ-004,REQ-005,REQ-006,REQ-007
#include <gtest/gtest.h>

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>

#include "cut/buffered_file_source.hpp"
#include "cut/file_source.hpp"
#include "cut/make_file_source.hpp"
#include "cut/mmap_file_source.hpp"
#include "cut/stdin_source.hpp"

using cc_cut::BufferedFileSource;
using cc_cut::FileSource;
using cc_cut::make_file_source;
using cc_cut::mmap_threshold;
using cc_cut::MmapFileSource;
using cc_cut::StdinSource;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
namespace {

auto make_temp_file(const std::string& content) -> std::filesystem::path {
  const auto path = std::filesystem::temp_directory_path() / "sp04_test.tmp";
  std::ofstream ofs{path};
  ofs << content;
  return path;
}

// Exposes FileSource::next_line for direct testing.
struct NextLineTester : cc_cut::FileSource {
  void load() override {}
  auto getline() -> std::optional<std::string_view> override {
    return std::nullopt;
  }
  static auto test_next_line(std::string_view buf, std::size_t& cur)
      -> std::optional<std::string_view> {
    return next_line(buf, cur);
  }
};

}  // namespace

// ---------------------------------------------------------------------------
// REQ-001: mmap_threshold constant
// ---------------------------------------------------------------------------

// spec_id: SPEC-4  validates_req: REQ-001  tc: TC-REQ001-01
static_assert(mmap_threshold > 0,
              "TC-REQ001-01: mmap_threshold must be positive");

// spec_id: SPEC-4  validates_req: REQ-001  tc: TC-REQ001-02
static_assert(std::is_same_v<decltype(mmap_threshold), const std::size_t>,
              "TC-REQ001-02: mmap_threshold must be const std::size_t");

// ---------------------------------------------------------------------------
// REQ-002: next_line logic
// ---------------------------------------------------------------------------

// spec_id: SPEC-4  validates_req: REQ-002  tc: TC-REQ002-01
TEST(NextLineTest, BasicSplit) {
  std::size_t cur = 0;
  auto result = NextLineTester::test_next_line("a\nb\n", cur);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, "a");  // NOLINT(bugprone-unchecked-optional-access)
  EXPECT_EQ(cur, 2U);
}

// spec_id: SPEC-4  validates_req: REQ-002  tc: TC-REQ002-02
TEST(NextLineTest, SecondLine) {
  std::size_t cur = 2;
  auto result = NextLineTester::test_next_line("a\nb\n", cur);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, "b");  // NOLINT(bugprone-unchecked-optional-access)
  EXPECT_EQ(cur, 4U);
}

// spec_id: SPEC-4  validates_req: REQ-002  tc: TC-REQ002-03
TEST(NextLineTest, AtEnd) {
  std::size_t cur = 4;
  EXPECT_FALSE(NextLineTester::test_next_line("a\nb\n", cur).has_value());
}

// spec_id: SPEC-4  validates_req: REQ-002  tc: TC-REQ002-04
TEST(NextLineTest, NoTrailingNewline) {
  std::size_t cur = 2;
  auto result = NextLineTester::test_next_line("a\nb", cur);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, "b");  // NOLINT(bugprone-unchecked-optional-access)
  EXPECT_EQ(cur, 3U);
}

// spec_id: SPEC-4  validates_req: REQ-002  tc: TC-REQ002-05
TEST(NextLineTest, CrlfIncluded) {
  std::size_t cur = 0;
  auto result = NextLineTester::test_next_line("a\r\nb", cur);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, "a\r");  // NOLINT(bugprone-unchecked-optional-access)
  EXPECT_EQ(cur, 3U);
}

// spec_id: SPEC-4  validates_req: REQ-002  tc: TC-REQ002-06
TEST(NextLineTest, EmptyBuffer) {
  std::size_t cur = 0;
  EXPECT_FALSE(NextLineTester::test_next_line("", cur).has_value());
}

// spec_id: SPEC-4  validates_req: REQ-002  tc: TC-REQ002-07
TEST(NextLineTest, BlankLine) {
  std::size_t cur = 0;
  auto result = NextLineTester::test_next_line("\n", cur);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, "");  // NOLINT(bugprone-unchecked-optional-access)
  EXPECT_EQ(cur, 1U);
}

// ---------------------------------------------------------------------------
// REQ-003: StdinSource
// ---------------------------------------------------------------------------

// spec_id: SPEC-4  validates_req: REQ-003  tc: TC-REQ003-01
TEST(StdinSourceTest, TwoLinesWithTrailingNewline) {
  const std::istringstream input_ss{"line1\nline2\n"};
  auto* const old_buf = std::cin.rdbuf(input_ss.rdbuf());
  StdinSource src;
  src.load();
  std::cin.rdbuf(old_buf);
  EXPECT_EQ(src.getline(), "line1");
  EXPECT_EQ(src.getline(), "line2");
  EXPECT_FALSE(src.getline().has_value());
}

// spec_id: SPEC-4  validates_req: REQ-003  tc: TC-REQ003-02
TEST(StdinSourceTest, NoTrailingNewline) {
  const std::istringstream input_ss{"line1\nline2"};
  auto* const old_buf = std::cin.rdbuf(input_ss.rdbuf());
  StdinSource src;
  src.load();
  std::cin.rdbuf(old_buf);
  EXPECT_EQ(src.getline(), "line1");
  EXPECT_EQ(src.getline(), "line2");
  EXPECT_FALSE(src.getline().has_value());
}

// spec_id: SPEC-4  validates_req: REQ-003  tc: TC-REQ003-03
TEST(StdinSourceTest, EmptyStdin) {
  const std::istringstream input_ss{""};
  auto* const old_buf = std::cin.rdbuf(input_ss.rdbuf());
  StdinSource src;
  src.load();
  std::cin.rdbuf(old_buf);
  EXPECT_FALSE(src.getline().has_value());
}

// spec_id: SPEC-4  validates_req: REQ-003  tc: TC-REQ003-04
TEST(StdinSourceTest, CrlfPreserved) {
  const std::istringstream input_ss{"a\r\nb"};
  auto* const old_buf = std::cin.rdbuf(input_ss.rdbuf());
  StdinSource src;
  src.load();
  std::cin.rdbuf(old_buf);
  EXPECT_EQ(src.getline(), "a\r");
  EXPECT_EQ(src.getline(), "b");
}

// ---------------------------------------------------------------------------
// REQ-004: BufferedFileSource
// ---------------------------------------------------------------------------

// spec_id: SPEC-4  validates_req: REQ-004  tc: TC-REQ004-01
TEST(BufferedFileSourceTest, TwoLines) {
  const auto path = make_temp_file("a\nb\n");
  BufferedFileSource src{path};
  src.load();
  EXPECT_EQ(src.getline(), "a");
  EXPECT_EQ(src.getline(), "b");
  EXPECT_FALSE(src.getline().has_value());
  std::filesystem::remove(path);
}

// spec_id: SPEC-4  validates_req: REQ-004  tc: TC-REQ004-02
TEST(BufferedFileSourceTest, NoTrailingNewline) {
  const auto path = make_temp_file("a\nb");
  BufferedFileSource src{path};
  src.load();
  EXPECT_EQ(src.getline(), "a");
  EXPECT_EQ(src.getline(), "b");
  EXPECT_FALSE(src.getline().has_value());
  std::filesystem::remove(path);
}

// spec_id: SPEC-4  validates_req: REQ-004  tc: TC-REQ004-03
TEST(BufferedFileSourceTest, NonExistentThrows) {
  BufferedFileSource src{"/tmp/sp04_no_exist_buf.txt"};
  EXPECT_THROW(src.load(), std::ios_base::failure);
}

// spec_id: SPEC-4  validates_req: REQ-004  tc: TC-REQ004-04
TEST(BufferedFileSourceTest, EmptyFile) {
  const auto path = make_temp_file("");
  BufferedFileSource src{path};
  src.load();
  EXPECT_FALSE(src.getline().has_value());
  std::filesystem::remove(path);
}

// ---------------------------------------------------------------------------
// REQ-005: MmapFileSource
// ---------------------------------------------------------------------------

// spec_id: SPEC-4  validates_req: REQ-005  tc: TC-REQ005-01
TEST(MmapFileSourceTest, TwoLines) {
  const auto path = make_temp_file("a\nb\n");
  MmapFileSource src{path};
  src.load();
  EXPECT_EQ(src.getline(), "a");
  EXPECT_EQ(src.getline(), "b");
  EXPECT_FALSE(src.getline().has_value());
  std::filesystem::remove(path);
}

// spec_id: SPEC-4  validates_req: REQ-005  tc: TC-REQ005-02
TEST(MmapFileSourceTest, NoTrailingNewline) {
  const auto path = make_temp_file("a\nb");
  MmapFileSource src{path};
  src.load();
  EXPECT_EQ(src.getline(), "a");
  EXPECT_EQ(src.getline(), "b");
  EXPECT_FALSE(src.getline().has_value());
  std::filesystem::remove(path);
}

// spec_id: SPEC-4  validates_req: REQ-005  tc: TC-REQ005-03
TEST(MmapFileSourceTest, NonExistentThrows) {
  MmapFileSource src{"/tmp/sp04_no_exist_mmap.txt"};
  EXPECT_THROW(src.load(), std::ios_base::failure);
}

// spec_id: SPEC-4  validates_req: REQ-005  tc: TC-REQ005-04
TEST(MmapFileSourceTest, EmptyFile) {
  const auto path = make_temp_file("");
  MmapFileSource src{path};
  src.load();
  EXPECT_FALSE(src.getline().has_value());
  std::filesystem::remove(path);
}

// ---------------------------------------------------------------------------
// REQ-006 + REQ-007: make_file_source factory
// ---------------------------------------------------------------------------

// spec_id: SPEC-4  validates_req: REQ-006  tc: TC-REQ006-01
TEST(MakeFileSourceTest, StdinPath) {
  auto result = make_file_source("-");
  ASSERT_TRUE(result.has_value());
  EXPECT_NE(result->get(), nullptr);
  EXPECT_NE(dynamic_cast<StdinSource*>(result->get()), nullptr);
}

// spec_id: SPEC-4  validates_req: REQ-006  tc: TC-REQ006-02
TEST(MakeFileSourceTest, SmallFilePath) {
  const auto path = make_temp_file("hello\n");
  auto result = make_file_source(path.string());
  ASSERT_TRUE(result.has_value());
  EXPECT_NE(dynamic_cast<BufferedFileSource*>(result->get()), nullptr);
  std::filesystem::remove(path);
}

// spec_id: SPEC-4  validates_req: REQ-006  tc: TC-REQ006-03
TEST(MakeFileSourceTest, NonExistentReturnsError) {
  auto result = make_file_source("/tmp/sp04_no_such_xyz.txt");
  ASSERT_FALSE(result.has_value());
  EXPECT_NE(result.error().find("/tmp/sp04_no_such_xyz.txt"),
            std::string::npos);
}

// spec_id: SPEC-4  validates_req: REQ-006  tc: TC-REQ006-04
TEST(MakeFileSourceTest, FactoryDoesNotCallLoad) {
  const std::istringstream input_ss{"hello\n"};
  auto* const old_buf = std::cin.rdbuf(input_ss.rdbuf());
  auto result = make_file_source("-");
  ASSERT_TRUE(result.has_value());
  result->get()->load();
  std::cin.rdbuf(old_buf);
  EXPECT_EQ(result->get()->getline(), "hello");
}

// spec_id: SPEC-4  validates_req: REQ-007  tc: TC-REQ007-01
TEST(MakeFileSourceTest, ErrorStartsWithProgramName) {
  auto result = make_file_source("/no/such/path");
  ASSERT_FALSE(result.has_value());
  EXPECT_TRUE(result.error().starts_with("cc-cut-tool: "));
}

// spec_id: SPEC-4  validates_req: REQ-007  tc: TC-REQ007-02
TEST(MakeFileSourceTest, ErrorContainsPath) {
  auto result = make_file_source("/no/such/path");
  ASSERT_FALSE(result.has_value());
  EXPECT_NE(result.error().find("/no/such/path"), std::string::npos);
}

// spec_id: SPEC-4  validates_req: REQ-007  tc: TC-REQ007-03
TEST(MakeFileSourceTest, ErrorNoTrailingNewline) {
  auto result = make_file_source("/no/such/path");
  ASSERT_FALSE(result.has_value());
  EXPECT_NE(result.error().back(), '\n');
}
