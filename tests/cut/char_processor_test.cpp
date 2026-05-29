// tests/cut/char_processor_test.cpp
// spec_id: SPEC-7
#include "cut/char_processor.hpp"
#include "cut/mode.hpp"
#include "cut/processor.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <type_traits>

using cc_cut::CharProcessor;

namespace {

auto make_list(std::initializer_list<int> idxs) -> cc_cut::CutList {
  cc_cut::CutList list;
  list.indices = std::set<int>{idxs};
  return list;
}

auto make_open_list(int from) -> cc_cut::CutList {
  cc_cut::CutList list;
  list.open_from = from;
  return list;
}

}  // namespace

// ---------------------------------------------------------------------------
// REQ-001: CharProcessor class structure
// ---------------------------------------------------------------------------

// spec_id: SPEC-7  validates_req: REQ-001  tc: TC-REQ001-01
static_assert(!std::is_abstract_v<cc_cut::CharProcessor>,
              "CharProcessor must be concrete");

// spec_id: SPEC-7  validates_req: REQ-001  tc: TC-REQ001-02
static_assert(std::is_base_of_v<cc_cut::Processor, cc_cut::CharProcessor>,
              "CharProcessor must inherit Processor");

// spec_id: SPEC-7  validates_req: REQ-001  tc: TC-REQ001-03
static_assert(std::is_abstract_v<cc_cut::Processor>,
              "Processor base must remain abstract");

TEST(CharProcessorStructureTest, StaticAssertsCompile) {
  // static_asserts above enforce class structure at compile time.
  SUCCEED();
}
