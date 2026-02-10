#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "ir.h"
#include "read.h"

// Maximum length for the test string
#ifndef MAX_PROGRAM_LEN
#define MAX_PROGRAM_LEN 4
#endif

int main() {
        // Allocate a buffer for the program string
        char program[MAX_PROGRAM_LEN + 1];
        __CPROVER_assume(__CPROVER_r_ok(program, MAX_PROGRAM_LEN + 1));
        for (size_t i = 0; i < MAX_PROGRAM_LEN; ++i) {
                unsigned char c = nondet_uchar();
                __CPROVER_assume(c == '+' || c == '-' || c == '>' || c == '<' ||
                                 c == '.' || c == ',' || c == '[' || c == ']' ||
                                 c == '\n' || c == '\r' || c == '\t' ||
                                 c == '\v' || c == '\f' || c == ' ' ||
                                 c == '\0');
                program[i] = c;
                if (c == '\0') {
                        for (size_t j = i + 1; j < MAX_PROGRAM_LEN + 1; ++j)
                                program[j] = '\0';
                        break;
                }
        }
        program[MAX_PROGRAM_LEN] = '\0';
        struct ReadReturn result = validate(program, strlen(program));
        if (result.type == ERROR) {
                return 0;
        }
        struct program p = string_to_program(result.value.program_str);
        return 0;
}
