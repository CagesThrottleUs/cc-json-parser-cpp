# cc-json-parser-cpp

JSON parser in C++ for Coding Challenges by John Crickett.

## Configuration (do this first)

**Before editing**, configure once so clangd and the build use the same toolchain:

```bash
cmake --preset clang-release
```

Restart clangd after changing config: **Cmd+Shift+P** → "clangd: Restart language server".

### Paths (default: Homebrew LLVM on macOS)

| Purpose            | Default path |
|--------------------|--------------|
| LLVM install       | `/opt/homebrew/opt/llvm` |
| C/C++ compilers    | `/opt/homebrew/opt/llvm/bin/clang`, `clang++` |
| C++ stdlib headers | `/opt/homebrew/opt/llvm/include/c++/v1` |

If your toolchain lives elsewhere, update both **`.clangd`** and **`cmake/HomebrewLLVM.cmake`** so the IDE and build use the same paths.

### `.clangd`

- **CompileFlags.Add** — Uses a single C++ stdlib path to avoid "using declaration conflicts": `-nostdinc++` (skip default C++ stdlib path), then `-isystem` + path (e.g. `/opt/homebrew/opt/llvm/include/c++/v1`). If your stdlib is elsewhere, change only that path.

### `cmake/HomebrewLLVM.cmake`

Used by the `clang-release` preset. When Homebrew LLVM exists, the build uses it.

- **Compiler check** — `if(EXISTS "/opt/homebrew/opt/llvm/bin/clang++")`; change the path if your LLVM is elsewhere (or remove the block to use the default compiler).
- **Compilers** — `CMAKE_C_COMPILER` / `CMAKE_CXX_COMPILER`; set to your `clang` / `clang++` if not under `/opt/homebrew/opt/llvm/bin/`.
- **C++ stdlib** — `CMAKE_CXX_FLAGS` appends `-isystem /opt/homebrew/opt/llvm/include/c++/v1`; change this path if your stdlib headers are elsewhere, and keep it in sync with `.clangd`.


## Dependencies

| Dependency   | Version / Notes |
|-------------|------------------|
| CMake       | ≥ 3.10           |
| C/C++ compiler | Clang or GCC (Clang recommended) |
| Ninja       | Build generator (used by preset) |
| **Optional** | |
| Homebrew LLVM | If present at `/opt/homebrew/opt/llvm`, the build uses it via `cmake/HomebrewLLVM.cmake`. Otherwise the default compiler is used. |

### Installing (macOS, optional)

```bash
brew install cmake ninja llvm
```

## Build

```bash
cmake --preset clang-release
cmake --build out/build/clang-release
```

Executable: `out/build/clang-release/cc-json-parser-cpp`
