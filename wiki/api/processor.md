# `cc_cut::Processor` — API Reference

**Header:** `include/cut/processor.hpp`
**Namespace:** `cc_cut`
**Spec:** SPEC-6 (`.ai/specs/2026-05-29-sp06-byte-mode.md`)
**Since:** SP-06

---

## Overview

Abstract base class for all cut mode processors. Subclasses implement `run()` for a
specific `CutMode`. All processors are consumed only via `std::unique_ptr<Processor>` —
copy and move are deleted.

Use `make_processor` to obtain a concrete instance; callers need not know the concrete
type.

---

## Class: `Processor`

```cpp
class Processor {
 public:
  Processor() = default;
  Processor(const Processor&) = delete;
  Processor(Processor&&) = delete;
  auto operator=(const Processor&) -> Processor& = delete;
  auto operator=(Processor&&) -> Processor& = delete;
  virtual ~Processor() = default;

  virtual auto run(std::ostream& out,
                   const std::vector<std::string>& files,
                   std::ostream& err) -> int = 0;
};
```

### `run`

```cpp
virtual auto run(std::ostream& out,
                 const std::vector<std::string>& files,
                 std::ostream& err) -> int = 0;
```

Processes all files. Continues past individual file errors.

| Parameter | Description |
|-----------|-------------|
| `out` | Output stream for selected content |
| `files` | File paths to process. Empty or `"-"` → reads stdin |
| `err` | Error stream for diagnostics |

**Returns:** 0 on full success; 1 if any file error occurred.

**Note:** `string_view` lines from `FileSource::getline()` alias the source's internal
buffer. Subclasses must consume each line before calling `getline()` again.

**Note:** `run()` may be called more than once on the same instance.

---

## Function: `make_processor`

```cpp
auto make_processor(const CutOptions& opts)
    -> std::expected<std::unique_ptr<Processor>, std::string>;
```

Factory that creates the appropriate `Processor` for `opts.mode`.

| Mode | Returns |
|------|---------|
| `CutMode::BYTE` | `std::make_unique<ByteProcessor>(opts)` |
| `CutMode::FIELD` | `std::make_unique<FieldProcessor>(opts)` |
| `CutMode::CHARACTER` | `std::unexpected{"cc-cut-tool: character mode not yet implemented"}` |

**Error:** The unexpected error string is a user-visible message suitable for printing
directly to stderr.

### Example

```cpp
auto proc = cc_cut::make_processor(opts);
if (!proc) {
    std::cerr << proc.error() << '\n';
    return 1;
}
return (*proc)->run(std::cout, files, std::cerr);
```

---

## Implementing a new mode

1. Add a concrete class inheriting `Processor` (see `ByteProcessor`, `FieldProcessor`).
2. Implement `run()` with `override`.
3. Add a `case CutMode::NEW_MODE:` branch in `src/processor.cpp::make_processor`.
4. No changes to `src/main.cpp` are needed.

---

## See Also

- `cc_cut::ByteProcessor` — `include/cut/byte_processor.hpp` (SP-06)
- `cc_cut::FieldProcessor` — `include/cut/field_processor.hpp` (SP-05)
- `cc_cut::CutOptions` — `include/cut/options.hpp` (SP-03)
