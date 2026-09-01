#ifndef IR_H
#define IR_H

#include <stddef.h>

/// bf command categories used by the internal IR.
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
        /// Synthetic: multiply-add loop (replaces `[-offset1*factor1...]`).
        CMD_MULTIPLY,
};

/// Maximum number of target cells in a CMD_MULTIPLY instruction.
#define MULTIPLY_MOVES_MAX 8

/// One (offset, factor) pair in a CMD_MULTIPLY instruction.
struct multiply_move {
        /// Cell offset from the current data pointer.
        int offset;
        /// Multiplier applied to the loop counter cell.
        int factor;
};

/// One compressed instruction in the internal bf IR.
struct cmd {
        /// Command opcode.
        enum cmd_type type;
        /// Command payload data.
        union {
                /// Repeat count for simple commands.
                size_t simple_count;
                /// Matching bracket command index.
                size_t jump_index;
                /// Moves for CMD_MULTIPLY.
                struct {
                        struct multiply_move moves[MULTIPLY_MOVES_MAX];
                        size_t n_moves;
                } multiply;
        } value;
};

/// Parsed bf program represented as an array of commands.
struct program {
        /// Heap-allocated command array.
        struct cmd *cmds;
        /// Number of entries in `cmds`.
        size_t length;
};

/// Compute the length of the flattened bf source string.
/// @param program Parsed program.
/// @return Length of expanded bf source string.
size_t program_str_length(struct program *program);

/// Parse a cleaned bf string into the internal program form.
/// @param source_str Cleaned bf source string.
/// @return Parsed program with heap-allocated command array.
struct program string_to_program(char *source_str);

/// Map a command type back to its bf character.
/// @param command_type Command type.
/// @return Corresponding bf symbol.
char cmd_type_to_char(enum cmd_type command_type);

/// Expand a compressed program back into a bf source string.
/// @param program Parsed program.
/// @return Heap-allocated source string; caller must free it.
char *program_to_string(struct program *program);

/// Render a command range as a bounded bf snippet for use in a label.
/// Unlike program_to_string(), synthetic commands are rendered rather than
/// skipped (CMD_CLEAR as `[-]`, CMD_MULTIPLY as `[mul]`), and output longer
/// than `out_size` is truncated with a trailing `...`.
/// @param program Parsed program.
/// @param start First command index, inclusive.
/// @param end One past the last command index.
/// @param out Destination buffer, always NUL-terminated on return.
/// @param out_size Size of `out` in bytes, including the terminator; min 4.
void program_range_to_label(struct program *program, size_t start, size_t end,
                            char *out, size_t out_size);

/// Return whether a program contains any output commands.
/// @param program Parsed program.
/// @return 1 if output exists; otherwise 0.
char program_contains_output(struct program *program);

/// Return whether a program contains any input commands.
/// @param program Parsed program.
/// @return 1 if input exists; otherwise 0.
char program_contains_input(struct program *program);

/// Apply IR-level optimisations to a parsed program in-place.
/// @param program Program to optimise.
void optimise_program(struct program *program);

#endif
