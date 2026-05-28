---
spec_id: SPEC-5
title: SP-05 Field Mode
status: draft
---

# SP-05: Field Mode — Specification

**Version:** 1.0 | **Date:** 2026-05-28 | **Status:** Draft

---

## Context

SP-05 delivers the first working end-to-end cut mode: `-f`. It implements
the `FieldProcessor` class that owns `CutOptions`, exposes pure static helpers
for splitting and selecting fields, and a `run()` method that wires file
reading (SP-04) to field processing and stdout output. It also wires
`main.cpp` so the tool is usable for the first time.

## Scope

**In scope:**
- `cc_cut::FieldProcessor` class in `include/cut/field_processor.hpp` +
  `src/field_processor.cpp`
- `FieldProcessor::split_fields` — pure static, splits line on delimiter
- `FieldProcessor::select_fields` — pure static, selects fields by CutList
- `FieldProcessor::process_line` — handles -s suppression, splits, selects,
  writes one output line to `std::ostream&`
- `FieldProcessor::run` — iterates files (or stdin if empty), calls process_line,
  collects errors, returns int exit code
- `main.cpp` wired: parse_args → help → FieldProcessor::run
- Byte and character modes: return exit code 1 with message
  `"cc-cut-tool: byte and character modes not yet implemented"`

**Out of scope:**
- Byte mode (-b) — SP-06
- Character mode (-c) — SP-07
- `--complement` flag — not in SYNOPSIS
- Output to a file (output is always stdout)
- Performance optimisation for large field counts
- Whitespace-mode output format (`delim=nullopt` through `process_line`) — not
  reachable from CLI in SP-05 since arg parser always sets `delim=some('\t')`
  for `-f` mode; defined behavior deferred to when `-w` flag is implemented

---

## Requirements

### REQ-001: FieldProcessor Class

**Statement:** The project SHALL define `class cc_cut::FieldProcessor` in
`include/cut/field_processor.hpp` with a constructor that takes a `CutOptions`
by value. The class SHALL be in `namespace cc_cut` and have no public data
members.

**Acceptance Criteria:**
- [ ] `cc_cut::FieldProcessor{opts}` constructs from a `CutOptions` value
- [ ] No public data members (all state private)
- [ ] Defined in `namespace cc_cut`

**Dependencies:**
| Dependency | Assumed Behavior |
|-----------|-----------------|
| `CutOptions` (SPEC-1 REQ-003) | `mode`, `list`, `delim`, `suppress`, `no_split` members accessible |

**Test Cases:**
- TC-REQ001-01: `cc_cut::FieldProcessor{cc_cut::CutOptions{}}` constructs without error
- TC-REQ001-02: `static_assert(std::is_constructible_v<cc_cut::FieldProcessor, cc_cut::CutOptions>)` compiles and passes — confirms the single-argument CutOptions constructor exists

---

### REQ-002: split_fields — Exact Delimiter

**Statement:** `FieldProcessor::split_fields(std::string_view line, char delim)`
SHALL split `line` on every occurrence of `delim` and return a
`std::vector<std::string_view>`. Empty fields produced by adjacent or
leading/trailing delimiters SHALL be preserved as empty string_views.

**Acceptance Criteria:**
- [ ] `"a,b,c"` split on `,` → `["a","b","c"]` (3 elements)
- [ ] `"a,,c"` split on `,` → `["a","","c"]` (empty field preserved)
- [ ] `",a"` split on `,` → `["","a"]` (leading delimiter → empty first field)
- [ ] `"a,"` split on `,` → `["a",""]` (trailing delimiter → empty last field)
- [ ] `""` split on `,` → `[""]` (one empty field)
- [ ] Returned views alias `line` — valid as long as `line` is alive

**Dependencies:**
| Dependency | Assumed Behavior |
|-----------|-----------------|
| `std::string_view` | Non-owning; caller guarantees `line` lifetime covers all uses of returned views |

