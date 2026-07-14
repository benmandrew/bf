#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "cfg_dot.h"
#include "ir.h"
#include "llvm.h"
#include "read.h"

void print_usage(const char *program_name) {
        printf("Usage: %s [OPTIONS] <input_file>\n", program_name);
        printf("Options:\n");
        printf("  -O, --optimise       Enable optimisations\n");
        printf(
            "  -L, --label-blocks   Append Brainfuck source spans to basic\n");
        printf("                       block names (for CFG inspection)\n");
        printf(
            "  -C, --emit-cfg-dot   Emit the control-flow graph as Graphviz\n");
        printf("                       dot instead of LLVM IR\n");
        printf("  -h, --help           Show this help message\n");
        printf("\nArguments:\n");
        printf("  input_file           Brainfuck source file to compile\n");
}

static struct option long_options[] = {{"optimise", no_argument, 0, 'O'},
                                       {"label-blocks", no_argument, 0, 'L'},
                                       {"emit-cfg-dot", no_argument, 0, 'C'},
                                       {"help", no_argument, 0, 'h'},
                                       {0, 0, 0, 0}};

int parse_options(int argc, char **argv, bool *optimise, bool *label_blocks,
                  bool *emit_cfg, char **program) {
        int opt;
        while ((opt = getopt_long(argc, argv, "OLCh", long_options, NULL)) !=
               -1) {
                switch (opt) {
                case 'O':
                        *optimise = true;
                        break;
                case 'L':
                        *label_blocks = true;
                        break;
                case 'C':
                        *emit_cfg = true;
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
                *program = malloc(8192);
                ssize_t result = read(STDIN_FILENO, *program, 8192);
                if (result == -1) {
                        fprintf(stderr, "Error reading from stdin\n");
                        return 1;
                }
                (*program)[result] = '\0';
                clean_whitespace(*program);
                return 0;
        }
        if (optind + 1 < argc) {
                fprintf(stderr, "Error: Too many arguments\n\n");
                print_usage(argv[0]);
                return 1;
        }
        char *program_file = argv[optind];
        struct ReadReturn read_result = read_file(program_file);
        if (read_result.type == ERROR) {
                fprintf(stderr, "Error: %s\n", read_result.value.error.message);
                return 1;
        }
        *program = read_result.value.program_str;
        return 0;
}

int main(int argc, char **argv) {
        bool optimise = false;
        bool label_blocks = false;
        bool emit_cfg = false;
        char *program_str = NULL;
        if (parse_options(argc, argv, &optimise, &label_blocks, &emit_cfg,
                          &program_str) != 0) {
                return 1;
        }
        struct program parsed_program = string_to_program(program_str);
        free(program_str);
        optimise_program(&parsed_program);
        LLVMModuleRef module =
            generate(&parsed_program, optimise, label_blocks);
        if (emit_cfg) {
                emit_cfg_dot(module);
        } else {
                char *err = NULL;
                LLVMPrintModuleToFile(module, "/dev/stdout", &err);
                if (err)
                        LLVMDisposeMessage(err);
        }
        dispose_module(module);
        free(parsed_program.cmds);
        return 0;
}
