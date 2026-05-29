// src/char_processor.cpp
// spec_id: SPEC-7
#include "cut/char_processor.hpp"

#include <cstddef>
#include <ios>
#include <iterator>
#include <limits>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "cut/config.hpp"
#include "cut/list.hpp"
#include "cut/make_file_source.hpp"
#include "cut/options.hpp"
#include "utf8/core.h"

namespace cc_cut {

// spec_id: SPEC-7  req_id: REQ-001
CharProcessor::CharProcessor(CutOptions opts) : opts_(std::move(opts)) {}

// spec_id: SPEC-7  req_id: REQ-002,REQ-003
auto CharProcessor::select_chars(std::string_view /*line*/,
                                 const CutList& /*list*/) -> std::string {
  return {};
}

// spec_id: SPEC-7  req_id: REQ-004
void CharProcessor::process_line(std::string_view line, std::ostream& out) const {
  out << select_chars(line, opts_.list) << '\n';
}

// spec_id: SPEC-7  req_id: REQ-005
auto CharProcessor::run(std::ostream& /*out*/,
                        const std::vector<std::string>& /*files*/,
                        std::ostream& /*err*/) -> int {
  return 0;
}

}  // namespace cc_cut
