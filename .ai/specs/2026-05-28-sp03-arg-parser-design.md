---
spec_id: SPEC-3
title: SP-03 Arg Parser
status: draft
---

# SP-03: Arg Parser — Specification

**Version:** 1.0 | **Date:** 2026-05-28 | **Status:** Draft

---

## Context

SP-03 translates raw CLI arguments (`argc`/`argv`) into a `CutOptions` +
file-path list ready for the processing pipeline (SP-05 onwards). The arg
parser is the only place in the codebase that knows the POSIX cut SYNOPSIS,
produces coreutils-compatible error messages, and bridges the CLI surface to
the typed domain model defined in SP-01 and SP-02.

The implementation is a sequential pipeline of named, independently-testable
helper functions orchestrated by `parse_args`. This mirrors the modularity of
`getopt` without its global state or C API.

A CMake-generated `config.hpp` provides the program name, version, and help
hint as single-source-of-truth constants so no string is duplicated across the
codebase.

## Scope

**In scope:**
- `ParseResult` struct (`CutOptions` + `std::vector<std::string>` files + `bool help_requested`)
- `include/cut/config.hpp.in` template + CMake `configure_file` wiring
- `include/cut/arg_parser.hpp` — declare `parse_args` + five helpers
- `src/arg_parser.cpp` — implement all six functions
- `namespace cc_cut` for all new symbols
- Coreutils-compatible error strings (prefix + hint on one error value)
- File-path deduplication (first-occurrence, includes `-` for stdin)
- `--help` flag: print help text to stdout, set `ParseResult::help_requested = true`

**Out of scope:**
- `--version` flag — not in SYNOPSIS
- `-w` whitespace-delimiter flag — not required for initial implementation
- `--complement` flag — not in SYNOPSIS
- `--output-delimiter` — not in SYNOPSIS
- `-z` (NUL-terminated lines) — not in SYNOPSIS
- Actual file opening / FileSource construction — SP-04
- Processing algorithm — SP-05 through SP-07

---

## Requirements

### REQ-001: ParseResult Struct

**Statement:** The project SHALL define `struct cc_cut::ParseResult` with three
members: `CutOptions opts`, `std::vector<std::string> files`, and
`bool help_requested = false` in `include/cut/parse_result.hpp`.

**Acceptance Criteria:**
- [ ] Default-constructed `ParseResult` has `opts == CutOptions{}`,
      `files.empty() == true`, and `help_requested == false`
- [ ] `files` holds plain file-path strings (not `FileSource` objects)
- [ ] `help_requested == true` signals caller to exit 0 after printing help
- [ ] Defined inside `namespace cc_cut {}`

**Dependencies:**
| Dependency | Assumed Behavior |
|-----------|-----------------|
| `CutOptions` (SPEC-1 REQ-003) | Default-constructs with `mode=FIELD`, `delim=nullopt`, `suppress=false`, `no_split=false` |

**Test Cases:**
- TC-REQ001-01: Default `ParseResult` has `opts.mode == CutMode::FIELD`, `files.empty() == true`, and `help_requested == false`
- TC-REQ001-02: Assigning `files = {"a.txt", "b.txt"}` gives `files.size() == 2`
- TC-REQ001-03: Setting `help_requested = true` and reading it back returns `true`

---

### REQ-002: config.hpp Generation

**Statement:** The CMake build SHALL generate `include/cut/config.hpp` from
`include/cut/config.hpp.in` via `configure_file`, providing:
- `cc_cut::config::program_name` — `std::string_view` equal to `"cc-cut-tool"`
  (the CMake `PROJECT_NAME`)
- `cc_cut::config::version` — `std::string_view` equal to `"0.1.0"`
  (the CMake `PROJECT_VERSION`)
- `cc_cut::config::help_hint` — `std::string_view` equal to
  `"Try 'cc-cut-tool --help' for more information."`

**Acceptance Criteria:**
- [ ] `cmake --build` generates `include/cut/config.hpp` before any source
      that includes it is compiled
- [ ] `cc_cut::config::program_name` equals `"cc-cut-tool"` at runtime
- [ ] `cc_cut::config::help_hint` equals
      `"Try 'cc-cut-tool --help' for more information."` at runtime
- [ ] Changing `project(cc-cut-tool VERSION X.Y.Z)` in `CMakeLists.txt`
      automatically updates `cc_cut::config::version` on next build

