---
spec_id: SPEC-7
title: SP-07 Character Mode (-c)
status: draft
---

# SP-07 Character Mode — Specification

**Version:** 1.0 | **Date:** 2026-05-29 | **Status:** Draft

---

## Context

`cut -c list` selects Unicode codepoints by position from each input line. Unlike byte mode
(`-b`), positions refer to codepoints — not raw byte offsets — so a 4-byte emoji at position 1
occupies exactly one position. The existing arg parser already routes `-c` to
`CutMode::CHARACTER`; `make_processor` currently returns an error for that mode. SP-07
implements `CharProcessor` to close that gap.

The implementation follows the exact same class shape as `ByteProcessor` (SP-06): a static
pure helper `select_chars`, a `process_line` dispatcher, and a `run` file loop. The only
novel logic is in `select_chars`, which iterates by UTF-8 codepoint instead of byte.

---

## Scope

**In scope:**
- `CharProcessor` class inheriting `Processor`
- `select_chars(line, list)` static method — codepoint-index selection via utf8cpp
- `process_line` + `run` I/O wiring (mirrors ByteProcessor)
- `make_processor` wired for `CutMode::CHARACTER`
- Unit tests for all REQs
- Wiki docs and AGENTS.md update

**Out of scope:**
- `-c` combined with `-n` (POSIX does not define this combination; `-n` is byte-mode-only)
- Locale-specific encodings other than UTF-8
- BOM detection or stripping
- Surrogate pair handling (pairs are not valid UTF-8 scalar values)
- Integration test pass (SP-08)

---

## Requirements

### REQ-001: CharProcessor class structure

**Statement:** `CharProcessor` SHALL be a concrete class inheriting `Processor`, with an
explicit constructor taking `CutOptions` by value, a static `select_chars` method, a
`process_line` method, and a `run` override. No public data members.

**Acceptance Criteria:**
- [ ] `std::is_abstract_v<cc_cut::Processor>` is true
- [ ] `std::is_abstract_v<cc_cut::CharProcessor>` is false
- [ ] `std::is_base_of_v<cc_cut::Processor, cc_cut::CharProcessor>` is true
- [ ] `CharProcessor` has no public data members (verified by code review)

**Dependencies:**
| Dependency | Assumed Behavior |
|-----------|-----------------|
| `Processor` abstract base | Declares pure virtual `run(ostream&, vector<string>&, ostream&) -> int` |
| `CutOptions` | POD struct with `mode`, `list`, `no_split`, `suppress`, `delim` fields |

**Test Cases:**
- TC-REQ001-01: `static_assert(!std::is_abstract_v<cc_cut::CharProcessor>)` compiles
- TC-REQ001-02: `static_assert(std::is_base_of_v<cc_cut::Processor, cc_cut::CharProcessor>)` compiles
- TC-REQ001-03: `static_assert(std::is_abstract_v<cc_cut::Processor>)` compiles

---

### REQ-002: select_chars — codepoint-index selection

**Statement:** `CharProcessor::select_chars(line, list)` SHALL iterate the UTF-8 input
`line` codepoint by codepoint, assigning each codepoint a 0-based index, and return a
`std::string` containing the full UTF-8 byte sequences of all codepoints whose index is
in `list.indices` or is `>= list.open_from` (when `open_from` has a value).
Out-of-range indices are silently skipped.

**Acceptance Criteria:**
- [ ] ASCII input: codepoint index N selects the Nth byte (identical to byte selection for ASCII)
- [ ] 2-byte codepoint at index I selected → both bytes emitted in result
- [ ] 3-byte codepoint at index I selected → all 3 bytes emitted in result
- [ ] 4-byte codepoint at index I selected → all 4 bytes emitted in result
- [ ] Codepoint at index I NOT in list → its bytes are absent from result
- [ ] Open range `open_from = N` → all codepoints at indices >= N emitted
- [ ] Index beyond end of string → empty result (no crash)
- [ ] Empty line → empty result

