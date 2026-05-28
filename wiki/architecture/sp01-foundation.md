# SP-01: Foundation Types — Architecture Decision Record

**Date:** 2026-05-28 | **Status:** Accepted | **Spec:** SPEC-1

---

## Context

SP-01 establishes the shared type vocabulary that all subsequent sub-projects
(SP-02 through SP-08) depend on. Getting these types right before any logic is
written prevents cascading refactors across the codebase.

## Decisions

### D-001: `CutList::open_from` as `std::optional<int>`

**Rejected:** `int open_from = -1` (magic sentinel)  
**Chosen:** `std::optional<int> open_from`

`nullopt` is self-documenting (no open range). A sentinel of `-1` violates
Explicit-over-Implicit — readers need a comment to understand it.

### D-002: `CutOptions::delim` as `std::optional<char>`

**Chosen:** `std::optional<char> delim`

`nullopt` = whitespace-split mode (default). `some(c)` = exact single-char
delimiter. Avoids a separate boolean flag and keeps the two cases unambiguous
at the type level.

### D-003: `FileSource` deletes copy and move

**Chosen:** delete all copy/move special members.

`FileSource` manages a backing buffer. Copying it would duplicate potentially
large mmap'd memory. All callers use `std::unique_ptr<FileSource>` — no copy
or move semantics are needed or safe.

### D-004: `FileSource::load()` throws on I/O failure

**Chosen:** `load()` throws `std::ios_base::failure`.

Returning `void` with no failure path forces implementors to choose between
silently swallowing errors (unsafe) and storing error state (implicit contract
violation). Documented throws make the failure contract explicit at the
interface boundary.

### D-005: `#pragma once` with clang-tidy suppression

**Rejected:** traditional `#ifndef` include guards  
**Chosen:** `#pragma once` with `-portability-avoid-pragma-once` suppressed.

`#pragma once` is supported by all major compilers and is less error-prone
than matching guard macro names. The portability check is overly conservative
for this macOS Clang-only project.

## Type Inventory

| Type | File | REQ |
|------|------|-----|
| `enum class CutMode : uint8_t` | `include/cut/mode.hpp` | REQ-001 |
| `struct CutList` | `include/cut/list.hpp` | REQ-002 |
| `struct CutOptions` | `include/cut/options.hpp` | REQ-003 |
| `class FileSource` (abstract) | `include/cut/file_source.hpp` | REQ-004 |
