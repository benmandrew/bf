#include "interp.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct context_t init_context(struct program program) {
        struct context_t context = {
            .pc = 0, .program = program, .dp = 0, .max_dp = 0};
        memset(context.data, 0, DATA_SIZE);
        return context;
}

// Convert an abstract program counter `pc` (index into cmds array) to a
// concrete PC (index into the string representation of the program)
size_t abstract_to_concrete_pc(size_t abstract_pc, struct program *program) {
        assert(abstract_pc <= program->length);
        size_t concrete_pc = 0;
        for (size_t cmd_index = 0; cmd_index < abstract_pc; cmd_index++) {
                switch (program->cmds[cmd_index].type) {
                case CMD_SIMPLE_INC:
                case CMD_SIMPLE_DEC:
                case CMD_SIMPLE_RIGHT:
                case CMD_SIMPLE_LEFT:
                case CMD_SIMPLE_OUTPUT:
                case CMD_SIMPLE_INPUT:
                        concrete_pc +=
                            program->cmds[cmd_index].value.simple_count;
                        break;
                case CMD_JUMP_FORWARD:
                case CMD_JUMP_BACK:
                        concrete_pc++;
                        break;
                case CMD_CLEAR:
                case CMD_MULTIPLY:
                        break;
                default:
                        fprintf(stderr, "Unrecognised cmd_type '%c'\n",
                                program->cmds[cmd_index].type);
                        exit(1);
                }
        }
        return concrete_pc;
}

char *context_to_string(struct context_t *ctx) {
        size_t program_length = program_str_length(&ctx->program);
        size_t concrete_pc = abstract_to_concrete_pc(ctx->pc, &ctx->program);
        size_t buffer_size =
            8                        // "---\n    "
            + program_length         // program string
            + 5                      // "\nPC: "
            + concrete_pc            // spaces for PC
            + 6                      // "^\n    "
            + (ctx->max_dp + 1) * 4  // data values (max "255 ")
            + 5                      // "\nDP: "
            + ctx->dp * 4            // spaces for DP (max 4 per position)
            + 3;                     // "^\n\0"
        char *out = malloc(buffer_size);
        if (!out) {
                fprintf(stderr, "Memory allocation failed\n");
                exit(1);
        }
        char *front = out;
        memcpy(out, "---\n    ", 8);
        out += 8;
        char *program_string = program_to_string(&ctx->program);
        memcpy(out, program_string, program_length);
        free(program_string);
        out += program_length;
        memcpy(out, "\nPC: ", 5);
        out += 5;
        size_t data_index = 0;
        for (data_index = 0; data_index < concrete_pc; data_index++) {
                out[data_index] = ' ';
        }
        out += concrete_pc;
        memcpy(out, "^\n    ", 6);
        out += 6;
        char intermediate[5];
        for (data_index = 0; data_index <= ctx->max_dp; data_index++) {
                snprintf(intermediate, sizeof(intermediate), "%u ",
                         ctx->data[data_index]);
                size_t len = strlen(intermediate);
                memcpy(out, intermediate, len);
                out += len;
        }
        memcpy(out, "\nDP: ", 5);
        out += 5;
        for (data_index = 0; data_index < ctx->dp; data_index++) {
                if (ctx->data[data_index] >= 100) {
                        memcpy(out, "    ", 4);
                        out += 4;
                } else if (ctx->data[data_index] >= 10) {
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
        char c_in = 0;
        ssize_t ret = read(in_fd, &c_in, 1);
        if (ret <= 0) {
                fprintf(stderr, "Read error %zd\n", ret);
                exit(1);
        }
        ctx->data[ctx->dp] = c_in;
}

int interp(struct context_t *ctx, int out_fd, int in_fd, bool byte_output) {
        if (ctx->pc >= ctx->program.length)
                return 1;
        struct cmd current_cmd = ctx->program.cmds[ctx->pc];
        switch (current_cmd.type) {
        case CMD_SIMPLE_INC:
                ctx->data[ctx->dp] += current_cmd.value.simple_count;
                break;
        case CMD_SIMPLE_DEC:
                ctx->data[ctx->dp] -= current_cmd.value.simple_count;
                break;
        case CMD_SIMPLE_RIGHT:
                if (current_cmd.value.simple_count >= DATA_SIZE - ctx->dp)
                        return -1;
                ctx->dp += current_cmd.value.simple_count;
                if (ctx->dp > ctx->max_dp) {
                        ctx->max_dp = ctx->dp;
                }
                break;
        case CMD_SIMPLE_LEFT:
                if (current_cmd.value.simple_count > ctx->dp)
                        return -1;
                ctx->dp -= current_cmd.value.simple_count;
                break;
        case CMD_SIMPLE_OUTPUT:
                for (size_t output_index = 0;
                     output_index < current_cmd.value.simple_count;
                     output_index++) {
                        interp_dot(ctx, out_fd, byte_output);
                }
                break;
        case CMD_SIMPLE_INPUT:
                for (size_t input_index = 0;
                     input_index < current_cmd.value.simple_count;
                     input_index++) {
                        interp_comma(ctx, in_fd);
                }
                break;
        case CMD_JUMP_FORWARD:
                if (ctx->data[ctx->dp] == 0) {
                        ctx->pc = current_cmd.value.jump_index;
                }
                break;
        case CMD_JUMP_BACK:
                if (ctx->data[ctx->dp] > 0) {
                        ctx->pc = current_cmd.value.jump_index;
                }
                break;
        case CMD_CLEAR:
                ctx->data[ctx->dp] = 0;
                break;
        case CMD_MULTIPLY:
                // Check every target before writing any of them, so an
                // out-of-bounds move leaves the tape untouched, as the simple
                // moves above do.
                for (size_t i = 0; i < current_cmd.value.multiply.n_moves;
                     i++) {
                        int offset = current_cmd.value.multiply.moves[i].offset;
                        if (offset < 0) {
                                if ((size_t)-offset > ctx->dp)
                                        return -1;
                        } else if ((size_t)offset >= DATA_SIZE - ctx->dp) {
                                return -1;
                        }
                }
                for (size_t i = 0; i < current_cmd.value.multiply.n_moves;
                     i++) {
                        int target = (int)ctx->dp +
                                     current_cmd.value.multiply.moves[i].offset;
                        ctx->data[target] +=
                            ctx->data[ctx->dp] *
                            (uint8_t)current_cmd.value.multiply.moves[i].factor;
                }
                ctx->data[ctx->dp] = 0;
                break;
        default:
                fprintf(stderr, "Invalid character '%c'\n",
                        cmd_type_to_char(current_cmd.type));
                exit(1);
        }
        ctx->pc++;
        if (ctx->pc >= ctx->program.length) {
                return 1;
        }
        return 0;
}
