# Language Review Fixes — SP-09
**Date:** 2026-05-29  
**Branch:** `feat/sp-09/language-review-fixes`  
**Source:** Language expert review (Opus-powered, 20+ yr C++ veteran)

---

## Summary

Applied all Critical and Major findings from the language review. Zero clang-tidy warnings post-fix. Unit test count unchanged: 197/198 (1 pre-existing `integ_cut_pl` failure unrelated to this work).

---

## Fixes Applied

### Critical

| ID | File | Fix |
|----|------|-----|
| D3-CRIT | `src/processor.cpp` | Added `std::unreachable()` after exhaustive `CutMode` switch — function previously fell off non-void switch without returning, which is UB per [stmt.return]/2 |

### Major

| ID | Files | Fix |
|----|-------|-----|
| D3-MAJ | `src/list_parser.cpp` | Added `list_max_position = 1,000,000` cap; extracted `insert_bounded_range()` helper; `-M` and `N-M` range tokens now return error for values exceeding cap — prevents OOM/DoS from `cut -f 1-2147483647` |
| D1 | `include/cut/list.hpp` | `CutList::indices` changed `std::set<int>` → `std::set<std::size_t>`; `open_from` changed `std::optional<int>` → `std::optional<std::size_t>` — eliminates per-byte `numeric_limits<int>::max()` clamping guards throughout all three processors |
| D7-NODISC | `include/cut/{arg_parser,list_parser,make_file_source,processor}.hpp` + `include/cut/file_source.hpp` | Added `[[nodiscard]]` to all `std::expected`-returning APIs, `Processor::run()`, and `FileSource::getline()` — discarded errors were undetectable by compiler |
| D8-PERF | `src/{byte,char,field}_processor.cpp` | Added `result.reserve(line.size())` in all three `select_*` helpers — eliminates repeated reallocations on long input lines |
| D6-DUP | `include/cut/processor.hpp` + `src/processor.cpp` + all three processor `.cpp` | Moved shared `run()` implementation to `Processor` base class (non-virtual, concrete); each derived class now only implements `process_line`; removed 3 identical try/catch+source-loop bodies |
| D7-PRIV | `include/cut/{byte,char,field}_processor.hpp` | `process_line` made `protected` pure virtual in `Processor` base; declared `public override` in derived classes (public for direct test access; base hides it from `Processor*` callers). NOLINT annotation explains intentional visibility promotion |
| D4-WRAP | `include/cut/utf8_util.hpp` (new) | Wrapped `utf8::internal::validate_next` in `cc_cut::detail::utf8_advance_one` — isolates the vendor `::internal::` namespace to a single call site; both `byte_processor.cpp` and `char_processor.cpp` updated to use it |
| D2-FINAL | `include/cut/{buffered_file_source,mmap_file_source,stdin_source}.hpp` | Added `final` to all three concrete `FileSource` subclasses — enables devirtualization of `load()`/`getline()` call sites |

### Minor

- Removed `<vector>` from `byte_processor.hpp` and `char_processor.hpp` (unused after `run()` removal)
- Removed `<string>` from `field_processor.hpp` (unused after `run()` removal)
- Removed `<set>`, `<span>` from `field_processor.cpp`; added `<limits>` for sentinel
- Added `<cstddef>` to `buffered_file_source.hpp`, `mmap_file_source.hpp`, `stdin_source.hpp` — `std::size_t` used via `cursor_` member
- Updated all test helpers (`make_list`, `make_open_list`, `make_opts`, `make_opts_field`) to use `std::size_t` types
- Fixed one char_processor test that discarded `run()` return value (now captures and asserts `== 1`)

---

## Deferred

| Item | Reason |
|------|--------|
| `std::set` → `flat_set` hot-path performance | Requires Boost.Container or custom sorted vector; type correctness fix (`size_t`) delivers the cast elimination, which was the correctness concern. Performance tuning is a separate pass. |
| `process_line` fully private | Tests call `process_line` directly via concrete objects; making it private would require rewriting 16 test cases to use `run()` with temp files. Documented with NOLINT + comment; deferring to a future test refactor. |
| `BufferedFileSource` double-load guard | Load-twice is UB by doc contract only. Adding `assert(!loaded_)` or `std::expected` return is a separate hardening task. |

---

## Verification

```
clang-tidy: 0 warnings (project files, system/vendor excluded)
cmake --build: clean (0 errors, 0 warnings)
ctest (unit): 197/198 passing  ← unchanged from pre-branch baseline
integ_cut_pl: 33/100 failures  ← pre-existing, not introduced here
```
