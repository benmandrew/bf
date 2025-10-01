#ifndef INTERP_H
#define INTERP_H

#include <stdbool.h>
#include <unistd.h>

#define DATA_SIZE (65536)

struct context_t {
        size_t pc;
        char *program;
        size_t program_len;
        size_t dp;
        unsigned char data[DATA_SIZE];
        // Keep track of the largest data pointer seen so far,
        // for pretty-printing the context
        size_t max_dp;
};

struct context_t init_context(char *);
int interp(struct context_t *ctx, int, int, bool);
char *context_to_string(struct context_t *ctx);

#endif
