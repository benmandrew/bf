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
        /// Synthetic: set current cell to zero (replaces `[-]`/`[+]`).
        CMD_CLEAR,
};

/// One compressed instruction in the internal Brainfuck IR.
struct cmd {
        /// Command opcode.
        enum cmd_type type;
        /// Command payload data.
        union {
                /// Repeat count for simple commands.
                size_t simple_count;
                /// Matching bracket command index.
                size_t jump_index;
        } value;
};

/// Parsed Brainfuck program represented as an array of commands.
struct program {
        /// Heap-allocated command array.
        struct cmd *cmds;
        /// Number of entries in `cmds`.
        size_t length;
};

/// Compute the length of the flattened Brainfuck source string.
/// @param program Parsed program.
/// @return Length of expanded Brainfuck source string.
size_t program_str_length(struct program *program);

/// Parse a cleaned Brainfuck string into the internal program form.
/// @param source_str Cleaned Brainfuck source string.
/// @return Parsed program with heap-allocated command array.
struct program string_to_program(char *source_str);

/// Release a program's heap-allocated command buffer.
/// @param program Program whose command array should be released.
void free_program(struct program *program);

/// Map a command type back to its Brainfuck character.
/// @param command_type Command type.
/// @return Corresponding Brainfuck symbol.
char cmd_type_to_char(enum cmd_type command_type);

/// Expand a compressed program back into a Brainfuck source string.
/// @param program Parsed program.
/// @return Heap-allocated source string; caller must free it.
char *program_to_string(struct program *program);

/// Return whether a program contains any output commands.
/// @param program Parsed program.
/// @return 1 if output exists; otherwise 0.
char program_contains_output(struct program *program);

/// Return whether a program contains any input commands.
/// @param program Parsed program.
/// @return 1 if input exists; otherwise 0.
char program_contains_input(struct program *program);

/// Validate that a source string contains only balanced Brainfuck commands.
/// @param source_str Source string to validate.
/// @return 1 if valid; otherwise 0.
char program_is_valid(char *source_str);

/// Apply IR-level optimisations to a parsed program in-place.
/// @param program Program to optimise.
void optimise_program(struct program *program);

#endif
