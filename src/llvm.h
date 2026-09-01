#ifndef LLVM_H
#define LLVM_H

#include <stdbool.h>

#include <llvm-c/Core.h>

#include "ir.h"

/// Generate LLVM IR for a parsed bf program.
/// @param program Parsed bf program.
/// @param optimise Run the LLVM `default<O2>` optimisation pipeline.
/// @param label_blocks Append each basic block's bf source span to its
///                     name. Intended for CFG inspection; `optimise` merges and
///                     renames blocks, so the two are rarely useful together.
/// @return Generated LLVM module.
LLVMModuleRef generate(struct program *program, bool optimise,
                       bool label_blocks);

/// Release an LLVM module created by generate().
/// @param module LLVM module created by `generate`.
void dispose_module(LLVMModuleRef module);

#endif
