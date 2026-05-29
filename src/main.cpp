// src/main.cpp
#include <iostream>

#include "cut/arg_parser.hpp"
#include "cut/processor.hpp"

// spec_id: SPEC-6  req_id: REQ-007
auto main(int argc, char** argv) -> int {
  auto result = cc_cut::parse_args(argc, argv);
  if (!result) {
    std::cerr << result.error() << '\n';
    return 1;
  }
  if (result->help_requested) {
    return 0;
  }
  auto proc = cc_cut::make_processor(result->opts);
  if (!proc) {
    std::cerr << proc.error() << '\n';
    return 1;
  }
  return (*proc)->run(std::cout, result->files, std::cerr);
}
