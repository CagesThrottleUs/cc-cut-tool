#pragma once
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

#include "cut/file_source.hpp"

namespace cc_cut {

// spec_id: SPEC-4  req_id: REQ-004
/// Reads a file entirely into memory on load().
/// Throws std::ios_base::failure if the file cannot be opened or read.
class BufferedFileSource : public FileSource {
 public:
  explicit BufferedFileSource(std::filesystem::path path);
  void load() override;
  auto getline() -> std::optional<std::string_view> override;

 private:
  std::filesystem::path path_;
  std::string buffer_;
  std::size_t cursor_{0};
};

}  // namespace cc_cut
