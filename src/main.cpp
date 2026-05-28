// src/main.cpp
#include <iostream>

#include "cut/arg_parser.hpp"
#include "cut/field_processor.hpp"
#include "cut/mode.hpp"

// spec_id: SPEC-5  req_id: REQ-007
// NOLINTNEXTLINE(bugprone-exception-escape)
auto main(int argc, char** argv) -> int {
  auto result = cc_cut::parse_args(argc, argv);
  if (!result) {
    std::cerr << result.error() << '\n';
    return 1;
  }
  if (result->help_requested) {
    return 0;
  }
  if (result->opts.mode != cc_cut::CutMode::FIELD) {
    std::cerr << "cc-cut-tool: byte and character modes not yet implemented\n";
    return 1;
  }
  return cc_cut::FieldProcessor{result->opts}.run(std::cout, result->files,
                                                  std::cerr);
}
