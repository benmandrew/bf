#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "interp.h"
#include "ir.h"
#include "llvm.h"
#include "read.h"

void print_usage(const char *program_name) {
        printf("Usage: %s [OPTIONS] <input_file>\n", program_name);
        printf("Options:\n");
        printf("  -b, --byte-output    Output bytes as numbers instead of "
               "characters\n");
        printf("  -h, --help           Show this help message\n");
        printf("\nArguments:\n");
        printf("  input_file           Brainfuck source file to execute\n");
}

static struct option long_options[] = {{"byte-output", no_argument, 0, 'b'},
                                       {"help", no_argument, 0, 'h'},
                                       {0, 0, 0, 0}};

int parse_options(int argc, char **argv, bool *byte_output, char **program) {
        int opt;
        while ((opt = getopt_long(argc, argv, "bh", long_options, NULL)) !=
               -1) {
                switch (opt) {
                case 'b':
                        *byte_output = true;
                        break;
                case 'h':
                        print_usage(argv[0]);
                        exit(0);
                default:
                        print_usage(argv[0]);
                        return 1;
                }
        }
        if (optind >= argc) {
                fprintf(stderr, "Error: Input file must be provided\n\n");
                print_usage(argv[0]);
                return 1;
        }
        if (optind + 1 < argc) {
                fprintf(stderr, "Error: Too many arguments\n\n");
                print_usage(argv[0]);
                return 1;
        }
        char *program_file = argv[optind];
        struct ReadReturn *read_result = read_file(program_file);
        if (read_result->type == ERROR) {
                fprintf(stderr, "Error: %s\n",
                        read_result->value.error.message);
                return 1;
        }
        *program = read_result->value.program_str;
        free(read_result);
        return 0;
}

int main(int argc, char **argv) {
        bool byte_output = false;
        char *program_str = NULL;
        if (parse_options(argc, argv, &byte_output, &program_str) != 0) {
                return 1;
        }
        struct program p = string_to_program(program_str);
        free(program_str);
        struct context_t ctx = init_context(p);
        while (!interp(&ctx, STDOUT_FILENO, STDIN_FILENO, byte_output)) {
        };
        free(p.cmds);
        return 0;
}
