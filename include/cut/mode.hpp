#pragma once
#include <cstdint>

namespace cc_cut {

// spec_id: SPEC-1  req_id: REQ-001
/// Operating mode for the cut tool, selected by the first CLI flag.
///
/// Exactly one mode is active per invocation. Modes are mutually exclusive.
///
/// @code
///   cc_cut::CutMode m = cc_cut::CutMode::FIELD;  // selected by -f
/// @endcode
enum class CutMode : uint8_t {
    BYTE,       ///< Byte-position mode (-b). Selects raw bytes from each line.
    CHARACTER,  ///< UTF-8 codepoint mode (-c). Selects Unicode characters.
    FIELD       ///< Field mode (-f). Splits lines on a delimiter character.
};

}  // namespace cc_cut
