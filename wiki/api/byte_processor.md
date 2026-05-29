# `cc_cut::ByteProcessor` — API Reference

**Header:** `include/cut/byte_processor.hpp`
**Namespace:** `cc_cut`
**Inherits:** `Processor`
**Spec:** SPEC-6 (`.ai/specs/2026-05-29-sp06-byte-mode.md`)
**Since:** SP-06

---

## Overview

Implements `-b` byte mode. Selects raw bytes from each input line by 0-based position
according to `CutList`. With the `-n` flag (`opts.no_split = true`), selection is
UTF-8 boundary-aware: a character is included if and only if its lead byte position is
in the list.

---

## Class: `ByteProcessor`

```cpp
class ByteProcessor : public Processor {
 public:
  explicit ByteProcessor(CutOptions opts);

  static auto select_bytes(std::string_view line, const CutList& list) -> std::string;
  static auto select_bytes_no_split(std::string_view line, const CutList& list) -> std::string;

  void process_line(std::string_view line, std::ostream& out) const;

  auto run(std::ostream& out, const std::vector<std::string>& files,
           std::ostream& err) -> int override;

 private:
  CutOptions opts_;
};
```

---

## Constructor

```cpp
explicit ByteProcessor(CutOptions opts);
```

Stores `opts` by value. Typically constructed via `make_processor`.

---

## Static Methods

### `select_bytes`

```cpp
static auto select_bytes(std::string_view line, const CutList& list) -> std::string;
```

Selects raw bytes from `line` at 0-based positions specified by `list`.

- Positions beyond `line.size()` are silently skipped.
- `list.open_from = n` selects all bytes from position `n` to end-of-line.
- `list.indices` and `list.open_from` are ORed — a position matching either is included.

**Returns:** Owned `std::string` of selected bytes in order.

#### Examples

```cpp
ByteProcessor::select_bytes("hello", {.indices={0}})      // → "h"
ByteProcessor::select_bytes("hello", {.indices={0,1,4}})  // → "heo"
ByteProcessor::select_bytes("hello", {.open_from=3})      // → "lo"
ByteProcessor::select_bytes("hello", {.indices={9}})      // → ""
```

---

### `select_bytes_no_split`

```cpp
static auto select_bytes_no_split(std::string_view line, const CutList& list) -> std::string;
```

Selects bytes from `line` with UTF-8 boundary awareness (`-n` flag). A character is
included if and only if its **lead byte** position is in `list`.

- Pure ASCII input: identical result to `select_bytes`.
- Multi-byte characters: entire character is included or excluded atomically.
- Invalid UTF-8 bytes: treated as 1-byte characters (ASM-002) — never skipped or crashed on.

Uses `utf8::internal::validate_next` from the vendored `include/utf8.h`.

#### Examples

```cpp
// é = \xC3\xA9 (2 bytes). Lead byte at position 0.
ByteProcessor::select_bytes_no_split("\xC3\xA9", {.indices={0}})  // → "\xC3\xA9" (full char)
ByteProcessor::select_bytes_no_split("\xC3\xA9", {.indices={1}})  // → "" (lead not selected)

// € = \xE2\x82\xAC (3 bytes). Lead byte at position 0.
ByteProcessor::select_bytes_no_split("\xE2\x82\xAC", {.indices={0}})  // → "\xE2\x82\xAC"

// Invalid byte: treated as 1-byte char
ByteProcessor::select_bytes_no_split("\x80", {.indices={0}})  // → "\x80"
```

---

## Instance Methods

### `process_line`

```cpp
void process_line(std::string_view line, std::ostream& out) const;
```

Selects bytes from `line` and writes `result + '\n'` to `out`. Always writes at least a
newline, even for an empty input line.

Dispatches to `select_bytes_no_split` if `opts_.no_split`, else `select_bytes`.

---

### `run`

```cpp
auto run(std::ostream& out, const std::vector<std::string>& files,
         std::ostream& err) -> int override;
```

Processes all files via the `FileSource` layer. Continues past individual file errors.

- Empty `files` → reads from stdin (via `make_file_source("-")`).
- File-open errors: written to `err`, `exit_code` set to 1, processing continues.
- `ios_base::failure` from `load()`: same error treatment.

**Returns:** 0 on full success; 1 if any file error occurred.

---

## Behavior Reference

| Condition | Behavior |
|-----------|----------|
| Position beyond line length | Silently skipped |
| Empty input line | Outputs `"\n"` |
| Invalid UTF-8 with `-n` | Treat as 1-byte char; include/exclude based on lead position |
| No delimiter | No suppress logic — all lines always processed (`-b` mode has no `-s`) |
| Byte positions in CLI | 1-based; stored 0-based internally (existing `CutList` convention) |

---

## See Also

- `cc_cut::Processor` — `include/cut/processor.hpp` (SP-06)
- `cc_cut::make_processor` — `include/cut/processor.hpp` (SP-06)
- `cc_cut::CutList` — `include/cut/list.hpp` (SP-01)
- `cc_cut::CutOptions` — `include/cut/options.hpp` (SP-03)
