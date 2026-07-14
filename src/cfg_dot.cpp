#include "cfg_dot.h"

#include <llvm/Analysis/BranchProbabilityInfo.h>
#include <llvm/Analysis/CFGPrinter.h>
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/IR/Dominators.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/GraphWriter.h>
#include <llvm/Support/raw_ostream.h>

// Fold the `opt -passes=dot-cfg-only` stage into bfc itself. The dot-cfg
// printer is a Function pass with no llvm-c binding, and as a pass it
// writes .<function>.dot into the working directory rather than a stream;
// calling WriteGraph directly here writes to stdout instead, which is both
// what the --emit-cfg-dot flag promises and safe to run concurrently.
//
// DOTFuncInfo constructed without profile data defaults to heat colours
// off, matching the -cfg-heat-colors=false that scripts/cfg.sh passed opt:
// heat encodes block frequency, and without PGO every block falls back to
// the same weight while still stamping inline colours that defeat theming.
// WriteGraph's ShortNames selects between the two printers: true is
// dot-cfg-only (block labels), false is dot-cfg (labels plus instructions).
extern "C" void emit_cfg_dot(LLVMModuleRef module, bool include_instructions) {
        llvm::Module *m = llvm::unwrap(module);
        // bfc emits a single defined function, main; putchar/getchar and
        // friends are declarations and carry no CFG to print.
        for (llvm::Function &f : *m) {
                if (f.isDeclaration())
                        continue;
                // WriteGraph's getEdgeAttributes dereferences
                // DOTFuncInfo::getBPI() to form an edge tooltip. LLVM 19
                // (the Debian runtime image) does this before its
                // showEdgeWeights() guard, so a null BPI crashes on any
                // branch; LLVM 21+ guards first. Build a real one so both
                // are safe -- it is unused where the guard fires.
                llvm::DominatorTree dt(f);
                llvm::LoopInfo li(dt);
                llvm::BranchProbabilityInfo bpi(f, li);
                llvm::DOTFuncInfo cfg_info(&f, /*BFI=*/nullptr, &bpi,
                                           /*MaxFreq=*/0);
                // Draw no probability labels: keeps the graph themeable and
                // the output identical to opt -passes=dot-cfg[-only].
                cfg_info.setEdgeWeights(false);
                cfg_info.setRawEdgeWeights(false);
                llvm::WriteGraph(llvm::outs(), &cfg_info,
                                 /*ShortNames=*/!include_instructions);
        }
        llvm::outs().flush();
}
