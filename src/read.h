#ifndef READ_H
#define READ_H

#include <stdlib.h>

/// Validation error payload returned by file/parse helpers.
struct Error {
        /// Human-readable error message.
        char *message;
};

union ProgramOrError {
        /// Normalized source on success.
        char *program_str;
        /// Error payload on failure.
        struct Error error;
};

/// Discriminator values for ReadReturn.
enum ReadResultType {
        /// Read and validation succeeded.
        OK,
        /// Read and validation failed.
        ERROR,
};

/// Tagged return type for reading and validating source input.
struct ReadReturn {
        /// Result discriminator.
        enum ReadResultType type;
        /// Result payload.
        union ProgramOrError value;
};

/// Remove non-Brainfuck characters from a mutable source buffer.
void clean_whitespace(char *s);
/// Read a file, validate it, and return normalized source.
struct ReadReturn read_file(char *fname);
/// Validate and normalize a raw program buffer before parsing it.
struct ReadReturn validate(char *program, size_t len);

#endif
