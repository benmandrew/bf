# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

A Brainfuck-to-LLVM-IR compiler frontend written in C17. Produces two executables:

- **`bfc`** — compiler: reads `.b` files (or stdin), emits LLVM IR to stdout, or the control flow graph as Graphviz dot with `--emit-cfg-dot`
- **`bfi`** — interpreter: reads `.b` files and executes them directly

Run the web demo with `docker compose up` (served at `http://localhost:8080`); it shows the source, compiled IR, and control flow graph side by side, the graph's blocks holding syntax-highlighted IR. The backend renders via `bfc --emit-cfg-dot --cfg-instructions` → `highlight.py` → `dot`; its image is built with nix (`nix build .#bfcImage`, see `flake.nix`), not a Dockerfile, so bfc/Graphviz/Python match the dev shell — Debian's Graphviz is too old to draw the instruction-level labels.

## Build

The Nix flake (`flake.nix`) provides a devShell with every tool the build needs — cmake, LLVM/clang, `check`, `expect`, `clang-format`, `cpplint`, Doxygen, Graphviz, Python, `ruff`, `shfmt`, `shellcheck` — pinned via `flake.lock`. This is the primary, CI-verified workflow. Sphinx is the exception: `docs/CMakeLists.txt` pip-installs it into a venv from `docs/requirements.txt` at build time, so the docs target needs network access and is not pinned by the flake.

```bash
nix develop                           # enter the devShell; run everything below inside it
```

Manual dependency installation (`cmake`, `llvm-dev`, see README) remains an alternative if not using Nix. If LLVM is not found, cmake will warn and skip all library/executable targets — only docs and formatting targets remain.

```bash
cmake -B build                        # configure (Debug by default, includes ASan+UBSan)
cmake --build build                   # build bfc and bfi

cmake -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release           # release build, no sanitizers
```

After building, `bfc` and `bfi` are in the build directory.

```bash
# Compile a .b file to binary via LLVM IR
build/bfc test/res/helloworld.b > main.ll
clang main.ll -o main && ./main

# Interpret directly
build/bfi test/res/helloworld.b
```

`bfc` also reads from stdin when no file argument is given.

```bash
# Graph the CFG (raw dot; scripts/cfg.sh themes it — see README)
build/bfc --emit-cfg-dot --label-blocks test/res/fib.b | dot -Tpng -o cfg.png
```

`--emit-cfg-dot` makes `bfc` emit the control flow graph as Graphviz dot instead of LLVM IR (`--cfg-instructions` includes each block's instructions). `--label-blocks` appends each block's Brainfuck source span to its name; use it without `-O`, since `simplifycfg` merges and renames blocks.

## Common Build Targets

```bash
cmake --build build --target fmt          # clang-format (C) + shfmt/ruff (scripts)
cmake --build build --target fmt-ci       # check formatting (exits non-zero if dirty)
cmake --build build --target lint         # cpplint + clang analyzer + shellcheck/ruff
cmake --build build --target tests        # run all test suites
cmake --build build --target unittest     # unit tests only (requires check)
cmake --build build --target expecttest   # expect tests only (requires expect)
cmake --build build --target filecheck    # FileCheck tests only (requires LLVM FileCheck)
cmake --build build --target docs         # build Sphinx+Doxygen docs
```

## Code Architecture

The compiler pipeline flows through four modules in `src/`:

```
read.c  →  ir.c  →  llvm.c   (bfc path)
read.c  →  ir.c  →  interp.c (bfi path)
```

**`read.c/h`** — Input validation and normalisation. `clean_whitespace()` strips non-BF characters in-place. `read_file()` and `validate()` both return a tagged `struct ReadReturn` (discriminated union: `OK` with `char *program_str` or `ERROR` with `struct Error`). This module is formally verified for memory safety up to 13-command inputs via CBMC (see `MODELCHECKING.md`).

**`ir.c/h`** — Parsing into the internal IR. `string_to_program()` converts a cleaned source string into a `struct program` — a heap-allocated array of `struct cmd`. The IR compresses consecutive identical simple commands into a single entry with a `simple_count` field, and pre-computes matching bracket indices stored in `jump_index` (no runtime bracket matching needed during execution).

**`llvm.c/h`** — LLVM IR code generation. `generate()` takes a `struct program` and returns an `LLVMModuleRef`. Uses the LLVM-C API (`llvm-c/Core.h`).

**`cfg_dot.cpp/h`** — Control-flow-graph emission, behind `bfc --emit-cfg-dot`. `emit_cfg_dot()` writes the module's CFG as Graphviz dot via LLVM's C++ `WriteGraph`. The project's only C++ translation unit, so the devShell builds with the wrapped-clang stdenv (see `flake.nix`).

**`interp.c/h`** — Tree-walking interpreter. Execution state is held in `struct context_t` (program counter, data tape of `DATA_SIZE` = 65536 bytes, data pointer). `interp()` steps one command per call and returns 1 when the program completes.

**`common.h`** — Shared constant: `DATA_SIZE 65536`.

## Tests

Three independent test suites, all invoked via cmake:

| Suite | Tool | Files | What it tests |
|---|---|---|---|
| Unit tests | `check` framework | `test/test_ir.c`, `test/test_interp.c` | IR parsing, interpreter step logic |
| Expect tests | `expect` | `test/*.exp` | End-to-end `bfi` output |
| FileCheck tests | LLVM `FileCheck` | `test/*.filecheck` | `bfc` LLVM IR output structure |

FileCheck tests pipe `bfc <input.b>` output through the corresponding `.filecheck` file. `FileCheck` is on `PATH` automatically inside `nix develop`; for a manual macOS install, add LLVM tools to PATH: `export PATH="$PATH:$(brew --prefix)/opt/llvm/bin"`.

## Coding Standards

- **C17** (`gnu17` extensions enabled), `-Wall -Wextra -Werror`
- **Formatting**: clang-format LLVM style, `IndentWidth: 8` (see `.clang-format`)
- **Scripts**: shell is `shfmt -i 4 -ci` + shellcheck, Python is `ruff format` + `ruff check`; both wired into `fmt`/`fmt-ci`/`lint` by `cmake/scripts.cmake`
- **Linting**: cpplint at `linelength=80`; suppressed filters: `legal/copyright`, `build/include_subdir`, `build/header_guard`, `readability/braces`
- Debug builds automatically enable `-fsanitize=address,undefined`; on macOS suppress the spurious ASan warning with `MallocNanoZone=0`

## Formal Verification and Fuzzing

Neither of these is provided by the Nix devShell (`goto-cc` and AFL are outside its scope) — install `cbmc` separately, or use Docker for fuzzing.

- **CBMC** model checking targets `read.c` memory safety. Build the harness with `cmake --build build --target cbmc`, then run `./verification/cbmc_run.sh [MAX_PROGRAM_LEN]`. Start at length 4; memory exhaustion occurs at 14+.
- **AFL fuzzing** runs in Docker: `docker run -ti -v .:/src benmandrew/bf:fuzz`. Crashes land in `build-fuzz/fuzz_output/default/crashes/`.
