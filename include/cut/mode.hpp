#pragma once
#include <cstdint>

/// Operating mode for the cut tool, selected by the first CLI flag.
///
/// Exactly one mode is active per invocation. Modes are mutually exclusive.
///
/// @code
///   CutMode m = CutMode::FIELD;  // selected by -f
/// @endcode
enum class CutMode : uint8_t {
    BYTE,       ///< Byte-position mode (-b). Selects raw bytes from each line.
    CHARACTER,  ///< UTF-8 codepoint mode (-c). Selects Unicode characters.
    FIELD       ///< Field mode (-f). Splits lines on a delimiter character.
};
