#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

char program_has_valid_chars(char *s) {
        size_t program_len = strlen(s);
        for (size_t str_i = 0; str_i < program_len; str_i++) {
                switch (s[str_i]) {
                case '+':
                case '-':
                case '>':
                case '<':
                case '.':
                case ',':
                case '[':
                case ']':
                case '\n':
                case '\r':
                case '\t':
                case '\v':
                case '\f':
                case ' ':
                case '\0':
                        break;
                default:
                        return 0;
                }
        }
        return 1;
}

char program_has_balanced_jumps(char *s) {
        size_t program_len = strlen(s);
        size_t jump_stack_size = 0;
        for (size_t str_i = 0; str_i < program_len; str_i++) {
                switch (s[str_i]) {
                case '[':
                        jump_stack_size++;
                        break;
                case ']':
                        if (jump_stack_size == 0) {
                                return 0;
                        }
                        jump_stack_size--;
                        break;
                default:
                        break;
                }
        }
        return jump_stack_size == 0;
}

struct ReadReturn *validate(char *program) {
        struct ReadReturn *result = malloc(sizeof(struct ReadReturn));
        if (!program_has_valid_chars(program)) {
                free(program);
                result->type = ERROR;
                result->value.error.message =
                    "Program contains invalid characters";
                return result;
        }
        if (!program_has_balanced_jumps(program)) {
                free(program);
                result->type = ERROR;
                result->value.error.message = "Program has unbalanced jumps";
                return result;
        }
        clean_whitespace(program);
        result->type = OK;
        result->value.program_str = program;
        return result;
}

struct ReadReturn *read_file(char *fname) {
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
        return validate(program);
}
