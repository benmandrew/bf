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
const wasmIr = (src, optimise) =>
  M.ccall("bf_compile_ir", "string", ["string", "number", "number"],
    [src, optimise, 0]);
const wasmCfg = (src, optimise) =>
  M.ccall("bf_compile_cfg_dot", "string",
    ["string", "number", "number", "number"], [src, optimise, 1, 0]);
const nativeRun = (src, args = []) =>
  execFileSync(nativeBfc, args, { input: src }).toString();

// The optimisation pipeline runs by default and -U turns it off, so the wasm
// flag and the native flag are opposites. Both settings are checked: the two
// pipelines diverging in only one of them is exactly the kind of drift this
// gate exists to catch.
const MODES = [
  { name: "-U", optimise: 0, args: ["--unoptimised"] },
  { name: "-O", optimise: 1, args: [] },
];

// CFG dot node names embed heap addresses, which differ between two processes;
// normalise them before comparing so only the graph structure is checked.
const normalise = (s) => s.replace(/0x[0-9a-fA-F]+/g, "0xNODE");

let pass = 0;
let fail = 0;
for (const file of readdirSync(resDir).filter((f) => f.endsWith(".b")).sort()) {
  const src = readFileSync(`${resDir}/${file}`, "utf8");
  for (const mode of MODES) {
    const irOk = nativeRun(src, mode.args) === wasmIr(src, mode.optimise);
    const cfgOk =
      normalise(nativeRun(src, [...mode.args, "--emit-cfg-dot", "--label-blocks"])) ===
      normalise(wasmCfg(src, mode.optimise));
    if (irOk && cfgOk) {
      pass++;
    } else {
      fail++;
      console.log(
        `DIFF ${file} ${mode.name}: ir=${irOk ? "ok" : "DIFF"} cfg=${cfgOk ? "ok" : "DIFF"}`);
    }
  }
}
console.log(`\nwasm parity: ${pass} pass, ${fail} fail`);
process.exit(fail ? 1 : 0);
