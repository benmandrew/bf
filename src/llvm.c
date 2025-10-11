#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "llvm.h"

void add(LLVMValueRef dp, LLVMValueRef data, size_t value,
         LLVMBuilderRef builder) {
        LLVMValueRef dp_value =
            LLVMBuildLoad2(builder, LLVMInt32Type(), dp, "dptmp");
        LLVMValueRef indices[] = {LLVMConstInt(LLVMInt32Type(), 0, 0),
                                  dp_value};
        LLVMValueRef data_ptr =
            LLVMBuildGEP2(builder, LLVMArrayType(LLVMInt8Type(), DATA_SIZE),
                          data, indices, 2, "data_ptr");
        LLVMValueRef current_value =
            LLVMBuildLoad2(builder, LLVMInt8Type(), data_ptr, "current_val");
        LLVMValueRef new_value =
            LLVMBuildAdd(builder, current_value,
                         LLVMConstInt(LLVMInt8Type(), value, 0), "addtmp");
        LLVMBuildStore(builder, new_value, data_ptr);
}

void sub(LLVMValueRef dp, LLVMValueRef data, size_t value,
         LLVMBuilderRef builder) {
        LLVMValueRef dp_value =
            LLVMBuildLoad2(builder, LLVMInt32Type(), dp, "dptmp");
        LLVMValueRef indices[] = {LLVMConstInt(LLVMInt32Type(), 0, 0),
                                  dp_value};
        LLVMValueRef data_ptr =
            LLVMBuildGEP2(builder, LLVMArrayType(LLVMInt8Type(), DATA_SIZE),
                          data, indices, 2, "data_ptr");
        LLVMValueRef current_value =
            LLVMBuildLoad2(builder, LLVMInt8Type(), data_ptr, "current_val");
        LLVMValueRef new_value =
            LLVMBuildSub(builder, current_value,
                         LLVMConstInt(LLVMInt8Type(), value, 0), "subtmp");
        LLVMBuildStore(builder, new_value, data_ptr);
}

void right(LLVMValueRef dp, size_t value, LLVMBuilderRef builder) {
        LLVMValueRef dp_value =
            LLVMBuildLoad2(builder, LLVMInt32Type(), dp, "dptmp");
        LLVMValueRef new_dp =
            LLVMBuildAdd(builder, dp_value,
                         LLVMConstInt(LLVMInt32Type(), value, 0), "righttmp");
        LLVMBuildStore(builder, new_dp, dp);
}

void left(LLVMValueRef dp, size_t value, LLVMBuilderRef builder) {
        LLVMValueRef dp_value =
            LLVMBuildLoad2(builder, LLVMInt32Type(), dp, "dptmp");
        LLVMValueRef new_dp =
            LLVMBuildSub(builder, dp_value,
                         LLVMConstInt(LLVMInt32Type(), value, 0), "lefttmp");
        LLVMBuildStore(builder, new_dp, dp);
}

LLVMModuleRef create_module_preamble(const char *name) {
        LLVMModuleRef module = LLVMModuleCreateWithName(name);
        LLVMValueRef dp = LLVMAddGlobal(module, LLVMInt32Type(), "dp");
        LLVMSetInitializer(dp, LLVMConstNull(LLVMInt32Type()));
        LLVMValueRef data = LLVMAddGlobal(
            module, LLVMArrayType(LLVMInt8Type(), DATA_SIZE), "data");
        LLVMSetInitializer(
            data, LLVMConstNull(LLVMArrayType(LLVMInt8Type(), DATA_SIZE)));
        return module;
}

void dispose_module(LLVMModuleRef mod) { LLVMDisposeModule(mod); }

LLVMBuilderRef add_basic_block(LLVMValueRef func, const char *block_name) {
        LLVMBasicBlockRef block = LLVMAppendBasicBlock(func, block_name);
        if (!block) {
                fprintf(stderr, "Failed to create basic block '%s'\n",
                        block_name);
                exit(1);
        }
        LLVMBuilderRef builder = LLVMCreateBuilder();
        if (!builder) {
                fprintf(stderr, "Failed to create LLVM builder\n");
                exit(1);
        }
        LLVMPositionBuilderAtEnd(builder, block);
        return builder;
}

LLVMValueRef add_main_function(LLVMModuleRef mod) {
        LLVMTypeRef ret_type = LLVMInt32Type();
        LLVMTypeRef func_type = LLVMFunctionType(ret_type, NULL, 0, 0);
        LLVMValueRef main = LLVMAddFunction(mod, "main", func_type);
        if (!main) {
                fprintf(stderr, "Failed to add main function to module\n");
                exit(1);
        }
        return main;
}

LLVMModuleRef generate(struct program *p) {
        LLVMModuleRef module = create_module_preamble("main");
        LLVMValueRef main = add_main_function(module);
        LLVMBuilderRef builder = add_basic_block(main, "entry");
        LLVMValueRef dp = LLVMGetNamedGlobal(module, "dp");
        LLVMValueRef data = LLVMGetNamedGlobal(module, "data");
        for (size_t i = 0; i < p->length; i++) {
                struct cmd c = p->cmds[i];
                switch (c.type) {
                case CMD_SIMPLE_INC:
                        add(dp, data, c.simple_count, builder);
                        break;
                case CMD_SIMPLE_DEC:
                        sub(dp, data, c.simple_count, builder);
                        break;
                case CMD_SIMPLE_RIGHT:
                        right(dp, c.simple_count, builder);
                        break;
                case CMD_SIMPLE_LEFT:
                        left(dp, c.simple_count, builder);
                        break;
                default:
                        fprintf(stderr, "Unsupported cmd_type '%c'\n", c.type);
                        exit(1);
                }
        }
        LLVMBuildRet(builder, LLVMConstInt(LLVMInt32Type(), 0, 0));
        LLVMDisposeBuilder(builder);
        return module;
}
