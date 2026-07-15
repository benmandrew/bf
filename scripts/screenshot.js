// Regenerate docs/screenshot.png, the README image of the web UI.
//
// Drives a running instance with Playwright and captures the three-pane
// layout (Brainfuck, LLVM IR, control flow graph). The viewport and device
// scale factor are chosen to reproduce the committed 3000x1640 image.
//
// Prerequisites:
//   - A running instance of the web UI, e.g. `docker compose up` served at
//     http://localhost:8080 (the default URL below).
//   - Dependencies installed once: `cd scripts && npm install` (see
//     scripts/package.json). This pulls playwright-core only, with no
//     bundled browser download.
//   - Google Chrome: the script uses `channel: 'chrome'` to drive the
//     system Chrome rather than a downloaded browser.
//
// Usage (from the repository root):
//   node scripts/screenshot.js [URL] [OUTPUT]
//     URL     page to capture   (default: http://localhost:8080)
//     OUTPUT  file to write      (default: docs/screenshot.png)

const { chromium } = require('playwright-core');

const URL = process.argv[2] || 'http://localhost:8080';
const OUTPUT = process.argv[3] || 'docs/screenshot.png';

(async () => {
  const browser = await chromium.launch({ channel: 'chrome' });
  const context = await browser.newContext({
    // 2000x1093 at 1.5x renders a 3000x1640 PNG, matching the committed image.
    viewport: { width: 2000, height: 1093 },
    deviceScaleFactor: 1.5,
  });
  const page = await context.newPage();
  await page.goto(URL, { waitUntil: 'networkidle' });

  // The page auto-compiles the default program on load. Wait for the CFG
  // SVG to be inlined and both panels to report OK before capturing.
  await page.waitForSelector('#cfgView svg', { timeout: 30000 });
  await page.waitForFunction(() => {
    const s = document.getElementById('status');
    const c = document.getElementById('cfgStatus');
    return s && c && s.textContent === 'OK' && c.textContent === 'OK';
  }, { timeout: 30000 });
  await page.waitForTimeout(500);

  await page.screenshot({ path: OUTPUT, fullPage: false });
  await browser.close();
  console.log(`wrote ${OUTPUT}`);
})().catch((e) => {
  console.error(e);
  process.exit(1);
});
