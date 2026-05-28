// include/cut/parse_result.hpp
#pragma once
#include "cut/options.hpp"

#include <string>
#include <vector>

namespace cc_cut {

// spec_id: SPEC-3  req_id: REQ-001
/// Aggregated result of parsing CLI arguments.
///
/// `help_requested == true` signals the caller to exit 0 after
/// printing help — parse_args has already written to stdout.
///
/// @code
///   auto result = cc_cut::parse_args(argc, argv);
///   if (result && result->help_requested) { return 0; }
/// @endcode
struct ParseResult {
  CutOptions               opts;
  std::vector<std::string> files;
  bool                     help_requested = false;
};

}  // namespace cc_cut
