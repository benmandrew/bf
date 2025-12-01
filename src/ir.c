#include "ir.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define JUMP_STACK_MAX_SIZE (128)

struct jump_stack_frame {
        struct cmd *c;
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

static void jump_stack_push(struct jump_stack *js, struct cmd *c,
                            size_t index) {
        assert(js->head < JUMP_STACK_MAX_SIZE - 1);
        js->stack[js->head] = (struct jump_stack_frame){
            .c = c,
            .index = index,
        };
        js->head++;
}

static struct jump_stack_frame jump_stack_pop(struct jump_stack *js) {
        assert(js->head > 0);
        js->head--;
        return js->stack[js->head];
}

size_t program_str_length(struct program *p) {
        size_t length = 0;
        for (size_t i = 0; i < p->length; i++) {
                switch (p->cmds[i].type) {
                case CMD_SIMPLE_INC:
                case CMD_SIMPLE_DEC:
                case CMD_SIMPLE_RIGHT:
                case CMD_SIMPLE_LEFT:
                case CMD_SIMPLE_OUTPUT:
                case CMD_SIMPLE_INPUT:
                        length += p->cmds[i].simple_count;
                        break;
                case CMD_JUMP_FORWARD:
                case CMD_JUMP_BACK:
                        length++;
                        break;
                default:
                        fprintf(stderr, "Unrecognised cmd_type '%c'\n",
                                p->cmds[i].type);
                        exit(1);
                }
        }
        return length;
}

size_t n_simple_consecutive(char *s, size_t start, struct cmd *c) {
        size_t i = 0;
        char first = s[start];
        switch (first) {
        case '+':
                *c = (struct cmd){.type = CMD_SIMPLE_INC, .simple_count = 1};
                break;
        case '-':
                *c = (struct cmd){.type = CMD_SIMPLE_DEC, .simple_count = 1};
                break;
        case '>':
                *c = (struct cmd){.type = CMD_SIMPLE_RIGHT, .simple_count = 1};
                break;
        case '<':
                *c = (struct cmd){.type = CMD_SIMPLE_LEFT, .simple_count = 1};
                break;
        case '.':
                *c = (struct cmd){.type = CMD_SIMPLE_OUTPUT, .simple_count = 1};
                break;
        case ',':
                *c = (struct cmd){.type = CMD_SIMPLE_INPUT, .simple_count = 1};
                break;
        default:
                fprintf(stderr, "Invalid character '%c'\n", s[i]);
                exit(1);
        }
        size_t len = strlen(s);
        while (s[start + i + 1] == first && start + i + 1 < len) {
                i++;
                c->simple_count++;
        }
        return c->simple_count - 1;
}

struct program string_to_program(char *s) {
        size_t max_cmds = strlen(s);
        struct cmd *cmd_arena = malloc(max_cmds * sizeof(struct cmd));
        if (!cmd_arena) {
                fprintf(stderr, "Memory allocation failed\n");
                exit(1);
        }
        struct jump_stack js = jump_stack_new();
        struct jump_stack_frame back_jump_frame;
        size_t program_len = strlen(s);
        size_t arena_i = 0, str_i = 0;
        for (str_i = 0; str_i < program_len; str_i++) {
                switch (s[str_i]) {
                case '+':
                case '-':
                case '>':
                case '<':
                case '.':
                case ',':
                        str_i +=
                            n_simple_consecutive(s, str_i, &cmd_arena[arena_i]);
                        break;
                case '[':
                        cmd_arena[arena_i] =
                            (struct cmd){.type = CMD_JUMP_FORWARD};
                        jump_stack_push(&js, &cmd_arena[arena_i], arena_i);
                        break;
                case ']':
                        back_jump_frame = jump_stack_pop(&js);
                        cmd_arena[arena_i] =
                            (struct cmd){.type = CMD_JUMP_BACK,
                                         .jump_index = back_jump_frame.index};
                        back_jump_frame.c->jump_index = arena_i;
                        break;
                default:
                        fprintf(stderr, "Invalid character '%c'\n", s[str_i]);
                        exit(1);
                }
                arena_i++;
        }
        assert(js.head == 0);
        return (struct program){.cmds = cmd_arena, .length = arena_i};
}

char cmd_type_to_char(enum cmd_type t) {
        switch (t) {
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
                fprintf(stderr, "Unrecognised cmd_type '%c'\n", t);
                exit(1);
        }
}

char *program_to_string(struct program *program) {
        size_t program_str_len = program_str_length(program);
        char *out = malloc(program_str_len * sizeof(char) + 1);
        size_t str_i = 0;
        for (size_t i = 0; i < program->length; i++) {
                switch (program->cmds[i].type) {
                case CMD_SIMPLE_INC:
                case CMD_SIMPLE_DEC:
                case CMD_SIMPLE_RIGHT:
                case CMD_SIMPLE_LEFT:
                case CMD_SIMPLE_OUTPUT:
                case CMD_SIMPLE_INPUT:
                        for (size_t j = 0; j < program->cmds[i].simple_count;
                             j++) {
                                out[str_i++] =
                                    cmd_type_to_char(program->cmds[i].type);
                        }
                        break;
                case CMD_JUMP_FORWARD:
                case CMD_JUMP_BACK:
                        out[str_i++] = cmd_type_to_char(program->cmds[i].type);
                        break;
                default:
                        fprintf(stderr, "Unrecognised cmd_type '%c'\n",
                                program->cmds[i].type);
                        exit(1);
                }
        }
        out[program_str_len] = '\0';
        return out;
}

char program_contains_output(struct program *p) {
        for (size_t i = 0; i < p->length; i++) {
                if (p->cmds[i].type == CMD_SIMPLE_OUTPUT) {
                        return 1;
                }
        }
        return 0;
}

char program_contains_input(struct program *p) {
        for (size_t i = 0; i < p->length; i++) {
                if (p->cmds[i].type == CMD_SIMPLE_INPUT) {
                        return 1;
                }
        }
        return 0;
}
