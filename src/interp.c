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

static unsigned int max_dp = 0;

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

char *context_to_string(struct context_t *ctx) {
        char *out = malloc(20 + max_dp * 2 * 4);
        char *front = out;
        char intermediate[20];
        sprintf(intermediate, "PC: %u\n    ", ctx->pc);
        size_t len = strlen(intermediate);
        memcpy(out, intermediate, len);
        out += len;
        unsigned int i;
        for (i = 0; i <= max_dp; i++) {
                sprintf(intermediate, "%u ", ctx->data[i]);
                len = strlen(intermediate);
                memcpy(out, intermediate, len);
                out += len;
        }
        memcpy(out, "\nDP: ", 5);
        out += 5;
        for (i = 0; i < ctx->dp; i++) {
                if (ctx->data[i] >= 100) {
                        memcpy(out, "    ", 4);
                        out += 4;
                } else if (ctx->data[i] >= 10) {
                        memcpy(out, "   ", 3);
                        out += 3;
                } else {
                        memcpy(out, "  ", 2);
                        out += 2;
                }
        }
        out[0] = '^';
        out[1] = '\n';
        out[2] = '\0';
        return front;
}

void interp_l_brac(struct context_t *ctx) {
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

int interp(struct context_t *ctx, int out_fd, int in_fd) {
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
                if (ctx->dp > max_dp) {
                        max_dp = ctx->dp;
                }
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
                if (write(out_fd, &ctx->data[ctx->dp], 1) < 0) {
                        fprintf(stderr, "Write error '%c'\n",
                                ctx->data[ctx->dp]);
                        exit(1);
                }
                break;
        case ',':
                if (read(in_fd, &c_in, 1) > 0) {
                        ctx->data[ctx->dp] = c_in;
                }
                break;
        default:
                fprintf(stderr, "Invalid character '%c'\n", c);
                exit(1);
        }
        ctx->pc++;
        if (ctx->pc >= ctx->program_len) {
                return 1;
        }
        return 0;
}
