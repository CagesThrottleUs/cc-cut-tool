// include/cut/processor.hpp
#pragma once
#include <expected>
#include <iosfwd>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "cut/options.hpp"

namespace cc_cut {

// spec_id: SPEC-6  req_id: REQ-006
/// Abstract base for all cut mode processors.
///
/// Subclasses implement process_line() for a specific CutMode.
/// Copy and move are deleted — processors are consumed only via unique_ptr.
/// run() drives the file loop and delegates each line to process_line().
class Processor {
 public:
  Processor() = default;
  Processor(const Processor&) = delete;
  Processor(Processor&&) = delete;
  auto operator=(const Processor&) -> Processor& = delete;
  auto operator=(Processor&&) -> Processor& = delete;
  virtual ~Processor() = default;

  /// Processes all files (or stdin if files is empty).
  /// Continues past individual file errors; writes error messages to err.
  /// @param out Output stream for selected bytes/fields.
  /// @param files Paths to process; "-" or empty → reads from stdin.
  /// @param err Error stream for diagnostics.
  /// @returns 0 on full success, 1 if any file error occurred.
  [[nodiscard]] auto run(std::ostream& out,
                         const std::vector<std::string>& files,
                         std::ostream& err) -> int;

 protected:
  /// Selects and writes one line's output + newline to out.
  /// Called by run() for every input line.
  virtual void process_line(std::string_view line, std::ostream& out) const = 0;
};

// spec_id: SPEC-6  req_id: REQ-006
// spec_id: SPEC-7  req_id: REQ-006
/// Factory that returns a Processor for the mode specified in opts.
/// @param opts Parsed CLI options; opts.mode determines which processor is
/// created.
/// @returns ByteProcessor for BYTE mode, FieldProcessor for FIELD mode,
///          CharProcessor for CHARACTER mode.
[[nodiscard]] auto make_processor(const CutOptions& opts)
    -> std::expected<std::unique_ptr<Processor>, std::string>;

}  // namespace cc_cut
