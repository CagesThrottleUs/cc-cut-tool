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
