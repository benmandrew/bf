#ifndef IR_H
#define IR_H

#include <stddef.h>

/// Brainfuck command categories used by the internal IR.
enum cmd_type {
        /// `'+'`: increment current cell value.
        CMD_SIMPLE_INC,
        /// `'-'`: decrement current cell value.
        CMD_SIMPLE_DEC,
        /// `'>'`: move data pointer right.
        CMD_SIMPLE_RIGHT,
        /// `'<'`: move data pointer left.
        CMD_SIMPLE_LEFT,
        /// `'.'`: write current cell as output.
        CMD_SIMPLE_OUTPUT,
        /// `','`: read input into current cell.
        CMD_SIMPLE_INPUT,
        /// `'['`: jump forward if current cell is zero.
        CMD_JUMP_FORWARD,
        /// `']'`: jump back if current cell is non-zero.
        CMD_JUMP_BACK,
};

/// One compressed instruction in the internal Brainfuck IR.
struct cmd {
        /// Command opcode.
        enum cmd_type type;
        union {
                /// Repeat count for simple commands.
                size_t simple_count;
                /// Matching bracket command index.
                size_t jump_index;
        };
};

/// Parsed Brainfuck program represented as an array of commands.
struct program {
        /// Heap-allocated command array.
        struct cmd *cmds;
        /// Number of entries in `cmds`.
        size_t length;
};

/// Compute the length of the flattened Brainfuck source string.
size_t program_str_length(struct program *p);
/// Parse a cleaned Brainfuck string into the internal program form.
struct program string_to_program(char *s);
/// Release a program's heap-allocated command buffer.
void free_program(struct program *p);
/// Map a command type back to its Brainfuck character.
char cmd_type_to_char(enum cmd_type t);
/// Expand a compressed program back into a Brainfuck source string.
char *program_to_string(struct program *program);

/// Return whether a program contains any output commands.
char program_contains_output(struct program *p);
/// Return whether a program contains any input commands.
char program_contains_input(struct program *p);

/// Validate that a source string contains only balanced Brainfuck commands.
char program_is_valid(char *s);

#endif
