# Web demo

The demo compiles bf to LLVM IR and to its control-flow graph entirely
in the browser. `bfc` is built to WebAssembly (wasm), so there is no backend:
the page is a bundle of static files that any static host can serve.

## Pipeline

Everything runs in a *Web Worker* (`worker.js`), off the main thread, so
Graphviz layout never blocks typing:

```
source ─► bfc.wasm ─► LLVM IR                              ─► shown, Prism-highlighted
       └► bfc.wasm ─► CFG dot ─► highlight.js ─► graphviz.wasm ─► SVG ─► shown, pan/zoom
```

`highlight.js` is a port of `scripts/highlight.py`; the Graphviz wasm is
vendored under `vendor/`. `app.js` validates input (character set, balanced
loops, length) as a usability guard before handing it to the worker.

## Files

| File | Role |
|---|---|
| `index.html`, `style.css` | Page shell and layout |
| `app.js` | UI: input validation, worker calls, pane updates, pan/zoom |
| `worker.js` | Module worker hosting `bfc.wasm`, Graphviz, and `highlight.js` |
| `highlight.js` | Themes the CFG dot, syntax-highlighting the IR |
| `vendor/` | Vendored `@hpcc-js/wasm-graphviz` (Apache 2.0) |
| `prism.js`, `prism.css` | LLVM IR highlighting for the IR pane |
| `wasm/` | Built `bfc.{mjs,wasm}` — a build artefact, not committed |

## Building

The wasm module is built by `scripts/build-wasm.sh`, which needs
[Emscripten](https://emscripten.org/) (`emcc`/`emcmake`) and `ninja` — both
provided by the Nix devShell, so run it inside `nix develop`. It builds the LLVM
libraries once — slow — then links `bfc` into `wasm/`:

```bash
nix develop -c scripts/build-wasm.sh           # -> web/wasm/bfc.{mjs,wasm}
```

The `site` target then stages the whole bundle into `build/site`, ready to
deploy:

```bash
nix develop -c cmake -B build
nix develop -c cmake --build build --target site   # -> build/site/
```

CI publishes `build/site` as the `site` artifact on every run.

## Serving

The bundle is self-contained static files, but two MIME types **must** be set
correctly or the page fails to load:

| Extension | Content-Type | Why |
|---|---|---|
| `.mjs` | `text/javascript` | The module worker and its imports (including `bfc.mjs`) are rejected if not served as JavaScript. |
| `.wasm` | `application/wasm` | `WebAssembly.instantiateStreaming` requires it. |

Most static hosts, including GitHub Pages, set both already. For a local
preview, the devShell's Python maps them correctly:

```bash
cd build/site && nix develop -c python3 -m http.server 8080
```

An older `python3` (before 3.11) may not know `.wasm`; check with
`python3 -c 'import mimetypes; print(mimetypes.guess_type("x.wasm"))'` and use
a server that sets the two types above if it returns `None`.

## Consuming the bundle

`build/site` is a complete, dependency-free static site — drop it behind any
host that honours the MIME types above. Nothing else is required: no backend,
no runtime services, no network calls beyond loading the assets themselves.
