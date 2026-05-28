---
spec_id: SPEC-1
title: SP-01 Foundation
status: draft
---

# SP-01: Foundation — Specification

**Version:** 1.0 | **Date:** 2026-05-28 | **Status:** Draft

---

## Context

SP-01 establishes the shared type vocabulary for the entire cc-cut-tool project. All subsequent sub-projects (SP-02 through SP-08) depend on these types compiling clean and expressing the domain correctly.

No logic is implemented here — only the types, enums, and interface contracts that every other layer uses.

## Scope

**In scope:**
- `CutMode` enum class (BYTE, CHARACTER, FIELD)
- `CutList` struct (0-based index set + open-range marker)
- `CutOptions` struct (aggregated parsed arguments)
- `FileSource` abstract interface (`load()`, `getline()` contract)
- CMake: wire `include/` directory, find Boost (stub — not linked yet)
- All headers compile clean under C++23 with no warnings

**Out of scope:**
- Any parsing logic (argv → CutOptions) — SP-03
- Any FileSource implementation — SP-04
- Any processing algorithm — SP-05 through SP-07
- Boost linkage (declared in CMake but not used until SP-04)
- Behavior when Boost is not installed on the build host (build requires Boost; missing Boost = developer environment issue, not a tool error condition)

---

## Requirements

### REQ-001: CutMode Enum

**Statement:** The project SHALL define `enum class CutMode : uint8_t` with exactly three enumerators: `BYTE`, `CHARACTER`, `FIELD`.

**Acceptance Criteria:**
- [ ] `CutMode::BYTE`, `CutMode::CHARACTER`, `CutMode::FIELD` all compile and are distinct values
- [ ] Underlying type is `uint8_t`
- [ ] Defined in `include/cut/mode.hpp`

**Dependencies:**
| Dependency | Assumed Behavior |
|-----------|-----------------|
| C++23 compiler | `enum class` with explicit underlying type compiles without warning |

**Test Cases:**
- TC-REQ001-01: `static_assert(CutMode::BYTE != CutMode::CHARACTER)` compiles and passes
- TC-REQ001-02: `static_assert(CutMode::CHARACTER != CutMode::FIELD)` compiles and passes
- TC-REQ001-03: `static_assert(sizeof(CutMode) == 1)` confirms `uint8_t` underlying type

---

### REQ-002: CutList Struct

**Statement:** The project SHALL define `struct CutList` with two members: `std::set<int> indices` (0-based selected positions) and `std::optional<int> open_from` (`std::nullopt` means finite/closed selection; `some(n)` where `n ≥ 0` means "select all positions from index `n` to end of line inclusive").

**Acceptance Criteria:**
- [ ] `CutList` default-initialises with `indices` empty and `open_from == std::nullopt`
- [ ] `open_from == std::nullopt` unambiguously represents a finite/closed selection
- [ ] `open_from == some(n)` where `n >= 0` unambiguously represents an open-ended range starting at 0-based index `n`
- [ ] Defined in `include/cut/list.hpp`

**Dependencies:**
| Dependency | Assumed Behavior |
|-----------|-----------------|
| `<set>` | `std::set<int>` default-constructs empty |
| `<optional>` | `std::optional<int>` default-constructs to `std::nullopt` |

**Test Cases:**
- TC-REQ002-01: Default-constructed `CutList` has `open_from == std::nullopt` and `indices.empty() == true`
- TC-REQ002-02: Setting `open_from = 2` gives `open_from.has_value() == true` and `open_from.value() == 2`
- TC-REQ002-03: Inserting `{0, 2, 4}` into `indices` and checking `indices.size() == 3` passes

---

### REQ-003: CutOptions Struct

**Statement:** The project SHALL define `struct CutOptions` aggregating all parsed CLI arguments with the following members and defaults:

| Member | Type | Default | Meaning |
|--------|------|---------|---------|
| `mode` | `CutMode` | `CutMode::FIELD` | Active mode |
| `list` | `CutList` | default `CutList` | Selection list |
| `delim` | `std::optional<char>` | `std::nullopt` | `nullopt` = whitespace-split; `some(c)` = exact char delimiter |
| `suppress` | `bool` | `false` | `-s`: skip lines without delimiter |
| `no_split` | `bool` | `false` | `-n`: do not split multibyte chars |

**Acceptance Criteria:**
- [ ] Default-constructed `CutOptions` has `mode == CutMode::FIELD`, `delim == nullopt`, `suppress == false`, `no_split == false`
- [ ] `delim = std::nullopt` unambiguously means whitespace-split mode
- [ ] `delim = some('|')` stores and returns `'|'` via `delim.value()`
- [ ] Defined in `include/cut/options.hpp`, which includes `mode.hpp` and `list.hpp`

**Dependencies:**
| Dependency | Assumed Behavior |
|-----------|-----------------|
| `<optional>` | `std::optional<char>` default-constructs to `nullopt` |
| REQ-001 | `CutMode` defined |
| REQ-002 | `CutList` defined |

**Depends on:** REQ-001, REQ-002

**Test Cases:**
- TC-REQ003-01: Default `CutOptions` has `mode == CutMode::FIELD`, `delim == std::nullopt`, `suppress == false`, `no_split == false`
- TC-REQ003-02: Setting `opts.delim = ','` then `opts.delim.has_value() == true` and `opts.delim.value() == ','`
- TC-REQ003-03: Setting `opts.suppress = true` then reading it back returns `true`

