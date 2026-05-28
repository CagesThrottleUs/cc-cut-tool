// spec_id: SPEC-1  validates_req: REQ-004
#include <gtest/gtest.h>
#include <memory>
#include <type_traits>
#include "cut/file_source.hpp"

using namespace cc_cut;

// TC-REQ004-01: FileSource must be abstract.
static_assert(std::is_abstract_v<FileSource>,
              "TC-REQ004-01: FileSource must be abstract");

class StubFileSource : public FileSource {
public:
    void load() override {}
    auto getline() -> std::optional<std::string_view> override { return std::nullopt; }
};

// spec_id: SPEC-1  validates_req: REQ-004  tc: TC-REQ004-02
TEST(FileSourceTest, ConcreteSubclassInstantiable) {
    StubFileSource stub;
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
    std::unique_ptr<FileSource> p = std::make_unique<StubFileSource>();
    EXPECT_EQ(p->getline(), std::nullopt);
}
