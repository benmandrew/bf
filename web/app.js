(function () {
  const bfEditor = document.getElementById("bfEditor");
  const irView = document.getElementById("irView");
  const cfgView = document.getElementById("cfgView");
  const btnCompile = document.getElementById("btnCompile");
  const btnDownload = document.getElementById("btnDownload");
  const statusEl = document.getElementById("status");
  const cfgStatusEl = document.getElementById("cfgStatus");

  // dot-cfg record labels render dark to match the UI (see highlight.js).
  const THEME = "dark";

  function setStatus(msg) { if (statusEl) statusEl.textContent = msg; }
  function setCfgStatus(msg) { if (cfgStatusEl) cfgStatusEl.textContent = msg; }

  // Validate input before it reaches the wasm compiler. This used to be the
  // server's job (server.go sanitiseInput); client-side it is a UX guard that
  // gives a clear message and keeps malformed programs -- unbalanced loops
  // especially -- from reaching bfc at all.
  const ALLOWED = new Set(["+", "-", ">", "<", ".", ",", "[", "]", "\n", " "]);
  function sanitiseInput(input) {
    if (input.length > 10000) return { error: "Input too long" };
    let depth = 0;
    for (let i = 0; i < input.length; i++) {
      const ch = input[i];
      if (!ALLOWED.has(ch)) {
        return { error: `Invalid character ${JSON.stringify(ch)} at location ${i + 1}` };
      }
      if (ch === "[") depth++;
      else if (ch === "]") depth--;
    }
    if (depth !== 0) return { error: "Mismatched loops" };
    return { value: input };
  }

  // Defence in depth for inlining the SVG: Graphviz emits none of these for a
  // dot-cfg graph and the labels are HTML-escaped by highlight.js, but strip
  // active content anyway (mirrors server.go sanitiseSVG).
  function sanitiseSVG(svg) {
    return svg
      .replace(/<script[\s\S]*?<\/script\s*>/gi, "")
      .replace(/<foreignObject[\s\S]*?<\/foreignObject\s*>/gi, "")
      .replace(/\son[a-z]+\s*=\s*("[^"]*"|'[^']*')/gi, "");
  }

  // One module worker runs bfc + Graphviz. Requests are tagged with an
  // incrementing id so a slow render from an earlier keystroke cannot
  // overwrite the result of a later one.
  const worker = new Worker("worker.js", { type: "module" });
  let reqId = 0;
  let lastApplied = 0;

  worker.onmessage = (e) => {
    const { id, ir, svg, error } = e.data;
    if (id < lastApplied) return;
    lastApplied = id;
    if (error) {
      irView.textContent = error;
      cfgView.innerHTML = "";
      setStatus("Error");
      setCfgStatus("Error");
      return;
    }
    irView.textContent = ir;
    if (window.Prism) Prism.highlightElement(irView);
    setStatus("OK");

    const clean = sanitiseSVG(svg);
    const i = clean.indexOf("<svg");
    cfgView.innerHTML = i >= 0 ? clean.slice(i) : clean;
    initPanZoom(cfgView);
    setCfgStatus("OK");
  };

  worker.onerror = (e) => {
    setStatus("Worker error");
    setCfgStatus("Worker error");
    irView.textContent = "Worker error: " + (e.message || "failed to load");
  };

  function compile() {
    const code = bfEditor.value;
    if (!code) {
      irView.textContent = "";
      cfgView.innerHTML = "";
      setStatus("No input");
      setCfgStatus("No input");
      return;
    }
    const checked = sanitiseInput(code);
    if (checked.error) {
      irView.textContent = "Invalid input: " + checked.error;
      cfgView.innerHTML = "";
      setStatus("Error");
      setCfgStatus("Error");
      return;
    }
    setStatus("Compiling…");
    setCfgStatus("Rendering…");
    worker.postMessage({ id: ++reqId, code: checked.value, theme: THEME });
  }

  // Turn the injected <svg> into a pan/zoom viewport by driving its viewBox.
  // Handlers are assigned (not addEventListener) so re-rendering replaces them
  // instead of stacking duplicates.
  function initPanZoom(container) {
    const svg = container.querySelector("svg");
    if (!svg) return;
    svg.setAttribute("width", "100%");
    svg.setAttribute("height", "100%");
    svg.setAttribute("preserveAspectRatio", "xMidYMid meet");

    let vb = (svg.getAttribute("viewBox") || "").split(/[\s,]+/).map(Number);
    if (vb.length !== 4 || vb.some(Number.isNaN)) {
      try {
        const b = svg.getBBox();
        vb = [b.x, b.y, b.width, b.height];
      } catch (_) {
        vb = [0, 0, 100, 100];
      }
    }
    const home = vb.slice();
    let [x, y, w, h] = vb;
    const apply = () => svg.setAttribute("viewBox", `${x} ${y} ${w} ${h}`);
    apply();

    container.onwheel = (e) => {
      e.preventDefault();
      const r = container.getBoundingClientRect();
      const mx = (e.clientX - r.left) / r.width;
      const my = (e.clientY - r.top) / r.height;
      const factor = Math.exp(e.deltaY * 0.0015);
      const nw = w * factor, nh = h * factor;
      if (nw < home[2] / 50 || nw > home[2] * 50) return;
      x += mx * (w - nw);
      y += my * (h - nh);
      w = nw; h = nh;
      apply();
    };

    let dragging = false, px = 0, py = 0;
    container.onpointerdown = (e) => {
      dragging = true; px = e.clientX; py = e.clientY;
      container.setPointerCapture(e.pointerId);
    };
    container.onpointermove = (e) => {
      if (!dragging) return;
      const r = container.getBoundingClientRect();
      x -= ((e.clientX - px) / r.width) * w;
      y -= ((e.clientY - py) / r.height) * h;
      px = e.clientX; py = e.clientY;
      apply();
    };
    const endDrag = (e) => {
      dragging = false;
      try { container.releasePointerCapture(e.pointerId); } catch (_) {}
    };
    container.onpointerup = endDrag;
    container.onpointercancel = endDrag;
    container.ondblclick = () => { [x, y, w, h] = home; apply(); };
  }

  if (btnCompile) btnCompile.addEventListener("click", compile);

  if (btnDownload)
    btnDownload.addEventListener("click", function () {
      const text = irView.textContent || "";
      const blob = new Blob([text], { type: "text/plain" });
      const url = URL.createObjectURL(blob);
      const a = document.createElement("a");
      a.href = url;
      a.download = "main.ll";
      document.body.appendChild(a);
      a.click();
      a.remove();
      URL.revokeObjectURL(url);
    });

  bfEditor.addEventListener("input", compile);

  bfEditor.addEventListener("keydown", function (e) {
    if ((e.ctrlKey || e.metaKey) && e.key === "Enter") {
      e.preventDefault();
      compile();
    }
  });

  bfEditor.value = "+++[.-]";
  window.addEventListener("load", function () { compile(); });
})();
