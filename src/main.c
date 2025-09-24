#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "interp.h"
#include "read.h"

void print_usage(const char *program_name) {
        printf("Usage: %s [OPTIONS] <input_file>\n", program_name);
        printf("Options:\n");
        printf("  -b, --byte-output    Output bytes as numbers instead of "
               "characters\n");
        printf("  -h, --help          Show this help message\n");
        printf("\nArguments:\n");
        printf("  input_file          Brainfuck source file to execute\n");
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
                case '?':
                        print_usage(argv[0]);
                        return 1;
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
        char *program_file = argv[optind];
        if (optind + 1 < argc) {
                fprintf(stderr, "Error: Too many arguments\n\n");
                print_usage(argv[0]);
                return 1;
        }
        *program = read_file(program_file);
        if (!*program) {
                fprintf(stderr, "Error: Could not read file '%s'\n",
                        program_file);
                return 1;
        }
        return 0;
}

int main(int argc, char **argv) {
        bool byte_output = false;
        char *program = NULL;
        if (parse_options(argc, argv, &byte_output, &program) != 0) {
                return 1;
        }
        struct context_t ctx = init_context(program);
        while (!interp(&ctx, STDOUT_FILENO, STDIN_FILENO, byte_output))
                ;
        free(program);
        printf("\n");
        return 0;
}