**Dependencies:**
| Dependency | Assumed Behavior |
|-----------|-----------------|
| CMake `configure_file` | Substitutes `@PROJECT_NAME@`, `@PROJECT_VERSION@` from cache variables |

**Test Cases:**
- TC-REQ002-01: `cc_cut::config::program_name == std::string_view{"cc-cut-tool"}` — static_assert or runtime check
- TC-REQ002-02: `cc_cut::config::help_hint.ends_with("for more information.")` — confirms format

---

### REQ-003: parse_args Function Signature

**Statement:** The project SHALL declare in `include/cut/arg_parser.hpp` and
implement in `src/arg_parser.cpp` the free function
`cc_cut::parse_args(int argc, char** argv) -> std::expected<ParseResult, std::string>`.
The function orchestrates `detect_mode`, `extract_list_spec`, `parse_list`,
`parse_mode_properties`, and `collect_files` in that order, returning the first
error encountered.

**Acceptance Criteria:**
- [ ] `parse_args(1, argv)` (no arguments) returns error containing
      `"you must specify a list of bytes, characters, or fields"`
- [ ] `parse_args` with valid `-f1` and no files returns `ParseResult` with
      `opts.mode == CutMode::FIELD`, `opts.list.indices == {0}`, `files.empty()`
- [ ] Error strings are prefixed with `cc_cut::config::program_name + ": "`
      and suffixed with `"\n" + cc_cut::config::help_hint`

**Dependencies:**
| Dependency | Assumed Behavior |
|-----------|-----------------|
| `cc_cut::parse_list` (SPEC-2) | Returns `expected<CutList, string>` where error string has no prefix |
| `cc_cut::config::program_name` (REQ-002) | Equals `"cc-cut-tool"` |

**Test Cases:**
- TC-REQ003-01: `parse_args(1, {"cut"})` → error `"cc-cut-tool: you must specify a list of bytes, characters, or fields\nTry 'cc-cut-tool --help' for more information."`
- TC-REQ003-02: `parse_args(3, {"cut", "-f", "1"})` → `ParseResult{opts.mode=FIELD, opts.list.indices={0}, files={}}`
- TC-REQ003-03: `parse_args(4, {"cut", "-f1", "a.txt", "b.txt"})` → `files == {"a.txt", "b.txt"}`

---

### REQ-004: detect_mode

**Statement:** `detect_mode(std::string_view flag)` SHALL return `CutMode::BYTE`
for `"-b..."`, `CutMode::CHARACTER` for `"-c..."`, `CutMode::FIELD` for
`"-f..."`. It SHALL return an error for any other string.

**Acceptance Criteria:**
- [ ] `"-b"` → `CutMode::BYTE`
- [ ] `"-b1,3"` → `CutMode::BYTE` (list attached; only first two chars checked)
- [ ] `"-c"` → `CutMode::CHARACTER`
- [ ] `"-f"` → `CutMode::FIELD`
- [ ] `"-x"` → error `"cc-cut-tool: invalid option -- 'x'\nTry 'cc-cut-tool --help' for more information."`
- [ ] `""` (empty) or `"-"` (lone dash) → same error

**Dependencies:**
| Dependency | Assumed Behavior |
|-----------|-----------------|
| `cc_cut::config::program_name` | `"cc-cut-tool"` |

**Test Cases:**
- TC-REQ004-01: `detect_mode("-b")` → `CutMode::BYTE`
- TC-REQ004-02: `detect_mode("-c3-5")` → `CutMode::CHARACTER`
- TC-REQ004-03: `detect_mode("-f")` → `CutMode::FIELD`
- TC-REQ004-04: `detect_mode("-x")` → error contains `"invalid option -- 'x'"`
- TC-REQ004-05: `detect_mode("-")` → error contains `"invalid option"`
- TC-REQ004-06: `detect_mode("")` → error contains `"invalid option"`

---

### REQ-005: extract_list_spec

**Statement:** `extract_list_spec(std::string_view flag, int argc, char** argv, int& index)`
SHALL extract the list specification string. `flag` is the mode flag string
(e.g. `"-f"` or `"-f1,3"`); `index` is the index of the first argument AFTER
the flag (i.e., starts at 2 when flag is `argv[1]`). If `flag.size() > 2`
(list attached, e.g. `"-f1,3"`), return `flag.substr(2)` and leave `index`
unchanged. Otherwise consume `argv[index]` as the list and increment `index`
by 1. If `index >= argc` when the separate form is needed, return an error.

