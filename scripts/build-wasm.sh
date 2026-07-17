#!/bin/bash
#
# Link bfc's compiler core to a WebAssembly module for the client-side web demo.
#
# The module exposes the same string-returning entry points the native binary
# uses (see src/wasm_api.c): bf_compile_ir and bf_compile_cfg_dot. The browser
# calls them directly, so the demo needs no server -- see PLAN.md.
#
# The LLVM libraries this links against come from build-wasm-deps.sh, which is
# run automatically if its prefix is missing. Only the linking here is cheap;
# the LLVM build is the slow, cached step.
#
# Env:
#   LLVM_WASM_PREFIX   LLVM wasm libs+headers (default build-wasm/llvm).
#   WASM_OUT           Output dir for bfc.mjs + bfc.wasm (default web/wasm).
set -euo pipefail

ROOT=$(cd "$(dirname "$0")/.." && pwd)
LLVM_WASM_PREFIX="${LLVM_WASM_PREFIX:-$ROOT/build-wasm/llvm}"
WASM_OUT="${WASM_OUT:-$ROOT/web/wasm}"

if ! command -v emcc >/dev/null 2>&1; then
    echo "error: emcc not found; run inside 'nix develop' (or activate an Emscripten SDK)" >&2
    exit 1
fi

export LLVM_WASM_PREFIX
"$ROOT/scripts/build-wasm-deps.sh"

mkdir -p "$WASM_OUT"

# The compiler core, minus the interpreter and the CLI entry point. cfg_dot.cpp
# is C++ and, like the LLVM libraries, is built with RTTI and exceptions off so
# its object matches them (the native build gets this from LLVM_DEFINITIONS).
sources=(
    "$ROOT/src/ir.c"
    "$ROOT/src/read.c"
    "$ROOT/src/llvm.c"
    "$ROOT/src/cfg_dot.cpp"
    "$ROOT/src/wasm_api.c"
)

echo "Linking $WASM_OUT/bfc.mjs"
# --start-group lets wasm-ld resolve the LLVM archives' mutual references
# without a hand-maintained link order. The exported functions are the
# wasm_api surface plus malloc/free so callers can own returned strings.
emcc -Oz -flto -fno-rtti -fno-exceptions \
    -sMODULARIZE=1 -sEXPORT_ES6=1 -sENVIRONMENT=node,web,worker \
    -sALLOW_MEMORY_GROWTH=1 -sGROWABLE_ARRAYBUFFERS=0 -sEXIT_RUNTIME=0 -sINVOKE_RUN=0 \
    -sEXPORTED_FUNCTIONS=_bf_compile_ir,_bf_compile_cfg_dot,_bf_free,_malloc,_free \
    -sEXPORTED_RUNTIME_METHODS=ccall,cwrap \
    -I "$ROOT/src" -I "$LLVM_WASM_PREFIX/include" \
    "${sources[@]}" \
    -Wl,--start-group "$LLVM_WASM_PREFIX"/lib/*.a -Wl,--end-group \
    -o "$WASM_OUT/bfc.mjs"

echo "Done: $(du -h "$WASM_OUT/bfc.wasm" | cut -f1) $WASM_OUT/bfc.wasm"
