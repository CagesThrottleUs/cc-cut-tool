---
spec_id: SPEC-4
title: SP-04 File Interface
status: draft
---

# SP-04: File Interface — Specification

**Version:** 1.0 | **Date:** 2026-05-28 | **Status:** Draft

---

## Context

SP-04 provides concrete implementations of the `FileSource` abstract interface
(SPEC-1 REQ-004) so the processing pipeline (SP-05 onwards) can read lines from
stdin, small files, and large files transparently. Three implementations share a
single line-splitting strategy via a `protected static` method on the base class,
keeping each subclass responsible only for its storage strategy.

Lines are delimited by `\n`. The returned `string_view` includes any `\r`
before the `\n` — callers receive raw line content with no CRLF normalization.
The field/byte/character processors (SP-05 through SP-07) decide how to handle
`\r` in field values.

## Scope

**In scope:**
- `mmap_threshold` constexpr in `include/cut/file_source.hpp`
- `FileSource::next_line()` protected static method (line-splitting logic)
- `StdinSource` — reads all stdin into memory on `load()`
- `BufferedFileSource` — reads file < `mmap_threshold` into memory
- `MmapFileSource` — maps file ≥ `mmap_threshold` via Boost.Iostreams
- `make_file_source(path)` factory → `std::expected<std::unique_ptr<FileSource>, std::string>`
- Error strings prefixed with `cc_cut::config::program_name`

**Out of scope:**
- CRLF normalization — `\r` is part of line content; SP-05 handles it
- stdin size detection — stdin always uses `StdinSource` regardless of size
- Parallel file reading — one file per `FileSource` instance
- File watching or re-read on change
- Processing algorithm — SP-05 through SP-07
- Verifying that factory does not call `load()` internally — enforced by the
  SPEC-1 REQ-004 contract; requires mocking infrastructure not in scope here

---

## Requirements

### REQ-001: mmap_threshold Constant

**Statement:** `include/cut/file_source.hpp` SHALL define
`inline constexpr std::size_t cc_cut::mmap_threshold = 100ULL * 1024ULL * 1024ULL`
(100 MiB). Files with `std::filesystem::file_size(path) >= mmap_threshold` use
`MmapFileSource`; smaller files use `BufferedFileSource`.

**Acceptance Criteria:**
- [ ] `cc_cut::mmap_threshold == 104857600` (100 × 1024 × 1024)
- [ ] Defined in `include/cut/file_source.hpp`, inside `namespace cc_cut`
- [ ] Changing the constant and rebuilding changes the dispatch threshold without
      editing any other file

**Dependencies:**
| Dependency | Assumed Behavior |
|-----------|-----------------|
| `include/cut/file_source.hpp` (SPEC-1) | Header already exists; constant is added to it |

**Test Cases:**
- TC-REQ001-01: `static_assert(cc_cut::mmap_threshold > 0)` compiles — confirms the constant exists and is positive; value is not asserted so it can be changed without breaking tests
- TC-REQ001-02: `static_assert(std::is_same_v<decltype(cc_cut::mmap_threshold), const std::size_t>)` compiles — confirms the type is `std::size_t`

---

### REQ-002: next_line Protected Static Method

**Statement:** `FileSource` SHALL declare and define
`static auto next_line(std::string_view buffer, std::size_t& cursor) -> std::optional<std::string_view>`
as a `protected` member. The method SHALL:
1. Return `std::nullopt` when `cursor >= buffer.size()`.
2. Search for `\n` in `buffer` starting at `cursor`.
3. If `\n` found at position `pos`: return `buffer.substr(cursor, pos - cursor)`,
   set `cursor = pos + 1`, return the view.
4. If `\n` not found: return `buffer.substr(cursor)` (remainder of buffer),
   set `cursor = buffer.size()`, return the view.

The returned `string_view` does NOT strip `\r`. It aliases `buffer` — valid
as long as the buffer is alive.

**Acceptance Criteria:**
- [ ] `next_line("hello\nworld\n", cursor=0)` → `"hello"`, cursor becomes 6
- [ ] `next_line("hello\nworld\n", cursor=6)` → `"world"`, cursor becomes 12
- [ ] `next_line("hello\nworld\n", cursor=12)` → `nullopt`
- [ ] `next_line("hello\nworld", cursor=6)` (no trailing `\n`) → `"world"`,
      cursor becomes 11
- [ ] `next_line("hello\r\nworld\r\n", cursor=0)` → `"hello\r"`, cursor becomes 7
- [ ] `next_line("", cursor=0)` → `nullopt`

**Dependencies:**
| Dependency | Assumed Behavior |
|-----------|-----------------|
| `FileSource` (SPEC-1 REQ-004) | Abstract base class exists; `protected` section added |

**Test Cases:**
- TC-REQ002-01: `next_line("a\nb\n", c=0)` → `"a"`, c=2
- TC-REQ002-02: `next_line("a\nb\n", c=2)` → `"b"`, c=4
- TC-REQ002-03: `next_line("a\nb\n", c=4)` → `nullopt`
- TC-REQ002-04: `next_line("a\nb", c=2)` (no trailing `\n`) → `"b"`, c=3
- TC-REQ002-05: `next_line("a\r\nb", c=0)` → `"a\r"`, c=3 (CRLF: `\r` included)
- TC-REQ002-06: `next_line("", c=0)` → `nullopt`
- TC-REQ002-07: `next_line("\n", c=0)` → `""` (empty string_view), c=1