**Acceptance Criteria:**
- [ ] `flag = "-f1,3"`, `index = 2` → returns `"1,3"`, `index` stays 2
- [ ] `flag = "-f"`, `argv[2] = "1,3"`, `index = 2` → returns `"1,3"`, `index` becomes 3
- [ ] `flag = "-f"`, `index = 2`, `argc = 2` → error
      `"cc-cut-tool: option requires an argument -- 'f'\nTry 'cc-cut-tool --help' for more information."`

**Dependencies:**
| Dependency | Assumed Behavior |
|-----------|-----------------|
| `flag` | Valid null-terminated C-string; `flag.size()` returns byte count |
| `argv` | Array of at least `argc` valid pointers |

**Test Cases:**
- TC-REQ005-01: `flag="-f1,3"`, index=2 → spec `"1,3"`, index stays 2
- TC-REQ005-02: `flag="-f"`, `argv[2]="1,3"`, index=2, argc=4 → spec `"1,3"`, index becomes 3
- TC-REQ005-03: `flag="-f"`, index=2, argc=2 → error contains `"option requires an argument -- 'f'"`
- TC-REQ005-04: `flag="-b"`, index=2, argc=2 → error contains `"option requires an argument -- 'b'"`

---

### REQ-006: parse_mode_properties

**Statement:** `parse_mode_properties(int argc, char** argv, int& index, CutOptions& opts)`
SHALL consume zero or more mode-specific flags starting at `argv[index]`,
updating `opts` and advancing `index` for each consumed flag. It stops when it
encounters an argument that is not a mode-specific flag (a file path or `-`).

| Mode | Flags consumed | Effect on `opts` |
|------|---------------|-----------------|
| BYTE | `-n` | `opts.no_split = true` |
| FIELD | `-d <char>` | `opts.delim = char`; error if delimiter is more than one byte |
| FIELD | `-d<char>` (attached) | `opts.delim = char`; error if more than one byte after `-d` |
| FIELD | `-s` | `opts.suppress = true` |
| CHARACTER | _(none)_ | no-op; advance 0 |

**Acceptance Criteria:**
- [ ] BYTE mode, `-n` present → `opts.no_split == true`
- [ ] FIELD mode, `-d ,` → `opts.delim == ','`
- [ ] FIELD mode, `-d ","` (multi-char) → error
      `"cc-cut-tool: the delimiter must be a single character\nTry '...'"` 
- [ ] FIELD mode, `-s` → `opts.suppress == true`
- [ ] FIELD mode, `-d ,` and `-s` in sequence → both `opts.delim == ','` and `opts.suppress == true`
- [ ] Flag not recognised as a mode property (e.g. `-n` in FIELD mode, `-s` in BYTE mode,
      any unknown `-x` flag) is NOT consumed — index stays at that position so the caller
      treats it as a file argument
- [ ] `-d` with no following character (index >= argc when separate form needed) → error
      `"cc-cut-tool: option requires an argument -- 'd'\nTry 'cc-cut-tool --help' for more information."`

**Dependencies:**
| Dependency | Assumed Behavior |
|-----------|-----------------|
| `CutOptions` (SPEC-1 REQ-003) | `delim` is `optional<char>`; `no_split`/`suppress` are `bool` |

**Test Cases:**
- TC-REQ006-01: BYTE mode, `argv = ["-n", "file"]`, index=0 → `opts.no_split == true`, index=1
- TC-REQ006-02: FIELD mode, `argv = ["-d", ",", "file"]`, index=0 → `opts.delim == ','`, index=2
- TC-REQ006-03: FIELD mode, `argv = ["-d,", "file"]` (attached) → `opts.delim == ','`, index=1
- TC-REQ006-04: FIELD mode, `argv = ["-d", ",,"]` → error contains `"the delimiter must be a single character"`
- TC-REQ006-05: FIELD mode, `argv = ["-s", "file"]`, index=0 → `opts.suppress == true`, index=1
- TC-REQ006-07: FIELD mode, `argv = ["-d", ",", "-s", "file"]`, index=0 → `opts.delim == ','` AND `opts.suppress == true`, index=3
- TC-REQ006-08: BYTE mode, `argv = ["-s", "file"]`, index=0 → index stays 0, opts unchanged (not consumed)
- TC-REQ006-09: FIELD mode, `argv = ["-d"]`, index=0, argc=1 → error contains `"option requires an argument -- 'd'"`
- TC-REQ006-06: CHARACTER mode, any argv → index unchanged, opts unchanged

