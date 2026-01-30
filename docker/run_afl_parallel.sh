#!/bin/bash

set -euo pipefail

CORES=$(nproc)

SYNC_DIR="fuzz_output"
INPUT_DIR="test/fuzz"
TARGET="${1:-${CMAKE_BINARY_DIR}/bfc_fuzz}"

export MallocNanoZone=0
export UBSAN_OPTIONS=print_stacktrace=1

mkdir -p "$SYNC_DIR"

afl-fuzz -i "$INPUT_DIR" -o "$SYNC_DIR" -M fuzzer01 "$TARGET" @@ >/dev/null 2>&1 &

for i in $(seq 2 "$CORES"); do
    fuzzer=$(printf "fuzzer%02d" "$i")
    afl-fuzz -i "$INPUT_DIR" -o "$SYNC_DIR" -S "$fuzzer" "$TARGET" @@ >/dev/null 2>&1 &
done

watch -n 0.5 -- afl-whatsup -s "$SYNC_DIR"

wait
