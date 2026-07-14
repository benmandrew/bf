#ifndef CFG_DOT_H
#define CFG_DOT_H

#include <llvm-c/Core.h>

#ifdef __cplusplus
extern "C" {
#endif

/// Emit the control-flow graph of the module's defined functions as
/// Graphviz dot on stdout. Reproduces the output of
/// `opt -passes=dot-cfg-only -cfg-heat-colors=false`, so the result can be
/// fed straight into scripts/highlight.py and `dot`.
/// @param module LLVM module created by `generate`.
void emit_cfg_dot(LLVMModuleRef module);

#ifdef __cplusplus
}
#endif

#endif
