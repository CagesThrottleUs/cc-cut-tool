# cc-cut-tool — Master Plan

**Date:** 2026-05-28  
**Standard:** C++23  
**Build:** CMake 4.0 + Clang  
**Test:** GoogleTest (unit) + existing integration suite  

---

## Goal

Implement a POSIX/BSD-compatible `cut` tool in C++ with:
- `-b list [-n]` — byte mode
- `-c list` — character mode (UTF-8)
- `-f list [-d delim] [-s]` — field mode

Industry-standard code: proper design patterns, clean separation of concerns, MMIO for large files.

---

## Sub-Projects (sequential, each builds on previous)

| SP | Name | Deliverable | Entry Gate |
|----|------|------------|-----------|
| SP-01 | Foundation | `CutMode` enum, `CutOptions` struct, `CutList` type stubs, CMake include wiring | Clean build |
| SP-02 | List Parser | `parse_list()` — ranges, open ranges, comma/space sep → set + open-range flag | SP-01 done |
| SP-03 | Arg Parser | `parse_args()` — full argv → `CutOptions`; validates mutually exclusive modes | SP-02 done |
| SP-04 | File Interface | `FileSource` abstraction: stdin (always in-memory) + file (in-memory <100 MB, MMIO ≥100 MB via Boost) | SP-03 done |
| SP-05 | Field Mode | End-to-end `-f`: delimiter split, field selection, `-s` suppression, output | SP-04 done |
| SP-06 | Byte Mode | End-to-end `-b`: byte-range selection, `-n` (no-split of multibyte chars) | SP-05 done |
| SP-07 | Char Mode | End-to-end `-c`: UTF-8 codepoint-aware selection | SP-06 done |
| SP-08 | Integration Pass | All 8 user cases + coreutils Perl tests green; wire `main.cpp` | SP-07 done |

---

## Architecture Sketch

```
main.cpp
  └─ parse_args(argc, argv) → CutOptions
       ├─ parse_list()      → CutList (set<int> + open_from)
       └─ detect_mode()     → CutMode
  └─ build_sources(opts)    → vector<FileSource>   [dedup applied here]
  └─ run(opts, sources)     → exit code

CutOptions
  ├─ mode: CutMode          (BYTE | CHARACTER | FIELD)
  ├─ list: CutList
  ├─ delim: char            (FIELD only, default '\t')
  ├─ suppress: bool         (FIELD -s)
  └─ no_split: bool         (BYTE -n)

FileSource (interface)
  ├─ load()                 → void
  ├─ getline()              → optional<string_view>
  ├─ StdinSource            (always in-memory)
  ├─ SmallFileSource        (in-memory, <100 MB)
  └─ MmapFileSource         (Boost.Iostreams mapped_file, ≥100 MB)

Algorithm — run():
  for each source in sources:
    source.load()
    while line = source.getline():
      values = get_fields(line, opts)   // dispatches on opts.mode
      print_values(values, opts)        // rejoins with delim, writes stdout

Processor — get_fields(line, opts):
  ├─ process_field(line, opts) → vector<string_view>
  ├─ process_byte(line, opts)  → vector<string_view>
  └─ process_char(line, opts)  → vector<string_view>

Output — print_values(values, opts):
  └─ join values with opts.delim, write '\n'
```

---

## Key Decisions (resolved)

| Decision | Choice |
|----------|--------|
| C++ standard | C++23 |
| Character mode encoding | UTF-8 only (no locale) |
| MMIO library | Boost.Iostreams `mapped_file_source` |
| MMIO threshold | ≥100 MB |
| stdin | Always buffered in memory (not mappable) |
| File dedup | De-duplicate identical paths and multiple `-` before processing |
| Error on missing file | Continue remaining files; exit 1 at end |
| List separator | Comma (primary), space (secondary when no comma present) |

---

## Open Questions (decide per SP spec)

- SP-06: Exact behavior of `-n` on non-UTF8 byte sequences?
- SP-08: Which coreutils Perl test vectors currently pass vs need new features?

---

## Current Status

- [ ] SP-01: Foundation
- [ ] SP-02: List Parser
- [ ] SP-03: Arg Parser
- [ ] SP-04: File Interface
- [ ] SP-05: Field Mode
- [ ] SP-06: Byte Mode
- [ ] SP-07: Char Mode
- [ ] SP-08: Integration Pass
