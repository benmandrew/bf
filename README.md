# Brainf*ck LLVM Compiler Frontend

A compiler frontend for the [Brainf*ck language](https://en.wikipedia.org/wiki/Brainfuck) that outputs code in LLVM Intermediate Representation (IR), which can then be compiled to any desired target architecture with `clang`.

## Dependencies

- CMake
- LLVM development libraries
- Check framework (for unit tests)
- expect (for integration tests)
- clang-format (optional, for code formatting)

#### Ubuntu/Debian
```bash
$ sudo apt-get install cmake llvm-dev libcheck-dev expect clang-format
```

#### macOS (Homebrew)
```bash
$ brew install cmake llvm check expect clang-format
```

## Building

```bash
# Create build directory
$ mkdir build
$ cd build
# Configure (Release mode)
$ cmake ..
# Or configure with debug symbols
$ cmake -DCMAKE_BUILD_TYPE=Debug ..
# Build
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
$ cmake --build build --target test
```

### Code Formatting

```bash
$ cmake --build build --target fmt
```

## Useful Links for Learning the LLVM Intermediate Representation

Compiling to the LLVM IR is a niche topic, and it is hard to find resources for learning. Here are a few useful ones I found:

- https://mapping-high-level-constructs-to-llvm-ir.readthedocs.io/en/latest/
- https://mukulrathi.com/create-your-own-programming-language/llvm-ir-cpp-api-tutorial/
- https://llvm.org/docs/tutorial/MyFirstLanguageFrontend/
