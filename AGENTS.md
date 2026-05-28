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

## Build

```bash
cmake --preset clang-debug         # configure
cmake --build --preset clang-debug  # build
```

Release: replace `clang-debug` with `clang-release`.

## Test

```bash
cd out/build/clang-debug && ctest --output-on-failure
```

### CTest Labels

| Label | What runs |
|-------|-----------|
| _(no label)_ | All tests |
| `integration` | Shell + Perl integration tests |
| `user` | 8 user-specified cases |
| `coreutils` | Perl coreutils test vectors |

Unit tests only (exclude integration):

```bash
ctest --output-on-failure --exclude-regex "integ_"
```

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
- Boost.Iostreams declared in CMake but not linked until SP-04

## Sub-Project Status

| SP | Status |
|----|--------|
| SP-01 Foundation | ✅ Complete |
| SP-02 List Parser | Pending |
| SP-03 Arg Parser | Pending |
| SP-04 File Interface | Pending |
| SP-05 Field Mode | Pending |
| SP-06 Byte Mode | Pending |
| SP-07 Char Mode | Pending |
| SP-08 Integration Pass | Pending |
