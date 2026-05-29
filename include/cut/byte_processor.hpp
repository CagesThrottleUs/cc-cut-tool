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
class ByteProcessor : public Processor {
 public:
  explicit ByteProcessor(CutOptions opts);

  // spec_id: SPEC-6  req_id: REQ-002
  static auto select_bytes(std::string_view line, const CutList& list)
      -> std::string;

  // spec_id: SPEC-6  req_id: REQ-003
  static auto select_bytes_no_split(std::string_view line, const CutList& list)
      -> std::string;

  // spec_id: SPEC-6  req_id: REQ-004
  void process_line(std::string_view line, std::ostream& out);

  // spec_id: SPEC-6  req_id: REQ-005
  auto run(std::ostream& out, const std::vector<std::string>& files,
           std::ostream& err) -> int override;

 private:
  CutOptions opts_;
};

}  // namespace cc_cut
