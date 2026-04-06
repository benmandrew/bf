#include "llvm.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "ir.h"

struct llvm_function {
        LLVMValueRef func;
        LLVMTypeRef type;
};

#define JUMP_STACK_MAX_SIZE (128)

struct entry_exit_pair {
        LLVMBasicBlockRef entry;
        LLVMBasicBlockRef exit;
};

struct llvm_jump_stack {
        struct entry_exit_pair stack[JUMP_STACK_MAX_SIZE];
        size_t head;
};

static struct llvm_jump_stack jump_stack_new() {
        return (struct llvm_jump_stack){
            .head = 0,
        };
}

static void jump_stack_push(struct llvm_jump_stack *jump_stack,
                            LLVMBasicBlockRef entry_block,
                            LLVMBasicBlockRef exit_block) {
        assert(jump_stack->head < JUMP_STACK_MAX_SIZE - 1);
        jump_stack->stack[jump_stack->head].entry = entry_block;
        jump_stack->stack[jump_stack->head].exit = exit_block;
        jump_stack->head++;
}

static struct entry_exit_pair
jump_stack_pop(struct llvm_jump_stack *jump_stack) {
        assert(jump_stack->head > 0);
        jump_stack->head--;
        return jump_stack->stack[jump_stack->head];
}

struct llvm_context {
        LLVMContextRef context;
        LLVMModuleRef module;
        LLVMBuilderRef builder;
        LLVMValueRef main;
        LLVMValueRef dp;
        LLVMValueRef data;
        struct llvm_jump_stack js;
        struct llvm_function putchar;
        struct llvm_function getchar;
};

static LLVMTypeRef int32_type(struct llvm_context *ctx) {
        return LLVMInt32TypeInContext(ctx->context);
}

static LLVMTypeRef int8_type(struct llvm_context *ctx) {
        return LLVMInt8TypeInContext(ctx->context);
}

static LLVMTypeRef data_array_type(struct llvm_context *ctx) {
        return LLVMArrayType(int8_type(ctx), DATA_SIZE);
}

void create_putchar_declaration(struct llvm_context *ctx) {
        LLVMTypeRef i32 = int32_type(ctx);
        ctx->putchar.type = LLVMFunctionType(i32, (LLVMTypeRef[]){i32}, 1, 0);
        ctx->putchar.func =
            LLVMAddFunction(ctx->module, "putchar", ctx->putchar.type);
}

void create_getchar_declaration(struct llvm_context *ctx) {
        ctx->getchar.type = LLVMFunctionType(int32_type(ctx), NULL, 0, 0);
        ctx->getchar.func =
            LLVMAddFunction(ctx->module, "getchar", ctx->getchar.type);
}

struct llvm_context create_module_preamble(struct program *program,
                                           const char *name) {
        struct llvm_context ctx;
        ctx.context = LLVMContextCreate();
        ctx.module = LLVMModuleCreateWithNameInContext(name, ctx.context);
        ctx.builder = LLVMCreateBuilderInContext(ctx.context);
        if (program_contains_output(program)) {
                create_putchar_declaration(&ctx);
        }
        if (program_contains_input(program)) {
                create_getchar_declaration(&ctx);
        }
        ctx.dp = LLVMAddGlobal(ctx.module, int32_type(&ctx), "dp");
        LLVMSetInitializer(ctx.dp, LLVMConstNull(int32_type(&ctx)));
        ctx.data = LLVMAddGlobal(ctx.module, data_array_type(&ctx), "data");
        LLVMSetInitializer(ctx.data, LLVMConstNull(data_array_type(&ctx)));
        ctx.js = jump_stack_new();
        return ctx;
}

void dispose_module(LLVMModuleRef module) {
        LLVMContextRef context = LLVMGetModuleContext(module);
        LLVMDisposeModule(module);
        LLVMContextDispose(context);
}

void create_main_function(struct llvm_context *ctx) {
        LLVMTypeRef main_type = LLVMFunctionType(int32_type(ctx), NULL, 0, 0);
        ctx->main = LLVMAddFunction(ctx->module, "main", main_type);
        LLVMBasicBlockRef entry_block =
            LLVMAppendBasicBlockInContext(ctx->context, ctx->main, "entry");
        LLVMPositionBuilderAtEnd(ctx->builder, entry_block);
}

LLVMValueRef get_dataptr(struct llvm_context *ctx) {
        LLVMValueRef dp_value =
            LLVMBuildLoad2(ctx->builder, int32_type(ctx), ctx->dp, "dptmp");
        LLVMValueRef indices[] = {LLVMConstInt(int32_type(ctx), 0, 0),
                                  dp_value};
        LLVMValueRef data_ptr =
            LLVMBuildGEP2(ctx->builder, data_array_type(ctx), ctx->data,
                          indices, 2, "data_ptr");
        return data_ptr;
}

void add(struct llvm_context *ctx, size_t value) {
        LLVMValueRef data_ptr = get_dataptr(ctx);
        LLVMValueRef current_value = LLVMBuildLoad2(
            ctx->builder, int8_type(ctx), data_ptr, "current_val");
        LLVMValueRef new_value =
            LLVMBuildAdd(ctx->builder, current_value,
                         LLVMConstInt(int8_type(ctx), value, 0), "addtmp");
        LLVMBuildStore(ctx->builder, new_value, data_ptr);
}