---

### REQ-007: collect_files

**Statement:** `collect_files(int argc, char** argv, int index)` SHALL collect
all `argv[index..argc-1]` as file-path strings, deduplicated by first
occurrence. The string `"-"` (stdin sentinel) is treated as a path and
deduplicated like any other. The result is `std::vector<std::string>`.

**Acceptance Criteria:**
- [ ] `["a.txt", "b.txt"]` → `{"a.txt", "b.txt"}`
- [ ] `["a.txt", "-", "b.txt", "-", "a.txt"]` → `{"a.txt", "-", "b.txt"}`
  (second `-` and second `"a.txt"` dropped)
- [ ] `[]` (no files) → empty vector
- [ ] Order of first occurrences is preserved

**Dependencies:**
| Dependency | Assumed Behavior |
|-----------|-----------------|
| None | std::string copy of each argv element |

**Test Cases:**
- TC-REQ007-01: `["a.txt", "b.txt"]` → `size == 2`, `[0] == "a.txt"`, `[1] == "b.txt"`
- TC-REQ007-02: `["a.txt", "-", "b.txt", "-", "a.txt"]` → `size == 3`, `[2] == "b.txt"`
- TC-REQ007-03: `[]` → `size == 0`
- TC-REQ007-04: `["-", "-"]` → `size == 1`, `[0] == "-"`

---

### REQ-008: Error Message Format

**Statement:** Every error string returned by `parse_args` or any helper SHALL
have the form:
```
<program_name>: <message>\n<help_hint>
```
where `<program_name>` is `cc_cut::config::program_name`, `<message>` is the
specific error, and `<help_hint>` is `cc_cut::config::help_hint`. The string
does NOT end with a trailing newline (caller adds it when writing to stderr).

**Acceptance Criteria:**
- [ ] Every error returned starts with `"cc-cut-tool: "`
- [ ] Every error returned ends with `"Try 'cc-cut-tool --help' for more information."`
- [ ] No trailing `'\n'` at the end of the returned string
- [ ] Errors translated from `parse_list` replace `"values may not include zero"`
      with the mode-specific zero-position message:
      BYTE/CHARACTER → `"byte/character positions are numbered from 1"`;
      FIELD → `"fields are numbered from 1"`

**Dependencies:**
| Dependency | Assumed Behavior |
|-----------|-----------------|
| `cc_cut::config::program_name` | `"cc-cut-tool"` |
| `cc_cut::config::help_hint` | `"Try 'cc-cut-tool --help' for more information."` |
| `cc_cut::parse_list` error strings | Match SPEC-2 REQ-008..REQ-010 exactly |

**Test Cases:**
- TC-REQ008-01: Any error from `parse_args` starts with `"cc-cut-tool: "` — verified by `error.starts_with("cc-cut-tool: ")`
- TC-REQ008-02: Any error ends with `"Try 'cc-cut-tool --help' for more information."` — verified by `error.ends_with(cc_cut::config::help_hint)`
- TC-REQ008-03: `-b 0` → error `"cc-cut-tool: byte/character positions are numbered from 1\nTry 'cc-cut-tool --help' for more information."`
- TC-REQ008-04: `-f 0` → error `"cc-cut-tool: fields are numbered from 1\nTry 'cc-cut-tool --help' for more information."`
- TC-REQ008-05: `-c 0` → error `"cc-cut-tool: byte/character positions are numbered from 1\nTry 'cc-cut-tool --help' for more information."`

---

### REQ-009: No Mode Specified

**Statement:** When `argc < 2` or `argv[1]` does not start with `-b`, `-c`,
or `-f`, `parse_args` SHALL return error
`"cc-cut-tool: you must specify a list of bytes, characters, or fields\nTry 'cc-cut-tool --help' for more information."`.

**Acceptance Criteria:**
- [ ] `argc == 1` → this exact error
- [ ] `argv[1] == "file.txt"` (no flag) → this exact error
- [ ] `argv[1] == "--"` → this exact error

