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
/// @param source_str Null-terminated source buffer to clean in place.
void clean_whitespace(char *source_str);

/// Read a file, validate it, and return normalized source.
/// @param fname Path to the source file.
/// @return Tagged result containing normalized source or an error.
struct ReadReturn read_file(char *fname);

/// Validate and normalize a raw program buffer before parsing it.
/// @param program Mutable source buffer.
/// @param source_len Number of bytes to validate from `program`.
/// @return Tagged result containing normalized source or an error.
struct ReadReturn validate(char *program, size_t source_len);

#endif
