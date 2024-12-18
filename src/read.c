#include <stdio.h>
#include <stdlib.h>

char* read_file(char* fname) {
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
  s[len] = 0;
  return s;
}