void sub(struct llvm_context *ctx, size_t value) {
        LLVMValueRef data_ptr = get_dataptr(ctx);
        LLVMValueRef current_value = LLVMBuildLoad2(
            ctx->builder, int8_type(ctx), data_ptr, "current_val");
        LLVMValueRef new_value =
            LLVMBuildSub(ctx->builder, current_value,
                         LLVMConstInt(int8_type(ctx), value, 0), "subtmp");
        LLVMBuildStore(ctx->builder, new_value, data_ptr);
}

void right(struct llvm_context *ctx, size_t value) {
        LLVMValueRef dp_value =
            LLVMBuildLoad2(ctx->builder, int32_type(ctx), ctx->dp, "dptmp");
        LLVMValueRef new_dp =
            LLVMBuildAdd(ctx->builder, dp_value,
                         LLVMConstInt(int32_type(ctx), value, 0), "righttmp");
        LLVMBuildStore(ctx->builder, new_dp, ctx->dp);
}

void left(struct llvm_context *ctx, size_t value) {
        LLVMValueRef dp_value =
            LLVMBuildLoad2(ctx->builder, int32_type(ctx), ctx->dp, "dptmp");
        LLVMValueRef new_dp =
            LLVMBuildSub(ctx->builder, dp_value,
                         LLVMConstInt(int32_type(ctx), value, 0), "lefttmp");
        LLVMBuildStore(ctx->builder, new_dp, ctx->dp);
}

void dot(struct llvm_context *ctx) {
        LLVMValueRef data_ptr = get_dataptr(ctx);
        LLVMValueRef current_value = LLVMBuildLoad2(
            ctx->builder, int8_type(ctx), data_ptr, "current_val");
        LLVMValueRef extended_value = LLVMBuildZExt(
            ctx->builder, current_value, int32_type(ctx), "extended_val");
        LLVMBuildCall2(ctx->builder, ctx->putchar.type, ctx->putchar.func,
                       &extended_value, 1, "callputchar_tmp");
}

void comma(struct llvm_context *ctx) {
        LLVMValueRef data_ptr = get_dataptr(ctx);
        LLVMValueRef getchar_result =
            LLVMBuildCall2(ctx->builder, ctx->getchar.type, ctx->getchar.func,
                           NULL, 0, "callgetchar_tmp");
        LLVMValueRef char_value = LLVMBuildTrunc(ctx->builder, getchar_result,
                                                 int8_type(ctx), "char_val");
        LLVMBuildStore(ctx->builder, char_value, data_ptr);
}

void left_bracket(struct llvm_context *ctx) {
        LLVMValueRef data_ptr = get_dataptr(ctx);
        LLVMValueRef current_value = LLVMBuildLoad2(
            ctx->builder, int8_type(ctx), data_ptr, "current_val");
        LLVMValueRef condition =
            LLVMBuildICmp(ctx->builder, LLVMIntNE, current_value,
                          LLVMConstInt(int8_type(ctx), 0, 0), "loopcond");
        LLVMBasicBlockRef entry =
            LLVMAppendBasicBlockInContext(ctx->context, ctx->main, "entry");
        LLVMBasicBlockRef exit =
            LLVMAppendBasicBlockInContext(ctx->context, ctx->main, "exit");
        jump_stack_push(&ctx->js, entry, exit);
        LLVMBuildCondBr(ctx->builder, condition, entry, exit);
        LLVMPositionBuilderAtEnd(ctx->builder, entry);
}

void right_bracket(struct llvm_context *ctx) {
        struct entry_exit_pair pair = jump_stack_pop(&ctx->js);
        LLVMValueRef data_ptr = get_dataptr(ctx);
        LLVMValueRef current_value = LLVMBuildLoad2(
            ctx->builder, int8_type(ctx), data_ptr, "current_val");
        LLVMValueRef condition =
            LLVMBuildICmp(ctx->builder, LLVMIntNE, current_value,
                          LLVMConstInt(int8_type(ctx), 0, 0), "loopcond");
        LLVMBuildCondBr(ctx->builder, condition, pair.entry, pair.exit);
        LLVMPositionBuilderAtEnd(ctx->builder, pair.exit);
}

LLVMModuleRef generate(struct program *program) {
        struct llvm_context ctx = create_module_preamble(program, "main");
        create_main_function(&ctx);
        for (size_t cmd_index = 0; cmd_index < program->length; cmd_index++) {
                struct cmd command = program->cmds[cmd_index];
                switch (command.type) {
                case CMD_SIMPLE_INC:
                        add(&ctx, command.value.simple_count);
                        break;
                case CMD_SIMPLE_DEC:
                        sub(&ctx, command.value.simple_count);
                        break;
                case CMD_SIMPLE_RIGHT:
                        right(&ctx, command.value.simple_count);
                        break;
                case CMD_SIMPLE_LEFT:
                        left(&ctx, command.value.simple_count);
                        break;
                case CMD_SIMPLE_OUTPUT:
                        for (size_t output_index = 0;
                             output_index < command.value.simple_count;
                             output_index++) {
                                dot(&ctx);
                        }
                        break;
                case CMD_SIMPLE_INPUT:
                        for (size_t input_index = 0;
                             input_index < command.value.simple_count;
                             input_index++) {
                                comma(&ctx);
                        }
                        break;
                case CMD_JUMP_FORWARD:
                        left_bracket(&ctx);
                        break;
                case CMD_JUMP_BACK:
                        right_bracket(&ctx);
                        break;
                default:
                        fprintf(stderr, "Unsupported cmd_type '%c'\n",
                                command.type);
                        exit(1);
                }
        }
        LLVMBuildRet(ctx.builder, LLVMConstInt(int32_type(&ctx), 0, 0));
        LLVMDisposeBuilder(ctx.builder);
        return ctx.module;
}
