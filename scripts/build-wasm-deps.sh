#!/bin/bash
#
# Cross-compile the LLVM libraries bfc links against to WebAssembly.
#
# This is the slow, cacheable half of the wasm build: it downloads an LLVM
# release, configures it with the Emscripten toolchain, and builds only the
# component libraries bfc needs (core, support, irreader, passes, analysis)
# plus their transitive closure. bfc never emits machine code -- only textual
# IR and Graphviz dot -- so no target backends are built (LLVM_TARGETS_TO_BUILD
# empty), which is what keeps the result to tens of megabytes rather than the
# clang-in-browser range.
#
# The build is a native cross-compile: LLVM builds its own host llvm-tblgen
# (the NATIVE sub-build) unless LLVM_TABLEGEN points at one, so no host LLVM of
# a matching version is required. emcc/emcmake and ninja must be on PATH.
#
# Output is an install-style prefix ($LLVM_WASM_PREFIX) holding lib/*.a and a
# merged include/ tree, consumed by build-wasm.sh. It is keyed on nothing but
# LLVM_VERSION and these flags, so CI can cache it across runs and rebuild only
# on a version bump.
#
# Env:
#   LLVM_VERSION       LLVM release to build (default 22.1.8).
#   LLVM_WASM_PREFIX   Where to install libs+headers (default build-wasm/llvm).
#   WASM_WORK          Scratch dir for source and build (default build-wasm/work).
#   LLVM_TABLEGEN      Optional prebuilt host llvm-tblgen to skip building one.
set -euo pipefail

LLVM_VERSION="${LLVM_VERSION:-22.1.8}"
ROOT=$(cd "$(dirname "$0")/.." && pwd)
LLVM_WASM_PREFIX="${LLVM_WASM_PREFIX:-$ROOT/build-wasm/llvm}"
WASM_WORK="${WASM_WORK:-$ROOT/build-wasm/work}"

if ! command -v emcmake >/dev/null 2>&1; then
    echo "error: emcmake not found; run inside 'nix develop' (or activate an Emscripten SDK)" >&2
    exit 1
fi

# Idempotent: a populated prefix is a cache hit, nothing to do.
if [ -f "$LLVM_WASM_PREFIX/lib/libLLVMCore.a" ]; then
    echo "LLVM wasm libraries already present in $LLVM_WASM_PREFIX; skipping"
    exit 0
fi

mkdir -p "$WASM_WORK"
src_dir="$WASM_WORK/llvm-project-$LLVM_VERSION.src"
if [ ! -d "$src_dir/llvm" ]; then
    tarball="$WASM_WORK/llvm-$LLVM_VERSION.src.tar.xz"
    url="https://github.com/llvm/llvm-project/releases/download/llvmorg-$LLVM_VERSION/llvm-project-$LLVM_VERSION.src.tar.xz"
    echo "Downloading LLVM $LLVM_VERSION source"
    curl -fL --retry 3 -o "$tarball" "$url"
    # bfc links only the llvm subproject; skip clang/lld to save time and space.
    tar -C "$WASM_WORK" -xf "$tarball" \
        "llvm-project-$LLVM_VERSION.src/llvm" \
        "llvm-project-$LLVM_VERSION.src/cmake" \
        "llvm-project-$LLVM_VERSION.src/third-party"
fi

build_dir="$WASM_WORK/build"
tablegen_arg=()
if [ -n "${LLVM_TABLEGEN:-}" ]; then
    tablegen_arg=(-DLLVM_TABLEGEN="$LLVM_TABLEGEN")
fi

echo "Configuring LLVM wasm cross-build"
emcmake cmake -S "$src_dir/llvm" -B "$build_dir" -G Ninja \
    -DCMAKE_BUILD_TYPE=MinSizeRel \
    -DLLVM_TARGETS_TO_BUILD="" \
    -DLLVM_ENABLE_THREADS=OFF \
    -DLLVM_BUILD_TOOLS=OFF \
    -DLLVM_INCLUDE_TOOLS=OFF \
    -DLLVM_INCLUDE_UTILS=OFF \
    -DLLVM_INCLUDE_TESTS=OFF \
    -DLLVM_INCLUDE_EXAMPLES=OFF \
    -DLLVM_INCLUDE_BENCHMARKS=OFF \
    -DLLVM_ENABLE_ZLIB=OFF \
    -DLLVM_ENABLE_ZSTD=OFF \
    -DLLVM_ENABLE_LIBXML2=OFF \
    -DLLVM_ENABLE_LIBEDIT=OFF \
    -DLLVM_ENABLE_LIBPFM=OFF \
    -DLLVM_ENABLE_BACKTRACES=OFF \
    -DLLVM_ENABLE_CRASH_OVERRIDES=OFF \
    ${tablegen_arg[@]+"${tablegen_arg[@]}"}

echo "Building LLVM component libraries"
ninja -C "$build_dir" \
    LLVMCore LLVMSupport LLVMIRReader LLVMPasses LLVMAnalysis

echo "Assembling prefix $LLVM_WASM_PREFIX"
rm -rf "$LLVM_WASM_PREFIX"
mkdir -p "$LLVM_WASM_PREFIX/lib" "$LLVM_WASM_PREFIX/include"
cp "$build_dir"/lib/*.a "$LLVM_WASM_PREFIX/lib/"
# Merge the source headers with the build-tree generated ones (llvm/Config/*)
# into a single include dir so build-wasm.sh needs only one -I.
cp -R "$src_dir/llvm/include/." "$LLVM_WASM_PREFIX/include/"
cp -R "$build_dir/include/." "$LLVM_WASM_PREFIX/include/"

lib_count=$(find "$LLVM_WASM_PREFIX/lib" -name '*.a' | wc -l | tr -d ' ')
echo "Done: $lib_count libraries"
