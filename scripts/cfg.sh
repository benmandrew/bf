#!/bin/bash
#
# Render the control flow graph of a Brainfuck program.
#
# Pipeline: bfc --label-blocks -> opt -passes=dot-cfg[-only] -> dot
#
# LLVM's CFG printer enables heat colours by default, which stamp a
# per-node colour, fillcolor and fontname inline on every node. Those
# inline attributes take precedence over dot's -N/-E/-G defaults, so the
# graph cannot be themed while they are on. They also encode nothing
# here: heat is derived from block frequency, and without PGO profile
# data every block falls back to the same default weight. Hence
# -cfg-heat-colors=false; highlight.py then does the styling.

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
                (-passes=dot-cfg rather than dot-cfg-only)
  -O            Run bfc with -O. Off by default: simplifycfg merges and
                renames blocks, degrading the source-span labels.
  -h            Show this help message
EOF
}

BUILD_DIR=build
OUTPUT=cfg.png
OPTIMISE=()
PASS=dot-cfg-only

while getopts "b:o:iOh" opt; do
    case "${opt}" in
        b) BUILD_DIR="${OPTARG}" ;;
        o) OUTPUT="${OPTARG}" ;;
        i) PASS=dot-cfg ;;
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

for tool in opt dot python3; do
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

# bfc statically links LLVM, so its version is not visible via ldd. Take
# it from the LLVMConfig.cmake that cmake resolved at configure time.
CACHE="${BUILD_DIR}/CMakeCache.txt"
if [ ! -f "${CACHE}" ]; then
    echo "cfg.sh: no CMakeCache.txt in ${BUILD_DIR}; configure it first" >&2
    exit 1
fi

LLVM_DIR=$(sed -n 's/^LLVM_DIR:PATH=//p' "${CACHE}")
if [ -z "${LLVM_DIR}" ] || [ ! -f "${LLVM_DIR}/LLVMConfig.cmake" ]; then
    echo "cfg.sh: cannot resolve LLVM_DIR from ${CACHE}" >&2
    exit 1
fi

BFC_MAJOR=$(sed -n 's/^set(LLVM_VERSION_MAJOR \([0-9]*\)).*/\1/p' \
    "${LLVM_DIR}/LLVMConfig.cmake")
OPT_MAJOR=$(opt --version | sed -n 's/.*LLVM version \([0-9]*\)\..*/\1/p')

# A mismatch is not always a hard error: an older opt may parse bfc's
# output with only a warning and still emit a plausible-looking graph, so
# fail loudly rather than let a silently wrong CFG through.
if [ "${BFC_MAJOR}" != "${OPT_MAJOR}" ]; then
    cat >&2 <<EOF
cfg.sh: LLVM version mismatch.
  bfc was built against LLVM ${BFC_MAJOR} (${LLVM_DIR})
  opt on PATH is LLVM ${OPT_MAJOR} ($(command -v opt))
A mismatched opt may reject bfc's output, or quietly emit a misleading
graph. Run inside 'nix develop' so both come from the same toolchain.
EOF
    exit 1
fi

WORK=$(mktemp -d)
trap 'rm -rf "${WORK}"' EXIT

"${BFC}" --label-blocks "${OPTIMISE[@]}" "${INPUT}" >"${WORK}/cfg.ll"

# opt writes .<function>.dot into the working directory. bfc emits a
# single function, main, so the name is fixed. opt announces that write
# on stderr; drop the announcement but keep anything else it says.
if ! (cd "${WORK}" && opt "-passes=${PASS}" -cfg-heat-colors=false \
    -disable-output cfg.ll >/dev/null 2>"${WORK}/opt.err"); then
    cat "${WORK}/opt.err" >&2
    exit 1
fi
grep -v "^Writing '\..*\.dot'\.\.\.$" "${WORK}/opt.err" >&2 || true

DOT="${WORK}/.main.dot"
if [ ! -f "${DOT}" ]; then
    echo "cfg.sh: opt did not produce ${DOT}" >&2
    exit 1
fi

# Theme the graph and syntax-highlight the IR. Doing this in Python
# rather than sed is what buys per-token colour: record labels cannot
# carry it, so highlight.py rewrites them into HTML-like table labels.
python3 "${SCRIPT_DIR}/highlight.py" <"${DOT}" >"${WORK}/themed.dot"

DOTFLAGS=()
if [ "${FORMAT}" = png ]; then
    DOTFLAGS=(-Gdpi=140)
fi

dot "-T${FORMAT}" "${DOTFLAGS[@]}" "${WORK}/themed.dot" -o "${OUTPUT}"

echo "cfg.sh: wrote ${OUTPUT}"
