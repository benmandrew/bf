#include "llvm.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <llvm-c/Transforms/PassBuilder.h>

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

/// Buffer size for the Brainfuck snippet appended to a block name, including
/// the terminator.
#define BF_LABEL_MAX (32)

/// Buffer size for a full block name: structural prefix, space, snippet.
#define BLOCK_NAME_MAX (BF_LABEL_MAX + 64)

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
        /// Shared exit block returning 1 when a tape index leaves the tape.
        /// Created on first use, so programs that cannot leave the tape after
        /// optimisation carry no extra block.
        LLVMBasicBlockRef oob;
        /// Append Brainfuck source spans to block names.
        bool label_blocks;
        /// Index of the first command in the span currently being built.
        size_t block_start_cmd;
        /// Block the current span started in. A span runs until the next
        /// bracket, but a bounds check splits it across several blocks, so the
        /// label belongs to the block the span opened in rather than whichever
        /// one it happens to end in.
        LLVMBasicBlockRef span_block;
};

static LLVMTypeRef int32_type(struct llvm_context *ctx) {
        return LLVMInt32TypeInContext(ctx->context);
}

static LLVMTypeRef int8_type(struct llvm_context *ctx) {
        return LLVMInt8TypeInContext(ctx->context);
}

/// Type of the data pointer and of every GEP index into the tape. Making the
/// index i64 keeps the pointer arithmetic sign-extension-free, so a loop that
/// moves the pointer becomes a clean pointer recurrence for LLVM's induction
/// variable and loop-idiom passes.
static LLVMTypeRef index_type(struct llvm_context *ctx) {
        return LLVMInt64TypeInContext(ctx->context);
}

static LLVMTypeRef data_array_type(struct llvm_context *ctx) {
        return LLVMArrayType(int8_type(ctx), DATA_SIZE);
}

/// Encoded value of the `memory(inaccessiblemem: readwrite)` function
/// attribute: ModRefInfo::ModRef (3) shifted into the InaccessibleMem slot
/// (location 1, two bits per location). It tells LLVM that putchar/getchar
/// touch only state the module cannot see — never the tape — so loads and
/// stores to the tape may be optimised across an I/O call.
#define MEMORY_INACCESSIBLEMEM_READWRITE (3ULL << 2)

/// Attach `memory(inaccessiblemem: readwrite)` to a declared I/O function.
static void set_io_memory_effects(struct llvm_context *ctx, LLVMValueRef func) {
        unsigned kind = LLVMGetEnumAttributeKindForName("memory", 6);
        LLVMAttributeRef attr = LLVMCreateEnumAttribute(
            ctx->context, kind, MEMORY_INACCESSIBLEMEM_READWRITE);
        LLVMAddAttributeAtIndex(func, LLVMAttributeFunctionIndex, attr);
}

void create_putchar_declaration(struct llvm_context *ctx) {
        LLVMTypeRef i32 = int32_type(ctx);
        ctx->putchar.type = LLVMFunctionType(i32, (LLVMTypeRef[]){i32}, 1, 0);
        ctx->putchar.func =
            LLVMAddFunction(ctx->module, "putchar", ctx->putchar.type);
        set_io_memory_effects(ctx, ctx->putchar.func);
}

void create_getchar_declaration(struct llvm_context *ctx) {
        ctx->getchar.type = LLVMFunctionType(int32_type(ctx), NULL, 0, 0);
        ctx->getchar.func =
            LLVMAddFunction(ctx->module, "getchar", ctx->getchar.type);
        set_io_memory_effects(ctx, ctx->getchar.func);
}

struct llvm_context create_module_preamble(struct program *program,
                                           const char *name,
                                           bool label_blocks) {
        struct llvm_context ctx;
        ctx.label_blocks = label_blocks;
        ctx.block_start_cmd = 0;
        ctx.oob = NULL;
        ctx.context = LLVMContextCreate();
        ctx.module = LLVMModuleCreateWithNameInContext(name, ctx.context);
        ctx.builder = LLVMCreateBuilderInContext(ctx.context);
        if (program_contains_output(program)) {
                create_putchar_declaration(&ctx);
        }
        if (program_contains_input(program)) {
                create_getchar_declaration(&ctx);
        }
        ctx.data = LLVMAddGlobal(ctx.module, data_array_type(&ctx), "data");
        LLVMSetInitializer(ctx.data, LLVMConstNull(data_array_type(&ctx)));
        // The tape is only ever touched from within main; private linkage lets
        // whole-module passes reason about it (and shrink it to the cells the
        // program actually uses).
        LLVMSetLinkage(ctx.data, LLVMPrivateLinkage);
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
        ctx->span_block = entry_block;
        ctx->dp = LLVMBuildAlloca(ctx->builder, index_type(ctx), "dp");
        LLVMBuildStore(ctx->builder, LLVMConstInt(index_type(ctx), 0, 0),
                       ctx->dp);
}

