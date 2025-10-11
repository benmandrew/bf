# Brainf*ck LLVM compiler

## Dependencies

- CMake
- LLVM development libraries
- Check framework (for unit tests)
- expect (for integration tests)
- clang-format (optional, for code formatting)

#### Ubuntu/Debian
```bash
sudo apt-get install cmake llvm-dev libcheck-dev expect clang-format
```

#### macOS (Homebrew)
```bash
brew install cmake llvm check expect clang-format
```

## Building

```bash
# Create build directory
mkdir build
cd build
# Configure (Release mode)
cmake ..
# Or configure with debug symbols
cmake -DCMAKE_BUILD_TYPE=Debug ..
# Build
cmake --build .
```

## Running

After building, the executable will be in the `build` directory:
```bash
./build/bf --help
./build/bf examples/hello.b
```

### Tests
```bash
make -C build test
```

### Code Formatting

```bash
make -C build fmt
```

## Useful Links for Learning the LLVM Intermediate Representation

Compiling to the LLVM IR is a niche topic, and it is hard to find resources for learning. Here are a few useful ones I found:

- https://mapping-high-level-constructs-to-llvm-ir.readthedocs.io/en/latest/
- https://mukulrathi.com/create-your-own-programming-language/llvm-ir-cpp-api-tutorial/
- https://llvm.org/docs/tutorial/MyFirstLanguageFrontend/
