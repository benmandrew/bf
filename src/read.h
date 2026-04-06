#ifndef READ_H
#define READ_H

#include <stdlib.h>

struct Error {
        char *message;
};

struct ReadReturn {
        enum { OK, ERROR } type;

        union {
                char *program_str;
                struct Error error;
        } value;
};

/** Remove non-Brainfuck characters from a mutable source buffer. */
void clean_whitespace(char *s);
/** Read a program file, validate it, and return the normalized source string.
 */
struct ReadReturn read_file(char *fname);
/** Validate and normalize a raw program buffer before parsing it. */
struct ReadReturn validate(char *program, size_t len);

#endif
