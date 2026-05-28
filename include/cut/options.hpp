#pragma once
#include <optional>
#include "cut/list.hpp"
#include "cut/mode.hpp"

/// Aggregated, validated options for a single cut invocation.
///
/// Populated by the argument parser (SP-03) from argv. A
/// default-constructed CutOptions is valid for field mode reading
/// stdin with whitespace splitting and no flags.
///
/// @code
///   CutOptions opts;
///   opts.mode  = CutMode::FIELD;
///   opts.delim = ',';              // CSV input
///   opts.list.indices = {0, 2};   // first and third fields
/// @endcode
struct CutOptions {
    CutMode             mode     = CutMode::FIELD; ///< Active cut mode.
    CutList             list;                      ///< Field/byte/char selection list.
    /// Delimiter for FIELD mode.
    /// nullopt = split on contiguous whitespace (default).
    /// some(c) = split exactly on character c.
    std::optional<char> delim;
    bool suppress = false; ///< -s: skip lines that contain no delimiter.
    bool no_split = false; ///< -n: do not split multibyte chars (BYTE mode only).
};
