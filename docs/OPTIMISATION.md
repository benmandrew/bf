# Optimisation

`bfc` emits LLVM *intermediate representation* (IR) and, with `-O`, optimises it
in-process before printing. The generated code is deliberately shaped so the
optimiser can do its job: the frontend states facts about the program that LLVM
cannot otherwise recover. This document records those choices and why each one
matters.

## Why the frontend has to help

The documented workflow compiles the emitted IR with `clang main.ll -o main`,
which runs at `-O0`. Whatever `bfc -O` does not fold, nothing downstream folds
either. Piping the output through `opt -O3` does not rescue a weak frontend
because the missing facts — that a pointer never wraps, that an access stays in
bounds, that a call never touches the tape — are unrecoverable once the IR omits
them. The frontend is the only place they can be asserted.

So `-O` runs the full `default<O2>` pipeline, the same one `clang -O2` uses,
rather than a hand-picked handful of passes. This brings *dead-store
elimination* (DSE), *loop-invariant code motion* (LICM), induction-variable
simplification, and *loop-idiom recognition* to bear.

## The hints

**A sign-extension-free pointer.** The data pointer is an `i64` index, and every
tape access is an `inbounds getelementptr` whose index arithmetic carries the
`nsw` (no-signed-wrap) flag. An `i32` index forced a `sext` to `i64` on every
access, and without `nsw` the optimiser could not prove `sext(dp + 1)` equals
`sext(dp) + 1`. That defeated induction-variable widening and pinned the address
computation inside the loop. With the flags, a moving pointer becomes a clean
pointer recurrence that loop-idiom recognition can match.

**Pure I/O with respect to the tape.** `putchar` and `getchar` are declared
`memory(inaccessiblemem: readwrite)`. An undeclared external function must be
assumed to read and write `@data`, which forces a reload of the current cell
after every I/O call. The attribute tells LLVM the calls touch only state the
module cannot see — the standard streams — so cell values survive in registers
across them.

**A private tape.** The `@data` global has private linkage. While it is
externally visible, whole-module passes cannot reason about it as a unit. Once
private, *global optimisation* may rewrite it — for a program that touches a
handful of cells, the 65536-byte array shrinks to just those cells.

**A bounded input.** The `getchar` result carries `!range !{i32 -1, i32 256}`.
The wrapped range covers end-of-file (`-1`) and every byte value `0..255`, which
lets LLVM fold the truncation and any comparison that follows an input.

**Reused output.** Repeated `.` (for example `...`) loads and zero-extends the
current cell once and feeds the value to every `putchar` call, since the cell
cannot change between writes.

## Worked examples

A pointer scan lowers to a library call. The program `+[->+>+<<]>[>]<.` contains
a `[>]` scan to the first zero cell:

```bash
$ printf '+[->+>+<<]>[>]<.' | bfc -O | grep strlen
  %strlen = tail call i64 @strlen(ptr noundef nonnull dereferenceable(1) getelementptr inbounds nuw (i8, ptr @data, i64 2))
```

The scalar, byte-at-a-time loop is gone, replaced by `strlen` — which the C
library implements with vectorised code.

The tape collapses to its footprint. The program `+.+.` uses one cell:

```bash
$ printf '+.+.' | bfc -O | grep '@data'
@data.0 = internal unnamed_addr global i8 0, align 1
```

The 65536-byte global has become a single `i8`, and the cell is no longer
reloaded between the two `putchar` calls.

## The in-bounds contract

These hints assume the data pointer stays within `[0, 65536)`. `bfc` emits no
bounds checks, so moving the pointer outside the tape was already unchecked.
`inbounds` and `nsw` let the optimiser *rely* on it: an out-of-range move is now
undefined behaviour rather than a wild-but-executed access. Programs that respect
the tape bounds are unaffected; this only sharpens what the compiler is entitled
to assume.

One category of arithmetic is deliberately left un-flagged. Cell updates (`+`,
`-`) stay wrapping `i8` operations with no `nsw`/`nuw`, because bf cells
wrap modulo 256 by definition. Only the pointer arithmetic is promised not to
overflow.
