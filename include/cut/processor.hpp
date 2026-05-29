// include/cut/processor.hpp
#pragma once
#include <expected>
#include <iosfwd>
#include <memory>
#include <string>
#include <vector>

#include "cut/options.hpp"

namespace cc_cut {

// spec_id: SPEC-6  req_id: REQ-006
class Processor {
 public:
  Processor() = default;
  Processor(const Processor&) = delete;
  Processor(Processor&&) = delete;
  auto operator=(const Processor&) -> Processor& = delete;
  auto operator=(Processor&&) -> Processor& = delete;
  virtual ~Processor() = default;
  virtual auto run(std::ostream& out, const std::vector<std::string>& files,
                   std::ostream& err) -> int = 0;
};

// spec_id: SPEC-6  req_id: REQ-006
auto make_processor(const CutOptions& opts)
    -> std::expected<std::unique_ptr<Processor>, std::string>;

}  // namespace cc_cut
