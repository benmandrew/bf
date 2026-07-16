#ifndef CFG_DOT_H
#define CFG_DOT_H

#include <stdbool.h>

#include <llvm-c/Core.h>

#ifdef __cplusplus
extern "C" {
#endif

/// Render the control-flow graph of the module's defined functions as
/// Graphviz dot, for feeding into scripts/highlight.py and `dot`.
/// @param module LLVM module created by `generate`.
/// @param include_instructions Include each block's LLVM instructions in
///        its label. When true, reproduces
///        `opt -passes=dot-cfg -cfg-heat-colors=false`; when false, the
///        block-label-only `opt -passes=dot-cfg-only -cfg-heat-colors=false`.
/// @return Heap-allocated NUL-terminated dot string; free with `free`. The
///         caller owns it — this keeps the renderer free of stdio so it works
///         both natively and compiled to WebAssembly.
char *emit_cfg_dot(LLVMModuleRef module, bool include_instructions);

#ifdef __cplusplus
}
#endif

#endif
