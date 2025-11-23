#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "read.h"

#define IS_BF_CHAR(c)                                                          \
        ((c) == '+' || (c) == '-' || (c) == '>' || (c) == '<' || (c) == '.' || \
         (c) == ',' || (c) == '[' || (c) == ']')

// Clean whitespace and other extraneous characters from a BF program
void clean_whitespace(char *s) {
        int64_t j = 0, i = 0;
        while (s[i] != '\0') {
                if (IS_BF_CHAR(s[i])) {
                        s[j] = s[i];
                        j++;
                }
                i++;
        }
        s[j] = '\0';
}

char *read_file(char *fname) {
        FILE *f = fopen(fname, "r");
        if (f == NULL) {
                fprintf(stderr, "Opening '%s' failed\n", fname);
                exit(1);
        }
        fseek(f, 0, SEEK_END);
        int64_t len = ftell(f);
        if (len < 0) {
                fprintf(stderr, "Getting file size for '%s' failed\n", fname);
                fclose(f);
                exit(1);
        }
        fseek(f, 0, SEEK_SET);
        char *program = malloc(len + 1);
        if (fread(program, len, 1, f) == 0) {
                fprintf(stderr, "Reading '%s' failed\n", fname);
                exit(1);
        }
        fclose(f);
        program[len] = '\0';
        clean_whitespace(program);
        return program;
}
