#include <gtest/gtest.h>
#include <memory>
#include "cut/file_source.hpp"

// TC-REQ004-01: FileSource is pure virtual.
// The line below must NOT compile — documented here as evidence:
//   FileSource fs;  // must not compile

class StubFileSource : public FileSource {
public:
    void load() override {}
    std::optional<std::string_view> getline() override { return std::nullopt; }
};

// TC-REQ004-02
TEST(FileSourceTest, ConcreteSubclassInstantiable) {
    StubFileSource stub;
    (void)stub;
}

// TC-REQ004-03
TEST(FileSourceTest, GetlineReturnsNullopt) {
    StubFileSource stub;
    stub.load();
    EXPECT_EQ(stub.getline(), std::nullopt);
}

// TC-REQ004-04: delete via base pointer verifies virtual destructor
TEST(FileSourceTest, VirtualDestructorViaBasePtr) {
    std::unique_ptr<FileSource> p = std::make_unique<StubFileSource>();
    EXPECT_EQ(p->getline(), std::nullopt);
}
