#include "interp.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct context_t init_context(struct program p) {
        struct context_t c =
            (struct context_t){.pc = 0, .p = p, .dp = 0, .max_dp = 0};
        memset(c.data, 0, DATA_SIZE);
        return c;
}

char *context_to_string(struct context_t *ctx) {
        char *out = malloc(ctx->p.length + 1000 + ctx->max_dp * 2 * 4);
        char *front = out;
        memcpy(out, "---\n    ", 8);
        out += 8;
        char *program_string = program_to_string(&ctx->p);
        memcpy(out, program_string, ctx->p.length);
        out += ctx->p.length;
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
        assert(ctx->pc < ctx->p.length);
        struct cmd c = ctx->p.cmds[ctx->pc];
        switch (c.type) {
        case CMD_SIMPLE_INC:
                ctx->data[ctx->dp] += c.simple_count;
                break;
        case CMD_SIMPLE_DEC:
                ctx->data[ctx->dp] -= c.simple_count;
                break;
        case CMD_SIMPLE_RIGHT:
                assert(ctx->dp < DATA_SIZE - c.simple_count);
                ctx->dp += c.simple_count;
                if (ctx->dp > ctx->max_dp) {
                        ctx->max_dp = ctx->dp;
                }
                break;
        case CMD_SIMPLE_LEFT:
                assert(ctx->dp > c.simple_count - 1);
                ctx->dp -= c.simple_count;
                break;
        case CMD_SIMPLE_OUTPUT:
                assert(c.simple_count == 1);
                interp_dot(ctx, out_fd, byte_output);
                break;
        case CMD_SIMPLE_INPUT:
                assert(c.simple_count == 1);
                interp_comma(ctx, in_fd);
                break;
        case CMD_JUMP_FORWARD:
                if (ctx->data[ctx->dp] == 0) {
                        ctx->pc = c.jump_index;
                }
                break;
        case CMD_JUMP_BACK:
                if (ctx->data[ctx->dp] > 0) {
                        ctx->pc = c.jump_index;
                }
                break;
        default:
                fprintf(stderr, "Invalid character '%c'\n",
                        cmd_type_to_char(c.type));
                exit(1);
        }
        ctx->pc++;
        if (ctx->pc >= ctx->p.length) {
                return 1;
        }
        return 0;
}
