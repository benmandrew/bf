#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "read.h"

// Maximum length for the test string
#define MAX_LEN 256

int main() {
        // Allocate a buffer for the program string
        char program[MAX_LEN + 1];
        __CPROVER_assume(__CPROVER_r_ok(program, MAX_LEN + 1));
        __CPROVER_assume(malloc(sizeof(struct ReadReturn)) != NULL);
        for (size_t i = 0; i < MAX_LEN; ++i) {
                unsigned char c = nondet_uchar();
                program[i] = c;
                if (c == '\0') {
                        for (size_t j = i + 1; j < MAX_LEN + 1; ++j)
                                program[j] = '\0';
                        break;
                }
        }
        program[MAX_LEN] = '\0';
        struct ReadReturn *result = validate(program, strlen(program));
        __CPROVER_assert(result != NULL,
                         "validate should return a non-NULL pointer");
        return 0;
}
