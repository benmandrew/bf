#ifndef LLVM_H
#define LLVM_H

#include <llvm-c/Core.h>

#include "ir.h"

LLVMModuleRef generate(struct program *p);
void dispose_module(LLVMModuleRef module);

#endif
