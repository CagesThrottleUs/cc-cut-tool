---
spec_id: SPEC-2
title: SP-02 List Parser
status: draft
---

# SP-02: List Parser — Specification

**Version:** 1.0 | **Date:** 2026-05-28 | **Status:** Draft

---

## Context

The `cut` tool accepts a list argument (e.g. `1,3-5,7-`) for the `-b`, `-c`, and
`-f` flags. This argument encodes which positions or fields to select. SP-02
provides the single pure function that converts this raw string into a `CutList`
(defined in SPEC-1) that the processing algorithm can consume.

SP-02 has no knowledge of mode (BYTE/CHARACTER/FIELD). Mode-specific error message
formatting belongs to the arg parser (SP-03).

## Scope

**In scope:**
- `parse_list(list_arg)` — free function declared in `include/cut/list_parser.hpp`,
  implemented in `src/list_parser.cpp`
- Tokenization: comma-split when comma present, whitespace-split otherwise
- Token classification: plain number, `N-M` range, `-M` open-start, `N-` open-end
- Error detection: empty input, position 0, decreasing range, lone dash,
  unrecognised token, multiple dashes
- Return type: `std::expected<CutList, std::string>`

**Out of scope:**
- Mode-specific error message formatting (e.g. "fields are numbered from 1") — SP-03
- Validation of `list_arg` against actual line length — SP-05 through SP-07
- Parsing of `-b`, `-c`, `-f` flag names — SP-03
- `--complement` flag — not in SYNOPSIS

---

## Requirements

### REQ-001: Function Signature

**Statement:** The project SHALL declare a free function
`parse_list(std::string_view list_arg) -> std::expected<CutList, std::string>`
in `include/cut/list_parser.hpp` and implement it in `src/list_parser.cpp`.

**Acceptance Criteria:**
- [ ] `#include "cut/list_parser.hpp"` compiles from any translation unit that
      includes `include/` in its search path
- [ ] Function is callable with a `std::string_view` argument and the return
      type is `std::expected<CutList, std::string>`
- [ ] Function has no side effects — calling it twice with the same input returns
      identical results

**Dependencies:**
| Dependency | Assumed Behavior |
|-----------|-----------------|
| `CutList` (SPEC-1 REQ-002) | `indices` is `std::set<int>`, `open_from` is `std::optional<int>` |
| `<expected>` (C++23) | `std::expected<T,E>` default-constructs T on success, E on failure |

**Test Cases:**
- TC-REQ001-01: `parse_list("1")` returns `CutList{indices={0}, open_from=nullopt}`; `result.has_value() == true` and `result->indices == std::set<int>{0}`
- TC-REQ001-02: `parse_list("")` returns an error; `result.has_value() == false`
- TC-REQ001-03: calling `parse_list("1")` twice returns identical `CutList` values

---

### REQ-002: Tokenization — Comma Mode

**Statement:** When `list_arg` contains at least one `,` character, `parse_list`
SHALL split it on every `,`, treating each substring between commas as a token.
Leading and trailing ASCII whitespace within each comma-mode token SHALL be
stripped before classification. Empty substrings (after stripping) produced by
adjacent or leading/trailing commas SHALL be treated as invalid tokens.

**Acceptance Criteria:**
- [ ] `"1,3,5"` produces exactly three tokens: `"1"`, `"3"`, `"5"`
- [ ] `"1, 3, 5"` (spaces after commas) produces tokens `"1"`, `"3"`, `"5"` after whitespace stripping
- [ ] `"1,,3"` returns error `"invalid field value: "` (empty string token between commas — token is the empty string after stripping)
- [ ] `"1,3,"` returns error `"invalid field value: "` (trailing comma produces empty string token)

**Dependencies:**
| Dependency | Assumed Behavior |
|-----------|-----------------|
| `list_arg` contains `,` | comma-mode is selected; whitespace inside tokens is not stripped |

**Test Cases:**
- TC-REQ002-01: `parse_list("1,3,5")` → `indices == {0,2,4}`, `open_from == nullopt`
- TC-REQ002-04: `parse_list("1, 3, 5")` → `indices == {0,2,4}`, `open_from == nullopt` (whitespace stripped)
- TC-REQ002-02: `parse_list("1,,3")` → error string contains `"invalid field value"`
- TC-REQ002-03: `parse_list(",1")` → error string contains `"invalid field value"`

---

### REQ-003: Tokenization — Whitespace Mode

**Statement:** When `list_arg` contains no `,` character, `parse_list` SHALL
split it on contiguous runs of whitespace characters (space, tab). Empty tokens
produced by leading or trailing whitespace SHALL be skipped silently.

**Acceptance Criteria:**
- [ ] `"1 3 5"` produces exactly three tokens: `"1"`, `"3"`, `"5"`
- [ ] `"  1  3  "` (leading/trailing spaces) produces tokens `"1"`, `"3"`
- [ ] `"1\t3"` (tab-separated) produces tokens `"1"`, `"3"`

**Dependencies:**
| Dependency | Assumed Behavior |
|-----------|-----------------|
| `list_arg` contains no `,` | whitespace-mode selected |

