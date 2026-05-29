// include/cut/char_processor.hpp
#pragma once
#include <iosfwd>
#include <string>
#include <string_view>
#include <vector>

#include "cut/list.hpp"
#include "cut/options.hpp"
#include "cut/processor.hpp"

namespace cc_cut {

// spec_id: SPEC-7  req_id: REQ-001,REQ-002,REQ-003,REQ-004,REQ-005
/// Processes cut's character mode (-c) for a given set of CutOptions.
///
/// select_chars iterates the input codepoint by codepoint via
/// utf8::internal::validate_next. CutList indices are 0-based codepoint
/// positions. Invalid UTF-8 bytes are treated as single codepoints (ASM-003).
///
/// @note CHARACTER mode has no additional flags (-n is BYTE-only, -s/-d are
///       FIELD-only).
class CharProcessor : public Processor {
 public:
  explicit CharProcessor(CutOptions opts);

  // spec_id: SPEC-7  req_id: REQ-002,REQ-003
  /// Selects codepoints from line by 0-based codepoint index according to list.
  /// Invalid UTF-8 bytes are treated as single codepoints (ASM-003).
  /// @param line  Input line (may contain arbitrary bytes including invalid
  /// UTF-8).
  /// @param list  Codepoint positions to select; open_from selects from that
  /// index to EOL.
  /// @returns Owned string of selected codepoints' UTF-8 bytes, in order.
  static auto select_chars(std::string_view line, const CutList& list)
      -> std::string;

  // spec_id: SPEC-7  req_id: REQ-004
  /// Calls select_chars and writes result + '\n' to out.
  /// Always writes the newline, even for empty input.
  void process_line(std::string_view line, std::ostream& out) const;

  // spec_id: SPEC-7  req_id: REQ-005
  /// Processes all files (or stdin when files is empty).
  /// Continues past individual file errors; writes errors to err.
  /// @returns 0 on full success, 1 if any file error occurred.
  auto run(std::ostream& out, const std::vector<std::string>& files,
           std::ostream& err) -> int override;

 private:
  CutOptions opts_;
};

}  // namespace cc_cut
