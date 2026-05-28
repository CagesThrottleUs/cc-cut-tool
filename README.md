# cc-cut-tool

The Cut Tool. Coding Challenges by John Crickett.

This project is created using AI - primarily because I was curious of how good a code can be created by my own private AI Harness.

This README would provide you on how to install and test the tool yourself.

I have also provided a [prompt file](./prompt/README.md) to let you know how did I start at it.

## Build

Requires: CMake 4.0+, Ninja, Clang, Boost.

```bash
cmake --preset clang-debug
cmake --build --preset clang-debug
```

## Test

```bash
cd out/build/clang-debug && ctest --output-on-failure
```

Unit tests only:

```bash
ctest --output-on-failure --exclude-regex "integ_"
```
