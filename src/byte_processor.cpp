// src/byte_processor.cpp
// spec_id: SPEC-6
#include "cut/byte_processor.hpp"

#include <cstddef>
#include <iterator>
#include <limits>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>

#include "cut/list.hpp"
#include "cut/options.hpp"
#include "cut/utf8_util.hpp"

namespace cc_cut {

// spec_id: SPEC-6  req_id: REQ-001
ByteProcessor::ByteProcessor(CutOptions opts) : opts_(std::move(opts)) {}

// spec_id: SPEC-6  req_id: REQ-002
auto ByteProcessor::select_bytes(std::string_view line, const CutList& list)
    -> std::string {
  std::string result;
  result.reserve(line.size());
  const std::size_t open_start =
      list.open_from.value_or(std::numeric_limits<std::size_t>::max());
  for (std::size_t pos = 0; pos < line.size(); ++pos) {
    if (list.indices.contains(pos) || pos >= open_start) {
      result.push_back(line.at(pos));
    }
  }
  return result;
}

// spec_id: SPEC-6  req_id: REQ-003
auto ByteProcessor::select_bytes_no_split(std::string_view line,
                                          const CutList& list) -> std::string {
  std::string result;
  result.reserve(line.size());
  const std::size_t open_start_sz =
      list.open_from.value_or(std::numeric_limits<std::size_t>::max());
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

    const auto err = detail::utf8_advance_one(cur, end_ptr);
    if (err != utf8::internal::UTF8_OK) {
      cur = std::next(seq_start);  // ASM-002: treat invalid byte as 1-byte char
    }

    if (list.indices.contains(start_pos) || start_pos >= open_start_sz) {
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

}  // namespace cc_cut
