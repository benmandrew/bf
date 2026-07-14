(function(){
  const bfEditor = document.getElementById('bfEditor');
  const irView = document.getElementById('irView');
  const cfgView = document.getElementById('cfgView');
  const btnCompile = document.getElementById('btnCompile');
  const btnDownload = document.getElementById('btnDownload');
  const statusEl = document.getElementById('status');
  const cfgStatusEl = document.getElementById('cfgStatus');

  let debounceTimer = null;
  const DEBOUNCE_MS = 600;

  function setStatus(msg){ if (statusEl) statusEl.textContent = msg; }
  function setCfgStatus(msg){ if (cfgStatusEl) cfgStatusEl.textContent = msg; }

  async function fetchIR(code){
    try {
      const res = await fetch('__BF_COMPILE_URL__' + encodeURIComponent(code));
      const text = await res.text();
      if (!res.ok){
        irView.textContent = text || `Compiler error: ${res.status}`;
        setStatus('Error');
      } else {
        irView.textContent = text;
        setStatus('OK');
      }
    } catch (err){
      irView.textContent = 'Network error: ' + err.message;
      setStatus('Network error');
    }
    if (window.Prism) Prism.highlightElement(irView);
  }

  async function fetchCFG(code){
    setCfgStatus('Rendering…');
    try {
      const res = await fetch('__BF_CFG_URL__' + encodeURIComponent(code));
      if (!res.ok){
        const text = await res.text();
        cfgView.innerHTML = '';
        setCfgStatus(text || `CFG error: ${res.status}`);
        return;
      }
      const svg = await res.text();
      // Inline the SVG, dropping the XML prolog/doctype so the HTML parser
      // treats <svg> as an element. The server strips active content.
      const i = svg.indexOf('<svg');
      cfgView.innerHTML = i >= 0 ? svg.slice(i) : svg;
      initPanZoom(cfgView);
      setCfgStatus('OK');
    } catch (err){
      cfgView.innerHTML = '';
      setCfgStatus('Network error: ' + err.message);
    }
  }

  async function compile(){
    const code = bfEditor.value;
    if (!code){
      irView.textContent = '';
      if (cfgView) cfgView.innerHTML = '';
      setStatus('No input');
      setCfgStatus('No input');
      return;
    }
    setStatus('Compiling…');
    const jobs = [fetchIR(code)];
    if (cfgView) jobs.push(fetchCFG(code));
    await Promise.all(jobs);
  }

  // Turn the injected <svg> into a pan/zoom viewport by driving its
  // viewBox. Handlers are assigned (not addEventListener) so re-rendering
  // replaces them instead of stacking duplicates.
  function initPanZoom(container){
    const svg = container.querySelector('svg');
    if (!svg) return;
    svg.setAttribute('width', '100%');
    svg.setAttribute('height', '100%');
    svg.setAttribute('preserveAspectRatio', 'xMidYMid meet');

    let vb = (svg.getAttribute('viewBox') || '').split(/[\s,]+/).map(Number);
    if (vb.length !== 4 || vb.some(Number.isNaN)){
      try { const b = svg.getBBox(); vb = [b.x, b.y, b.width, b.height]; }
      catch (_){ vb = [0, 0, 100, 100]; }
    }
    const home = vb.slice();
    let [x, y, w, h] = vb;
    const apply = () => svg.setAttribute('viewBox', `${x} ${y} ${w} ${h}`);
    apply();

    container.onwheel = (e) => {
      e.preventDefault();
      const r = container.getBoundingClientRect();
      const mx = (e.clientX - r.left) / r.width;
      const my = (e.clientY - r.top) / r.height;
      const factor = Math.exp(e.deltaY * 0.0015);
      const nw = w * factor, nh = h * factor;
      // Clamp zoom so the graph cannot invert or shrink/grow to nothing.
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
      x -= (e.clientX - px) / r.width * w;
      y -= (e.clientY - py) / r.height * h;
      px = e.clientX; py = e.clientY;
      apply();
    };
    const endDrag = (e) => {
      dragging = false;
      try { container.releasePointerCapture(e.pointerId); } catch (_){}
    };
    container.onpointerup = endDrag;
    container.onpointercancel = endDrag;
    container.ondblclick = () => { [x, y, w, h] = home; apply(); };
  }

  function scheduleCompile(){
    clearTimeout(debounceTimer);
    debounceTimer = setTimeout(compile, DEBOUNCE_MS);
  }

  if (btnCompile) btnCompile.addEventListener('click', compile);

  if (btnDownload) btnDownload.addEventListener('click', function(){
    const text = irView.textContent || '';
    const blob = new Blob([text], {type: 'text/plain'});
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = 'main.ll';
    document.body.appendChild(a);
    a.click();
    a.remove();
    URL.revokeObjectURL(url);
  });

  bfEditor.addEventListener('input', scheduleCompile);

  bfEditor.addEventListener('keydown', function(e){
    if ((e.ctrlKey || e.metaKey) && e.key === 'Enter'){
      e.preventDefault();
      compile();
    }
  });

  bfEditor.value = "+++[.-]";
  window.addEventListener('load', function(){ scheduleCompile(); });
})();