**Test Cases:**
- TC-REQ002-01: `split_fields("a,b,c", ',')` → size=3, [0]="a", [1]="b", [2]="c"
- TC-REQ002-02: `split_fields("a,,c", ',')` → size=3, [1]==""
- TC-REQ002-03: `split_fields(",a", ',')` → size=2, [0]==""
- TC-REQ002-04: `split_fields("", ',')` → size=1, [0]==""
- TC-REQ002-05: `split_fields("no-delim", ',')` → size=1, [0]=="no-delim"

---

### REQ-003: split_fields — Whitespace Delimiter

**Statement:** `FieldProcessor::split_fields(std::string_view line)` (no
delimiter parameter) SHALL split `line` on runs of ASCII space or tab,
discarding empty tokens (leading, trailing, consecutive whitespace produces
no empty fields). Returns `std::vector<std::string_view>`.

**Acceptance Criteria:**
- [ ] `"a b c"` → `["a","b","c"]`
- [ ] `"  a  b  "` → `["a","b"]` (leading/trailing whitespace discarded)
- [ ] `"a\tb"` → `["a","b"]`
- [ ] `""` → `[]` (empty input → empty vector)
- [ ] `"   "` → `[]` (whitespace-only → empty vector)

**Dependencies:**
| Dependency | Assumed Behavior |
|-----------|-----------------|
| `std::string_view` | Non-owning; views alias `line` |

**Test Cases:**
- TC-REQ003-01: `split_fields("a b c")` → size=3
- TC-REQ003-02: `split_fields("  a  b  ")` → size=2, [0]="a"
- TC-REQ003-03: `split_fields("a\tb")` → size=2
- TC-REQ003-04: `split_fields("")` → size=0
- TC-REQ003-05: `split_fields("   ")` → size=0

---

### REQ-004: select_fields

**Statement:** `FieldProcessor::select_fields(const std::vector<std::string_view>& fields, const CutList& list)`
SHALL return a `std::vector<std::string_view>` of the selected fields in
ascending position order. A position is selected if it is in `list.indices`
OR `list.open_from.has_value() && position >= list.open_from.value()`. If a
selected position exceeds `fields.size() - 1`, an empty `string_view` SHALL
be appended for that position (missing fields are not an error per POSIX).

**Acceptance Criteria:**
- [ ] `fields=["a","b","c"]`, `indices={0}`, `open_from=nullopt` → `["a"]`
- [ ] `fields=["a","b","c"]`, `indices={0,2}` → `["a","c"]`
- [ ] `fields=["a","b","c"]`, `open_from=1` → `["b","c"]`
- [ ] `fields=["a"]`, `indices={0,1,2}` → `["a","",""]` (missing → empty)
- [ ] `fields=["a","b"]`, `indices={0}`, `open_from=1` → `["a","b"]`
  (union of indices and open_from range, in order)
- [ ] Output order is ascending by original field position (set ordering)

**Dependencies:**
| Dependency | Assumed Behavior |
|-----------|-----------------|
| `CutList` (SPEC-1 REQ-002) | `indices` is `std::set<int>` (sorted ascending 0-based); `open_from` is `optional<int>` |

**Test Cases:**
- TC-REQ004-01: `fields=["a","b","c"]`, `indices={0}` → `["a"]`
- TC-REQ004-02: `fields=["a","b","c"]`, `indices={0,2}` → `["a","c"]`
- TC-REQ004-03: `fields=["a","b","c"]`, `open_from=1` → `["b","c"]`
- TC-REQ004-04: `fields=["a"]`, `indices={0,1}` → `["a",""]`
- TC-REQ004-05: `fields=["a","b","c"]`, `indices={0}`, `open_from=1` → `["a","b","c"]`

---

### REQ-005: process_line

