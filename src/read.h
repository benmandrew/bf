#ifndef READ_H
#define READ_H

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

void clean_whitespace(char *);
struct ReadReturn *read_file(char *);

#endif
