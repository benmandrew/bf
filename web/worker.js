// Compile worker: runs the whole pipeline off the main thread.
//
// bfc, compiled to WebAssembly, turns Brainfuck into LLVM IR and into the
// control-flow graph as Graphviz dot; highlight.js themes the dot (a port of
// scripts/highlight.py); and the Graphviz wasm lays it out as SVG. All three
// used to be a server round-trip; now they run in the browser, here in a
// worker so Graphviz layout never blocks typing.
import createModule from "./wasm/bfc.mjs";
import { Graphviz } from "./vendor/hpcc-graphviz-1.26.0.js";
import { highlightDot } from "./highlight.js";

// Load bfc and Graphviz once, lazily, and share the promise across requests.
let ready = null;
function init() {
  if (!ready) ready = Promise.all([createModule(), Graphviz.load()]);
  return ready;
}

self.onmessage = async (e) => {
  const { id, code, theme } = e.data;
  try {
    const [mod, graphviz] = await init();
    const ir = mod.ccall(
      "bf_compile_ir", "string",
      ["string", "number", "number"], [code, 0, 0]);
    // --cfg-instructions --label-blocks: the flags the server passed bfc.
    const dot = mod.ccall(
      "bf_compile_cfg_dot", "string",
      ["string", "number", "number", "number"], [code, 0, 1, 1]);
    const svg = graphviz.dot(highlightDot(dot, theme));
    self.postMessage({ id, ir, svg });
  } catch (err) {
    self.postMessage({ id, error: String((err && err.message) || err) });
  }
};
