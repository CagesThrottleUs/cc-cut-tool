// include/cut/arg_parser.hpp
#pragma once
#include <expected>
#include <string>
#include <string_view>
#include <vector>

#include "cut/mode.hpp"
#include "cut/options.hpp"
#include "cut/parse_result.hpp"

namespace cc_cut {

// Public API uses int argc, char** argv to match main()'s POSIX signature.
// Implementations convert to std::span<char*> internally for safe iteration.

// spec_id: SPEC-3  req_id: REQ-003,REQ-010
/// Parse CLI arguments into a typed ParseResult.
///
/// Orchestrates detect_mode, extract_list_spec, parse_list,
/// parse_mode_properties, and collect_files in order. Returns the first
/// error encountered. Checks for --help before mode detection; prints
/// help to stdout and sets help_requested on match.
///
/// @param argc  Argument count (including program name at argv[0]).
/// @param argv  Null-terminated argument vector.
/// @return      ParseResult on success; error string on failure.
///              Error string format: "cc-cut-tool: <msg>\nTry '...'".
/// @throws      Never throws.
auto parse_args(int argc, char** argv)
    -> std::expected<ParseResult, std::string>;

// spec_id: SPEC-3  req_id: REQ-004
/// Identify the cut mode from the first CLI flag.
///
/// @param flag  The first argument (e.g. "-f", "-b3-5").
/// @return      CutMode on "-b"/"-c"/"-f"; error on anything else.
///              Error format: "cc-cut-tool: invalid option -- '<c>'\n..."
auto detect_mode(std::string_view flag) -> std::expected<CutMode, std::string>;

// spec_id: SPEC-3  req_id: REQ-005
/// Extract the list specification string from argv.
///
/// If flag.size() > 2 (attached, e.g. "-f1,3") returns flag.substr(2)
/// and leaves index unchanged. Otherwise consumes argv[index] as the
/// list spec and increments index by 1.
///
/// @param flag   Mode flag (argv[1], e.g. "-f" or "-f1,3").
/// @param argc   Total argument count.
/// @param argv   Argument vector.
/// @param index  Position of first arg after the flag (starts at 2).
/// @return       List spec string_view on success; error on missing arg.
/// @pre          index >= 2 (flag has been consumed as argv[1]).
auto extract_list_spec(std::string_view flag, int argc, char** argv, int& index)
    -> std::expected<std::string_view, std::string>;

// spec_id: SPEC-3  req_id: REQ-006
/// Consume zero or more mode-specific flags from argv[index..].
///
/// Advances index for each consumed flag. Stops at argv[index] when
/// it is not a recognised mode property; leaves it for collect_files.
///
/// @param argc   Total argument count.
/// @param argv   Argument vector.
/// @param index  Current parse position; modified in-place.
/// @param opts   Options updated in-place.
/// @return       void on success; error if a property value is invalid.
auto parse_mode_properties(int argc, char** argv, int& index, CutOptions& opts)
    -> std::expected<void, std::string>;

// spec_id: SPEC-3  req_id: REQ-007
/// Collect remaining argv elements as file paths, first-occurrence dedup.
///
/// @param argc   Total argument count.
/// @param argv   Argument vector.
/// @param index  First position to collect from.
/// @return       Deduplicated file paths in first-occurrence order.
auto collect_files(int argc, char** argv, int index)
    -> std::vector<std::string>;

}  // namespace cc_cut
