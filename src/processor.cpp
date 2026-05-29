// src/processor.cpp
// spec_id: SPEC-6,SPEC-7
#include "cut/processor.hpp"

#include <expected>
#include <ios>
#include <memory>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "cut/byte_processor.hpp"
#include "cut/char_processor.hpp"
#include "cut/config.hpp"
#include "cut/field_processor.hpp"
#include "cut/make_file_source.hpp"
#include "cut/mode.hpp"
#include "cut/options.hpp"

namespace cc_cut {

// spec_id: SPEC-6  req_id: REQ-005
// spec_id: SPEC-7  req_id: REQ-005
auto Processor::run(std::ostream& out, const std::vector<std::string>& files,
                    std::ostream& err) -> int {
  int exit_code = 0;

  const auto process_source = [&](const std::string& path) -> void {
    auto source_result = make_file_source(path);
    if (!source_result) {
      err << source_result.error() << '\n';
      exit_code = 1;
      return;
    }
    try {
      (*source_result)->load();
    } catch (const std::ios_base::failure& ex) {
      err << config::program_name << ": " << path << ": " << ex.what() << '\n';
      exit_code = 1;
      return;
    }
    while (auto line = (*source_result)->getline()) {
      process_line(*line, out);
    }
  };

  if (files.empty()) {
    process_source("-");
  } else {
    for (const auto& path : files) {
      process_source(path);
    }
  }

  return exit_code;
}

// spec_id: SPEC-6  req_id: REQ-006
// spec_id: SPEC-7  req_id: REQ-006
auto make_processor(const CutOptions& opts)
    -> std::expected<std::unique_ptr<Processor>, std::string> {
  switch (opts.mode) {
    case CutMode::BYTE:
      return std::make_unique<ByteProcessor>(opts);
    case CutMode::FIELD:
      return std::make_unique<FieldProcessor>(opts);
    case CutMode::CHARACTER:
      return std::make_unique<CharProcessor>(opts);
  }
  std::unreachable();  // all CutMode enumerators handled above
}

}  // namespace cc_cut
