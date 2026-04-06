#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "read.h"

#define IS_BF_CHAR(character)                                                  \
        ((character) == '+' || (character) == '-' || (character) == '>' ||     \
         (character) == '<' || (character) == '.' || (character) == ',' ||     \
         (character) == '[' || (character) == ']')

void clean_whitespace(char *source_str) {
        int64_t write_index = 0;
        int64_t read_index = 0;
        while (source_str[read_index] != '\0') {
                if (IS_BF_CHAR(source_str[read_index])) {
                        source_str[write_index] = source_str[read_index];
                        write_index++;
                }
                read_index++;
        }
        source_str[write_index] = '\0';
}

char program_has_valid_chars(char *source_str, size_t source_len) {
        for (size_t str_index = 0; str_index < source_len; str_index++) {
                switch (source_str[str_index]) {
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

char program_has_balanced_jumps(char *source_str, size_t source_len) {
        size_t jump_stack_size = 0;
        for (size_t str_index = 0; str_index < source_len; str_index++) {
                switch (source_str[str_index]) {
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

struct ReadReturn validate(char *program, size_t source_len) {
        struct ReadReturn result = {.type = OK, .value.program_str = NULL};

        if (!program_has_valid_chars(program, source_len)) {
                result.type = ERROR;
                result.value.error.message =
                    "Program contains invalid characters";
                return result;
        }
        if (!program_has_balanced_jumps(program, source_len)) {
                result.type = ERROR;
                result.value.error.message = "Program has unbalanced jumps";
                return result;
        }
        clean_whitespace(program);
        result.type = OK;
        result.value.program_str = program;
        return result;
}

struct ReadReturn read_file(char *fname) {
        FILE *file_handle = fopen(fname, "r");
        if (file_handle == NULL) {
                fprintf(stderr, "Opening '%s' failed\n", fname);
                exit(1);
        }
        fseek(file_handle, 0, SEEK_END);
        int64_t file_len = ftell(file_handle);
        if (file_len < 0) {
                fprintf(stderr, "Getting file size for '%s' failed\n", fname);
                fclose(file_handle);
                exit(1);
        }
        fseek(file_handle, 0, SEEK_SET);
        char *program = malloc(file_len + 1);
        if (fread(program, file_len, 1, file_handle) == 0) {
                fprintf(stderr, "Reading '%s' failed\n", fname);
                exit(1);
        }
        fclose(file_handle);
        program[file_len] = '\0';
        struct ReadReturn result = validate(program, strlen(program));
        if (result.type == ERROR) {
                free(program);
        }
        return result;
}
