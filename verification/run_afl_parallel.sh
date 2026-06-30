#!/bin/bash

set -euo pipefail

CORES=$(nproc)

SYNC_DIR="fuzz_output"
INPUT_DIR="test/fuzz"
DICT="test/fuzz/bf.dict"
TARGET="${1:-${CMAKE_BINARY_DIR}/bfc_fuzz}"

export MallocNanoZone=0
export UBSAN_OPTIONS=print_stacktrace=1
export AFL_CUSTOM_MUTATOR_LIBRARY="$(dirname "$TARGET")/bf_mutator.so"

mkdir -p "$SYNC_DIR"

CMPLOG_TARGET="${TARGET}_cmplog"

# No @@ — harness reads from stdin, not a file argument
# -c passes the CmpLog binary to the main fuzzer only; secondaries don't need it
if [ -f "$CMPLOG_TARGET" ]; then
    afl-fuzz -i "$INPUT_DIR" -o "$SYNC_DIR" -x "$DICT" -c "$CMPLOG_TARGET" -M fuzzer01 "$TARGET" >/dev/null 2>&1 &
else
    afl-fuzz -i "$INPUT_DIR" -o "$SYNC_DIR" -x "$DICT" -M fuzzer01 "$TARGET" >/dev/null 2>&1 &
fi

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
