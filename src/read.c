#include <stdio.h>
#include <stdlib.h>

// Clean whitespace and other extraneous characters from a BF program
char *clean(char *s) {
        long len = 0, i = 0;
        while (s[i] != '\0') {
                if (s[i] == '+' || s[i] == '-' || s[i] == '>' || s[i] == '<' ||
                    s[i] == '.' || s[i] == ',' || s[i] == '[' || s[i] == ']') {
                        len++;
                }
                i++;
        }
        char *program = malloc(len + 1);
        long j = 0;
        i = 0;
        while (s[i] != '\0') {
                if (s[i] == '+' || s[i] == '-' || s[i] == '>' || s[i] == '<' ||
                    s[i] == '.' || s[i] == ',' || s[i] == '[' || s[i] == ']') {
                        program[j] = s[i];
                        j++;
                }
                i++;
        }
        program[j + 1] = '\0';
        return program;
}

char *read_file(char *fname) {
        FILE *f = fopen(fname, "r");
        if (f == NULL) {
                printf("Opening '%s' failed\n", fname);
        }
        fseek(f, 0, SEEK_END);
        long len = ftell(f);
        fseek(f, 0, SEEK_SET);
        char *s = malloc(len + 1);
        fread(s, len, 1, f);
        fclose(f);
        s[len] = '\0';
        char *program = clean(s);
        free(s);
        return program;
}
