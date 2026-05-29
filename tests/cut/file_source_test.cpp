// spec_id: SPEC-1  validates_req: REQ-004
#include "cut/file_source.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <optional>
#include <string_view>
#include <type_traits>

// spec_id: SPEC-2  validates_req: REQ-011
using cc_cut::FileSource;

// TC-REQ004-01: FileSource must be abstract.
static_assert(std::is_abstract_v<FileSource>,
              "TC-REQ004-01: FileSource must be abstract");

namespace {

class StubFileSource : public FileSource {
 public:
  void load() override {}
  auto getline() -> std::optional<std::string_view> override {
    return std::nullopt;
  }
};

}  // namespace

// spec_id: SPEC-1  validates_req: REQ-004  tc: TC-REQ004-02
TEST(FileSourceTest, ConcreteSubclassInstantiable) {
  const StubFileSource stub;
  (void)stub;
}

// spec_id: SPEC-1  validates_req: REQ-004  tc: TC-REQ004-03
TEST(FileSourceTest, GetlineReturnsNullopt) {
  StubFileSource stub;
  stub.load();
  EXPECT_EQ(stub.getline(), std::nullopt);
}

// spec_id: SPEC-1  validates_req: REQ-004  tc: TC-REQ004-04
TEST(FileSourceTest, VirtualDestructorViaBasePtr) {
  std::unique_ptr<FileSource> src = std::make_unique<StubFileSource>();
  EXPECT_EQ(src->getline(), std::nullopt);
}
