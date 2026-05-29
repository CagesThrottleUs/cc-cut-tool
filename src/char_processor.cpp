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
auto CharProcessor::select_chars(std::string_view line,
                                 const CutList& list) -> std::string {
  std::string result;

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  const auto* const base = reinterpret_cast<const utf8::utfchar8_t*>(line.data());
  const auto* const end_ptr =
      std::next(base, static_cast<std::ptrdiff_t>(line.size()));

  // NOLINTNEXTLINE(readability-qualified-auto)
  auto cur = base;
  std::size_t cp_idx = 0;

  while (cur != end_ptr) {
    const auto* const seq_start = cur;
    const auto err = utf8::internal::validate_next(cur, end_ptr);
    if (err != utf8::internal::UTF8_OK) {
      cur = std::next(seq_start);  // ASM-003: invalid byte = 1 codepoint
    }

    const bool in_indices =
        (cp_idx <= static_cast<std::size_t>(std::numeric_limits<int>::max())) &&
        list.indices.contains(static_cast<int>(cp_idx));
    const bool in_open_range =
        list.open_from.has_value() &&
        std::cmp_greater_equal(cp_idx, list.open_from.value());
    if (in_indices || in_open_range) {
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
      result.append(reinterpret_cast<const char*>(seq_start),
                    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
                    reinterpret_cast<const char*>(cur));
    }
    ++cp_idx;
  }
  return result;
}

// spec_id: SPEC-7  req_id: REQ-004
void CharProcessor::process_line(std::string_view line, std::ostream& out) const {
  out << select_chars(line, opts_.list) << '\n';
}

// spec_id: SPEC-7  req_id: REQ-005
auto CharProcessor::run(std::ostream& out, const std::vector<std::string>& files,
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

}  // namespace cc_cut
