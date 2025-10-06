# `bf` - Brainf*ck LLVM compiler

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
