// include/cut/byte_processor.hpp
#pragma once
#include <iosfwd>
#include <string>
#include <string_view>
#include <vector>

#include "cut/list.hpp"
#include "cut/options.hpp"
#include "cut/processor.hpp"

namespace cc_cut {

// spec_id: SPEC-6  req_id: REQ-001,REQ-002,REQ-003,REQ-004,REQ-005
/// Processes cut's byte mode (-b) for a given set of CutOptions.
///
/// Static helpers select_bytes and select_bytes_no_split are pure and
/// independently testable. process_line and run perform I/O through
/// std::ostream references.
///
/// @note opts.no_split controls UTF-8 boundary awareness (-n flag):
///       false → select_bytes (raw); true → select_bytes_no_split (UTF-8
///       aware).
class ByteProcessor : public Processor {
 public:
  explicit ByteProcessor(CutOptions opts);

  // spec_id: SPEC-6  req_id: REQ-002
  /// Selects raw bytes from line by 0-based position according to list.
  /// Out-of-bounds positions are silently skipped.
  /// @param line Input line (may contain arbitrary bytes including invalid
  /// UTF-8).
  /// @param list Byte positions to select; open_from selects from that position
  /// to EOL.
  /// @returns Owned string of selected bytes in order.
  static auto select_bytes(std::string_view line, const CutList& list)
      -> std::string;

  // spec_id: SPEC-6  req_id: REQ-003
  /// Selects bytes from line with UTF-8 boundary awareness (-n flag).
  /// A character is included if and only if its lead byte position is in list.
  /// Invalid UTF-8 bytes are treated as 1-byte characters (ASM-002).
  /// @param line Input line.
  /// @param list Lead-byte positions to select.
  /// @returns Owned string of selected complete UTF-8 characters.
  static auto select_bytes_no_split(std::string_view line, const CutList& list)
      -> std::string;

  // spec_id: SPEC-6  req_id: REQ-004
  /// Selects bytes from line and writes result + newline to out.
  /// Uses select_bytes_no_split if opts_.no_split, else select_bytes.
  /// Always writes at least a newline, even for empty input.
  void process_line(std::string_view line, std::ostream& out) const;

  // spec_id: SPEC-6  req_id: REQ-005
  /// Processes all files (or stdin if files is empty).
  /// Continues past individual file errors; writes errors to err.
  /// @param out Output stream for selected bytes.
  /// @param files Paths to process; empty → reads from stdin.
  /// @param err Error stream for diagnostics.
  /// @returns 0 on full success, 1 if any file error occurred.
  auto run(std::ostream& out, const std::vector<std::string>& files,
           std::ostream& err) -> int override;

 private:
  CutOptions opts_;
};

}  // namespace cc_cut
