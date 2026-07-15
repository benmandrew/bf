// Verify the WebAssembly build of bfc produces the same output as the native
// binary. For every test/res/*.b program it compares both exported entry
// points -- bf_compile_ir and bf_compile_cfg_dot -- against the native bfc,
// which is the oracle. This is what pins the wasm build to the same LLVM the
// Nix devShell uses: if they ever diverge, this fails.
//
// Usage: node wasm_parity.mjs <native-bfc> <test-res-dir> <bfc.mjs>
import { readFileSync, readdirSync } from "node:fs";
import { execFileSync } from "node:child_process";

const [nativeBfc, resDir, modulePath] = process.argv.slice(2);
if (!nativeBfc || !resDir || !modulePath) {
  console.error("usage: node wasm_parity.mjs <native-bfc> <test-res-dir> <bfc.mjs>");
  process.exit(2);
}

const createModule = (await import(modulePath)).default;
const M = await createModule();
const wasmIr = (src) =>
  M.ccall("bf_compile_ir", "string", ["string", "number", "number"], [src, 0, 0]);
const wasmCfg = (src) =>
  M.ccall("bf_compile_cfg_dot", "string",
    ["string", "number", "number", "number"], [src, 0, 1, 0]);
const nativeRun = (src, args = []) =>
  execFileSync(nativeBfc, args, { input: src }).toString();

// CFG dot node names embed heap addresses, which differ between two processes;
// normalise them before comparing so only the graph structure is checked.
const normalise = (s) => s.replace(/0x[0-9a-fA-F]+/g, "0xNODE");

let pass = 0;
let fail = 0;
for (const file of readdirSync(resDir).filter((f) => f.endsWith(".b")).sort()) {
  const src = readFileSync(`${resDir}/${file}`, "utf8");
  const irOk = nativeRun(src) === wasmIr(src);
  const cfgOk =
    normalise(nativeRun(src, ["--emit-cfg-dot", "--label-blocks"])) ===
    normalise(wasmCfg(src));
  if (irOk && cfgOk) {
    pass++;
  } else {
    fail++;
    console.log(`DIFF ${file}: ir=${irOk ? "ok" : "DIFF"} cfg=${cfgOk ? "ok" : "DIFF"}`);
  }
}
console.log(`\nwasm parity: ${pass} pass, ${fail} fail`);
process.exit(fail ? 1 : 0);
