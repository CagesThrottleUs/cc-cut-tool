#pragma once
#include <optional>
#include <string>
#include <string_view>

#include "cut/file_source.hpp"

namespace cc_cut {

// spec_id: SPEC-4  req_id: REQ-003
/// Reads all bytes from std::cin into memory on load().
/// getline() returns successive lines without their trailing newline.
class StdinSource : public FileSource {
 public:
  void load() override;
  auto getline() -> std::optional<std::string_view> override;

 private:
  std::string buffer_;
  std::size_t cursor_{0};
};

}  // namespace cc_cut
