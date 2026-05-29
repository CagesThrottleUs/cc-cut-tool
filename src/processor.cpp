// src/processor.cpp
// spec_id: SPEC-6,SPEC-7
#include "cut/processor.hpp"

#include <expected>
#include <memory>
#include <string>

#include "cut/byte_processor.hpp"
#include "cut/char_processor.hpp"
#include "cut/field_processor.hpp"
#include "cut/mode.hpp"
#include "cut/options.hpp"

namespace cc_cut {

// spec_id: SPEC-6  req_id: REQ-006
// spec_id: SPEC-7  req_id: REQ-006
auto make_processor(const CutOptions& opts)
    -> std::expected<std::unique_ptr<Processor>, std::string> {
  switch (opts.mode) {
    case CutMode::BYTE:
      return std::make_unique<ByteProcessor>(opts);
    case CutMode::FIELD:
      return std::make_unique<FieldProcessor>(opts);
    case CutMode::CHARACTER:
      return std::make_unique<CharProcessor>(opts);
  }
}

}  // namespace cc_cut
