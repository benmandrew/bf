#!/bin/bash
#
# Render the control flow graph of a Brainfuck program.
#
# Pipeline: bfc --emit-cfg-dot -> highlight.py -> dot
#
# bfc emits the CFG dot itself (see src/cfg_dot.cpp), so there is no
# separate `opt` stage and no LLVM-version skew to guard against: the
# graph always comes from the same toolchain bfc was built with. bfc emits
# with heat colours off, so highlight.py is free to theme the graph;
# without profile data heat encodes nothing but would stamp inline colours
# that override dot's -N/-E/-G defaults.

# Paths are taken relative to the working directory, not the script, so
# there is deliberately no cd to the repository root here: it would
# silently reinterpret a caller's INPUT and OUTPUT arguments.
set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)

usage() {
    cat <<'EOF'
Usage: scripts/cfg.sh [-b BUILD_DIR] [-o OUTPUT] [-i] [-O] INPUT.b

  -b BUILD_DIR  Directory containing bfc (default: build)
  -o OUTPUT     Output image; format is taken from the extension,
                either .png or .svg (default: cfg.png)
  -i            Include each block's LLVM instructions in the graph
                (bfc --cfg-instructions)
  -O            Run bfc with -O. Off by default: simplifycfg merges and
                renames blocks, degrading the source-span labels.
  -h            Show this help message
EOF
}

BUILD_DIR=build
OUTPUT=cfg.png
OPTIMISE=()
INSTRUCTIONS=()

while getopts "b:o:iOh" opt; do
    case "${opt}" in
        b) BUILD_DIR="${OPTARG}" ;;
        o) OUTPUT="${OPTARG}" ;;
        i) INSTRUCTIONS=(--cfg-instructions) ;;
        O) OPTIMISE=(-O) ;;
        h)
            usage
            exit 0
            ;;
        *)
            usage >&2
            exit 1
            ;;
    esac
done
shift $((OPTIND - 1))

if [ $# -ne 1 ]; then
    usage >&2
    exit 1
fi

INPUT=$1

case "${OUTPUT}" in
    *.png) FORMAT=png ;;
    *.svg) FORMAT=svg ;;
    *)
        echo "cfg.sh: output must end in .png or .svg, got '${OUTPUT}'" >&2
        exit 1
        ;;
esac

BFC="${BUILD_DIR}/bfc"

for f in "${BFC}" "${INPUT}"; do
    if [ ! -f "${f}" ]; then
        echo "cfg.sh: no such file: ${f}" >&2
        exit 1
    fi
done

for tool in dot python3; do
    if ! command -v "${tool}" >/dev/null 2>&1; then
        echo "cfg.sh: ${tool} not on PATH; run inside 'nix develop'" >&2
        exit 1
    fi
done

HIGHLIGHT="${SCRIPT_DIR}/highlight.py"
if [ ! -f "${HIGHLIGHT}" ]; then
    echo "cfg.sh: no such file: ${HIGHLIGHT}" >&2
    exit 1
fi

WORK=$(mktemp -d)
trap 'rm -rf "${WORK}"' EXIT

DOT="${WORK}/cfg.dot"
# The [@]+ guards let empty arrays expand to nothing under `set -u`, which
# the macOS system bash (3.2) needs; a bare "${arr[@]}" errors there.
"${BFC}" --emit-cfg-dot ${INSTRUCTIONS[@]+"${INSTRUCTIONS[@]}"} \
    --label-blocks ${OPTIMISE[@]+"${OPTIMISE[@]}"} "${INPUT}" >"${DOT}"

# Theme the graph and syntax-highlight the IR. Doing this in Python
# rather than sed is what buys per-token colour: record labels cannot
# carry it, so highlight.py rewrites them into HTML-like table labels.
python3 "${SCRIPT_DIR}/highlight.py" <"${DOT}" >"${WORK}/themed.dot"

DOTFLAGS=()
if [ "${FORMAT}" = png ]; then
    DOTFLAGS=(-Gdpi=140)
fi

dot "-T${FORMAT}" ${DOTFLAGS[@]+"${DOTFLAGS[@]}"} "${WORK}/themed.dot" \
    -o "${OUTPUT}"

echo "cfg.sh: wrote ${OUTPUT}"
