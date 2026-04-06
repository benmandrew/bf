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

/** Compute the length of the flattened Brainfuck source string. */
size_t program_str_length(struct program *p);
/** Parse a cleaned Brainfuck string into the internal program form. */
struct program string_to_program(char *s);
/** Release a program's heap-allocated command buffer. */
void free_program(struct program *p);
/** Map a command type back to its Brainfuck character. */
char cmd_type_to_char(enum cmd_type t);
/** Expand a compressed program back into a Brainfuck source string. */
char *program_to_string(struct program *program);

/** Return whether a program contains any output commands. */
char program_contains_output(struct program *p);
/** Return whether a program contains any input commands. */
char program_contains_input(struct program *p);

/** Validate that a source string contains only balanced Brainfuck commands. */
char program_is_valid(char *s);

#endif
