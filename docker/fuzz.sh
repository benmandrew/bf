#!/bin/bash

set -euo pipefail

cd /src || exit 1
CC=afl-clang-fast cmake -DCMAKE_BUILD_TYPE=Debug -B build-fuzz
cmake --build build-fuzz --target fuzz
