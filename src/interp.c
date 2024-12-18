#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "interp.h"

#define DATA_SIZE 65536

struct context_t init_context(char* program) {
  char* data = malloc(DATA_SIZE * sizeof(char));
  return (struct context_t) {
    .pc = 0,
    .program = program,
    .program_len = strlen(program),
    .dp = 0,
    .data = data
  };
}

void interp_l_brac(struct context_t* ctx) {
  if (ctx->data[ctx->dp] == 0) {
    while (ctx->program[ctx->pc] != ']') {
      ctx->pc++;
    }
  }
}

void interp_r_brac(struct context_t* ctx) {
      while (ctx->program[ctx->pc] != '[') {
        ctx->pc--;
      }
      ctx->pc--;
}

int interp(struct context_t* ctx, int out_fd) {
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
      interp_l_brac(ctx);
      break;
    case ']':
      interp_r_brac(ctx);
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