---

### REQ-003: StdinSource

**Statement:** `class cc_cut::StdinSource : public cc_cut::FileSource` SHALL read
all bytes from `std::cin` into a `std::string` member on `load()`. After `load()`,
repeated calls to `getline()` SHALL return successive lines via `next_line()`.
`StdinSource` SHALL reside in `include/cut/stdin_source.hpp` and
`src/stdin_source.cpp`.

**Acceptance Criteria:**
- [ ] `load()` reads all of `std::cin` into an internal `std::string` buffer
- [ ] `getline()` returns successive non-`\n`-terminated line views from the buffer
- [ ] `getline()` returns `std::nullopt` after the last line is consumed
- [ ] A file with no trailing `\n` has its last line returned without truncation

**Dependencies:**
| Dependency | Assumed Behavior |
|-----------|-----------------|
| `std::cin` | Readable; `std::istreambuf_iterator` drains it completely |

**Depends on:** REQ-002

**Test Cases:**
- TC-REQ003-01: stdin = `"line1\nline2\n"` → getline() × 3 returns `"line1"`,
  `"line2"`, `nullopt`
- TC-REQ003-02: stdin = `"line1\nline2"` (no trailing `\n`) → `"line1"`,
  `"line2"`, `nullopt`
- TC-REQ003-03: stdin = `""` (empty) → getline() returns `nullopt` immediately
- TC-REQ003-04: stdin = `"a\r\nb"` → `"a\r"`, `"b"`, `nullopt` (`\r` preserved)

---

### REQ-004: BufferedFileSource

**Statement:** `class cc_cut::BufferedFileSource : public cc_cut::FileSource`
SHALL read the entire file at `path` into a `std::string` member on `load()`.
If the file cannot be opened or read, `load()` SHALL throw
`std::ios_base::failure` with the OS error message. `BufferedFileSource` SHALL
reside in `include/cut/buffered_file_source.hpp` and
`src/buffered_file_source.cpp`.

**Acceptance Criteria:**
- [ ] `load()` reads all bytes from the file into an internal `std::string` buffer
- [ ] `getline()` returns successive lines via `next_line()`
- [ ] `load()` throws `std::ios_base::failure` when the file does not exist
- [ ] `load()` throws `std::ios_base::failure` when the file cannot be opened
      (e.g. permission denied)
- [ ] A file with no trailing `\n` has its last line returned without truncation

**Dependencies:**
| Dependency | Assumed Behavior |
|-----------|-----------------|
| `std::ifstream` | Opens and reads files; `exceptions(failbit | badbit)` triggers on error |

**Depends on:** REQ-002

**Test Cases:**
- TC-REQ004-01: file `"a\nb\n"` → `"a"`, `"b"`, `nullopt`
- TC-REQ004-02: file `"a\nb"` (no trailing `\n`) → `"a"`, `"b"`, `nullopt`
- TC-REQ004-03: non-existent path → `load()` throws `std::ios_base::failure`
- TC-REQ004-04: empty file → `load()` succeeds; `getline()` returns `nullopt`

---

### REQ-005: MmapFileSource

**Statement:** `class cc_cut::MmapFileSource : public cc_cut::FileSource`
SHALL memory-map the file at `path` using `boost::iostreams::mapped_file_source`
on `load()`. `getline()` SHALL return `string_view`s directly into the mapped
region (zero copy). If the file cannot be mapped, `load()` SHALL throw
`std::ios_base::failure`. `MmapFileSource` SHALL reside in
`include/cut/mmap_file_source.hpp` and `src/mmap_file_source.cpp`.

**Acceptance Criteria:**
- [ ] `load()` opens the file as a read-only memory map
- [ ] `getline()` returns successive lines via `next_line()` using the mapped
      region as the buffer (zero-copy is an implementation detail; behavior
      is verified by correctness TCs, not pointer aliasing)
- [ ] Each line returned by `getline()` has the same byte content as
      `BufferedFileSource` reading the same file
- [ ] `load()` throws `std::ios_base::failure` when the file cannot be mapped
- [ ] A file with no trailing `\n` has its last line returned without truncation

**Dependencies:**
| Dependency | Assumed Behavior |
|-----------|-----------------|
| `boost::iostreams::mapped_file_source` | Maps file read-only; `.data()` returns `const char*`; `.size()` returns byte count |
| Boost.Iostreams linked in CMakeLists.txt | `find_package(Boost REQUIRED)` + `target_link_libraries(... Boost::iostreams)` |

**Depends on:** REQ-002

**Test Cases:**
- TC-REQ005-01: file `"a\nb\n"` mapped → `"a"`, `"b"`, `nullopt`
- TC-REQ005-02: file `"a\nb"` (no trailing `\n`) → `"a"`, `"b"`, `nullopt`
- TC-REQ005-03: non-existent path → `load()` throws `std::ios_base::failure`
- TC-REQ005-04: empty file → `load()` succeeds; `getline()` returns `nullopt`