**Dependencies:**
| Dependency | Assumed Behavior |
|-----------|-----------------|
| `cc_cut::config::program_name` | `"cc-cut-tool"` |

**Test Cases:**
- TC-REQ009-01: `parse_args(1, ["cut"])` → error == `"cc-cut-tool: you must specify a list of bytes, characters, or fields\nTry 'cc-cut-tool --help' for more information."`
- TC-REQ009-02: `parse_args(2, ["cut", "file.txt"])` → same error
- TC-REQ009-03: `parse_args(2, ["cut", "--"])` → same error

---

### REQ-010: --help Flag

**Statement:** When `--help` appears as any argument in `argv[1..argc-1]`,
`parse_args` SHALL print the help text below to stdout and return a
`ParseResult` with `help_requested = true` (all other fields at their
default values).

Help text (exact, printed to stdout):
```
Usage: cc-cut-tool -b list [-n] [file ...]
       cc-cut-tool -c list [file ...]
       cc-cut-tool -f list [-d delim] [-s] [file ...]

  -b list  Cut by byte positions
  -c list  Cut by character positions (UTF-8)
  -f list  Cut by fields
  -d delim Field delimiter (default: tab)
  -n       Do not split multi-byte characters (byte mode)
  -s       Suppress lines with no delimiter (field mode)
  --help   Show this help
```

**Acceptance Criteria:**
- [ ] `parse_args` with `--help` anywhere in argv returns `ParseResult` with
      `help_requested == true`
- [ ] `parse_args` with `--help` writes the exact help text above to stdout
- [ ] `parse_args` with `--help` does NOT return an error (returns success)
- [ ] `--help` before a valid mode flag is still recognised (not treated as
      a file argument)

**Dependencies:**
| Dependency | Assumed Behavior |
|-----------|-----------------|
| `cc_cut::config::program_name` | `"cc-cut-tool"` used in Usage line |

**Test Cases:**
- TC-REQ010-01: `parse_args(2, ["cut", "--help"])` → `result.has_value() == true` and `result->help_requested == true`
- TC-REQ010-02: `parse_args(3, ["cut", "--help", "-f1"])` → `result->help_requested == true` (--help wins)
- TC-REQ010-03: `parse_args(2, ["cut", "--help"])` → stdout contains `"Usage: cc-cut-tool"`

---

## Assumptions

| ID | Assumption | Impact if Wrong | Verified By |
|----|-----------|----------------|-------------|
| ASM-001 | `argv[0]` is the program name but is NOT used for error messages; `config::program_name` is used instead | Error messages show wrong program name | TC-REQ008-01 |
| ASM-002 | `-b`, `-c`, `-f` flags are mutually exclusive; only the first is checked | Second mode flag silently ignored | TC-REQ004-01..03 |
| ASM-003 | Delimiter character is a single byte (ASCII or single UTF-8 start byte); multi-byte delimiters rejected | Non-ASCII single-codepoint delimiter fails incorrectly | TC-REQ006-04 |
| ASM-004 | `argv` pointers are valid null-terminated strings; lifetime covers parse_args call | UB on dangling pointer | Manual review |

---

## Test Coverage Matrix

| REQ-ID | Requirement | Test Cases | Status |
|--------|-------------|-----------|--------|
| REQ-001 | ParseResult struct | TC-REQ001-01, TC-REQ001-02, TC-REQ001-03 | 🔴 Pending |
| REQ-002 | config.hpp generation | TC-REQ002-01, TC-REQ002-02 | 🔴 Pending |
| REQ-003 | parse_args signature | TC-REQ003-01, TC-REQ003-02, TC-REQ003-03 | 🔴 Pending |
| REQ-004 | detect_mode | TC-REQ004-01..06 | 🔴 Pending |
| REQ-005 | extract_list_spec | TC-REQ005-01..04 | 🔴 Pending |
| REQ-006 | parse_mode_properties | TC-REQ006-01..09 | 🔴 Pending |
| REQ-007 | collect_files | TC-REQ007-01..04 | 🔴 Pending |
| REQ-008 | Error message format | TC-REQ008-01..05 | 🔴 Pending |
| REQ-009 | No mode specified | TC-REQ009-01..03 | 🔴 Pending |
| REQ-010 | --help flag | TC-REQ010-01, TC-REQ010-02, TC-REQ010-03 | 🔴 Pending |
