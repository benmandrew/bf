#include "interp.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define DATA_SIZE (65536)

static unsigned char data[DATA_SIZE] = {0};
static bool context_initialized = false;

struct context_t init_context(char *program) {
        if (!context_initialized) {
                context_initialized = true;
        } else {
                fprintf(stderr,
                        "Error: Context can only be initialized once\n");
                exit(1);
        }
        return (struct context_t){.pc = 0,
                                  .program = program,
                                  .program_len = strlen(program),
                                  .dp = 0,
                                  .data = data};
}

void interp_l_brac(struct context_t *ctx) {
        // printf("LBRAC pc=%u dp=%u data=%u\n", ctx->pc, ctx->dp,
        //        ctx->data[ctx->dp]);
        if (ctx->data[ctx->dp] == 0) {
                assert(ctx->pc < ctx->program_len);
                int matching = 0;
                while (ctx->program[ctx->pc] != ']' || matching > 0) {
                        if (ctx->program[ctx->pc] == '[' && matching > 0) {
                                matching++;
                        } else if (ctx->program[ctx->pc] == ']') {
                                assert(matching > 0);
                                matching--;
                        }
                        assert(ctx->pc < ctx->program_len - 1);
                        ctx->pc++;
                }
                assert(ctx->pc < ctx->program_len);
        }
}

void interp_r_brac(struct context_t *ctx) {
        // printf("RBRAC pc=%u dp=%u data=%u\n", ctx->pc, ctx->dp,
        //        ctx->data[ctx->dp]);
        if (ctx->data[ctx->dp] != 0) {
                assert(ctx->pc < ctx->program_len);
                int matching = 0;
                while (ctx->program[ctx->pc] != '[' || matching > 0) {
                        if (ctx->program[ctx->pc] == ']' && matching > 0) {
                                matching++;
                        } else if (ctx->program[ctx->pc] == '[') {
                                assert(matching > 0);
                                matching--;
                        }
                        assert(ctx->pc > 0);
                        ctx->pc--;
                }
                assert(ctx->pc < ctx->program_len);
        }
}

int interp(struct context_t *ctx) {
        assert(ctx->pc < ctx->program_len);
        char c = ctx->program[ctx->pc];
        char c_in;
        switch (c) {
        case '+':
                ctx->data[ctx->dp]++;
                break;
        case '-':
                ctx->data[ctx->dp]--;
                break;
        case '>':
                ctx->dp++;
                assert(ctx->dp < DATA_SIZE);
                break;
        case '<':
                assert(ctx->dp > 0);
                ctx->dp--;
                break;
        case '[':
                interp_l_brac(ctx);
                break;
        case ']':
                interp_r_brac(ctx);
                break;
        case '.':
                printf("%c", ctx->data[ctx->dp]);
                fflush(STDIN_FILENO);
                break;
        case ',':
                if (read(STDIN_FILENO, &c_in, 1) > 0) {
                        ctx->data[ctx->dp] = c_in;
                }
                break;
        default:
                fprintf(stderr, "Invalid character '%c'\n", c);
                break;
        }
        ctx->pc++;
        if (ctx->pc >= ctx->program_len) {
                return 1;
        }
        return 0;
}