---

### REQ-006: make_file_source Factory

**Statement:** `cc_cut::make_file_source(std::string_view path)`
SHALL return `std::expected<std::unique_ptr<cc_cut::FileSource>, std::string>`.
Dispatch rules:
- `path == "-"` → `StdinSource` (always; no size check)
- `path != "-"` and `file_size(path) < mmap_threshold` → `BufferedFileSource`
- `path != "-"` and `file_size(path) >= mmap_threshold` → `MmapFileSource`
- File not found or unreadable → error string
  `"cc-cut-tool: {path}: No such file or directory"` or
  `"cc-cut-tool: {path}: Permission denied"` as appropriate

The factory SHALL NOT call `load()` — the caller calls `load()` after receiving
the `FileSource`. `make_file_source` SHALL reside in
`include/cut/make_file_source.hpp` and `src/make_file_source.cpp`.

**Acceptance Criteria:**
- [ ] `path == "-"` returns `StdinSource` instance
- [ ] `path` to a file of size 0 returns `BufferedFileSource`
- [ ] `path` to a non-existent file returns error containing the path and reason
- [ ] The returned `unique_ptr` is non-null on success
- [ ] Calling `getline()` on the returned source (without calling `load()` first)
      produces undefined behaviour — factory does not advance state beyond construction

**Dependencies:**
| Dependency | Assumed Behavior |
|-----------|-----------------|
| `std::filesystem::file_size(path)` | Returns file size in bytes; throws `std::filesystem::filesystem_error` if not accessible |
| `cc_cut::config::program_name` (SPEC-3 REQ-002) | `"cc-cut-tool"` |

**Test Cases:**
- TC-REQ006-01: `make_file_source("-")` → `has_value() == true`; dynamic type
  is `StdinSource`
- TC-REQ006-02: `make_file_source(path_to_small_file)` → dynamic type is
  `BufferedFileSource`
- TC-REQ006-03: `make_file_source(path_to_nonexistent)` →
  `has_value() == false`; error contains path string
- TC-REQ006-04: `make_file_source("-")` returns source; immediately calling
  `load()` on it succeeds (verifies source is in pre-load state, not already
  loaded)

---

### REQ-007: Error String Format

**Statement:** All error strings returned by `make_file_source` SHALL have the
form `"cc-cut-tool: <path>: <reason>"` where `<reason>` is the OS-provided
error description (e.g. `"No such file or directory"`, `"Permission denied"`).
The string SHALL NOT contain a trailing newline.

**Acceptance Criteria:**
- [ ] Error string starts with `"cc-cut-tool: "`
- [ ] Error string contains the exact path passed to `make_file_source`
- [ ] Error string does NOT end with `'\n'`

**Dependencies:**
| Dependency | Assumed Behavior |
|-----------|-----------------|
| `cc_cut::config::program_name` (SPEC-3 REQ-002) | `"cc-cut-tool"` |

**Test Cases:**
- TC-REQ007-01: `make_file_source("/no/such/file").error().starts_with("cc-cut-tool: ")`
  → true
- TC-REQ007-02: `make_file_source("/no/such/file").error()` contains
  `"/no/such/file"` as a substring
- TC-REQ007-03: `make_file_source("/no/such/file").error().back() != '\n'` → true

---

## Assumptions

| ID | Assumption | Impact if Wrong | Verified By |
|----|-----------|----------------|-------------|
| ASM-001 | stdin is always fully available before processing begins — no streaming/incremental read | Large stdin inputs exhaust memory | TC-REQ003-01 (memory bound by test) |
| ASM-002 | `mmap_threshold` = 100 MiB covers typical workloads; tests do not create 100 MiB files | Wrong threshold choice | TC-REQ001-01 |
| ASM-003 | `boost::iostreams::mapped_file_source` is available on the build host | MmapFileSource fails to compile | Build verification |
| ASM-004 | `string_view` returned by `getline()` remains valid until the `FileSource` is destroyed or `load()` is called again | Use-after-free | TC-REQ002-01..07 + ASan |
| ASM-005 | Files do not change while being read (no TOCTOU between size check and open) | Wrong class selected or mapping fails | Manual review |

---

## Test Coverage Matrix

| REQ-ID | Requirement | Test Cases | Status |
|--------|-------------|-----------|--------|
| REQ-001 | mmap_threshold constant | TC-REQ001-01, TC-REQ001-02 | 🔴 Pending |
| REQ-002 | next_line logic | TC-REQ002-01..07 | 🔴 Pending |
| REQ-003 | StdinSource | TC-REQ003-01..04 | 🔴 Pending |
| REQ-004 | BufferedFileSource | TC-REQ004-01..04 | 🔴 Pending |
| REQ-005 | MmapFileSource | TC-REQ005-01..04 | 🔴 Pending |
| REQ-006 | make_file_source factory | TC-REQ006-01..04 | 🔴 Pending |
| REQ-007 | Error string format | TC-REQ007-01..03 | 🔴 Pending |
