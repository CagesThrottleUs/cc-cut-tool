#pragma once
#include "cut/list.hpp"
#include "cut/mode.hpp"
#include <expected>
#include <string>
#include <string_view>

namespace cc_cut {

// spec_id: SPEC-2  req_id: REQ-001
// spec_id: SPEC-2  req_id: REQ-011
/// Parses a cut list specification string into a CutList.
///
/// Tokenizes on comma when `list_arg` contains a comma; otherwise on
/// contiguous whitespace. Each token is classified as a plain 1-based
/// position, an N-M range, a -M open-start, or an N- open-end.
///
/// @param list_arg  Raw list string from a -b, -c, or -f argument
///                  (e.g. "1,3-5,7-" or "1 3 5").
/// @return          CutList on success; error string on the first
///                  invalid token encountered.
/// @throws          Never throws. All errors returned via std::unexpected.
///
/// @code
///   auto result = cc_cut::parse_list("1,3-5,7-");
///   if (result) { use(*result); }
///   else        { std::cerr << result.error() << '\n'; }
/// @endcode
auto parse_list(std::string_view list_arg)
    -> std::expected<CutList, std::string>;

}  // namespace cc_cut
