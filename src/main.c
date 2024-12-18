#include <stdio.h>

struct context_t {
  unsigned int pc;
  char program[256];
  unsigned int dp;
  char data[256];
};

struct context_t init_context(char program[256]) {
  return (struct context_t) {
    .pc = 0,
    .program = *program,
    .dp = 0,
    .data = { 0 }
  };
}

void interp(struct context_t* ctx) {
  char c = ctx->program[ctx->pc];
  printf("bruh %c\n", c);
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
      ctx->data[ctx->pc]++;
      break;
    case ']':
      ctx->data[ctx->pc]++;
      break;
    case '.':
      printf("%d\n", ctx->data[ctx->pc]);
      break;
    case ',':
      printf("%d\n", ctx->data[ctx->pc]);
      break;
    default:
      printf("Invalid character '%c'\n", c);
      break;
  }
  ctx->pc++;
}

int main(int argc, char** argv) {
  struct context_t ctx = init_context("++.");
  interp(&ctx);
  interp(&ctx);
  interp(&ctx);
  return 0;
}