**Test Cases:**
- TC-REQ003-01: `parse_list("1 3 5")` → `indices == {0,2,4}`, `open_from == nullopt`
- TC-REQ003-02: `parse_list("  1  3  ")` → `indices == {0,2}`, `open_from == nullopt`
- TC-REQ003-03: `parse_list("1\t3")` → `indices == {0,2}`, `open_from == nullopt`

---

### REQ-004: Plain Number Token

**Statement:** A token consisting entirely of decimal digits SHALL be interpreted
as 1-based position N. `parse_list` SHALL insert `N-1` into `CutList::indices`.

**Acceptance Criteria:**
- [ ] Token `"3"` → `indices` contains `2` (0-based)
- [ ] Token `"1"` → `indices` contains `0`
- [ ] Multiple plain-number tokens → all N-1 values inserted; duplicates silently
      collapsed by `std::set`

**Dependencies:**
| Dependency | Assumed Behavior |
|-----------|-----------------|
| Token contains only ASCII digits | no sign prefix, no leading zeros with special meaning |

**Depends on:** REQ-001

**Test Cases:**
- TC-REQ004-01: `parse_list("3")` → `indices == {2}`, `open_from == nullopt`
- TC-REQ004-02: `parse_list("1")` → `indices == {0}`, `open_from == nullopt`
- TC-REQ004-03: `parse_list("1 1")` (duplicate) → `indices == {0}`, size 1

---

### REQ-005: Range Token `N-M`

**Statement:** A token matching `<digits>-<digits>` SHALL be interpreted as an
inclusive 1-based range from N to M. `parse_list` SHALL first check for zero
positions (REQ-008), then check N > M. It SHALL insert every integer from `N-1`
to `M-1` inclusive into `CutList::indices`.

**Acceptance Criteria:**
- [ ] `"2-5"` → `indices == {1,2,3,4}`
- [ ] `"3-3"` (single-element range) → `indices == {2}`
- [ ] N > M and M > 0 → error `"invalid decreasing range"`
- [ ] N = 0 or M = 0 → error `"values may not include zero"` (REQ-008 checked first)

**Dependencies:**
| Dependency | Assumed Behavior |
|-----------|-----------------|
| Token has exactly one `-` with digits on both sides | token classified as N-M only when both sides are non-empty digits |

**Depends on:** REQ-001

**Test Cases:**
- TC-REQ005-01: `parse_list("2-5")` → `indices == {1,2,3,4}`, `open_from == nullopt`
- TC-REQ005-02: `parse_list("3-3")` → `indices == {2}`, `open_from == nullopt`
- TC-REQ005-03: `parse_list("5-3")` → error string == `"invalid decreasing range"` (M=3 > 0 so zero check does not fire)
- TC-REQ005-05: `parse_list("3-0")` → error string == `"values may not include zero"` (zero check fires before decreasing-range check)
- TC-REQ005-04: `parse_list("1,3-5,7")` → `indices == {0,2,3,4,6}`, `open_from == nullopt`

---

### REQ-006: Open-Start Token `-M`

**Statement:** A token matching `-<digits>` (leading dash, digits after) SHALL
be interpreted as a 1-based range from 1 to M. `parse_list` SHALL insert every
integer from `0` to `M-1` inclusive into `CutList::indices`.

**Acceptance Criteria:**
- [ ] `"-4"` → `indices == {0,1,2,3}`
- [ ] `"-1"` → `indices == {0}`

**Dependencies:**
| Dependency | Assumed Behavior |
|-----------|-----------------|
| Token starts with `-` and remainder is all digits | classified as open-start, not negative number |

**Depends on:** REQ-001

**Test Cases:**
- TC-REQ006-01: `parse_list("-4")` → `indices == {0,1,2,3}`, `open_from == nullopt`
- TC-REQ006-02: `parse_list("-1")` → `indices == {0}`, `open_from == nullopt`

---

### REQ-007: Open-End Token `N-`

**Statement:** A token matching `<digits>-` (digits, trailing dash) SHALL be
interpreted as "select from N to end of line". `parse_list` SHALL set
`CutList::open_from` to `N-1`.

**Acceptance Criteria:**
- [ ] `"3-"` → `open_from == 2`
- [ ] `"1-"` → `open_from == 0`
- [ ] When multiple open-end tokens appear, `open_from` is set to the minimum
      of all `N-1` values (smaller start subsumes larger)

**Dependencies:**
| Dependency | Assumed Behavior |
|-----------|-----------------|
| `CutList::open_from` | `std::optional<int>`; `nullopt` on construction |

**Depends on:** REQ-001

**Test Cases:**
- TC-REQ007-01: `parse_list("3-")` → `open_from == 2`, `indices.empty() == true`
- TC-REQ007-02: `parse_list("1-")` → `open_from == 0`
- TC-REQ007-03: `parse_list("3-,5-")` → `open_from == 2` (minimum of 2 and 4)
- TC-REQ007-04: `parse_list("1,3-")` → `indices == {0}`, `open_from == 2`

