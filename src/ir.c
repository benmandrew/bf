#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ir.h"

#define JUMP_STACK_MAX_SIZE (128)

struct jump_stack {
        struct cmd *stack[JUMP_STACK_MAX_SIZE];
        size_t head;
};

static struct jump_stack jump_stack_new() {
        return (struct jump_stack){
            .head = 0,
        };
}

static void jump_stack_push(struct jump_stack *js, struct cmd *c) {
        assert(js->head < JUMP_STACK_MAX_SIZE - 1);
        js->stack[js->head] = c;
        js->head++;
}

static struct cmd *jump_stack_pop(struct jump_stack *js) {
        assert(js->head > 0);
        js->head--;
        return js->stack[js->head];
}

int string_to_program_aux(char *s, struct cmd *cmd_arena) {
        struct jump_stack js = jump_stack_new();
        struct cmd *back_jump;
        int program_len = strlen(s);
        int body_len = 0;
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
                            .type = CMD_JUMP_FORWARD, .jump_loc = NULL};
                        jump_stack_push(&js, &cmd_arena[arena_i]);
                        break;
                case ']':
                        back_jump = jump_stack_pop(&js);
                        cmd_arena[arena_i] = (struct cmd){
                            .type = CMD_JUMP_BACK, .jump_loc = back_jump};
                        back_jump->jump_loc = &cmd_arena[arena_i];
                        break;
                default:
                        fprintf(stderr, "Invalid character '%c'\n", s[i]);
                        exit(1);
                }
                arena_i++;
                body_len++;
        }
        return body_len;
}

struct program string_to_program(char *s) {
        size_t max_cmds = strlen(s);
        struct cmd *cmd_arena = malloc(max_cmds * sizeof(struct cmd));
        if (!cmd_arena) {
                fprintf(stderr, "Memory allocation failed\n");
                exit(1);
        }
        int length = string_to_program_aux(s, cmd_arena);
        return (struct program){.cmds = cmd_arena, .length = length};
}

char *program_to_string(struct program *program) {
        char *out = malloc(program->length * sizeof(char) + 1);
        for (size_t i = 0; i < program->length; i++) {
                switch (program->cmds[i].type) {
                case CMD_SIMPLE_INC:
                        out[i] = '+';
                        break;
                case CMD_SIMPLE_DEC:
                        out[i] = '-';
                        break;
                case CMD_SIMPLE_RIGHT:
                        out[i] = '>';
                        break;
                case CMD_SIMPLE_LEFT:
                        out[i] = '<';
                        break;
                case CMD_SIMPLE_OUTPUT:
                        out[i] = '.';
                        break;
                case CMD_SIMPLE_INPUT:
                        out[i] = ',';
                        break;
                case CMD_JUMP_FORWARD:
                        out[i] = '[';
                        break;
                case CMD_JUMP_BACK:
                        out[i] = ']';
                        break;
                }
        }
        out[program->length] = '\0';
        return out;
}
