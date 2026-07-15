#!/bin/bash
#
# Build the wasm module and check it against native bfc for every test program.
#
# The native binary is the oracle; scripts/wasm_parity.mjs diffs both exported
# entry points against it. This is the CI gate that keeps the browser compiler
# honest against the Nix-built one.
#
# Env:
#   BFC        Native bfc binary (default build/bfc).
#   WASM_OUT   Output dir holding bfc.mjs (default web/wasm).
set -euo pipefail

ROOT=$(cd "$(dirname "$0")/.." && pwd)
BFC="${BFC:-$ROOT/build/bfc}"
WASM_OUT="${WASM_OUT:-$ROOT/web/wasm}"

if [ ! -x "$BFC" ]; then
    echo "error: native bfc not found at $BFC; build it first" >&2
    exit 1
fi

export WASM_OUT
"$ROOT/scripts/build-wasm.sh"

node "$ROOT/scripts/wasm_parity.mjs" "$BFC" "$ROOT/test/res" "$WASM_OUT/bfc.mjs"