**Statement:** `FieldProcessor::process_line(std::string_view line, std::ostream& out)`
SHALL process one line according to the stored `CutOptions` and write the
result to `out`. Behavior:
1. If `opts.suppress == true` AND the line contains no delimiter character
   (or no whitespace tokens if delim=nullopt): write nothing, return.
2. If `opts.suppress == false` AND the line contains no delimiter: write
   `line + '\n'` unchanged to `out`.
3. Otherwise: split, select, join selected fields with `opts.delim.value()`,
   write result + '\n' to `out`. (delim=nullopt path not reachable from CLI
   in SP-05 — see Out of Scope.)

"Contains delimiter" means:
- `delim=some(c)`: line contains at least one `c`
- `delim=nullopt`: line contains at least one space or tab

**Acceptance Criteria:**
- [ ] Field `"b"` selected from `"a,b,c"` (delimiter `,`, indices={1}) →
      writes `"b\n"`
- [ ] Fields `"a","c"` selected from `"a,b,c"` (indices={0,2}) →
      writes `"a,c\n"`
- [ ] `suppress=true`, `"hello"` with delimiter `,` (no comma) →
      writes nothing
- [ ] `suppress=false`, `"hello"` with delimiter `,` (no comma) →
      writes `"hello\n"`
- [ ] Open-end range: `open_from=1` on `"a,b,c"` → writes `"b,c\n"`
- [ ] Missing field: indices={0,3} on `"a,b"` → writes `"a,\n"` (field 3 empty)

**Dependencies:**
| Dependency | Assumed Behavior |
|-----------|-----------------|
| REQ-002 | split_fields on exact delimiter |
| REQ-003 | split_fields on whitespace |
| REQ-004 | select_fields returns views in order |

**Depends on:** REQ-002, REQ-003, REQ-004

**Test Cases:**
- TC-REQ005-01: `opts.delim=','`, `indices={1}`, line `"a,b,c"` → out contains `"b\n"`
- TC-REQ005-02: `opts.delim=','`, `indices={0,2}`, line `"a,b,c"` → out contains `"a,c\n"`
- TC-REQ005-03: `suppress=true`, `opts.delim=','`, line `"hello"` → out is empty
- TC-REQ005-04: `suppress=false`, `opts.delim=','`, line `"hello"` → out contains `"hello\n"`
- TC-REQ005-05: `opts.delim=','`, `open_from=1`, line `"a,b,c"` → out contains `"b,c\n"`
- TC-REQ005-06: `opts.delim=','`, `indices={0,3}`, line `"a,b"` → out contains `"a,\n"`

---

### REQ-006: run — File Loop

**Statement:** `FieldProcessor::run(std::ostream& out, const std::vector<std::string>& files, std::ostream& err)`
SHALL process all inputs and return `int` exit code (0 = all success, 1 = any error).
Behavior:
- If `files` is empty: process stdin via `make_file_source("-")`
- For each path in `files`: call `make_file_source(path)`:
  - On error: write `error_string + '\n'` to `err`, set exit_code=1, continue
  - On success: call `load()` (throws `ios_base::failure` on I/O error):
    - On throw: write `"cc-cut-tool: " + path + ": " + e.what() + '\n'` to `err`,
      set exit_code=1, continue
  - For each line from `getline()`: call `process_line(line, out)`
- Return exit_code

**Acceptance Criteria:**
- [ ] Empty files → reads stdin (make_file_source("-"))
- [ ] Valid file → processes all lines via process_line
- [ ] Non-existent file → writes error to err, continues, returns 1
- [ ] All files processed even after one fails
- [ ] Returns 0 when all files succeed

**Dependencies:**
| Dependency | Assumed Behavior |
|-----------|-----------------|
| `cc_cut::make_file_source` (SPEC-4 REQ-006) | Returns `expected<unique_ptr<FileSource>, string>`; "-" → StdinSource |
| `FileSource::load()` (SPEC-1 REQ-004) | Throws `ios_base::failure` on I/O error |
| `FileSource::getline()` (SPEC-1 REQ-004) | Returns lines without trailing `\n`; nullopt at EOF |

