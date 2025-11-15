#ifndef IR_H
#define IR_H

#include <stddef.h>

enum cmd_type {
        CMD_SIMPLE_INC,     // '+'
        CMD_SIMPLE_DEC,     // '-'
        CMD_SIMPLE_RIGHT,   // '>'
        CMD_SIMPLE_LEFT,    // '<'
        CMD_SIMPLE_OUTPUT,  // '.'
        CMD_SIMPLE_INPUT,   // ','
        CMD_JUMP_FORWARD,   // '['
        CMD_JUMP_BACK,      // ']'
};

struct cmd {
        enum cmd_type type;
        union {
                size_t simple_count;
                size_t jump_index;
        };
};

struct program {
        struct cmd *cmds;
        size_t length;
};

size_t program_str_length(struct program *p);
struct program string_to_program(char *s);
void free_program(struct program *p);
char cmd_type_to_char(enum cmd_type t);
char *program_to_string(struct program *program);

#endif
