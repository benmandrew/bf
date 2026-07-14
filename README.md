# Brainf*ck to LLVM IR Compiler Frontend

A compiler frontend for the [Brainf*ck language](https://en.wikipedia.org/wiki/Brainfuck) that outputs code in LLVM Intermediate Representation (IR), which can then be compiled to any desired target architecture with `clang`.

You can interact with it online [here](https://benmandrew.com/articles/compiler-frontend), or in a self-hosted web interface accessed at [`http://localhost:8080`](http://localhost:8080) after running

```bash
$ docker compose up
```

The input validation and parsing functionality is formally verified to be memory safe for inputs up to thirteen commands long. Details are in [MODELCHECKING.md](MODELCHECKING.md).

![alt](docs/screenshot.png)

## Dependencies

The project ships a [Nix flake](https://nixos.wiki/wiki/Flakes) with a devShell providing every tool the build needs — cmake, LLVM/clang, `check`, `expect`, `clang-format`, `cpplint`, Doxygen, Graphviz, Python/Sphinx, `shfmt`, and `shellcheck` — pinned via `flake.lock` for reproducibility. This is what CI uses, and is the recommended way to build locally:

```bash
$ nix develop
```

Every command in this README can be run unmodified inside that shell.

A `.envrc` is checked in, so with [direnv](https://direnv.net/) the shell loads on entering the directory — `direnv allow` once, and `nix develop` becomes unnecessary. Installing [nix-direnv](https://github.com/nix-community/nix-direnv) alongside it is worthwhile: it caches the shell so re-entry is instant and stops the garbage collector from reclaiming the dependencies.

<details>
<summary>Manual install (alternative to Nix)</summary>

#### Ubuntu/Debian
```bash
$ sudo apt-get install cmake llvm-dev check expect clang-format cpplint doxygen graphviz
```

#### MacOS (Homebrew)
```bash
$ brew install cmake llvm check expect clang-format cpplint doxygen graphviz
```

</details>

## Building

```bash
$ cmake -B build
$ cmake --build build
```

After building, the `bfc` (compiler) and `bfi` (interpreter) executables will be in the `build` directory.

To compile a `bf` program to a binary executable:

```bash
# Generate LLVM IR
$ bfc test/res/helloworld.b > main.ll
# Compile IR to binary
$ clang main.ll -o main
$ ./main
Hello, World!
```

To execute a `bf` program with the interpreter:

```bash
$ bfi test/res/helloworld.b
Hello, World!
```

### Visualising the Control Flow Graph

Basic blocks are named after the loop that creates them (`loop6.body`, `loop6.end`). Passing `--label-blocks` additionally appends the span of Brainf*ck source each block covers, which is enough to read a *control flow graph* (CFG) back against the original program:

```bash
$ bfc --label-blocks test/res/fib.b > fib.ll
$ opt -passes=dot-cfg-only -disable-output fib.ll   # writes .main.dot
$ dot -Tpng .main.dot -o fib_cfg.png
```

Both `opt` and `dot` are in the Nix devShell. Use `-passes=dot-cfg` instead of `dot-cfg-only` to include each block's instructions in the graph.

Two caveats. `opt` must match the LLVM version `bfc` links against, since older releases reject its opaque-pointer output. And `--label-blocks` is best left off `-O`: the `simplifycfg` pass merges and renames blocks, so the labels degrade and the graph no longer mirrors the source.

Loops that `optimise_program()` rewrites into `CMD_CLEAR` or `CMD_MULTIPLY` lower to straight-line code and so contribute no blocks. They appear inside a label as `[-]` and `[mul]` respectively. This rewriting is unconditional, which is why `helloworld.b` graphs as a single block.

### Formatting and Linting

```bash
$ cmake --build build --target fmt lint
```

### Documentation

Code docs can be accessed online at [benmandrew.com/docs/bf/](https://benmandrew.com/docs/bf/), or built locally with

```bash
$ cmake --build build --target docs
```

Depends on Doxygen and Sphinx, both provided by the Nix devShell. The generated HTML site is written to `build/docs/html/index.html`.

### Tests

```bash
$ cmake --build build --target tests
```

### Fuzzing

You can fuzz test with AFL:

```bash
$ docker run -ti -v .:/src benmandrew/bf:fuzz
```

If there are crashes, the offending inputs will be located in `build-fuzz/fuzz_output/default/crashes`.

## FAQ

> Why does ASan fail with `malloc: nano zone abandoned due to inability to reserve vm space.`?

On MacOS, every ASan-built binary prints this error. This is not an issue with the program, and can be fixed by setting the environment variable `MallocNanoZone=0`. See https://github.com/google/sanitizers/issues/1666.

## Useful Links for Learning the LLVM Intermediate Representation

Compiling to the LLVM IR is a niche topic, and it is hard to find resources for learning. Here are a few useful ones I found:

- *Mapping High Level Constructs to LLVM IR* ([link](https://mapping-high-level-constructs-to-llvm-ir.readthedocs.io/en/latest/))
- *A Complete Guide to LLVM for Programming Language Creators* ([link](https://mukulrathi.com/create-your-own-programming-language/llvm-ir-cpp-api-tutorial/))
- *My First Language Frontend with LLVM Tutorial* ([link](https://llvm.org/docs/tutorial/MyFirstLanguageFrontend/))
