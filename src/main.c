#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

struct context_t {
  unsigned int pc;
  char* program;
  unsigned int program_len;
  unsigned int dp;
  char* data;
};

struct context_t init_context(char* program) {
  char* data = malloc(256 * sizeof(char));
  for (int i = 0; i < 256; i++) {
    data[i] = 0;
  }
  return (struct context_t) {
    .pc = 0,
    .program = program,
    .program_len = strlen(program),
    .dp = 0,
    .data = data
  };
}

int interp(struct context_t* ctx) {
  if (ctx->pc + 1 >= ctx->program_len) {
    return 1;
  }
  char c = ctx->program[ctx->pc];
  char c_in;
  switch (c) {
    case '+':
      ctx->data[ctx->dp]++;
      break;
    case '-':
      ctx->data[ctx->dp]--;
      break;
    case '>':
      ctx->dp++;
      break;
    case '<':
      ctx->dp--;
      break;
    case '[':
      if (ctx->data[ctx->dp] == 0) {
        while (ctx->program[ctx->pc] != ']') {
          ctx->pc++;
        }
      }
      break;
    case ']':
      while (ctx->program[ctx->pc] != '[') {
        ctx->pc--;
      }
      ctx->pc--;
      break;
    case '.':
      printf("%c", ctx->data[ctx->dp]);
      break;
    case ',':
      if (read(STDIN_FILENO, &c_in, 1) > 0) {
        ctx->data[ctx->dp] = c_in;
      }
      break;
    default:
      printf("Invalid character '%c'\n", c);
      break;
  }
  ctx->pc++;
  return 0;
}

int main(int argc, char** argv) {
  struct context_t ctx = init_context(">++++++++[<+++++++++>-]<.>++++[<+++++++>-]<+.+++++++..+++.>>++++++[<+++++++>-]<++.------------.>++++++[<+++++++++>-]<+.<.+++.------.--------.>>>++++[<++++++++>-]<+.");
  while (!interp(&ctx));
  return 0;
}

