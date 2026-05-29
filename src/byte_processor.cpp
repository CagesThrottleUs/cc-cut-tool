// src/byte_processor.cpp
// spec_id: SPEC-6
#include "cut/byte_processor.hpp"

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

// spec_id: SPEC-6  req_id: REQ-001
ByteProcessor::ByteProcessor(CutOptions opts) : opts_(std::move(opts)) {}

// spec_id: SPEC-6  req_id: REQ-002
auto ByteProcessor::select_bytes(std::string_view line, const CutList& list)
    -> std::string {
  std::string result;
  const std::size_t open_start =
      list.open_from.has_value()
          ? static_cast<std::size_t>(list.open_from.value())
          : std::numeric_limits<std::size_t>::max();
  for (std::size_t pos = 0; pos < line.size(); ++pos) {
    const bool in_indices =
        (pos <= static_cast<std::size_t>(std::numeric_limits<int>::max())) &&
        list.indices.contains(static_cast<int>(pos));
    if (in_indices || pos >= open_start) {
      result += line.at(pos);
    }
  }
  return result;
}

// spec_id: SPEC-6  req_id: REQ-003
auto ByteProcessor::select_bytes_no_split(std::string_view line,
                                          const CutList& list) -> std::string {
  std::string result;
  const std::size_t open_start_sz =
      list.open_from.has_value()
          ? static_cast<std::size_t>(list.open_from.value())
          : std::numeric_limits<std::size_t>::max();
  const auto* const base =
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
      reinterpret_cast<const utf8::utfchar8_t*>(line.data());
  const auto* const end_ptr =
      std::next(base, static_cast<std::ptrdiff_t>(line.size()));

  // NOLINTNEXTLINE(readability-qualified-auto)
  auto cur = base;
  while (cur != end_ptr) {
    const auto* const seq_start = cur;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    const auto start_pos = static_cast<std::size_t>(seq_start - base);

    const auto err = utf8::internal::validate_next(cur, end_ptr);
    if (err != utf8::internal::UTF8_OK) {
      cur = std::next(seq_start);  // ASM-002: treat invalid byte as 1-byte char
    }

    const bool in_indices =
        (start_pos <=
         static_cast<std::size_t>(std::numeric_limits<int>::max())) &&
        list.indices.contains(static_cast<int>(start_pos));
    if (in_indices || start_pos >= open_start_sz) {
      result.append(
          // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
          reinterpret_cast<const char*>(seq_start),
          // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
          reinterpret_cast<const char*>(cur));
    }
  }
  return result;
}

// spec_id: SPEC-6  req_id: REQ-004
void ByteProcessor::process_line(std::string_view line,
                                 std::ostream& out) const {
  if (opts_.no_split) {
    out << select_bytes_no_split(line, opts_.list) << '\n';
  } else {
    out << select_bytes(line, opts_.list) << '\n';
  }
}

// spec_id: SPEC-6  req_id: REQ-005
auto ByteProcessor::run(std::ostream& out,
                        const std::vector<std::string>& files,
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
