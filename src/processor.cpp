// src/processor.cpp
#include "cut/processor.hpp"

#include <expected>
#include <memory>
#include <string>

#include "cut/options.hpp"

namespace cc_cut {

auto make_processor(const CutOptions& opts)
    -> std::expected<std::unique_ptr<Processor>, std::string> {
  (void)opts;
  return std::unexpected<std::string>{"not yet implemented"};
}

}  // namespace cc_cut
