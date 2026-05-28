#pragma once
#include <expected>
#include <memory>
#include <string>
#include <string_view>

#include "cut/file_source.hpp"

namespace cc_cut {

// spec_id: SPEC-4  req_id: REQ-006,REQ-007
/// Creates a FileSource appropriate for path.
///
/// Dispatch rules:
/// - path == "-"                       → StdinSource
/// - file_size(path) < mmap_threshold  → BufferedFileSource
/// - file_size(path) >= mmap_threshold → MmapFileSource
///
/// Returns error "cc-cut-tool: <path>: <reason>" on failure.
/// Does NOT call load() — caller must call load() after receiving the source.
///
/// @param path  File path or "-" for stdin.
/// @return      FileSource on success; error string on failure.
/// @throws      Never throws.
auto make_file_source(std::string_view path)
    -> std::expected<std::unique_ptr<FileSource>, std::string>;

}  // namespace cc_cut
