#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ir.h"

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

struct program string_to_program(char *s) {
        size_t max_cmds = strlen(s);
        struct cmd *cmd_arena = malloc(max_cmds * sizeof(struct cmd));
        if (!cmd_arena) {
                fprintf(stderr, "Memory allocation failed\n");
                exit(1);
        }
        struct jump_stack js = jump_stack_new();
        struct jump_stack_frame back_jump_frame;
        int program_len = strlen(s);
        int arena_i = 0;
        for (int i = 0; i < program_len; i++) {
                switch (s[i]) {
                case '+':
                        cmd_arena[arena_i] = (struct cmd){
                            .type = CMD_SIMPLE_INC, .simple_count = 1};
                        break;
                case '-':
                        cmd_arena[arena_i] = (struct cmd){
                            .type = CMD_SIMPLE_DEC, .simple_count = 1};
                        break;
                case '>':
                        cmd_arena[arena_i] = (struct cmd){
                            .type = CMD_SIMPLE_RIGHT, .simple_count = 1};
                        break;
                case '<':
                        cmd_arena[arena_i] = (struct cmd){
                            .type = CMD_SIMPLE_LEFT, .simple_count = 1};
                        break;
                case '.':
                        cmd_arena[arena_i] = (struct cmd){
                            .type = CMD_SIMPLE_OUTPUT, .simple_count = 1};
                        break;
                case ',':
                        cmd_arena[arena_i] = (struct cmd){
                            .type = CMD_SIMPLE_INPUT, .simple_count = 1};
                        break;
                case '[':
                        cmd_arena[arena_i] = (struct cmd){
                            .type = CMD_JUMP_FORWARD, .jump_index = 0};
                        jump_stack_push(&js, &cmd_arena[arena_i], arena_i);
                        break;
                case ']':
                        back_jump_frame = jump_stack_pop(&js);
                        cmd_arena[arena_i] =
                            (struct cmd){.type = CMD_JUMP_BACK,
                                         .jump_index = back_jump_frame.index};
                        back_jump_frame.c->jump_index = i;
                        break;
                default:
                        fprintf(stderr, "Invalid character '%c'\n", s[i]);
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
        char *out = malloc(program->length * sizeof(char) + 1);
        for (size_t i = 0; i < program->length; i++) {
                out[i] = cmd_type_to_char(program->cmds[i].type);
        }
        out[program->length] = '\0';
        return out;
}
