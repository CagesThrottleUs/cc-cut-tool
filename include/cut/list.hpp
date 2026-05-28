#pragma once
#include <optional>
#include <set>

/// Parsed selection list from a -b, -c, or -f argument.
///
/// A selection is finite (closed) or open-ended:
/// - Finite: `indices` holds all 0-based positions; `open_from` is nullopt.
/// - Open-ended: `open_from` holds the start index; all positions from
///   that index to end-of-line are implicitly selected in addition to
///   any positions in `indices` below it.
///
/// @invariant All values in `indices` are >= 0.
/// @invariant If `open_from` has a value, it is >= 0.
///
/// @code
///   CutList list;
///   list.indices = {0, 2};  // select positions 0 and 2
///   list.open_from = 4;     // also select position 4 to end-of-line
/// @endcode
struct CutList {
    std::set<int>      indices;   ///< 0-based positions to select (finite portion).
    std::optional<int> open_from; ///< If set, select from this 0-based index to EOL.
};
