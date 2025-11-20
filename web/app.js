(function(){
  const bfEditor = document.getElementById('bfEditor');
  const irView = document.getElementById('irView');
  const btnCompile = document.getElementById('btnCompile');
  const btnDownload = document.getElementById('btnDownload');
  const statusEl = document.getElementById('status');

  let debounceTimer = null;
  const DEBOUNCE_MS = 600;

  function setStatus(msg){ statusEl.textContent = msg; }

  async function compile(){
    const url = 'http://localhost:8000/compile';
    const code = bfEditor.value;
    if (!code){ irView.textContent = ''; setStatus('No input'); return; }
    setStatus('Compiling...');
    try{
      const res = await fetch(url, {
        method: 'POST',
        headers: { 'Content-Type': 'text/plain' },
        body: code
      });
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
  }

  function scheduleCompile(){
    clearTimeout(debounceTimer);
    debounceTimer = setTimeout(compile, DEBOUNCE_MS);
  }

  btnCompile.addEventListener('click', compile);

  btnDownload.addEventListener('click', function(){
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

  bfEditor.value = "++++++[-]";
  window.addEventListener('load', function(){ scheduleCompile(); });
})();