/// The block every failed bounds check branches to. It returns 1, matching the
/// exit status bfi produces for the same condition.
static LLVMBasicBlockRef oob_block(struct llvm_context *ctx) {
        if (ctx->oob == NULL) {
                LLVMBasicBlockRef current = LLVMGetInsertBlock(ctx->builder);
                ctx->oob = LLVMAppendBasicBlockInContext(ctx->context,
                                                         ctx->main, "tape.oob");
                LLVMPositionBuilderAtEnd(ctx->builder, ctx->oob);
                LLVMBuildRet(ctx->builder, LLVMConstInt(int32_type(ctx), 1, 0));
                LLVMPositionBuilderAtEnd(ctx->builder, current);
        }
        return ctx->oob;
}

/// Continue only when `index` is a valid tape index, otherwise leave for
/// oob_block(). A single unsigned comparison covers both ends: moving left past
/// zero wraps to near 2^64, which is not less than DATA_SIZE either. Leaves the
/// builder positioned in the continuation block.
static void emit_bounds_check(struct llvm_context *ctx, LLVMValueRef index) {
        LLVMValueRef in_range =
            LLVMBuildICmp(ctx->builder, LLVMIntULT, index,
                          LLVMConstInt(index_type(ctx), DATA_SIZE, 0), "");
        LLVMBasicBlockRef cont = LLVMAppendBasicBlockInContext(
            ctx->context, ctx->main, "tape.inbounds");
        LLVMBuildCondBr(ctx->builder, in_range, cont, oob_block(ctx));
        LLVMPositionBuilderAtEnd(ctx->builder, cont);
}

LLVMValueRef get_dataptr(struct llvm_context *ctx) {
        LLVMValueRef dp_value =
            LLVMBuildLoad2(ctx->builder, index_type(ctx), ctx->dp, "");
        LLVMValueRef indices[] = {LLVMConstInt(index_type(ctx), 0, 0),
                                  dp_value};
        LLVMValueRef data_ptr = LLVMBuildInBoundsGEP2(
            ctx->builder, data_array_type(ctx), ctx->data, indices, 2, "");
        return data_ptr;
}

void add(struct llvm_context *ctx, size_t value) {
        LLVMValueRef data_ptr = get_dataptr(ctx);
        LLVMValueRef current_value =
            LLVMBuildLoad2(ctx->builder, int8_type(ctx), data_ptr, "");
        LLVMValueRef new_value =
            LLVMBuildAdd(ctx->builder, current_value,
                         LLVMConstInt(int8_type(ctx), value, 0), "");
        LLVMBuildStore(ctx->builder, new_value, data_ptr);
}

void sub(struct llvm_context *ctx, size_t value) {
        LLVMValueRef data_ptr = get_dataptr(ctx);
        LLVMValueRef current_value =
            LLVMBuildLoad2(ctx->builder, int8_type(ctx), data_ptr, "");
        LLVMValueRef new_value =
            LLVMBuildSub(ctx->builder, current_value,
                         LLVMConstInt(int8_type(ctx), value, 0), "");
        LLVMBuildStore(ctx->builder, new_value, data_ptr);
}

// The moves below deliberately use wrapping add and subtract rather than their
// NSW forms: the bounds check reads the result, so overflow has to be a defined
// value rather than poison. The pointer is stored only once it is known good.

void right(struct llvm_context *ctx, size_t value) {
        LLVMValueRef dp_value =
            LLVMBuildLoad2(ctx->builder, index_type(ctx), ctx->dp, "");
        LLVMValueRef new_dp =
            LLVMBuildAdd(ctx->builder, dp_value,
                         LLVMConstInt(index_type(ctx), value, 0), "");
        emit_bounds_check(ctx, new_dp);
        LLVMBuildStore(ctx->builder, new_dp, ctx->dp);
}

void left(struct llvm_context *ctx, size_t value) {
        LLVMValueRef dp_value =
            LLVMBuildLoad2(ctx->builder, index_type(ctx), ctx->dp, "");
        LLVMValueRef new_dp =
            LLVMBuildSub(ctx->builder, dp_value,
                         LLVMConstInt(index_type(ctx), value, 0), "");
        emit_bounds_check(ctx, new_dp);
        LLVMBuildStore(ctx->builder, new_dp, ctx->dp);
}

/// Emit `count` copies of `.` (write current cell). The cell does not change
/// between writes and putchar cannot touch the tape, so the value is loaded and
/// zero-extended once and fed to every call.
void output(struct llvm_context *ctx, size_t count) {
        LLVMValueRef data_ptr = get_dataptr(ctx);
        LLVMValueRef current_value =
            LLVMBuildLoad2(ctx->builder, int8_type(ctx), data_ptr, "");
        LLVMValueRef extended_value =
            LLVMBuildZExt(ctx->builder, current_value, int32_type(ctx), "");
        for (size_t i = 0; i < count; i++) {
                LLVMBuildCall2(ctx->builder, ctx->putchar.type,
                               ctx->putchar.func, &extended_value, 1, "");
        }
}

