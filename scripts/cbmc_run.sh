#!/bin/bash

set -euo pipefail

cd "$(dirname "$0")/.." || exit 1

MAX_LEN=${1:-4} # Default to 4 if no argument provided

# Build with the specified MAX_PROGRAM_LEN
CFLAGS="-DMAX_PROGRAM_LEN=${MAX_LEN}" cmake --build build --target cbmc

CBMCFLAGS=(--unwind $((MAX_LEN + 1)) --no-malloc-may-fail --timestamp wall)

mkdir -p cbmc_outputs

echo "Running CBMC with unwind ${MAX_LEN}..."

nohup \
    time -p -o "cbmc_outputs/time_uw_${MAX_LEN}.log" \
    cbmc build/test/main_cbmc "${CBMCFLAGS[@]}" \
    >"cbmc_outputs/output_uw_${MAX_LEN}.log" 2>&1 &
