#ifndef LLVM_H
#define LLVM_H

#include <llvm-c/Core.h>

#include "ir.h"

/** Generate LLVM IR for a parsed Brainfuck program. */
LLVMModuleRef generate(struct program *p);
/** Release an LLVM module created by generate(). */
void dispose_module(LLVMModuleRef module);

#endif
