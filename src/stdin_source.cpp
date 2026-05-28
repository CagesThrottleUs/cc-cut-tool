#include "cut/stdin_source.hpp"

#include <iostream>
#include <iterator>
#include <optional>
#include <string_view>

namespace cc_cut {

void StdinSource::load() {
  buffer_.assign(std::istreambuf_iterator<char>{std::cin},
                 std::istreambuf_iterator<char>{});
  cursor_ = 0;
}

auto StdinSource::getline() -> std::optional<std::string_view> {
  return next_line(buffer_, cursor_);
}

}  // namespace cc_cut