/// Attach `!range !{i32 -1, i32 256}` to a getchar result: the wrapped range
/// covers EOF (-1) and every byte value 0..255, letting LLVM fold the
/// surrounding truncations and comparisons.
static void set_getchar_range(struct llvm_context *ctx, LLVMValueRef call) {
        LLVMTypeRef i32 = int32_type(ctx);
        LLVMMetadataRef bounds[] = {
            LLVMValueAsMetadata(LLVMConstInt(i32, (uint64_t)-1, 1)),
            LLVMValueAsMetadata(LLVMConstInt(i32, 256, 0)),
        };
        LLVMMetadataRef node = LLVMMDNodeInContext2(ctx->context, bounds, 2);
        unsigned kind = LLVMGetMDKindIDInContext(ctx->context, "range", 5);
        LLVMSetMetadata(call, kind, LLVMMetadataAsValue(ctx->context, node));
}

void comma(struct llvm_context *ctx) {
        LLVMValueRef data_ptr = get_dataptr(ctx);
        LLVMValueRef getchar_result = LLVMBuildCall2(
            ctx->builder, ctx->getchar.type, ctx->getchar.func, NULL, 0, "");
        set_getchar_range(ctx, getchar_result);
        LLVMValueRef char_value =
            LLVMBuildTrunc(ctx->builder, getchar_result, int8_type(ctx), "");
        LLVMBuildStore(ctx->builder, char_value, data_ptr);
}

void multiply(struct llvm_context *ctx, struct multiply_move *moves,
              size_t n_moves) {
        LLVMValueRef counter_ptr = get_dataptr(ctx);
        LLVMValueRef counter =
            LLVMBuildLoad2(ctx->builder, int8_type(ctx), counter_ptr, "");
        // The data pointer does not move during a multiply, so load it once and
        // derive every target from it.
        LLVMValueRef dp_value =
            LLVMBuildLoad2(ctx->builder, index_type(ctx), ctx->dp, "");
        assert(n_moves <= MULTIPLY_MOVES_MAX);
        LLVMValueRef target_idx[MULTIPLY_MOVES_MAX];
        for (size_t i = 0; i < n_moves; i++) {
                LLVMValueRef offset =
                    LLVMConstInt(index_type(ctx), (uint64_t)moves[i].offset, 1);
                target_idx[i] =
                    LLVMBuildAdd(ctx->builder, dp_value, offset, "");
        }
        // Check every target before writing any of them, so an out-of-bounds
        // move leaves the tape untouched, as interp.c does.
        for (size_t i = 0; i < n_moves; i++) {
                emit_bounds_check(ctx, target_idx[i]);
        }
        for (size_t i = 0; i < n_moves; i++) {
                LLVMValueRef indices[] = {LLVMConstInt(index_type(ctx), 0, 0),
                                          target_idx[i]};
                LLVMValueRef target_ptr =
                    LLVMBuildInBoundsGEP2(ctx->builder, data_array_type(ctx),
                                          ctx->data, indices, 2, "");
                LLVMValueRef target = LLVMBuildLoad2(
                    ctx->builder, int8_type(ctx), target_ptr, "");
                LLVMValueRef factor =
                    LLVMConstInt(int8_type(ctx), (uint64_t)moves[i].factor, 1);
                LLVMValueRef product =
                    LLVMBuildMul(ctx->builder, counter, factor, "");
                LLVMValueRef new_val =
                    LLVMBuildAdd(ctx->builder, target, product, "");
                LLVMBuildStore(ctx->builder, new_val, target_ptr);
        }
        LLVMBuildStore(ctx->builder, LLVMConstInt(int8_type(ctx), 0, 0),
                       counter_ptr);
}

void clear(struct llvm_context *ctx) {
        LLVMValueRef data_ptr = get_dataptr(ctx);
        LLVMBuildStore(ctx->builder, LLVMConstInt(int8_type(ctx), 0, 0),
                       data_ptr);
}

