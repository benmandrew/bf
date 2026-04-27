# Model Checking

The input validation and parsing functionality in [read.c](src/read.c) is formally verified to be memory safe for inputs up to thirteen commands long.

This uses [CBMC](https://github.com/diffblue/cbmc), a bounded model checker for checking the memory safety of C code. CBMC efficiently explores the state space of all of the program's possible executions and inputs. However, with longer inputs, the state space increases exponentially — this is the reason that memory safety has only been verified up to thirteen commands.

I ran the verification on a computer with an Intel i9-13900F, and 128GB of RAM.

| `bf` program length | Time to verify (s)          |
| ------------------- | --------------------------- |
| 3                   | Unwinding assertion failure |
| 4                   | 7.55                        |
| 5                   | 15.0                        |
| 6                   | 27.5                        |
| 7                   | 47.2                        |
| 8                   | 76.9                        |
| 9                   | 126                         |
| 10                  | 190                         |
| 11                  | 294                         |
| 12                  | 435                         |
| 13                  | 620                         |
| 14                  | Out-of-memory failure       |

- For a program of length $n$, we set the unwinding number to $n+1$. For $n\leq 3$ the unwinding number is not high enough to verify all loops.
- For $n \geq 14$, the computer runs out of memory during model checking.

If you would like to run the verification yourself for a given maximum program length, run:

```bash
$ cmake --build build --target cbmc
$ ./verification/cbmc_run.sh [MAX_PROGRAM_LEN]
```

Remember to start with a small program length, e.g. 4, and work your way up.
