// src/char_processor.cpp
// spec_id: SPEC-7
#include "cut/char_processor.hpp"

#include <cstddef>
#include <iterator>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>

#include "cut/list.hpp"
#include "cut/options.hpp"
#include "cut/utf8_util.hpp"

namespace cc_cut {

// spec_id: SPEC-7  req_id: REQ-001
CharProcessor::CharProcessor(CutOptions opts) : opts_(std::move(opts)) {}

// spec_id: SPEC-7  req_id: REQ-002,REQ-003
auto CharProcessor::select_chars(std::string_view line, const CutList& list)
    -> std::string {
  std::string result;
  result.reserve(line.size());

  const auto* const base =
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
      reinterpret_cast<const utf8::utfchar8_t*>(line.data());
  const auto* const end_ptr =
      std::next(base, static_cast<std::ptrdiff_t>(line.size()));

  // NOLINTNEXTLINE(readability-qualified-auto)
  auto cur = base;
  std::size_t cp_idx = 0;

  while (cur != end_ptr) {
    const auto* const seq_start = cur;
    const auto err = detail::utf8_advance_one(cur, end_ptr);
    if (err != utf8::internal::UTF8_OK) {
      cur = std::next(seq_start);  // ASM-003: invalid byte = 1 codepoint
    }

    const bool in_open_range =
        list.open_from.has_value() && cp_idx >= list.open_from.value();
    if (list.indices.contains(cp_idx) || in_open_range) {
      result.append(
          // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
          reinterpret_cast<const char*>(seq_start),
          // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
          reinterpret_cast<const char*>(cur));
    }
    ++cp_idx;
  }
  return result;
}

// spec_id: SPEC-7  req_id: REQ-004
void CharProcessor::process_line(std::string_view line,
                                 std::ostream& out) const {
  out << select_chars(line, opts_.list) << '\n';
}

}  // namespace cc_cut
