#include "ir.h"

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define JUMP_STACK_MAX_SIZE (128)

struct jump_stack_frame {
        struct cmd *command;
        size_t index;
};

struct jump_stack {
        struct jump_stack_frame stack[JUMP_STACK_MAX_SIZE];
        size_t head;
};

static struct jump_stack jump_stack_new() {
        return (struct jump_stack){
            .head = 0,
        };
}

static void jump_stack_push(struct jump_stack *jump_stack, struct cmd *command,
                            size_t index) {
        assert(jump_stack->head < JUMP_STACK_MAX_SIZE - 1);
        jump_stack->stack[jump_stack->head] = (struct jump_stack_frame){
            .command = command,
            .index = index,
        };
        jump_stack->head++;
}

static struct jump_stack_frame jump_stack_pop(struct jump_stack *jump_stack) {
        assert(jump_stack->head > 0);
        jump_stack->head--;
        return jump_stack->stack[jump_stack->head];
}

size_t program_str_length(struct program *program) {
        size_t length = 0;
        for (size_t cmd_index = 0; cmd_index < program->length; cmd_index++) {
                switch (program->cmds[cmd_index].type) {
                case CMD_SIMPLE_INC:
                case CMD_SIMPLE_DEC:
                case CMD_SIMPLE_RIGHT:
                case CMD_SIMPLE_LEFT:
                case CMD_SIMPLE_OUTPUT:
                case CMD_SIMPLE_INPUT:
                        length += program->cmds[cmd_index].value.simple_count;
                        break;
                case CMD_JUMP_FORWARD:
                case CMD_JUMP_BACK:
                        length++;
                        break;
                default:
                        fprintf(stderr, "Unrecognised cmd_type '%c'\n",
                                program->cmds[cmd_index].type);
                        exit(1);
                }
        }
        return length;
}

size_t n_simple_consecutive(char *source_str, size_t start, size_t source_len,
                            struct cmd *command) {
        size_t consecutive_count = 0;
        char first_char = source_str[start];
        switch (first_char) {
        case '+':
                *command = (struct cmd){.type = CMD_SIMPLE_INC,
                                        .value.simple_count = 1};
                break;
        case '-':
                *command = (struct cmd){.type = CMD_SIMPLE_DEC,
                                        .value.simple_count = 1};
                break;
        case '>':
                *command = (struct cmd){.type = CMD_SIMPLE_RIGHT,
                                        .value.simple_count = 1};
                break;
        case '<':
                *command = (struct cmd){.type = CMD_SIMPLE_LEFT,
                                        .value.simple_count = 1};
                break;
        case '.':
                *command = (struct cmd){.type = CMD_SIMPLE_OUTPUT,
                                        .value.simple_count = 1};
                break;
        case ',':
                *command = (struct cmd){.type = CMD_SIMPLE_INPUT,
                                        .value.simple_count = 1};
                break;
        default:
                fprintf(stderr, "Invalid character '%c'\n",
                        source_str[consecutive_count]);
                exit(1);
        }
        while (start + consecutive_count + 1 < source_len &&
               source_str[start + consecutive_count + 1] == first_char) {
                consecutive_count++;
                command->value.simple_count++;
        }
        return command->value.simple_count - 1;
}

struct program string_to_program(char *source_str) {
        size_t source_len = strlen(source_str);
        struct cmd *cmd_arena = malloc(source_len * sizeof(struct cmd));
        if (!cmd_arena) {
                fprintf(stderr, "Memory allocation failed\n");
                exit(1);
        }
        struct jump_stack jump_stack = jump_stack_new();
        struct jump_stack_frame back_jump_frame;
        size_t arena_index = 0;
        size_t str_index = 0;
        for (str_index = 0; str_index < source_len; str_index++) {
                switch (source_str[str_index]) {
                case '+':
                case '-':
                case '>':
                case '<':
                case '.':
                case ',':
                        str_index += n_simple_consecutive(
                            source_str, str_index, source_len,
                            &cmd_arena[arena_index]);
                        break;
                case '[':
                        cmd_arena[arena_index] =
                            (struct cmd){.type = CMD_JUMP_FORWARD};
                        jump_stack_push(&jump_stack, &cmd_arena[arena_index],
                                        arena_index);
                        break;
                case ']':
                        back_jump_frame = jump_stack_pop(&jump_stack);
                        cmd_arena[arena_index] = (struct cmd){
                            .type = CMD_JUMP_BACK,
                            .value.jump_index = back_jump_frame.index};
                        back_jump_frame.command->value.jump_index = arena_index;
                        break;
                default:
                        fprintf(stderr, "Invalid character '%c'\n",
                                source_str[str_index]);
                        exit(1);
                }
                arena_index++;
        }
        assert(jump_stack.head == 0);
        return (struct program){.cmds = cmd_arena, .length = arena_index};
}

char cmd_type_to_char(enum cmd_type command_type) {
        switch (command_type) {
        case CMD_SIMPLE_INC:
                return '+';
        case CMD_SIMPLE_DEC:
                return '-';
        case CMD_SIMPLE_RIGHT:
                return '>';
        case CMD_SIMPLE_LEFT:
                return '<';
        case CMD_SIMPLE_OUTPUT:
                return '.';
        case CMD_SIMPLE_INPUT:
                return ',';
        case CMD_JUMP_FORWARD:
                return '[';
        case CMD_JUMP_BACK:
                return ']';
        default:
                fprintf(stderr, "Unrecognised cmd_type '%c'\n", command_type);
                exit(1);
        }
}

char *program_to_string(struct program *program) {
        size_t program_str_len = program_str_length(program);
        char *out = malloc(program_str_len * sizeof(char) + 1);
        size_t str_index = 0;
        for (size_t cmd_index = 0; cmd_index < program->length; cmd_index++) {
                switch (program->cmds[cmd_index].type) {
                case CMD_SIMPLE_INC:
                case CMD_SIMPLE_DEC:
                case CMD_SIMPLE_RIGHT:
                case CMD_SIMPLE_LEFT:
                case CMD_SIMPLE_OUTPUT:
                case CMD_SIMPLE_INPUT:
                        for (size_t repeat_index = 0;
                             repeat_index <
                             program->cmds[cmd_index].value.simple_count;
                             repeat_index++) {
                                out[str_index++] = cmd_type_to_char(
                                    program->cmds[cmd_index].type);
                        }
                        break;
                case CMD_JUMP_FORWARD:
                case CMD_JUMP_BACK:
                        out[str_index++] =
                            cmd_type_to_char(program->cmds[cmd_index].type);
                        break;
                default:
                        fprintf(stderr, "Unrecognised cmd_type '%c'\n",
                                program->cmds[cmd_index].type);
                        exit(1);
                }
        }
        out[program_str_len] = '\0';
        return out;
}

char program_contains_output(struct program *program) {
        for (size_t cmd_index = 0; cmd_index < program->length; cmd_index++) {
                if (program->cmds[cmd_index].type == CMD_SIMPLE_OUTPUT) {
                        return 1;
                }
        }
        return 0;
}

char program_contains_input(struct program *program) {
        for (size_t cmd_index = 0; cmd_index < program->length; cmd_index++) {
                if (program->cmds[cmd_index].type == CMD_SIMPLE_INPUT) {
                        return 1;
                }
        }
        return 0;
}
