#ifndef INTERP_H
#define INTERP_H

#include <stdbool.h>
#include <unistd.h>

struct context_t {
        size_t pc;
        char *program;
        size_t program_len;
        size_t dp;
        unsigned char *data;
};

struct context_t init_context(char *);
int interp(struct context_t *ctx, int, int, bool);
char *context_to_string(struct context_t *ctx);

#endif
