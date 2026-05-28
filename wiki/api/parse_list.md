# `cc_cut::parse_list` — API Reference

**Header:** `include/cut/list_parser.hpp`  
**Spec:** SPEC-2 (`.ai/specs/2026-05-28-sp02-list-parser-design.md`)  
**Since:** SP-02

---

## Signature

```cpp
namespace cc_cut {

auto parse_list(std::string_view list_arg)
    -> std::expected<CutList, std::string>;

}  // namespace cc_cut
```

## Description

Parses a cut list specification string into a `CutList`. The string
is the raw value of a `-b`, `-c`, or `-f` argument (e.g. `"1,3-5,7-"`
or `"1 3 5"`).

**Mode-agnostic:** `parse_list` has no knowledge of byte, character,
or field mode. Mode-specific error messages belong to the arg parser
(SP-03).

**Pure function:** calling twice with the same input returns identical
results. No I/O, no global state, never throws.

## Tokenization Rules

- If `list_arg` contains a `,` → split on every comma; strip leading/trailing
  ASCII whitespace from each token
- Otherwise → split on contiguous whitespace runs; skip empty tokens

## Token Patterns

| Token form | Example | Effect |
|------------|---------|--------|
| Plain number `N` | `"3"` | Insert position `N-1` into `indices` |
| Range `N-M` | `"2-5"` | Insert positions `N-1` through `M-1` inclusive |
| Open-start `-M` | `"-4"` | Insert positions `0` through `M-1` |
| Open-end `N-` | `"3-"` | Set `open_from = N-1` (minimum if multiple) |

All positions are 1-based in the string; stored 0-based in `CutList`.

## Error Strings

| Condition | Error |
|-----------|-------|
| Empty input or whitespace-only | `"missing list specification"` |
| Position 0 (`"0"`, `"0-3"`, `"-0"`) | `"values may not include zero"` |
| Decreasing range (`"5-3"`) | `"invalid decreasing range"` |
| Lone dash (`"-"`) | `"invalid range with no endpoint: -"` |
| Any other invalid token | `"invalid field value: <token>"` |

Zero position is checked before decreasing range (e.g. `"3-0"` returns
zero error, not decreasing-range error).

## Example

```cpp
#include "cut/list_parser.hpp"
#include <iostream>

auto result = cc_cut::parse_list("1,3-5,7-");
if (result) {
    // result->indices == {0, 2, 3, 4}
    // result->open_from == 6
} else {
    std::cerr << result.error() << '\n';
}
```

## See Also

- `cc_cut::CutList` — `include/cut/list.hpp` (SPEC-1 REQ-002)
- SP-03 arg parser (pending) — wraps `parse_list` with mode-specific messaging
