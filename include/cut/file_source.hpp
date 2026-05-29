#pragma once
#include <cstddef>
#include <optional>
#include <string_view>

// spec_id: SPEC-2  req_id: REQ-011
namespace cc_cut {

// spec_id: SPEC-4  req_id: REQ-001
inline constexpr std::size_t mmap_threshold = 100ULL * 1024ULL * 1024ULL;

// spec_id: SPEC-1  req_id: REQ-004
/// Abstract source for line-oriented reading of a single input.
///
/// Implementations provide stdin (always buffered in memory) or
/// file-backed reading (buffered <100 MB, memory-mapped >=100 MB
/// via Boost.Iostreams — see SP-04).
///
/// Usage contract:
/// 1. Call load() exactly once to populate the internal buffer.
/// 2. Call getline() repeatedly until it returns nullopt.
///
/// @note getline() advances an internal cursor — it is a query with
///       a side effect, matching std::istream convention.
///
/// @code
///   std::unique_ptr<cc_cut::FileSource> src = make_file_source("data.tsv");
///   src->load();
///   while (auto line = src->getline()) {
///       process(*line);
///   }
/// @endcode
class FileSource {
 public:
  FileSource() = default;
  FileSource(const FileSource&) = delete;
  auto operator=(const FileSource&) -> FileSource& = delete;
  FileSource(FileSource&&) = delete;
  auto operator=(FileSource&&) -> FileSource& = delete;

  // spec_id: SPEC-1  req_id: REQ-004
  /// Reads the entire input into an internal buffer.
  ///
  /// Must be called exactly once before getline(). Calling more
  /// than once is undefined behaviour.
  ///
  /// @throws std::ios_base::failure If the underlying source cannot be
  ///         opened or read (e.g. file missing, permission denied, I/O
  ///         error). Implementations must not silently swallow errors.
  virtual void load() = 0;

  // spec_id: SPEC-1  req_id: REQ-004
  /// Returns the next line without its trailing newline character.
  ///
  /// @return A string_view into the internal buffer for the next
  ///         line, or nullopt when all lines are consumed.
  /// @pre    load() has been called.
  /// @warning The returned string_view aliases the internal buffer.
  ///          It is invalidated when load() is called again or when
  ///          the FileSource is destroyed. Storing it past either
  ///          event is undefined behaviour.
  virtual auto getline() -> std::optional<std::string_view> = 0;

  virtual ~FileSource() = default;

 protected:
  // spec_id: SPEC-4  req_id: REQ-002
  /// Returns [cursor, next-newline) without the newline. Advances cursor past
  /// it. Returns nullopt when cursor >= buffer.size(). CR before LF is included
  /// — no CRLF normalization.
  static auto next_line(std::string_view buffer, std::size_t& cursor)
      -> std::optional<std::string_view> {
    if (cursor >= buffer.size()) {
      return std::nullopt;
    }
    const auto pos = buffer.find('\n', cursor);
    if (pos == std::string_view::npos) {
      const auto result = buffer.substr(cursor);
      cursor = buffer.size();
      return result;
    }
    const auto result = buffer.substr(cursor, pos - cursor);
    cursor = pos + 1;
    return result;
  }
};

}  // namespace cc_cut
