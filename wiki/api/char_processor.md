# CharProcessor API

**Header:** `include/cut/char_processor.hpp`  
**Source:** `src/char_processor.cpp`  
**Spec:** SPEC-7

## Overview

`CharProcessor` implements `-c list` (character mode). Inherits `Processor`.
Iterates input lines codepoint-by-codepoint; `CutList` indices are 0-based
codepoint positions. Invalid UTF-8 bytes are treated as single codepoints (ASM-003).

## Constructor

```cpp
explicit CharProcessor(CutOptions opts);
```

Takes options by value; stores internally. No copy/move (inherited from `Processor`).

## Static Methods

### `select_chars`

```cpp
static auto select_chars(std::string_view line, const CutList& list) -> std::string;
```

Pure function. Iterates `line` codepoint-by-codepoint via
`utf8::internal::validate_next`. Includes the full UTF-8 byte sequence of
codepoint N if `N ∈ list.indices` or `N >= list.open_from`. Invalid bytes are
treated as 1-byte codepoints (ASM-003). Returns owned string.

**Example:**
```
"héllo" with list {1} → "\xC3\xA9"   (é, codepoint at index 1)
"héllo" with open_from=2 → "llo"     (codepoints 2,3,4)
```

## Instance Methods

### `process_line`

```cpp
void process_line(std::string_view line, std::ostream& out) const;
```

Calls `select_chars(line, opts_.list)` and writes result + `'\n'` to `out`.
Always writes the newline, even for empty input.

### `run`

```cpp
auto run(std::ostream& out, const std::vector<std::string>& files,
         std::ostream& err) -> int override;
```

Iterates `files` (or stdin when empty). Writes errors to `err` and continues
to remaining files on open failure. Returns 0 on full success, 1 if any file
error occurred.

## Differences from ByteProcessor

| Dimension | ByteProcessor | CharProcessor |
|-----------|--------------|--------------|
| List unit | byte offset (0-based) | codepoint index (0-based) |
| `-n` flag | supported | N/A |
| Invalid UTF-8 | 1-byte per invalid (ASM-002) | 1-byte per invalid (ASM-003) |
