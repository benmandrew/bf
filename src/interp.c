#include "interp.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct context_t init_context(char *program) {
        struct context_t c = (struct context_t){.pc = 0,
                                                .program = program,
                                                .program_len = strlen(program),
                                                .dp = 0,
                                                .max_dp = 0};
        memset(c.data, 0, DATA_SIZE);
        return c;
}

char *context_to_string(struct context_t *ctx) {
        char *out = malloc(ctx->program_len + 1000 + ctx->max_dp * 2 * 4);
        char *front = out;
        memcpy(out, "---\n    ", 8);
        out += 8;
        memcpy(out, ctx->program, ctx->program_len);
        out += ctx->program_len;
        memcpy(out, "\nPC: ", 5);
        out += 5;
        size_t i;
        for (i = 0; i < ctx->pc; i++) {
                out[i] = ' ';
        }
        out += ctx->pc;
        memcpy(out, "^\n    ", 6);
        out += 6;
        char intermediate[5];
        for (i = 0; i <= ctx->max_dp; i++) {
                sprintf(intermediate, "%u ", ctx->data[i]);
                size_t len = strlen(intermediate);
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
        assert(ctx->pc < ctx->program_len);
        if (ctx->data[ctx->dp] == 0) {
                int matching = 0;
                while (ctx->program[ctx->pc] != ']' || matching > 0) {
                        if (ctx->program[ctx->pc] == '[' && matching > 0) {
                                matching++;
                        } else if (ctx->program[ctx->pc] == ']') {
                                assert(matching > 0);
                                matching--;
                        }
                        ctx->pc++;
                }
        }
}

void interp_r_brac(struct context_t *ctx) {
        assert(ctx->pc > 0);
        assert(ctx->pc < ctx->program_len);
        if (ctx->data[ctx->dp] != 0) {
                ctx->pc--;
                int matching = 0;
                while (ctx->program[ctx->pc] != '[' || matching > 0) {
                        if (ctx->program[ctx->pc] == ']') {
                                matching++;
                        } else if (ctx->program[ctx->pc] == '[' &&
                                   matching > 0) {
                                matching--;
                        }
                        assert(ctx->pc > 0);
                        ctx->pc--;
                }
        }
}

void interp_dot(struct context_t *ctx, int out_fd, bool byte_output) {
        if (byte_output) {
                fprintf(stdout, "%u", ctx->data[ctx->dp]);

        } else {
                int ret = write(out_fd, &ctx->data[ctx->dp], 1);
                if (ret < 0) {
                        fprintf(stderr, "Write error %d: '%c'\n", ret,
                                ctx->data[ctx->dp]);
                        exit(1);
                }
        }
}

void interp_comma(struct context_t *ctx, int in_fd) {
        char c_in;
        int ret = read(in_fd, &c_in, 1);
        if (ret <= 0) {
                fprintf(stderr, "Read error %d\n", ret);
                exit(1);
        }
        ctx->data[ctx->dp] = c_in;
}

int interp(struct context_t *ctx, int out_fd, int in_fd, bool byte_output) {
        assert(ctx->pc < ctx->program_len);
        char c = ctx->program[ctx->pc];
        switch (c) {
        case '+':
                ctx->data[ctx->dp]++;
                break;
        case '-':
                ctx->data[ctx->dp]--;
                break;
        case '>':
                assert(ctx->dp < DATA_SIZE - 1);
                ctx->dp++;
                if (ctx->dp > ctx->max_dp) {
                        ctx->max_dp = ctx->dp;
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
                interp_dot(ctx, out_fd, byte_output);
                break;
        case ',':
                interp_comma(ctx, in_fd);
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
