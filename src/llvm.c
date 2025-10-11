#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "llvm.h"

struct llvm_function {
        LLVMValueRef func;
        LLVMTypeRef type;
};

#define JUMP_STACK_MAX_SIZE (128)

struct llvm_jump_stack {
        LLVMBasicBlockRef stack[JUMP_STACK_MAX_SIZE];
        size_t head;
};

static struct llvm_jump_stack jump_stack_new() {
        return (struct llvm_jump_stack){
            .head = 0,
        };
}

static void jump_stack_push(struct llvm_jump_stack *js, LLVMBasicBlockRef c) {
        assert(js->head < JUMP_STACK_MAX_SIZE - 1);
        js->stack[js->head] = c;
        js->head++;
}

static LLVMBasicBlockRef jump_stack_pop(struct llvm_jump_stack *js) {
        assert(js->head > 0);
        js->head--;
        return js->stack[js->head];
}

struct llvm_context {
        LLVMModuleRef module;
        LLVMBuilderRef builder;
        LLVMValueRef dp;
        LLVMValueRef data;
        struct llvm_jump_stack js;
        struct llvm_function putchar;
        struct llvm_function getchar;
};

void create_putchar_declaration(struct llvm_context *ctx) {
        ctx->putchar.type = LLVMFunctionType(
            LLVMInt32Type(), (LLVMTypeRef[]){LLVMInt32Type()}, 1, 0);
        ctx->putchar.func =
            LLVMAddFunction(ctx->module, "putchar", ctx->putchar.type);
}

void create_getchar_declaration(struct llvm_context *ctx) {
        ctx->getchar.type = LLVMFunctionType(LLVMInt32Type(), NULL, 0, 0);
        ctx->getchar.func =
            LLVMAddFunction(ctx->module, "getchar", ctx->getchar.type);
}

struct llvm_context create_module_preamble(const char *name) {
        struct llvm_context ctx;
        ctx.module = LLVMModuleCreateWithName(name);
        ctx.builder = LLVMCreateBuilder();
        create_putchar_declaration(&ctx);
        create_getchar_declaration(&ctx);
        ctx.dp = LLVMAddGlobal(ctx.module, LLVMInt32Type(), "dp");
        LLVMSetInitializer(ctx.dp, LLVMConstNull(LLVMInt32Type()));
        ctx.data = LLVMAddGlobal(
            ctx.module, LLVMArrayType(LLVMInt8Type(), DATA_SIZE), "data");
        LLVMSetInitializer(
            ctx.data, LLVMConstNull(LLVMArrayType(LLVMInt8Type(), DATA_SIZE)));
        ctx.js = jump_stack_new();
        return ctx;
}

void dispose_module(LLVMModuleRef module) { LLVMDisposeModule(module); }

void create_main_function(struct llvm_context *ctx) {
        LLVMTypeRef main_type = LLVMFunctionType(LLVMInt32Type(), NULL, 0, 0);
        LLVMValueRef main = LLVMAddFunction(ctx->module, "main", main_type);
        LLVMBasicBlockRef entry_block = LLVMAppendBasicBlock(main, "entry");
        LLVMPositionBuilderAtEnd(ctx->builder, entry_block);
        LLVMBasicBlockRef exit_block = LLVMAppendBasicBlock(main, "exit");
        jump_stack_push(&ctx->js, exit_block);
}

void finalise_main_function(struct llvm_context *ctx) {
        LLVMBasicBlockRef exit_block = jump_stack_pop(&ctx->js);
        LLVMBuildBr(ctx->builder, exit_block);
        LLVMPositionBuilderAtEnd(ctx->builder, exit_block);
        LLVMBuildRet(ctx->builder, LLVMConstInt(LLVMInt32Type(), 0, 0));
}

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
        LLVMBuildCall2(ctx->builder, ctx->putchar.type, ctx->putchar.func,
                       &extended_value, 1, "callputchar_tmp");
}

void comma(struct llvm_context *ctx) {
        LLVMValueRef data_ptr = index_data(ctx);
        LLVMValueRef getchar_result =
            LLVMBuildCall2(ctx->builder, ctx->getchar.type, ctx->getchar.func,
                           NULL, 0, "callgetchar_tmp");
        LLVMValueRef char_value = LLVMBuildTrunc(ctx->builder, getchar_result,
                                                 LLVMInt8Type(), "char_val");
        LLVMBuildStore(ctx->builder, char_value, data_ptr);
}

void left_bracket(struct llvm_context *ctx, LLVMBasicBlockRef loop_body,
                  LLVMBasicBlockRef after_loop) {
        LLVMValueRef data_ptr = index_data(ctx);
        LLVMValueRef current_value = LLVMBuildLoad2(
            ctx->builder, LLVMInt8Type(), data_ptr, "current_val");
        LLVMValueRef condition =
            LLVMBuildICmp(ctx->builder, LLVMIntNE, current_value,
                          LLVMConstInt(LLVMInt8Type(), 0, 0), "loopcond");
        LLVMBuildCondBr(ctx->builder, condition, loop_body, after_loop);
}

LLVMModuleRef generate(struct program *p) {
        struct llvm_context ctx = create_module_preamble("main");
        create_main_function(&ctx);
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
                case CMD_SIMPLE_INPUT:
                        for (size_t j = 0; j < c.simple_count; j++) {
                                comma(&ctx);
                        }
                        break;
                default:
                        fprintf(stderr, "Unsupported cmd_type '%c'\n", c.type);
                        exit(1);
                }
        }
        finalise_main_function(&ctx);
        LLVMDisposeBuilder(ctx.builder);
        return ctx.module;
}
