#include "cut/mmap_file_source.hpp"

#include <exception>
#include <filesystem>
#include <ios>
#include <optional>
#include <string_view>
#include <utility>

namespace cc_cut {

MmapFileSource::MmapFileSource(std::filesystem::path path)
    : path_(std::move(path)) {}

void MmapFileSource::load() {
  try {
    // mmap of a zero-byte file is undefined on most OSes; short-circuit.
    if (std::filesystem::file_size(path_) == 0) {
      buffer_ = std::string_view{};
      cursor_ = 0;
      return;
    }
    map_.open(path_.string());
  } catch (const std::exception& ex) {
    throw std::ios_base::failure(ex.what());
  }
  if (!map_.is_open()) {
    throw std::ios_base::failure("failed to map: " + path_.string());
  }
  buffer_ = std::string_view{map_.data(), map_.size()};
  cursor_ = 0;
}

auto MmapFileSource::getline() -> std::optional<std::string_view> {
  return next_line(buffer_, cursor_);
}

}  // namespace cc_cut
