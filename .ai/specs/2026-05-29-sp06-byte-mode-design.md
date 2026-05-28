---
spec_id: SPEC-6
title: SP-06 Byte Mode
status: draft
---

# SP-06: Byte Mode — Specification

**Version:** 1.0 | **Date:** 2026-05-29 | **Status:** Draft

---

## Context

SP-06 delivers end-to-end `-b` (byte mode) support. It mirrors the SP-05
FieldProcessor pattern: a `ByteProcessor` class with pure static helpers
and a `run()` method that wires FileSource iteration to byte selection and
stdout output.

Byte mode selects raw bytes from each input line by 0-based position.
The `-n` flag adds UTF-8 awareness: a multibyte character is included or
excluded as a unit based on whether its lead byte's position is selected.
Invalid UTF-8 bytes are each treated as a 1-byte character.

`main.cpp` is updated to dispatch `CutMode::BYTE` to `ByteProcessor` and
narrow the "not yet implemented" stub to character mode only.

---

## Scope

**In scope:**
- `cc_cut::ByteProcessor` class in `include/cut/byte_processor.hpp` +
  `src/byte_processor.cpp`
- `ByteProcessor::select_bytes` — pure static, raw byte selection
- `ByteProcessor::select_bytes_no_split` — pure static, UTF-8-boundary-aware
  selection (used when `opts.no_split == true`)
- `ByteProcessor::process_line` — selects bytes, writes one output line
- `ByteProcessor::run` — iterates files (or stdin), calls process_line,
  returns int exit code
- `main.cpp` updated: dispatch `CutMode::BYTE`, update stub message

**Out of scope:**
- Character mode (`-c`) — SP-07
- `--complement` flag — not in SYNOPSIS
- Output to a file (output is always stdout)
- Performance optimisation for large byte counts
- Multi-file deduplication (done by FileSource layer, SP-04)

---

## Requirements

### REQ-001: ByteProcessor Class

**Statement:** The project SHALL define `class cc_cut::ByteProcessor` in
`include/cut/byte_processor.hpp` with a constructor that takes a `CutOptions`
by value. The class SHALL be in `namespace cc_cut` and have no public data
members.

**Acceptance Criteria:**
- [ ] `cc_cut::ByteProcessor{opts}` constructs from a `CutOptions` value
- [ ] No public data members (all state private)
- [ ] Defined in `namespace cc_cut`

**Dependencies:**
| Dependency | Assumed Behavior |
|-----------|-----------------|
| `CutOptions` (SPEC-1 REQ-003) | `mode`, `list`, `no_split` members accessible |

**Test Cases:**
- TC-REQ001-01: `cc_cut::ByteProcessor{cc_cut::CutOptions{}}` constructs without error
- TC-REQ001-02: `static_assert(std::is_constructible_v<cc_cut::ByteProcessor, cc_cut::CutOptions>)` compiles and passes

---

### REQ-002: select_bytes — Raw Byte Selection

**Statement:** `ByteProcessor::select_bytes(std::string_view line, const CutList& list)`
SHALL return a `std::string` containing exactly the bytes of `line` whose
0-based positions are selected by `list`. Positions beyond `line.size() - 1`
are silently skipped. The selected bytes are concatenated in ascending
position order with no separator between them.

**Acceptance Criteria:**
- [ ] `"hello"`, `indices={0}` → `"h"` (1 byte)
- [ ] `"hello"`, `indices={0,1,4}` → `"heo"` (positions 0, 1, 4)
- [ ] `"hello"`, `indices={0,1,2}` → `"hel"` (contiguous range)
- [ ] `"hello"`, `open_from=3` → `"lo"` (open range from position 3)
- [ ] `"hello"`, `indices={9}` → `""` (position beyond line length silently skipped)
- [ ] `""`, `indices={0}` → `""` (empty line returns empty)
- [ ] Returns owned `std::string` (not a view)

**Dependencies:**
| Dependency | Assumed Behavior |
|-----------|-----------------|
| `CutList` (SPEC-1 REQ-002) | `indices` is `std::set<int>` sorted ascending 0-based; `open_from` is `optional<int>` |

