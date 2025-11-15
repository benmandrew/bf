#ifndef INTERP_H
#define INTERP_H

#include <stdbool.h>
#include <unistd.h>

#include "common.h"
#include "ir.h"

struct context_t {
        size_t pc;
        struct program p;
        size_t dp;
        unsigned char data[DATA_SIZE];
        // Keep track of the largest data pointer seen so far,
        // for pretty-printing the context
        size_t max_dp;
};

struct context_t init_context(struct program p);
int interp(struct context_t *ctx, int, int, bool);
char *context_to_string(struct context_t *ctx);

#endif
