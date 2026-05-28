# cc-cut-tool

C++ implementation of the POSIX `cut` utility. Built with CMake + Ninja + Clang on macOS.

## Tech Stack

| Tool | Version | Purpose |
|------|---------|---------|
| C++ | 23 | Implementation language |
| CMake | 4.0 | Build system |
| Ninja | — | Build backend |
| Clang | system | Compiler (macOS) |
| GoogleTest | 8736d2c | Unit test framework (FetchContent) |
| Boost | system | MMIO via Boost.Iostreams (SP-04+) |
| clang-tidy | system | Static analysis (`.clang-tidy` config) |

## Build and Test

See [README.md](README.md) for build and test commands.

**Note:** CMake presets set `CMAKE_CXX_COMPILER=/opt/homebrew/opt/llvm/bin/clang++`
to ensure `clang-tidy -p out/build/clang-debug` resolves C++23 headers.

## Clangd / Clang-tidy Gotchas

- `Compiler:` override in `.clangd` required — compile_commands.json records ccache;
  clangd cannot introspect it for C++23 headers (`<expected>`, `<format>`).
- `-Werror` absent from `.clangd` intentionally — `-Weverything` fires on every `TEST()`
  macro; 85+ tests exceed the 20-error limit causing `fatal_too_many_errors`.
- `misc-use-internal-linkage` does NOT fire on functions declared in headers — do not
  add NOLINTNEXTLINE for this check on externally-linked functions; verify with
  `clang-tidy -p out/build/clang-debug <file>` before suppressing.
- `from_chars` requires raw `const char*` (no C++23 safe alternative) — 2 NOLINTNEXTLINE
  in `src/list_parser.cpp` are legitimate and unavoidable.
- Use `arg.starts_with("-d")` instead of `arg[1]=='d'` to avoid bounds-check warnings.
- Use `span.subspan(i).front()` instead of `argv[i]` to eliminate pointer arithmetic.

### CTest Labels

| Label | What runs |
|-------|-----------|
| _(no label)_ | All tests |
| `integration` | Shell + Perl integration tests |
| `user` | 8 user-specified cases |
| `coreutils` | Perl coreutils test vectors |

## Project Structure

```
include/cut/          Type-definition headers
  mode.hpp            CutMode enum (BYTE | CHARACTER | FIELD)
  list.hpp            CutList struct (indices set + open_from)
  options.hpp         CutOptions struct (parsed CLI args)
  file_source.hpp     FileSource abstract interface

src/main.cpp          Entry point
tests/cut/            Unit tests (one file per header)
integrations/         Integration test scripts and Perl coreutils vectors
sample/               Sample data files (TSV, CSV)

.ai/
  specs/              Feature specs (SPEC-N format)
  plans/              Implementation plans
  reports/            Quality gate reports
  master-plan.md      Sub-project roadmap (SP-01..SP-08)
```

## Key Design Decisions

- `CutList::open_from` is `std::optional<int>` — `nullopt` = finite, `some(n)` = select from n to EOL
- `CutOptions::delim` is `std::optional<char>` — `nullopt` = whitespace-split, `some(c)` = exact char
- `FileSource` deletes copy/move — consumed only via `unique_ptr<FileSource>`
- Boost.Iostreams linked in CMake (SP-04); used by MmapFileSource

## Sub-Project Status

| SP | Status |
|----|--------|
| SP-01 Foundation | ✅ Complete |
| SP-02 List Parser | ✅ Complete |
| SP-03 Arg Parser | Pending |
| SP-04 File Interface | ✅ Complete |
| SP-05 Field Mode | ✅ Complete |
| SP-06 Byte Mode | Pending |
| SP-07 Char Mode | Pending |
| SP-08 Integration Pass | Pending |