**Test Cases:**
- TC-REQ002-01: `select_bytes("hello", {indices={0}})` → `"h"`
- TC-REQ002-02: `select_bytes("hello", {indices={0,1,4}})` → `"heo"`
- TC-REQ002-03: `select_bytes("hello", {open_from=3})` → `"lo"`
- TC-REQ002-04: `select_bytes("hello", {indices={9}})` → `""`
- TC-REQ002-05: `select_bytes("", {indices={0}})` → `""`
- TC-REQ002-06: `select_bytes("abc", {indices={0,1,2}})` → `"abc"` (all bytes selected)

---

### REQ-003: select_bytes_no_split — UTF-8-Boundary-Aware Selection

**Statement:** `ByteProcessor::select_bytes_no_split(std::string_view line, const CutList& list)`
SHALL return a `std::string` of selected bytes, treating the line as a
sequence of UTF-8 characters. A character is included if and only if its
lead byte's 0-based position is selected by `list`; all bytes of an included
character are appended together. Invalid bytes (lead byte invalid per UTF-8,
or continuation bytes without a valid lead) are each treated as a 1-byte
character and included/excluded by their individual position.

The function SHALL use `utf8::internal::validate_next` from `utf8.h` to
determine character boundaries. On `UTF8_OK`, the character spans from
`seq_start` to the advanced iterator. On any error, exactly one byte at
`seq_start` is treated as a 1-byte character.

**Acceptance Criteria:**
- [ ] Pure ASCII line — result matches `select_bytes` for the same list
- [ ] 2-byte char (e.g., `"\xC3\xA9"` = é, bytes at positions 0–1): `indices={0}` → `"\xC3\xA9"` (full char included)
- [ ] 2-byte char at positions 0–1: `indices={1}` → `""` (lead byte 0 not in list → char excluded)
- [ ] 3-byte char (e.g., `"\xE2\x82\xAC"` = €, positions 0–2): `indices={0}` → `"\xE2\x82\xAC"`
- [ ] 3-byte char at positions 0–2: `indices={1,2}` → `""` (lead byte 0 not in list)
- [ ] Invalid lead byte `"\x80"` at position 0: `indices={0}` → `"\x80"` (1-byte char included)
- [ ] Invalid lead byte `"\x80"` at position 0: `indices={}` → `""` (1-byte char excluded)
- [ ] Truncated sequence `"\xC3"` (1-byte incomplete): `indices={0}` → `"\xC3"` (treated as 1-byte char)
- [ ] Mixed `"a\xC3\xA9b"` (4 bytes, 3 chars): `indices={0,1}` → `"a\xC3\xA9"` (chars with leads at byte pos 0 ('a') and 1 ('é'))
- [ ] Mixed `"a\xC3\xA9b"` (4 bytes, 3 chars): `indices={0,2}` → `"a"` (byte 2 is continuation of 'é' whose lead at 1 is not selected; 'b' lead at 3 not selected)

**Dependencies:**
| Dependency | Assumed Behavior |
|-----------|-----------------|
| `utf8::internal::validate_next(it, end)` | Advances `it` past one valid UTF-8 sequence on `UTF8_OK`; error-case advancement is implementation-defined (see ASM-002) |
| `CutList` (SPEC-1 REQ-002) | Same as REQ-002 |

**Test Cases:**
- TC-REQ003-01: `select_bytes_no_split("hello", {indices={0,1,4}})` → `"heo"` (ASCII = same as select_bytes)
- TC-REQ003-02: `select_bytes_no_split("\xC3\xA9", {indices={0}})` → `"\xC3\xA9"` (2-byte char, lead selected)
- TC-REQ003-03: `select_bytes_no_split("\xC3\xA9", {indices={1}})` → `""` (lead byte 0 not selected)
- TC-REQ003-04: `select_bytes_no_split("\xE2\x82\xAC", {indices={0}})` → `"\xE2\x82\xAC"` (3-byte char)
- TC-REQ003-05: `select_bytes_no_split("\x80", {indices={0}})` → `"\x80"` (invalid byte = 1-byte char)
- TC-REQ003-06: `select_bytes_no_split("\x80", {indices={}})` → `""` (invalid byte excluded)
- TC-REQ003-07: `select_bytes_no_split("a\xC3\xA9b", {indices={0,1}})` → `"a\xC3\xA9"` (select chars with leads at byte 0 ('a') and byte 1 ('é'))
- TC-REQ003-08: `select_bytes_no_split("a\xC3\xA9b", {indices={0,2}})` → `"a"` (byte 2 is continuation of 'é'; lead at byte 1 not in list → 'é' excluded)

