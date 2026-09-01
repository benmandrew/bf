#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "read.h"
#include "wasm_api.h"

void print_usage(const char *program_name) {
        printf("Usage: %s [OPTIONS] <input_file>\n", program_name);
        printf("Options:\n");
        printf("  -U, --unoptimised    Skip the LLVM optimisation\n");
        printf("                       pipeline; it runs by default.\n");
        printf("                       Without it, a bounds check splits\n");
        printf("                       every pointer move into a block\n");
        printf("  -L, --label-blocks   Append bf source spans to basic\n");
        printf("                       block names (for CFG inspection).\n");
        printf("                       Pair with -U: simplifycfg merges and\n");
        printf("                       renames the blocks it labels\n");
        printf(
            "  -C, --emit-cfg-dot   Emit the control-flow graph as Graphviz\n");
        printf("                       dot instead of LLVM IR\n");
        printf("      --cfg-instructions   Include each block's LLVM\n");
        printf("                       instructions in the --emit-cfg-dot "
               "graph\n");
        printf("  -h, --help           Show this help message\n");
        printf("\nArguments:\n");
        printf("  input_file           bf source file to compile\n");
}

static struct option long_options[] = {
    {"unoptimised", no_argument, 0, 'U'},
    {"label-blocks", no_argument, 0, 'L'},
    {"emit-cfg-dot", no_argument, 0, 'C'},
    {"cfg-instructions", no_argument, 0, 'I'},
    {"help", no_argument, 0, 'h'},
    {0, 0, 0, 0}};

int parse_options(int argc, char **argv, bool *optimise, bool *label_blocks,
                  bool *emit_cfg, bool *cfg_instructions, char **program) {
        int opt;
        while ((opt = getopt_long(argc, argv, "ULCh", long_options, NULL)) !=
               -1) {
                switch (opt) {
                case 'U':
                        *optimise = false;
                        break;
                case 'L':
                        *label_blocks = true;
                        break;
                case 'C':
                        *emit_cfg = true;
                        break;
                case 'I':
                        *cfg_instructions = true;
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
        // Optimisations run unless -U asks otherwise. Every pointer move
        // carries a bounds check, so an unoptimised control-flow graph is
        // mostly check blocks; the pipeline folds the redundant ones away.
        bool optimise = true;
        bool label_blocks = false;
        bool emit_cfg = false;
        bool cfg_instructions = false;
        char *program_str = NULL;
        if (parse_options(argc, argv, &optimise, &label_blocks, &emit_cfg,
                          &cfg_instructions, &program_str) != 0) {
                return 1;
        }
        char *output = emit_cfg
                           ? bf_compile_cfg_dot(program_str, optimise,
                                                label_blocks, cfg_instructions)
                           : bf_compile_ir(program_str, optimise, label_blocks);
        free(program_str);
        fputs(output, stdout);
        bf_free(output);
        return 0;
}