**Dependencies:**
| Dependency | Assumed Behavior |
|-----------|-----------------|
| `CutList` | `indices` is `std::set<int>` of 0-based positions; `open_from` is `std::optional<int>` |
| `utf8::internal::validate_next(it, end)` | Advances `it` past one valid codepoint on `UTF8_OK`; leaves `it` unchanged otherwise |

**Test Cases:**
- TC-REQ002-01: `select_chars("hello", {0})` → `"h"` (single ASCII codepoint)
- TC-REQ002-02: `select_chars("hello", {0,1,4})` → `"heo"` (non-contiguous ASCII)
- TC-REQ002-03: `select_chars("hello", open_from=3)` → `"lo"` (open range)
- TC-REQ002-04: `select_chars("hello", {9})` → `""` (out of range)
- TC-REQ002-05: `select_chars("", {0})` → `""` (empty line)
- TC-REQ002-06: `select_chars("\xC3\xA9", {0})` → `"\xC3\xA9"` (é: 2-byte codepoint at index 0)
- TC-REQ002-07: `select_chars("\xE2\x82\xAC", {0})` → `"\xE2\x82\xAC"` (€: 3-byte codepoint)
- TC-REQ002-08: `select_chars("\xF0\x9F\x98\x80", {0})` → `"\xF0\x9F\x98\x80"` (😀: 4-byte codepoint)
- TC-REQ002-09: `select_chars("h\xC3\xA9llo", {1})` → `"\xC3\xA9"` (é is codepoint #1 in "héllo")
- TC-REQ002-10: `select_chars("h\xC3\xA9llo", {0,2})` → `"hl"` (codepoints 0 and 2; é excluded)
- TC-REQ002-11: `select_chars("\xC3\xA9", {1})` → `""` (only 1 codepoint in string; index 1 out of range)

---

### REQ-003: Invalid UTF-8 bytes treated as single codepoints

**Statement:** When `select_chars` encounters a byte sequence that `utf8::internal::validate_next`
does not classify as `UTF8_OK`, it SHALL treat the first byte of that sequence as a
single codepoint occupying one codepoint index, advance the iterator by exactly 1 byte,
and include or exclude that byte according to the list, exactly as for a 1-byte codepoint.

**Acceptance Criteria:**
- [ ] Invalid lead byte at index 0, selected → that 1 byte emitted
- [ ] Invalid lead byte at index 0, not selected → empty result
- [ ] Valid codepoint following an invalid byte is assigned index 1

**Dependencies:**
| Dependency | Assumed Behavior |
|-----------|-----------------|
| `utf8::internal::validate_next` | Returns non-`UTF8_OK` and leaves iterator unchanged on invalid sequence |

**Test Cases:**
- TC-REQ003-01: `select_chars("\x80", {0})` → `"\x80"` (invalid lead byte, selected)
- TC-REQ003-02: `select_chars("\x80", {})` → `""` (invalid lead byte, not selected)
- TC-REQ003-03: `select_chars("\x80\x61", {1})` → `"a"` (invalid byte at 0 = index 0; 'a' = index 1)
- TC-REQ003-04: `select_chars("\xC3", {0})` → `"\xC3"` (truncated 2-byte sequence at EOL treated as 1 codepoint)

---

### REQ-004: process_line writes selected codepoints + newline

**Statement:** `CharProcessor::process_line(line, out)` SHALL call `select_chars(line, opts_.list)`
and write the result followed by exactly one `'\n'` to `out`. It SHALL always write the
newline even when `select_chars` returns an empty string.

**Acceptance Criteria:**
- [ ] `process_line("hello", out)` with list `{0}` writes `"h\n"` to `out`
- [ ] `process_line("", out)` writes `"\n"` to `out`

**Dependencies:**
| Dependency | Assumed Behavior |
|-----------|-----------------|
| `select_chars` | Returns correct string per REQ-002 and REQ-003 |

**Test Cases:**
- TC-REQ004-01: `process_line("hello", ss)` with list `{0}` → `ss.str() == "h\n"`
- TC-REQ004-02: `process_line("", ss)` → `ss.str() == "\n"`

---

### REQ-005: run() processes files and accumulates errors

**Statement:** `CharProcessor::run(out, files, err)` SHALL: (a) read from stdin when
`files` is empty; (b) process each file path in order, calling `process_line` on each
line; (c) write an error message starting with `"cc-cut-tool: "` to `err` and continue
to remaining files when a file cannot be opened; (d) return 0 if no file errors
occurred, 1 otherwise.

**Acceptance Criteria:**
- [ ] Empty `files` → reads from stdin
- [ ] Non-existent file path → error written to `err`, exits with 1
- [ ] Error message starts with `"cc-cut-tool: "`
- [ ] Error on one file does not abort processing of subsequent files

**Dependencies:**
| Dependency | Assumed Behavior |
|-----------|-----------------|
| `make_file_source(path)` | Returns `std::expected<unique_ptr<FileSource>, string>`; error string starts with `"cc-cut-tool: "` |

**Test Cases:**
- TC-REQ005-01: `run(out, {}, err)` with stdin containing `"ab\n"`, list `{0}` → `out == "a\n"`
- TC-REQ005-02: `run(out, {"/no/such"}, err)` → returns 1, `err` starts with `"cc-cut-tool: "`
- TC-REQ005-03: error message for missing file contains the file path
- TC-REQ005-04: `run(out, {"/no/such", valid_file}, err)` with valid_file containing `"ab\n"`, list `{0}` → returns 1 AND `out == "a\n"` (second file processed despite first failing)

---

### REQ-006: make_processor returns CharProcessor for CutMode::CHARACTER

**Statement:** `make_processor(opts)` SHALL return `std::make_unique<CharProcessor>(opts)`
(wrapped in `std::expected`) when `opts.mode == CutMode::CHARACTER`. It SHALL NOT return
`std::unexpected` for `CutMode::CHARACTER`.

**Acceptance Criteria:**
- [ ] `make_processor` with `mode = CHARACTER` returns a non-null `expected`
- [ ] The returned pointer is dynamically castable to `CharProcessor*`

**Dependencies:**
| Dependency | Assumed Behavior |
|-----------|-----------------|
| `make_processor` factory | Dispatches on `opts.mode`; existing cases: BYTE → ByteProcessor, FIELD → FieldProcessor |

**Test Cases:**
- TC-REQ006-01: `make_processor({CHARACTER, ...}).has_value()` is true
- TC-REQ006-02: `dynamic_cast<CharProcessor*>(make_processor({CHARACTER,...})->get()) != nullptr`

---

## Assumptions

| ID | Assumption | Impact if Wrong | Verified By |
|----|-----------|----------------|-------------|
| ASM-001 | CutList stores 0-based codepoint indices (parse_list converts 1-based user input) | select_chars would be off by 1 for all positions | TC-REQ002-01 (matches CLI smoke test) |
| ASM-002 | utf8::internal::validate_next leaves iterator unchanged on non-UTF8_OK | Invalid byte reset to seq_start+1 would double-advance | TC-REQ003-03 |
| ASM-003 | Each invalid byte is treated as a single codepoint (consistent with ByteProcessor ASM-002) | Codepoint numbering after invalid bytes would differ from spec | TC-REQ003-03 |

---

## Out of Scope

- `-c` + `-n` flag combination (POSIX does not define; `-n` is byte-mode-only)
- Locale-based encoding (UTF-8 only)
- BOM detection
- Surrogate pair handling (U+D800–U+DFFF are not valid in UTF-8)
- Integration test pass (SP-08)

---

## Test Coverage Matrix

| REQ-ID | Requirement | Test Cases | Status |
|--------|-------------|-----------|--------|
| REQ-001 | CharProcessor class structure | TC-REQ001-01..03 | 🔴 Pending |
| REQ-002 | select_chars codepoint selection | TC-REQ002-01..11 | 🔴 Pending |
| REQ-003 | Invalid UTF-8 as single codepoints | TC-REQ003-01..04 | 🔴 Pending |
| REQ-004 | process_line writes selected + newline | TC-REQ004-01..02 | 🔴 Pending |
| REQ-005 | run() file loop + error handling | TC-REQ005-01..04 | 🔴 Pending |
| REQ-006 | make_processor returns CharProcessor | TC-REQ006-01..02 | 🔴 Pending |
