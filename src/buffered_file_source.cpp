#include "cut/buffered_file_source.hpp"

#include <filesystem>
#include <fstream>
#include <iterator>
#include <optional>
#include <string_view>
#include <utility>

namespace cc_cut {

BufferedFileSource::BufferedFileSource(std::filesystem::path path)
    : path_(std::move(path)) {}

void BufferedFileSource::load() {
  std::ifstream ifs;
  ifs.exceptions(std::ifstream::failbit | std::ifstream::badbit);
  ifs.open(path_);
  buffer_.assign(std::istreambuf_iterator<char>{ifs},
                 std::istreambuf_iterator<char>{});
  cursor_ = 0;
}

auto BufferedFileSource::getline() -> std::optional<std::string_view> {
  return next_line(buffer_, cursor_);
}

}  // namespace cc_cut
