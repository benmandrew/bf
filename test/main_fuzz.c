#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ir.h"
#include "llvm.h"
#include "read.h"

#define MAX_INPUT_SIZE 8192

int main(int argc, char **argv) {
        static char input[MAX_INPUT_SIZE + 1];
        ssize_t input_len;
        (void)argc;
        (void)argv;

#ifdef __AFL_HAVE_MANUAL_CONTROL
        __AFL_INIT();
#endif

#ifdef __AFL_LOOP
        while (__AFL_LOOP(10000)) {
#else
        while (1) {
#endif
                input_len = read(STDIN_FILENO, input, MAX_INPUT_SIZE);
                if (input_len <= 0) {
                        break;
                }
                input[input_len] = '\0';
                clean_whitespace(input);
                struct ReadReturn r = validate(input, strlen(input));
                if (r.type == ERROR)
                        continue;
                struct program p = string_to_program(r.value.program_str);
                optimise_program(&p);
                LLVMModuleRef module = generate(&p, false);
                char *module_str = LLVMPrintModuleToString(module);
                LLVMDisposeMessage(module_str);
                dispose_module(module);
                free(p.cmds);
        }
        return 0;
}
