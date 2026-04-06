#ifndef INTERP_H
#define INTERP_H

#include <stdbool.h>
#include <unistd.h>

#include "common.h"
#include "ir.h"

/// Mutable execution state for the Brainfuck interpreter.
struct context_t {
        /// Current command index in `p.cmds`.
        size_t pc;
        /// Parsed program being executed.
        struct program p;
        /// Current data pointer position.
        size_t dp;
        /// Interpreter data tape.
        unsigned char data[DATA_SIZE];
        /// Largest data pointer reached for pretty-printing.
        size_t max_dp;
};

/// Initialize an interpreter context with the program loaded at pc zero.
struct context_t init_context(struct program p);
/// Execute the command at the current program counter and advance execution.
int interp(struct context_t *ctx, int, int, bool);
/// Render the current interpreter state as a human-readable trace string.
char *context_to_string(struct context_t *ctx);

#endif
