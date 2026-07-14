#ifndef LLVM_H
#define LLVM_H

#include <stdbool.h>

#include <llvm-c/Core.h>

#include "ir.h"

/// Generate LLVM IR for a parsed Brainfuck program.
/// @param program Parsed Brainfuck program.
/// @param optimise Run LLVM optimisation passes (mem2reg, instcombine, etc.).
/// @param label_blocks Append each basic block's Brainfuck source span to its
///                     name. Intended for CFG inspection; `optimise` merges and
///                     renames blocks, so the two are rarely useful together.
/// @return Generated LLVM module.
LLVMModuleRef generate(struct program *program, bool optimise,
                       bool label_blocks);

/// Release an LLVM module created by generate().
/// @param module LLVM module created by `generate`.
void dispose_module(LLVMModuleRef module);

#endif
