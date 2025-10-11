#ifndef LLVM_H
#define LLVM_H

#include "ir.h"

#include <llvm-c/Core.h>

LLVMModuleRef generate(struct program *p);
void dispose_module(LLVMModuleRef module);

#endif
