#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "read.h"

#define IS_BF_CHAR(c)                                                          \
        ((c) == '+' || (c) == '-' || (c) == '>' || (c) == '<' || (c) == '.' || \
         (c) == ',' || (c) == '[' || (c) == ']')

// Clean whitespace and other extraneous characters from a BF program
char *clean_whitespace(char *s) {
        int64_t len = 0, i = 0;
        while (s[i] != '\0') {
                if (IS_BF_CHAR(s[i])) {
                        len++;
                }
                i++;
        }
        char *program = malloc(len + 1);
        int64_t j = 0;
        i = 0;
        while (s[i] != '\0') {
                if (IS_BF_CHAR(s[i])) {
                        program[j] = s[i];
                        j++;
                }
                i++;
        }
        program[j] = '\0';
        assert(j == len);
        return program;
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
        char *s = malloc(len + 1);
        if (fread(s, len, 1, f) == 0) {
                fprintf(stderr, "Reading '%s' failed\n", fname);
                exit(1);
        }
        fclose(f);
        s[len] = '\0';
        char *program = clean_whitespace(s);
        free(s);
        return program;
}
