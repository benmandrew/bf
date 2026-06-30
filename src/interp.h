#ifndef INTERP_H
#define INTERP_H

#include <stdbool.h>
#include <unistd.h>

#include "common.h"
#include "ir.h"

/// Mutable execution state for the Brainfuck interpreter.
struct context_t {
        /// Current command index in `program.cmds`.
        size_t pc;
        /// Parsed program being executed.
        struct program program;
        /// Current data pointer position.
        size_t dp;
        /// Interpreter data tape.
        unsigned char data[DATA_SIZE];
        /// Largest data pointer reached for pretty-printing.
        size_t max_dp;
};

/// Initialize an interpreter context with the program loaded at pc zero.
/// @param program Parsed Brainfuck program to execute.
/// @return Initialized interpreter context with zeroed tape.
struct context_t init_context(struct program program);

/// Execute the command at the current program counter and advance execution.
/// @param ctx Interpreter context.
/// @param out_fd File descriptor used for output.
/// @param in_fd File descriptor used for input.
/// @param byte_output If true, emit numeric byte values.
/// @return 1 when execution completes normally, -1 if the tape pointer moves
///         out of bounds, 0 while still executing.
int interp(struct context_t *ctx, int, int, bool);

/// Render the current interpreter state as a human-readable trace string.
/// @param ctx Interpreter context to render.
/// @return Heap-allocated formatted string; caller must free it.
char *context_to_string(struct context_t *ctx);

#endif
