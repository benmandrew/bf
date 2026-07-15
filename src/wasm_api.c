#include "wasm_api.h"

#include <stdlib.h>
#include <string.h>

#include <llvm-c/Core.h>

#include "cfg_dot.h"
#include "ir.h"
#include "llvm.h"
#include "read.h"

/// Clean, parse, and optimise raw source into a program ready to codegen.
/// The caller owns the returned program and must free its `cmds`.
static struct program source_to_program(const char *source) {
        char *buf = strdup(source);
        clean_whitespace(buf);
        struct program parsed = string_to_program(buf);
        free(buf);
        optimise_program(&parsed);
        return parsed;
}

char *bf_compile_ir(const char *source, bool optimise, bool label_blocks) {
        struct program parsed = source_to_program(source);
        LLVMModuleRef module = generate(&parsed, optimise, label_blocks);
        // LLVMPrintModuleToString returns memory owned by LLVM; copy it into
        // a plain malloc'd buffer so bf_free (which calls free) is correct.
        char *ir = LLVMPrintModuleToString(module);
        char *out = strdup(ir);
        LLVMDisposeMessage(ir);
        dispose_module(module);
        free(parsed.cmds);
        return out;
}

char *bf_compile_cfg_dot(const char *source, bool optimise, bool label_blocks,
                         bool include_instructions) {
        struct program parsed = source_to_program(source);
        LLVMModuleRef module = generate(&parsed, optimise, label_blocks);
        char *dot = emit_cfg_dot(module, include_instructions);
        dispose_module(module);
        free(parsed.cmds);
        return dot;
}

void bf_free(char *ptr) { free(ptr); }
