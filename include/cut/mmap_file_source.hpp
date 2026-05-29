#pragma once
#include <boost/iostreams/device/mapped_file.hpp>
#include <cstddef>
#include <filesystem>
#include <optional>
#include <string_view>

#include "cut/file_source.hpp"

namespace cc_cut {

// spec_id: SPEC-4  req_id: REQ-005
/// Memory-maps a file via Boost.Iostreams on load().
/// getline() returns zero-copy string_view slices into the mapped region.
/// Throws std::ios_base::failure if the file cannot be mapped.
class MmapFileSource final : public FileSource {
 public:
  explicit MmapFileSource(std::filesystem::path path);
  void load() override;
  [[nodiscard]] auto getline() -> std::optional<std::string_view> override;

 private:
  std::filesystem::path path_;
  boost::iostreams::mapped_file_source map_;
  std::string_view buffer_;
  std::size_t cursor_{0};
};

}  // namespace cc_cut