---

### REQ-004: process_line

**Statement:** `ByteProcessor::process_line(std::string_view line, std::ostream& out)`
SHALL process one line and write the result to `out`. Behavior:
1. If `opts.no_split == false`: call `select_bytes(line, opts.list)`, write
   result + `'\n'` to `out`.
2. If `opts.no_split == true`: call `select_bytes_no_split(line, opts.list)`,
   write result + `'\n'` to `out`.

There is no delimiter or suppress concept in byte mode. Every line (including
empty lines) always produces exactly one output line.

**Acceptance Criteria:**
- [ ] `no_split=false`, `indices={0}`, line `"hello"` → writes `"h\n"`
- [ ] `no_split=false`, `indices={0,1,4}`, line `"hello"` → writes `"heo\n"`
- [ ] `no_split=true`, `indices={0}`, line `"\xC3\xA9"` → writes `"\xC3\xA9\n"` (full char)
- [ ] `no_split=true`, `indices={1}`, line `"\xC3\xA9"` → writes `"\n"` (lead not selected, empty result + newline)
- [ ] Empty line `""`, any list → writes `"\n"` (newline preserved)

**Dependencies:**
| Dependency | Assumed Behavior |
|-----------|-----------------|
| REQ-002 | `select_bytes` returns owned string |
| REQ-003 | `select_bytes_no_split` returns owned string |

**Depends on:** REQ-002, REQ-003

**Test Cases:**
- TC-REQ004-01: `no_split=false`, `indices={0}`, `"hello"` → out = `"h\n"`
- TC-REQ004-02: `no_split=false`, `indices={0,1,4}`, `"hello"` → out = `"heo\n"`
- TC-REQ004-03: `no_split=true`, `indices={0}`, `"\xC3\xA9"` → out = `"\xC3\xA9\n"`
- TC-REQ004-04: `no_split=true`, `indices={1}`, `"\xC3\xA9"` → out = `"\n"`
- TC-REQ004-05: `no_split=false`, empty line → out = `"\n"`

---

### REQ-005: run — File Loop

**Statement:** `ByteProcessor::run(std::ostream& out, const std::vector<std::string>& files, std::ostream& err)`
SHALL process all inputs and return `int` exit code (0 = all success, 1 = any
error). Behavior mirrors FieldProcessor::run (SPEC-5 REQ-006):
- If `files` is empty: process stdin via `make_file_source("-")`
- For each path in `files`: call `make_file_source(path)`:
  - On error: write `error_string + '\n'` to `err`, set exit_code=1, continue
  - On success: call `load()` (throws `ios_base::failure` on I/O error):
    - On throw: write `"cc-cut-tool: " + path + ": " + e.what() + '\n'` to `err`,
      set exit_code=1, continue
  - For each line from `getline()`: call `process_line(line, out)`
- Return exit_code

**Acceptance Criteria:**
- [ ] Empty files → reads stdin via `make_file_source("-")`
- [ ] Valid file → processes all lines via `process_line`
- [ ] Non-existent file → writes error to err, continues, returns 1
- [ ] All files processed even after one fails
- [ ] Returns 0 when all files succeed

**Dependencies:**
| Dependency | Assumed Behavior |
|-----------|-----------------|
| `cc_cut::make_file_source` (SPEC-4 REQ-006) | Returns `expected<unique_ptr<FileSource>, string>`; `"-"` → StdinSource |
| `FileSource::load()` (SPEC-1 REQ-004) | Throws `ios_base::failure` on I/O error |
| `FileSource::getline()` (SPEC-1 REQ-004) | Returns lines without trailing `\n`; nullopt at EOF |

**Depends on:** REQ-004

**Test Cases:**
- TC-REQ005-01: valid file with `"hello\n"`, `indices={0}` → out=`"h\n"`, returns 0
- TC-REQ005-02: non-existent path → err contains path, returns 1
- TC-REQ005-03: two files: first valid, second non-existent → first processed, err contains second path, returns 1
- TC-REQ005-04: empty files vector + stdin `"hello\n"` → out=`"h\n"` (tested via cin.rdbuf redirect)

