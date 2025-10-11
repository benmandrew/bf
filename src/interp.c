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

// Convert an abstract program counter `pc` (index into cmds array) to a
// concrete PC (index into the string representation of the program)
size_t abstract_to_concrete_pc(size_t pc, struct program *p) {
        assert(pc <= p->length);
        size_t concrete_pc = 0;
        for (size_t i = 0; i < pc; i++) {
                switch (p->cmds[i].type) {
                case CMD_SIMPLE_INC:
                case CMD_SIMPLE_DEC:
                case CMD_SIMPLE_RIGHT:
                case CMD_SIMPLE_LEFT:
                case CMD_SIMPLE_OUTPUT:
                case CMD_SIMPLE_INPUT:
                        concrete_pc += p->cmds[i].simple_count;
                        break;
                case CMD_JUMP_FORWARD:
                case CMD_JUMP_BACK:
                        concrete_pc++;
                        break;
                default:
                        fprintf(stderr, "Unrecognised cmd_type '%c'\n",
                                p->cmds[i].type);
                        exit(1);
                }
        }
        return concrete_pc;
}

char *context_to_string(struct context_t *ctx) {
        size_t program_length = program_str_length(&ctx->p);
        size_t concrete_pc = abstract_to_concrete_pc(ctx->pc, &ctx->p);
        size_t buffer_size = 8                       // "---\n    "
                             + program_length        // program string
                             + 5                     // "\nPC: "
                             + concrete_pc           // spaces for PC
                             + 6                     // "^\n    "
                             + (ctx->max_dp + 1) * 4 // data values (max "255 ")
                             + 5                     // "\nDP: "
                             + ctx->dp * 4 // spaces for DP (max 4 per position)
                             + 3;          // "^\n\0"
        char *out = malloc(buffer_size);
        if (!out) {
                fprintf(stderr, "Memory allocation failed\n");
                exit(1);
        }
        char *front = out;
        memcpy(out, "---\n    ", 8);
        out += 8;
        char *program_string = program_to_string(&ctx->p);
        memcpy(out, program_string, program_length);
        free(program_string);
        out += program_length;
        memcpy(out, "\nPC: ", 5);
        out += 5;
        size_t i;
        for (i = 0; i < concrete_pc; i++) {
                out[i] = ' ';
        }
        out += concrete_pc;
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
                ssize_t ret = write(out_fd, &ctx->data[ctx->dp], 1);
                if (ret < 0) {
                        fprintf(stderr, "Write error %zd: '%c'\n", ret,
                                ctx->data[ctx->dp]);
                        exit(1);
                }
        }
}

void interp_comma(struct context_t *ctx, int in_fd) {
        char c_in;
        ssize_t ret = read(in_fd, &c_in, 1);
        if (ret <= 0) {
                fprintf(stderr, "Read error %zd\n", ret);
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
                for (size_t i = 0; i < c.simple_count; i++) {
                        interp_dot(ctx, out_fd, byte_output);
                }
                break;
        case CMD_SIMPLE_INPUT:
                for (size_t i = 0; i < c.simple_count; i++) {
                        interp_comma(ctx, in_fd);
                }
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
