// src/byte_processor.cpp
#include "cut/byte_processor.hpp"

#include <cstddef>
#include <iterator>
#include <limits>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "cut/list.hpp"
#include "cut/options.hpp"
#include "utf8/core.h"

namespace cc_cut {

ByteProcessor::ByteProcessor(CutOptions opts) : opts_(std::move(opts)) {}

auto ByteProcessor::select_bytes(std::string_view line, const CutList& list)
    -> std::string {
  std::string result;
  const auto size = static_cast<int>(line.size());
  const int open_start =
      list.open_from.value_or(std::numeric_limits<int>::max());
  for (int pos = 0; pos < size; ++pos) {
    if (list.indices.contains(pos) || pos >= open_start) {
      result += line.at(static_cast<std::size_t>(pos));
    }
  }
  return result;
}

auto ByteProcessor::select_bytes_no_split(std::string_view line,
                                          const CutList& list) -> std::string {
  std::string result;
  const int open_start =
      list.open_from.value_or(std::numeric_limits<int>::max());

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  const auto* const base = reinterpret_cast<const utf8::utfchar8_t*>(line.data());
  const auto* const end_ptr = std::next(base, static_cast<std::ptrdiff_t>(line.size()));

  // NOLINTNEXTLINE(readability-qualified-auto)
  auto cur = base;
  while (cur != end_ptr) {
    const auto* const seq_start = cur;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    const int start_pos = static_cast<int>(seq_start - base);

    const auto err = utf8::internal::validate_next(cur, end_ptr);
    if (err != utf8::internal::UTF8_OK) {
      cur = std::next(seq_start);  // ASM-002: treat invalid byte as 1-byte char
    }

    if (list.indices.contains(start_pos) || start_pos >= open_start) {
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
      result.append(reinterpret_cast<const char*>(seq_start),
                    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
                    reinterpret_cast<const char*>(cur));
    }
  }
  return result;
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void ByteProcessor::process_line(std::string_view line, std::ostream& out) {
  (void)line;
  (void)out;
}

auto ByteProcessor::run(std::ostream& out, const std::vector<std::string>& files,
                        std::ostream& err) -> int {
  (void)out;
  (void)files;
  (void)err;
  return 0;
}

}  // namespace cc_cut
