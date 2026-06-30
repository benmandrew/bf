#!/bin/bash

set -euo pipefail

CORES=$(nproc)

SYNC_DIR="fuzz_output"
INPUT_DIR="test/fuzz"
DICT="test/fuzz/bf.dict"
TARGET="${1:-${CMAKE_BINARY_DIR}/bfc_fuzz}"

export MallocNanoZone=0
export UBSAN_OPTIONS=print_stacktrace=1

mkdir -p "$SYNC_DIR"

# No @@ — harness reads from stdin, not a file argument
afl-fuzz -i "$INPUT_DIR" -o "$SYNC_DIR" -x "$DICT" -M fuzzer01 "$TARGET" >/dev/null 2>&1 &

for i in $(seq 2 "$CORES"); do
    fuzzer=$(printf "fuzzer%02d" "$i")
    afl-fuzz -i "$INPUT_DIR" -o "$SYNC_DIR" -x "$DICT" -S "$fuzzer" "$TARGET" >/dev/null 2>&1 &
done

# Print stats every 30s without requiring a TTY (watch needs one)
while true; do
    sleep 30
    echo "=== $(date) ==="
    afl-whatsup -s "$SYNC_DIR" || true
done
