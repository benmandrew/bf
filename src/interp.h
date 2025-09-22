struct context_t {
        unsigned int pc;
        char *program;
        unsigned int program_len;
        unsigned int dp;
        unsigned char *data;
};

struct context_t init_context(char *);
int interp(struct context_t *ctx, int);