/// Close off the block under construction, which spans commands
/// [block_start_cmd, end_cmd). When labelling is enabled, its Brainfuck source
/// span is appended to the structural name assigned at creation.
static void finish_block(struct llvm_context *ctx, struct program *program,
                         size_t end_cmd) {
        if (ctx->label_blocks) {
                char bf[BF_LABEL_MAX];
                program_range_to_label(program, ctx->block_start_cmd, end_cmd,
                                       bf, sizeof(bf));
                if (bf[0] != '\0') {
                        LLVMValueRef block =
                            LLVMBasicBlockAsValue(ctx->span_block);
                        size_t prefix_len = 0;
                        const char *prefix =
                            LLVMGetValueName2(block, &prefix_len);
                        char name[BLOCK_NAME_MAX];
                        snprintf(name, sizeof(name), "%.*s %s", (int)prefix_len,
                                 prefix, bf);
                        LLVMSetValueName2(block, name, strlen(name));
                }
        }
        ctx->block_start_cmd = end_cmd + 1;
        // The next span opens wherever the bracket that follows leaves the
        // builder, which left_bracket and right_bracket record for us.
}

void left_bracket(struct llvm_context *ctx, size_t cmd_index) {
        LLVMValueRef data_ptr = get_dataptr(ctx);
        LLVMValueRef current_value =
            LLVMBuildLoad2(ctx->builder, int8_type(ctx), data_ptr, "");
        LLVMValueRef condition =
            LLVMBuildICmp(ctx->builder, LLVMIntNE, current_value,
                          LLVMConstInt(int8_type(ctx), 0, 0), "");
        // cmd_index is unique per loop, so these names never collide and LLVM
        // never appends a disambiguating suffix.
        char body_name[BLOCK_NAME_MAX];
        char end_name[BLOCK_NAME_MAX];
        snprintf(body_name, sizeof(body_name), "loop%zu.body", cmd_index);
        snprintf(end_name, sizeof(end_name), "loop%zu.end", cmd_index);
        LLVMBasicBlockRef entry =
            LLVMAppendBasicBlockInContext(ctx->context, ctx->main, body_name);
        LLVMBasicBlockRef exit =
            LLVMAppendBasicBlockInContext(ctx->context, ctx->main, end_name);
        jump_stack_push(&ctx->js, entry, exit);
        LLVMBuildCondBr(ctx->builder, condition, entry, exit);
        LLVMPositionBuilderAtEnd(ctx->builder, entry);
        ctx->span_block = entry;
}

void right_bracket(struct llvm_context *ctx) {
        struct entry_exit_pair pair = jump_stack_pop(&ctx->js);
        LLVMValueRef data_ptr = get_dataptr(ctx);
        LLVMValueRef current_value =
            LLVMBuildLoad2(ctx->builder, int8_type(ctx), data_ptr, "");
        LLVMValueRef condition =
            LLVMBuildICmp(ctx->builder, LLVMIntNE, current_value,
                          LLVMConstInt(int8_type(ctx), 0, 0), "");
        LLVMBuildCondBr(ctx->builder, condition, pair.entry, pair.exit);
        LLVMPositionBuilderAtEnd(ctx->builder, pair.exit);
        ctx->span_block = pair.exit;
}

LLVMModuleRef generate(struct program *program, bool optimise,
                       bool label_blocks) {
        struct llvm_context ctx =
            create_module_preamble(program, "main", label_blocks);
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
                        output(&ctx, command.value.simple_count);
                        break;
                case CMD_SIMPLE_INPUT:
                        for (size_t input_index = 0;
                             input_index < command.value.simple_count;
                             input_index++) {
                                comma(&ctx);
                        }
                        break;
                case CMD_JUMP_FORWARD:
                        finish_block(&ctx, program, cmd_index);
                        left_bracket(&ctx, cmd_index);
                        break;
                case CMD_JUMP_BACK:
                        finish_block(&ctx, program, cmd_index);
                        right_bracket(&ctx);
                        break;
                case CMD_CLEAR:
                        clear(&ctx);
                        break;
                case CMD_MULTIPLY:
                        multiply(&ctx, command.value.multiply.moves,
                                 command.value.multiply.n_moves);
                        break;
                default:
                        fprintf(stderr, "Unsupported cmd_type '%c'\n",
                                command.type);
                        exit(1);
                }
        }
        finish_block(&ctx, program, program->length);
        LLVMBuildRet(ctx.builder, LLVMConstInt(int32_type(&ctx), 0, 0));
        LLVMDisposeBuilder(ctx.builder);
        if (optimise) {
                LLVMPassBuilderOptionsRef opts = LLVMCreatePassBuilderOptions();
                // The full -O2 pipeline: adds DSE, LICM, IndVarSimplify and
                // LoopIdiomRecognize on top of the earlier hand-picked passes.
                // bfc's output is usually compiled at clang -O0, so whatever is
                // not folded here is not folded downstream either.
                LLVMErrorRef err =
                    LLVMRunPasses(ctx.module, "default<O2>", NULL, opts);
                if (err) {
                        char *msg = LLVMGetErrorMessage(err);
                        fprintf(stderr, "Pass error: %s\n", msg);
                        LLVMDisposeErrorMessage(msg);
                }
                LLVMDisposePassBuilderOptions(opts);
        }
        return ctx.module;
}