---

### REQ-006: main.cpp Update

**Statement:** `src/main.cpp` SHALL dispatch `CutMode::BYTE` to `ByteProcessor`
and update the "not yet implemented" stub to cover character mode only.
New dispatch logic:
1. If parse_args returns error: write error + '\n' to stderr, return 1
2. If `result.help_requested`: return 0
3. If `result->opts.mode == CutMode::BYTE`: return
   `cc_cut::ByteProcessor{result->opts}.run(std::cout, result->files, std::cerr)`
4. If `result->opts.mode == CutMode::FIELD`: return
   `cc_cut::FieldProcessor{result->opts}.run(std::cout, result->files, std::cerr)`
5. Otherwise (CHARACTER): write
   `"cc-cut-tool: character mode not yet implemented\n"` to stderr, return 1

**Acceptance Criteria:**
- [ ] `./cc-cut-tool -b1 <file>` prints first byte of each line, exits 0
- [ ] `./cc-cut-tool -b1-3 <file>` prints first 3 bytes of each line, exits 0
- [ ] `./cc-cut-tool -b1 -n <file>` respects UTF-8 char boundaries, exits 0
- [ ] `./cc-cut-tool -f1 <file>` still dispatches to FieldProcessor, exits 0
- [ ] `./cc-cut-tool -c1` exits 1 with `"cc-cut-tool: character mode not yet implemented"` on stderr

**Dependencies:**
| Dependency | Assumed Behavior |
|-----------|-----------------|
| `cc_cut::ByteProcessor` (REQ-001) | Constructs from `CutOptions` |
| `cc_cut::FieldProcessor` (SPEC-5 REQ-001) | Unchanged |
| `cc_cut::parse_args` (SPEC-3 REQ-003) | Returns `expected<ParseResult, string>` |

**Test Cases:**
- TC-REQ006-01: integration — `echo "hello" | ./cc-cut-tool -b1` → stdout `"h\n"`, exit 0
- TC-REQ006-02: integration — `echo "hello" | ./cc-cut-tool -b1-3` → stdout `"hel\n"`, exit 0
- TC-REQ006-03: integration — `printf '\xC3\xA9\n' | ./cc-cut-tool -b1 -n` → stdout `"\xC3\xA9\n"`, exit 0
- TC-REQ006-04: `./cc-cut-tool -c1` → exit 1, stderr = `"cc-cut-tool: character mode not yet implemented\n"`
- TC-REQ006-05: `./cc-cut-tool -f1 -d, sample/sample.csv` → dispatches to FieldProcessor, exit 0

---

## Assumptions

| ID | Assumption | Impact if Wrong | Verified By |
|----|-----------|----------------|-------------|
| ASM-001 | Byte positions in `CutList::indices` are 0-based; POSIX user input is 1-based (converted by SP-02) | Off-by-one in byte selection | TC-REQ002-01 |
| ASM-002 | `select_bytes_no_split` resets `it` to `seq_start + 1` after any `validate_next` error — i.e., treats the byte at `seq_start` as a 1-byte char regardless of how far `validate_next` advanced `it` | Wrong char boundary on invalid UTF-8 | TC-REQ003-05, TC-REQ003-06 |
| ASM-003 | `opts.no_split` is set to `true` by SP-03 arg parser when user passes `-n` | `-n` flag silently ignored | TC-REQ006-03 |
| ASM-004 | `string_view` returned from `FileSource::getline()` aliases the FileSource buffer; valid until next `getline()` call | UAF if process_line stores a view past next getline() | Code review |

---

## Test Coverage Matrix

| REQ-ID | Requirement | Test Cases | Status |
|--------|-------------|-----------|--------|
| REQ-001 | ByteProcessor class | TC-REQ001-01, TC-REQ001-02 | 🔴 Pending |
| REQ-002 | select_bytes (raw) | TC-REQ002-01..06 | 🔴 Pending |
| REQ-003 | select_bytes_no_split (UTF-8) | TC-REQ003-01..08 | 🔴 Pending |
| REQ-004 | process_line | TC-REQ004-01..05 | 🔴 Pending |
| REQ-005 | run (file loop) | TC-REQ005-01..04 | 🔴 Pending |
| REQ-006 | main.cpp update | TC-REQ006-01..05 | 🔴 Pending |
