# Brainf*ck to LLVM IR Compiler Frontend

A compiler frontend for the [Brainf*ck language](https://en.wikipedia.org/wiki/Brainfuck) that outputs code in LLVM Intermediate Representation (IR), which can then be compiled to any desired target architecture with `clang`.

You can interact with it online [here](https://benmandrew.com/articles/compiler-frontend), or in a self-hosted web interface accessed at [`http://localhost:8080`](http://localhost:8080) after running
```bash
$ docker compose up
```

![alt](doc/screenshot.png)

## Dependencies

#### Ubuntu/Debian
```bash
$ sudo apt-get install cmake llvm-dev check expect clang-format cpplint
```

#### MacOS (Homebrew)
```bash
$ brew install cmake llvm check expect clang-format cpplint
```

## Building

```bash
$ mkdir build
$ cd build
$ cmake ..
$ cmake --build .
```

## Running

After building, the `bfc` (compiler) and `bfi` (interpreter) executables will be in the `build` directory.

### Examples

Compiling to a binary executable:

```bash
# Generate LLVM IR
$ ./build/bfc test/res/helloworld.b > main.ll
# Compile IR to binary
$ clang main.ll -o main
$ ./main
Hello, World!
```

Executing with the interpreter:

```bash
$ ./build/bfi test/res/helloworld.b
Hello, World!
```

### Tests
```bash
$ cmake --build build --target tests
```

### Code Formatting

```bash
$ cmake --build build --target fmt
```

### Fuzzing

You can fuzz test with AFL

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
