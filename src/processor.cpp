// src/processor.cpp
#include "cut/processor.hpp"

#include <expected>
#include <memory>
#include <string>

#include "cut/byte_processor.hpp"
#include "cut/field_processor.hpp"
#include "cut/mode.hpp"
#include "cut/options.hpp"

namespace cc_cut {

auto make_processor(const CutOptions& opts)
    -> std::expected<std::unique_ptr<Processor>, std::string> {
  switch (opts.mode) {
    case CutMode::BYTE:
      return std::make_unique<ByteProcessor>(opts);
    case CutMode::FIELD:
      return std::make_unique<FieldProcessor>(opts);
    case CutMode::CHARACTER:
      return std::unexpected<std::string>{
          "cc-cut-tool: character mode not yet implemented"};
    default:
      return std::unexpected<std::string>{"cc-cut-tool: unknown mode"};
  }
}

}  // namespace cc_cut