**Depends on:** REQ-005

**Test Cases:**
- TC-REQ006-01: valid file with `"a,b\n"`, `indices={0}` → out=`"a\n"`, returns 0
- TC-REQ006-02: non-existent path → err contains path, returns 1
- TC-REQ006-03: two files: first valid, second non-existent → first processed,
  err contains second path, returns 1
- TC-REQ006-04: empty files vector + stdin `"a,b\n"` → out=`"a\n"` (tested via
  cin.rdbuf redirect)

---

### REQ-007: main.cpp Wiring

**Statement:** `src/main.cpp` SHALL call `cc_cut::parse_args(argc, argv)` and
dispatch as follows:
- If parse_args returns error: write error + '\n' to stderr, return 1
- If `result.help_requested`: return 0
- If `result.opts.mode != CutMode::FIELD`: write
  `"cc-cut-tool: byte and character modes not yet implemented\n"` to stderr,
  return 1
- Otherwise: return `cc_cut::FieldProcessor{result.opts}.run(std::cout, result.files, std::cerr)`

**Acceptance Criteria:**
- [ ] `./cc-cut-tool -f1 <file>` prints first field of each line and exits 0
- [ ] `./cc-cut-tool --help` exits 0 with no output to stderr
- [ ] `./cc-cut-tool -f1 /no/such/file` exits 1 with error on stderr
- [ ] `./cc-cut-tool -b1` exits 1 with "not yet implemented" on stderr

**Dependencies:**
| Dependency | Assumed Behavior |
|-----------|-----------------|
| `cc_cut::parse_args` (SPEC-3 REQ-003) | Returns `expected<ParseResult, string>` |
| `cc_cut::FieldProcessor` (REQ-001) | Constructs from CutOptions |

**Test Cases:**
- TC-REQ007-01: integration — `echo "a,b,c" | ./cc-cut-tool -f2 -d,` → stdout `"b\n"`, exit 0
- TC-REQ007-02: `./cc-cut-tool --help` → exit 0
- TC-REQ007-03: `./cc-cut-tool -f1 /no/such/file` → exit 1, stderr contains path
- TC-REQ007-04: `./cc-cut-tool -b1 sample.tsv` → exit 1, stderr contains "not yet implemented"

---

## Assumptions

| ID | Assumption | Impact if Wrong | Verified By |
|----|-----------|----------------|-------------|
| ASM-001 | Field positions in `CutList::indices` are 0-based; POSIX user input is 1-based (converted by SP-02) | Off-by-one in field selection | TC-REQ004-01 |
| ASM-002 | `string_view` returned from `getline()` aliases the FileSource buffer; views in split/select also alias it — all valid until next `getline()` call | UAF if process_line stores a view past the next getline() | Code review |
| ASM-003 | `opts.delim = some('\t')` is the default for `-f` without `-d` (set by SP-03); `delim=nullopt` is whitespace mode (unused from CLI until -w is added) | Wrong delimiter | TC-REQ007-01 |

---

## Test Coverage Matrix

| REQ-ID | Requirement | Test Cases | Status |
|--------|-------------|-----------|--------|
| REQ-001 | FieldProcessor class | TC-REQ001-01, TC-REQ001-02 | 🔴 Pending |
| REQ-002 | split_fields (exact delim) | TC-REQ002-01..05 | 🔴 Pending |
| REQ-003 | split_fields (whitespace) | TC-REQ003-01..05 | 🔴 Pending |
| REQ-004 | select_fields | TC-REQ004-01..05 | 🔴 Pending |
| REQ-005 | process_line | TC-REQ005-01..06 | 🔴 Pending |
| REQ-006 | run (file loop) | TC-REQ006-01..04 | 🔴 Pending |
| REQ-007 | main.cpp wiring | TC-REQ007-01..04 | 🔴 Pending |
