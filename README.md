# cc-json-parser-cpp

JSON parser in C++ for Coding Challenges by John Crickett.

## Configuration (do this first)

**Before editing**, configure once so clangd and the build use the same toolchain:

```bash
cmake --preset clang-release
```

Restart clangd after changing config: **Cmd+Shift+P** → "clangd: Restart language server".

### Paths (default: Homebrew LLVM on macOS)

| Purpose            | Default path                                  |
| ------------------ | --------------------------------------------- |
| LLVM install       | `/opt/homebrew/opt/llvm`                      |
| C/C++ compilers    | `/opt/homebrew/opt/llvm/bin/clang`, `clang++` |
| C++ stdlib headers | `/opt/homebrew/opt/llvm/include/c++/v1`       |

If your toolchain lives elsewhere, update both **`.clangd`**  so the IDE and build use the same paths.

### `.clangd`

Configures the clangd language server so the IDE matches the build. **CompileFlags.Add** includes:

| Flag / purpose   | Value (adjust if your paths differ)                                                                                                                        |
| ---------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------- |
| C++ standard     | `-std=c++23`                                                                                                                                               |
| C++ stdlib       | `-nostdinc++` then `-isystem /opt/homebrew/opt/llvm/include/c++/v1` (avoids using-declaration conflicts; change the path if your LLVM stdlib is elsewhere) |
| Project includes | `-I<project-root>/include` (add `-I<project-root>/src` if headers under `src/` do not resolve)                                                             |
| Boost            | `-isystem /opt/homebrew/opt/boost/include`                                                                                                                 |

After editing `.clangd`, restart clangd (**Cmd+Shift+P** → "clangd: Restart language server").


## Dependencies

| Dependency     | Version / Notes                                                                                                                   |
| -------------- | --------------------------------------------------------------------------------------------------------------------------------- |
| CMake          | ≥ 3.10                                                                                                                            |
| C/C++ compiler | Clang or GCC (Clang recommended)                                                                                                  |
| Ninja          | Build generator (used by preset)                                                                                                  |
| **Optional**   |                                                                                                                                   |
| Homebrew LLVM  | If present at `/opt/homebrew/opt/llvm`, the build uses it via `cmake/HomebrewLLVM.cmake`. Otherwise the default compiler is used. |

### Installing (macOS, optional)

```bash
brew install cmake ninja llvm
```

## Build

```bash
# Configure (once per preset)
cmake --preset clang-release   # or clang-debug
# Build
cmake --build --preset clang-release   # or clang-debug
```

| Preset        | Executable                                   |
| ------------- | -------------------------------------------- |
| clang-release | `out/build/clang-release/cc-json-parser-cpp` |
| clang-debug   | `out/build/clang-debug/cc-json-parser-cpp`   |

## Run & debug

The parser is invoked with a JSON file path as the first argument. Exit code **0** = valid JSON, **1** = invalid.

- **VS Code**: Use the **run** task or **Debug (Clang)** launch config; you’ll be prompted for the file path.
- **CLI**: `./out/build/clang-release/cc-json-parser-cpp path/to/file.json`

## Tests

Full suite (all `.json` under `tests/`, expected exit by filename prefix):

```bash
./run_tests.sh
```

Override parser or concurrency: `PARSER=/path/to/binary MAX_JOBS=8 ./run_tests.sh`

| Prefix                | Expected exit |
| --------------------- | ------------- |
| invalid, fail, n_, i_ | 1             |
| valid, pass, i_, y_   | 0             |
