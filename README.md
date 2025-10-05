# Brainfuck Interpreter with LLVM Support

This project has been converted from Make to CMake to support LLVM IR builder functionality.

## Dependencies

- CMake
- LLVM development libraries
- Check framework (for unit tests)
- expect (for integration tests)
- clang-format (optional, for code formatting)

### Installing Dependencies

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

## Testing

### Tests
```bash
make -C build test
```

### Code Formatting

```bash
make -C build fmt
```

## LLVM Integration

The CMake build system automatically finds and links LLVM libraries. The following LLVM components are included:
- `support` - Basic LLVM support utilities
- `core` - LLVM core IR functionality
- `irreader` - LLVM IR reading capabilities

You can extend this by modifying the `llvm_map_components_to_libnames` call in CMakeLists.txt.
