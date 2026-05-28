#include "cut/make_file_source.hpp"

#include <expected>
#include <filesystem>
#include <format>
#include <memory>
#include <string>
#include <string_view>

#include "cut/buffered_file_source.hpp"
#include "cut/config.hpp"
#include "cut/file_source.hpp"
#include "cut/mmap_file_source.hpp"
#include "cut/stdin_source.hpp"

namespace cc_cut {

auto make_file_source(std::string_view path)
    -> std::expected<std::unique_ptr<FileSource>, std::string> {
  if (path == "-") {
    return std::make_unique<StdinSource>();
  }
  try {
    const std::filesystem::path fpath{path};
    const auto size = std::filesystem::file_size(fpath);
    if (size >= mmap_threshold) {
      return std::make_unique<MmapFileSource>(fpath);
    }
    return std::make_unique<BufferedFileSource>(fpath);
  } catch (const std::filesystem::filesystem_error& ex) {
    return std::unexpected(std::format("{}: {}: {}", config::program_name, path,
                                       ex.code().message()));
  }
}

}  // namespace cc_cut
