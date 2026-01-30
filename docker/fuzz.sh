#!/bin/bash

set -euo pipefail

cd /src || exit 1
MallocNanoZone=0 CC=afl-clang-fast cmake -DCMAKE_BUILD_TYPE=Debug -B build-fuzz
MallocNanoZone=0 cmake --build build-fuzz --target fuzz
