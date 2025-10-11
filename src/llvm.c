#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "llvm.h"

struct llvm_context {
        LLVMModuleRef module;
        LLVMBuilderRef builder;
        LLVMValueRef dp;
        LLVMValueRef data;
        LLVMValueRef putchar_func;
        LLVMTypeRef putchar_func_type;
};

LLVMValueRef index_data(struct llvm_context *ctx) {
        LLVMValueRef dp_value =
            LLVMBuildLoad2(ctx->builder, LLVMInt32Type(), ctx->dp, "dptmp");
        LLVMValueRef indices[] = {LLVMConstInt(LLVMInt32Type(), 0, 0),
                                  dp_value};
        LLVMValueRef data_ptr = LLVMBuildGEP2(
            ctx->builder, LLVMArrayType(LLVMInt8Type(), DATA_SIZE), ctx->data,
            indices, 2, "data_ptr");
        return data_ptr;
}

void add(struct llvm_context *ctx, size_t value) {
        LLVMValueRef data_ptr = index_data(ctx);
        LLVMValueRef current_value = LLVMBuildLoad2(
            ctx->builder, LLVMInt8Type(), data_ptr, "current_val");
        LLVMValueRef new_value =
            LLVMBuildAdd(ctx->builder, current_value,
                         LLVMConstInt(LLVMInt8Type(), value, 0), "addtmp");
        LLVMBuildStore(ctx->builder, new_value, data_ptr);
}

void sub(struct llvm_context *ctx, size_t value) {
        LLVMValueRef data_ptr = index_data(ctx);
        LLVMValueRef current_value = LLVMBuildLoad2(
            ctx->builder, LLVMInt8Type(), data_ptr, "current_val");
        LLVMValueRef new_value =
            LLVMBuildSub(ctx->builder, current_value,
                         LLVMConstInt(LLVMInt8Type(), value, 0), "subtmp");
        LLVMBuildStore(ctx->builder, new_value, data_ptr);
}

void right(struct llvm_context *ctx, size_t value) {
        LLVMValueRef dp_value =
            LLVMBuildLoad2(ctx->builder, LLVMInt32Type(), ctx->dp, "dptmp");
        LLVMValueRef new_dp =
            LLVMBuildAdd(ctx->builder, dp_value,
                         LLVMConstInt(LLVMInt32Type(), value, 0), "righttmp");
        LLVMBuildStore(ctx->builder, new_dp, ctx->dp);
}

void left(struct llvm_context *ctx, size_t value) {
        LLVMValueRef dp_value =
            LLVMBuildLoad2(ctx->builder, LLVMInt32Type(), ctx->dp, "dptmp");
        LLVMValueRef new_dp =
            LLVMBuildSub(ctx->builder, dp_value,
                         LLVMConstInt(LLVMInt32Type(), value, 0), "lefttmp");
        LLVMBuildStore(ctx->builder, new_dp, ctx->dp);
}

void dot(struct llvm_context *ctx) {
        LLVMValueRef data_ptr = index_data(ctx);
        LLVMValueRef current_value = LLVMBuildLoad2(
            ctx->builder, LLVMInt8Type(), data_ptr, "current_val");
        LLVMValueRef extended_value = LLVMBuildZExt(
            ctx->builder, current_value, LLVMInt32Type(), "extended_val");
        LLVMBuildCall2(ctx->builder, ctx->putchar_func_type, ctx->putchar_func,
                       &extended_value, 1, "calltmp");
}

struct llvm_context create_module_preamble(const char *name) {
        struct llvm_context ctx;
        ctx.module = LLVMModuleCreateWithName(name);
        ctx.putchar_func_type = LLVMFunctionType(
            LLVMInt32Type(), (LLVMTypeRef[]){LLVMInt32Type()}, 1, 0);
        ctx.putchar_func =
            LLVMAddFunction(ctx.module, "putchar", ctx.putchar_func_type);
        ctx.dp = LLVMAddGlobal(ctx.module, LLVMInt32Type(), "dp");
        LLVMSetInitializer(ctx.dp, LLVMConstNull(LLVMInt32Type()));
        ctx.data = LLVMAddGlobal(
            ctx.module, LLVMArrayType(LLVMInt8Type(), DATA_SIZE), "data");
        LLVMSetInitializer(
            ctx.data, LLVMConstNull(LLVMArrayType(LLVMInt8Type(), DATA_SIZE)));
        return ctx;
}

void dispose_module(LLVMModuleRef module) { LLVMDisposeModule(module); }

LLVMBuilderRef add_basic_block(LLVMValueRef func, const char *block_name) {
        LLVMBasicBlockRef block = LLVMAppendBasicBlock(func, block_name);
        LLVMBuilderRef builder = LLVMCreateBuilder();
        LLVMPositionBuilderAtEnd(builder, block);
        return builder;
}

LLVMValueRef add_main_function(LLVMModuleRef mod) {
        LLVMTypeRef func_type = LLVMFunctionType(LLVMInt32Type(), NULL, 0, 0);
        return LLVMAddFunction(mod, "main", func_type);
}

LLVMModuleRef generate(struct program *p) {
        struct llvm_context ctx = create_module_preamble("main");
        LLVMValueRef main = add_main_function(ctx.module);
        ctx.builder = add_basic_block(main, "entry");
        for (size_t i = 0; i < p->length; i++) {
                struct cmd c = p->cmds[i];
                switch (c.type) {
                case CMD_SIMPLE_INC:
                        add(&ctx, c.simple_count);
                        break;
                case CMD_SIMPLE_DEC:
                        sub(&ctx, c.simple_count);
                        break;
                case CMD_SIMPLE_RIGHT:
                        right(&ctx, c.simple_count);
                        break;
                case CMD_SIMPLE_LEFT:
                        left(&ctx, c.simple_count);
                        break;
                case CMD_SIMPLE_OUTPUT:
                        for (size_t j = 0; j < c.simple_count; j++) {
                                dot(&ctx);
                        }
                        break;
                default:
                        fprintf(stderr, "Unsupported cmd_type '%c'\n", c.type);
                        exit(1);
                }
        }
        LLVMBuildRet(ctx.builder, LLVMConstInt(LLVMInt32Type(), 0, 0));
        LLVMDisposeBuilder(ctx.builder);
        return ctx.module;
}
