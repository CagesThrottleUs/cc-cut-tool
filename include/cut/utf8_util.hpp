// include/cut/utf8_util.hpp
// spec_id: SPEC-6,SPEC-7
#pragma once
#include "utf8/core.h"

namespace cc_cut::detail {

// Advances cur by one UTF-8 sequence and returns the error code.
// On success cur points past the sequence; on error cur is unchanged.
// Isolates utf8::internal:: to this single call site.
inline auto utf8_advance_one(const utf8::utfchar8_t*& cur,
                             const utf8::utfchar8_t* end)
    -> utf8::internal::utf_error {
  return utf8::internal::validate_next(cur, end);
}

}  // namespace cc_cut::detail
