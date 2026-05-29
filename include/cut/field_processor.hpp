// include/cut/field_processor.hpp
#pragma once
#include <iosfwd>
#include <string>
#include <string_view>
#include <vector>

#include "cut/list.hpp"
#include "cut/options.hpp"
#include "cut/processor.hpp"

namespace cc_cut {

// spec_id: SPEC-5  req_id: REQ-001,REQ-002,REQ-003,REQ-004,REQ-005,REQ-006
/// Processes cut's field mode (-f) for a given set of CutOptions.
///
/// Static helpers split_fields and select_fields are pure and independently
/// testable. process_line and run perform I/O through std::ostream references.
///
/// @note opts.delim is expected to have a value for all CLI-reachable paths
///       in SP-05; whitespace-mode output (delim=nullopt) is out of scope.
class FieldProcessor : public Processor {
 public:
  explicit FieldProcessor(CutOptions opts);

  // spec_id: SPEC-5  req_id: REQ-002
  /// Splits line on every occurrence of delim. Empty fields are preserved.
  /// An empty line produces exactly one empty-string field.
  static auto split_fields(std::string_view line, char delim)
      -> std::vector<std::string_view>;

  // spec_id: SPEC-5  req_id: REQ-003
  /// Splits line on runs of ASCII space/tab. Leading/trailing whitespace and
  /// consecutive whitespace produce no empty fields.
  static auto split_fields(std::string_view line)
      -> std::vector<std::string_view>;

  // spec_id: SPEC-5  req_id: REQ-004
  /// Returns fields selected by list in ascending position order.
  /// Positions beyond fields.size() produce empty string_view entries.
  static auto select_fields(const std::vector<std::string_view>& fields,
                            const CutList& list)
      -> std::vector<std::string_view>;

  // spec_id: SPEC-5  req_id: REQ-005
  /// Processes one line: splits, selects, joins with delimiter, writes to out.
  /// If no delimiter found and suppress=true: writes nothing.
  /// If no delimiter found and suppress=false: writes line unchanged + newline.
  void process_line(std::string_view line, std::ostream& out);

  // spec_id: SPEC-5  req_id: REQ-006
  /// Processes all files (or stdin if files is empty).
  /// Continues past individual file errors; writes errors to err.
  /// Returns 0 on full success, 1 if any file error occurred.
  auto run(std::ostream& out, const std::vector<std::string>& files,
           std::ostream& err) -> int override;

 private:
  CutOptions opts_;
};

}  // namespace cc_cut
