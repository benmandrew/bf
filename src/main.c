#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "interp.h"
#include "read.h"

int main(int argc, char **argv) {
        if (argc < 2) {
                printf("Input file must be passed!");
                return 1;
        }
        char *program = read_file(argv[1]);
        struct context_t ctx = init_context(program);
        while (!interp(&ctx))
                ;
        free(program);
        printf("\n");
        return 0;
}
