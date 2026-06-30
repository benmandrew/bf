#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "interp.h"
#include "ir.h"
#include "llvm.h"
#include "read.h"

#define MAX_INPUT_SIZE 8192
#define MAX_INTERP_STEPS 100000

int main(int argc, char **argv) {
        static char input[MAX_INPUT_SIZE + 1];
        ssize_t input_len;
        (void)argc;
        (void)argv;

        int out_fd = open("/dev/null", O_WRONLY);
        int in_fd = open("/dev/zero", O_RDONLY);

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

                LLVMModuleRef module = generate(&p, true);
                char *module_str = LLVMPrintModuleToString(module);
                LLVMDisposeMessage(module_str);
                dispose_module(module);

                struct context_t ctx = init_context(p);
                for (int steps = 0; steps < MAX_INTERP_STEPS; steps++) {
                        if (interp(&ctx, out_fd, in_fd, false))
                                break;
                }

                free(p.cmds);
        }

        close(out_fd);
        close(in_fd);
        return 0;
}
