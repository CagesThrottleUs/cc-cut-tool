#pragma once
#include <optional>
#include <string_view>

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
///       a side effect, matching std::istream convention (SPEC-1 REQ-004).
///
/// @code
///   std::unique_ptr<FileSource> src = make_file_source("data.tsv");
///   src->load();
///   while (auto line = src->getline()) {
///       process(*line);
///   }
/// @endcode
class FileSource {
public:
    FileSource()                           = default;
    FileSource(const FileSource&)                    = delete;
    auto operator=(const FileSource&) -> FileSource& = delete;
    FileSource(FileSource&&)                         = delete;
    auto operator=(FileSource&&)      -> FileSource& = delete;

    /// Reads the entire input into an internal buffer.
    ///
    /// Must be called exactly once before getline(). Calling more
    /// than once is undefined behaviour.
    virtual void load() = 0;

    /// Returns the next line without its trailing newline character.
    ///
    /// @return A string_view into the internal buffer for the next
    ///         line, or nullopt when all lines are consumed.
    /// @pre    load() has been called.
    /// @note   The returned string_view is valid until load() is
    ///         called again or the FileSource is destroyed.
    virtual auto getline() -> std::optional<std::string_view> = 0;

    virtual ~FileSource() = default;
};
