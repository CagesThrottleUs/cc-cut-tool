// src/byte_processor.cpp
#include "cut/byte_processor.hpp"

#include <cstddef>
#include <limits>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "cut/list.hpp"
#include "cut/options.hpp"

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
  (void)line;
  (void)list;
  return {};
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
