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
                case CMD_CLEAR:
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
                case CMD_CLEAR:
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

static int are_opposing(enum cmd_type a, enum cmd_type b) {
        return (a == CMD_SIMPLE_INC && b == CMD_SIMPLE_DEC) ||
               (a == CMD_SIMPLE_DEC && b == CMD_SIMPLE_INC) ||
               (a == CMD_SIMPLE_RIGHT && b == CMD_SIMPLE_LEFT) ||
               (a == CMD_SIMPLE_LEFT && b == CMD_SIMPLE_RIGHT);
}

static enum cmd_type opposite_type(enum cmd_type t) {
        switch (t) {
        case CMD_SIMPLE_INC:
                return CMD_SIMPLE_DEC;
        case CMD_SIMPLE_DEC:
                return CMD_SIMPLE_INC;
        case CMD_SIMPLE_RIGHT:
                return CMD_SIMPLE_LEFT;
        case CMD_SIMPLE_LEFT:
                return CMD_SIMPLE_RIGHT;
        default:
                return t;
        }
}

static void cancel_opposing(struct program *program) {
        struct cmd *new_cmds = malloc(program->length * sizeof(struct cmd));
        size_t *old_to_new = malloc(program->length * sizeof(size_t));
        size_t *new_to_old = malloc(program->length * sizeof(size_t));
        if (!new_cmds || !old_to_new || !new_to_old) {
                fprintf(stderr, "Memory allocation failed\n");
                exit(1);
        }
        size_t new_len = 0;

        for (size_t old = 0; old < program->length; old++) {
                struct cmd curr = program->cmds[old];
                if (new_len > 0) {
                        struct cmd *prev = &new_cmds[new_len - 1];
                        if (are_opposing(prev->type, curr.type)) {
                                size_t pc = prev->value.simple_count;
                                size_t cc = curr.value.simple_count;
                                if (cc > pc) {
                                        prev->type = opposite_type(prev->type);
                                        prev->value.simple_count = cc - pc;
                                } else if (cc < pc) {
                                        prev->value.simple_count = pc - cc;
                                } else {
                                        old_to_new[new_to_old[new_len - 1]] =
                                            SIZE_MAX;
                                        new_len--;
                                }
                                old_to_new[old] = SIZE_MAX;
                                continue;
                        }
                }
                old_to_new[old] = new_len;
                new_to_old[new_len] = old;
                new_cmds[new_len++] = curr;
        }

        for (size_t i = 0; i < new_len; i++) {
                if (new_cmds[i].type == CMD_JUMP_FORWARD ||
                    new_cmds[i].type == CMD_JUMP_BACK) {
                        size_t old_target =
                            program->cmds[new_to_old[i]].value.jump_index;
                        assert(old_to_new[old_target] != SIZE_MAX);
                        new_cmds[i].value.jump_index = old_to_new[old_target];
                }
        }

        free(program->cmds);
        free(old_to_new);
        free(new_to_old);
        program->cmds = new_cmds;
        program->length = new_len;
}

static void detect_clear_loops(struct program *program) {
        struct cmd *new_cmds = malloc(program->length * sizeof(struct cmd));
        size_t *old_to_new = malloc(program->length * sizeof(size_t));
        size_t *new_to_old = malloc(program->length * sizeof(size_t));
        if (!new_cmds || !old_to_new || !new_to_old) {
                fprintf(stderr, "Memory allocation failed\n");
                exit(1);
        }
        size_t new_len = 0;

        for (size_t old = 0; old < program->length;) {
                struct cmd c = program->cmds[old];
                if (c.type == CMD_JUMP_FORWARD && old + 2 < program->length) {
                        struct cmd body = program->cmds[old + 1];
                        struct cmd close = program->cmds[old + 2];
                        if ((body.type == CMD_SIMPLE_INC ||
                             body.type == CMD_SIMPLE_DEC) &&
                            close.type == CMD_JUMP_BACK &&
                            c.value.jump_index == old + 2) {
                                old_to_new[old] = new_len;
                                old_to_new[old + 1] = SIZE_MAX;
                                old_to_new[old + 2] = SIZE_MAX;
                                new_to_old[new_len] = old;
                                new_cmds[new_len++] =
                                    (struct cmd){.type = CMD_CLEAR};
                                old += 3;
                                continue;
                        }
                }
                old_to_new[old] = new_len;
                new_to_old[new_len] = old;
                new_cmds[new_len++] = c;
                old++;
        }

        for (size_t i = 0; i < new_len; i++) {
                if (new_cmds[i].type == CMD_JUMP_FORWARD ||
                    new_cmds[i].type == CMD_JUMP_BACK) {
                        size_t old_target =
                            program->cmds[new_to_old[i]].value.jump_index;
                        assert(old_to_new[old_target] != SIZE_MAX);
                        new_cmds[i].value.jump_index = old_to_new[old_target];
                }
        }

        free(program->cmds);
        free(old_to_new);
        free(new_to_old);
        program->cmds = new_cmds;
        program->length = new_len;
}

void optimise_program(struct program *program) {
        cancel_opposing(program);
        detect_clear_loops(program);
}

char program_contains_input(struct program *program) {
        for (size_t cmd_index = 0; cmd_index < program->length; cmd_index++) {
                if (program->cmds[cmd_index].type == CMD_SIMPLE_INPUT) {
                        return 1;
                }
        }
        return 0;
}