---

### REQ-008: Zero Position Error

**Statement:** Any token that resolves to 1-based position 0 (i.e. the literal
string `"0"`, range starting at 0 such as `"0-3"`, or open-start `"-0"`)
SHALL cause `parse_list` to return error `"values may not include zero"`.

**Acceptance Criteria:**
- [ ] `"0"` → error `"values may not include zero"`
- [ ] `"0-3"` → error `"values may not include zero"`
- [ ] `"-0"` → error `"values may not include zero"`

**Dependencies:**
| Dependency | Assumed Behavior |
|-----------|-----------------|
| Position numbering | 1-based; 0 is always invalid per POSIX cut |

**Test Cases:**
- TC-REQ008-01: `parse_list("0")` → error string == `"values may not include zero"`
- TC-REQ008-02: `parse_list("0-3")` → error string == `"values may not include zero"`
- TC-REQ008-03: `parse_list("-0")` → error string == `"values may not include zero"`

---

### REQ-009: Invalid Token Error

**Statement:** Any token that does not match a plain number, `N-M`, `-M`, or
`N-` pattern SHALL cause `parse_list` to return error
`"invalid field value: <token>"` where `<token>` is the exact unrecognised
token string.

**Acceptance Criteria:**
- [ ] Lone `-` → error `"invalid range with no endpoint: -"`
- [ ] Token with non-digit, non-dash characters (e.g. `"a"`, `"1a"`) →
      error contains `"invalid field value: "` followed by the token
- [ ] Token with multiple dashes (e.g. `"1-2-3"`, `"--3"`) →
      error contains `"invalid field value: "` followed by the token

**Dependencies:**
| Dependency | Assumed Behavior |
|-----------|-----------------|
| None | catch-all for any token not matching REQ-004..REQ-007 patterns |

**Test Cases:**
- TC-REQ009-01: `parse_list("-")` → error string == `"invalid range with no endpoint: -"`
- TC-REQ009-02: `parse_list("a")` → error string == `"invalid field value: a"`
- TC-REQ009-03: `parse_list("1a")` → error string == `"invalid field value: 1a"`
- TC-REQ009-04: `parse_list("1-2-3")` → error string == `"invalid field value: 1-2-3"`
- TC-REQ009-05: `parse_list("--3")` → error string == `"invalid field value: --3"`

---

### REQ-010: Empty Input Error

**Statement:** When `list_arg` is empty or contains only whitespace characters,
`parse_list` SHALL return error `"missing list specification"`.

**Acceptance Criteria:**
- [ ] `""` → error `"missing list specification"`
- [ ] `"   "` (whitespace only) → error `"missing list specification"`

**Test Cases:**
- TC-REQ010-01: `parse_list("")` → error string == `"missing list specification"`
- TC-REQ010-02: `parse_list("   ")` → error string == `"missing list specification"`

---

## Assumptions

| ID | Assumption | Impact if Wrong | Verified By |
|----|-----------|----------------|-------------|
| ASM-001 | Positions fit in `int` — no list argument exceeds `INT_MAX` entries | integer overflow in range expansion | TC-REQ005-01 + code review |
| ASM-002 | `list_arg` is valid UTF-8; classifier reads only ASCII digits and `-` | non-ASCII digits classified as invalid token | TC-REQ009-02 |
| ASM-003 | `std::set::insert` silently ignores duplicate keys | duplicate positions not an error | TC-REQ004-03 |

---

## Test Coverage Matrix

| REQ-ID | Requirement | Test Cases | Status |
|--------|-------------|-----------|--------|
| REQ-001 | Function signature | TC-REQ001-01, TC-REQ001-02, TC-REQ001-03 | 🔴 Pending |
| REQ-002 | Comma tokenization | TC-REQ002-01, TC-REQ002-02, TC-REQ002-03, TC-REQ002-04 | 🔴 Pending |
| REQ-003 | Whitespace tokenization | TC-REQ003-01, TC-REQ003-02, TC-REQ003-03 | 🔴 Pending |
| REQ-004 | Plain number token | TC-REQ004-01, TC-REQ004-02, TC-REQ004-03 | 🔴 Pending |
| REQ-005 | Range token N-M | TC-REQ005-01, TC-REQ005-02, TC-REQ005-03, TC-REQ005-04, TC-REQ005-05 | 🔴 Pending |
| REQ-006 | Open-start token -M | TC-REQ006-01, TC-REQ006-02 | 🔴 Pending |
| REQ-007 | Open-end token N- | TC-REQ007-01, TC-REQ007-02, TC-REQ007-03, TC-REQ007-04 | 🔴 Pending |
| REQ-008 | Zero position error | TC-REQ008-01, TC-REQ008-02, TC-REQ008-03 | 🔴 Pending |
| REQ-009 | Invalid token error | TC-REQ009-01, TC-REQ009-02, TC-REQ009-03, TC-REQ009-04, TC-REQ009-05 | 🔴 Pending |
| REQ-010 | Empty input error | TC-REQ010-01, TC-REQ010-02 | 🔴 Pending |
