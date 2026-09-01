#ifndef WASM_API_H
#define WASM_API_H

#include <stdbool.h>

/// String-returning entry points shared by the native `bfc` binary and the
/// WebAssembly build. Each takes raw bf source (non-command characters
/// are stripped internally), so a browser can pass a textarea's contents
/// straight in. The returned strings are heap-allocated and owned by the
/// caller; release them with `bf_free`.

/// Compile bf source to LLVM IR text.
/// @param source Raw bf source; cleaned internally.
/// @param optimise Run the LLVM optimisation pipeline.
/// @param label_blocks Append each block's bf source span to its name.
/// @return Heap-allocated NUL-terminated LLVM IR; free with `bf_free`.
char *bf_compile_ir(const char *source, bool optimise, bool label_blocks);

/// Compile bf source to the Graphviz dot of its control-flow graph.
/// @param source Raw bf source; cleaned internally.
/// @param optimise Run the LLVM optimisation pipeline before graphing.
/// @param label_blocks Append each block's bf source span to its name.
/// @param include_instructions Include each block's LLVM instructions in the
///        graph (dot-cfg vs dot-cfg-only).
/// @return Heap-allocated NUL-terminated dot string; free with `bf_free`.
char *bf_compile_cfg_dot(const char *source, bool optimise, bool label_blocks,
                         bool include_instructions);

/// Free a string returned by the bf_compile_* functions.
void bf_free(char *ptr);

#endif