---

### REQ-004: FileSource Abstract Interface

**Statement:** The project SHALL define `class FileSource` as an abstract base with exactly two pure virtual methods: `load()` returning `void`, and `getline()` returning `std::optional<std::string_view>`.

**Acceptance Criteria:**
- [ ] `FileSource` cannot be instantiated directly (pure virtual)
- [ ] `getline()` contract documented: returns content from internal cursor to next `\n` (exclusive); advances cursor past `\n`; returns `std::nullopt` at EOF
- [ ] `load()` contract documented: reads source into internal buffer; must be called before `getline()`
- [ ] Virtual destructor defined
- [ ] Defined in `include/cut/file_source.hpp`

**Dependencies:**
| Dependency | Assumed Behavior |
|-----------|-----------------|
| `<optional>` | `std::optional<std::string_view>` usable as return type |
| `<string_view>` | `string_view` is non-owning; backing buffer must outlive the view |

**Test Cases:**
- TC-REQ004-01: Attempting to instantiate `FileSource` directly fails to compile (pure virtual)
- TC-REQ004-02: A concrete subclass implementing both methods compiles and is instantiable
- TC-REQ004-03: A concrete stub implementing `load()` as no-op and `getline()` returning `std::nullopt` compiles, links, and when called once returns `std::nullopt` — verified by `EXPECT_EQ(stub.getline(), std::nullopt)` under AddressSanitizer with exit code 0
- TC-REQ004-04: Deleting a heap-allocated concrete subclass via a `FileSource*` base pointer under AddressSanitizer exits 0 with no "heap-use-after-free" or "new-delete-type-mismatch" diagnostic — confirms virtual destructor is present and called

---

### REQ-005: CMake Include Wiring

**Statement:** The CMake build SHALL add `include/` as a private include directory for both the main executable and the test executable, and SHALL call `find_package(Boost REQUIRED)` as a stub declaration (Boost not linked until SP-04).

**Acceptance Criteria:**
- [ ] `#include "cut/mode.hpp"` resolves from `src/main.cpp` without additional flags
- [ ] `#include "cut/options.hpp"` resolves from any file under `src/` or test sources
- [ ] CMake configure step completes without error when Boost is present
- [ ] No Boost library is linked to any target (link step deferred to SP-04)

**Dependencies:**
| Dependency | Assumed Behavior |
|-----------|-----------------|
| Boost installed on build host | `find_package(Boost REQUIRED)` locates Boost headers and sets `Boost_INCLUDE_DIRS`; if absent, CMake configure exits non-zero with "Could not find Boost" diagnostic |

**Test Cases:**
- TC-REQ005-01: `cmake --build` succeeds with `src/main.cpp` including all four new headers; build exits 0
- TC-REQ005-02: `otool -L <binary>` output contains no line matching `libboost`; exit 0 from the check command confirms no Boost dylib linked

---

### REQ-006: Clean Compile

**Statement:** All headers in `include/cut/` SHALL compile clean under C++23 with `-Wall -Wextra -Werror` and Clang's `-Weverything` (minus `-Wno-c++98-compat`).

**Acceptance Criteria:**
- [ ] `cmake --build` exits 0 with zero diagnostic lines on stderr
- [ ] All headers compile with `-std=c++23 -Wall -Wextra -Werror` and produce no output to stderr
- [ ] `clang-tidy` run on all headers in `include/cut/` exits 0 with zero warnings

**Dependencies:**
| Dependency | Assumed Behavior |
|-----------|-----------------|
| `.clang-tidy` config | Existing project tidy config applied |

**Test Cases:**
- TC-REQ006-01: `cmake --build` exits 0 with no diagnostic output for headers-only change
- TC-REQ006-02: `clang-tidy include/cut/*.hpp` reports zero issues

---

## Assumptions

| ID | Assumption | Impact if Wrong | Verified By |
|----|-----------|----------------|-------------|
| ASM-001 | Boost is installed on all developer machines | SP-05 `find_package` fails | TC-REQ005-01 |
| ASM-002 | C++23 compiler available (Clang on macOS) | Headers may not compile | TC-REQ006-01 |
| ASM-003 | `string_view` backing buffer outlives all `getline()` calls | Use-after-free in consumers | TC-REQ004-03 + code review |

---

## Test Coverage Matrix

| REQ-ID | Requirement | Test Cases | Status |
|--------|-------------|-----------|--------|
| REQ-001 | CutMode enum | TC-REQ001-01, TC-REQ001-02, TC-REQ001-03 | ✅ Pass |
| REQ-002 | CutList struct | TC-REQ002-01, TC-REQ002-02, TC-REQ002-03 | ✅ Pass |
| REQ-003 | CutOptions struct | TC-REQ003-01, TC-REQ003-02, TC-REQ003-03 | ✅ Pass |
| REQ-004 | FileSource interface | TC-REQ004-01, TC-REQ004-02, TC-REQ004-03, TC-REQ004-04 | ✅ Pass |
| REQ-005 | CMake include wiring | TC-REQ005-01, TC-REQ005-02 | ✅ Pass |
| REQ-006 | Clean compile | TC-REQ006-01, TC-REQ006-02 | ✅ Pass |
