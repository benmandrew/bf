# Optimisation Plan

Five optimisations for the produced LLVM IR, implemented as separate commits.

## Status

- [x] 1. Cancel opposing simple commands
- [ ] 2. `CMD_CLEAR` — zero-loop `[-]` → `store i8 0`
- [ ] 3. `CMD_MULTIPLY` — multiply-loop `[->N*+<]` → multiply-add
- [ ] 4. `dp` as `alloca` (enables `mem2reg`)
- [ ] 5. LLVM pass pipeline gated on `-O`
- [ ] 6. Per-optimisation TOML config file

---

## 1. Cancel opposing simple commands

**Where**: `ir.c` — new `optimise_program()` called from both `main_bfc.c` and `main_bfi.c`.

Peephole pass over the `cmds` array: adjacent INC/DEC or RIGHT/LEFT pairs subtract counts and, if they fully cancel, are removed. Re-computes bracket jump indices after compaction.

**Example**: `+++--` → `CMD_SIMPLE_INC(1)` instead of `CMD_SIMPLE_INC(3), CMD_SIMPLE_DEC(2)`.

Test changes: none (no existing test program uses cancellable patterns).

---

## 2. CMD_CLEAR — zero-loop detection

**Where**: `ir.c` `optimise_program()`, `llvm.c`, `interp.c`.

New `CMD_CLEAR` IR node. Pattern detected after cancellation pass: `[` + single INC or DEC body + `]`. Replaced with a single `store i8 0` in codegen, `ctx->data[ctx->dp] = 0` in interpreter.

**Example**: `+++++[-]` → `CMD_SIMPLE_INC(5), CMD_CLEAR`.

Test changes: update `test/test_simple_loop.filecheck` (program is `+++++[-]`).

---

## 3. CMD_MULTIPLY — multiply-loop detection

**Where**: `ir.c` `optimise_program()`, `llvm.c`, `interp.c`.

New `CMD_MULTIPLY` IR node with up to `MULTIPLY_MOVES_MAX` (8) offset/factor pairs. A loop body matches when: only `+`/`-`/`>`/`<` inside, net pointer movement is zero, loop counter cell has net delta −1. Each non-counter cell touched becomes a `{offset, factor}` move.

Codegen: load counter, for each move `data[dp+offset] += counter * factor`, then `store i8 0` to counter cell. Interpreter: same arithmetic.

**Example**: `[->+<]` at dp=1 → `CMD_MULTIPLY {moves=[{offset=-1, factor=1}]}`.

Test changes: add `test/res/multiply.b` and `test/test_multiply.filecheck`.

---

## 4. `dp` as `alloca`

**Where**: `llvm.c` — `create_main_function()` creates `dp` as an alloca instead of a global.

`dp` is removed from the global section and created with `LLVMBuildAlloca` in the entry block, immediately initialised to 0. The `LLVMValueRef ctx->dp` is still a pointer (alloca ptr vs global ptr) so all downstream load/store calls are unchanged.

Without LLVM passes the IR still has explicit load/store; the benefit is unlocked in commit 5 when `mem2reg` promotes the alloca to a register.

Test changes: update all FileCheck tests — remove `@dp = global i32 0` check, change `ptr @dp` references to `ptr %dp`.

---

## 5. LLVM pass pipeline gated on `-O`

**Where**: `llvm.c`, `llvm.h`, `cmake/llvm.cmake`, `main_bfc.c`.

Wires the already-parsed `--optimise`/`-O` flag from `main_bfc.c` through `generate(struct program *, bool optimise)`. When `optimise` is true, runs `"mem2reg,instcombine,simplifycfg,gvn"` via `LLVMRunPasses` (LLVM new pass manager, LLVM ≥ 14). Adds `passes` to `llvm_map_components_to_libnames` in cmake.

`mem2reg` promotes the `dp` alloca to a register; `gvn` eliminates redundant loads of `@data` elements; `instcombine` and `simplifycfg` clean up the resulting IR.

Test changes: none (FileCheck tests do not pass `-O`).

---

## 6. Per-optimisation TOML config file

**Where**: new `src/config.h` / `src/config.c`, updated `main_bfc.c`, updated `optimise_program()` and `generate()` signatures.

A flat TOML file (default `bf.toml` in the current directory, overridable with `-c`/`--config`) controls each optimisation independently:

```toml
[optimisations]
cancel_opposing = true
clear_loop      = true
multiply_loop   = true
dp_alloca       = true
llvm_passes     = false
```

`struct opt_config` holds a boolean for each flag; a minimal built-in parser handles `[section]` headers and `key = true/false` lines. Missing file → all optimisations enabled by default. The `-O` flag becomes a shorthand for enabling all flags.

`optimise_program(struct program *, const struct opt_config *)` and `generate(struct program *, const struct opt_config *)` are updated to gate each pass on its flag.

Test changes: add `test/res/bf.toml` with specific flags for FileCheck regression tests if needed.
