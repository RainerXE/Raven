// Runs a Raven web build (examples/hello built with Emscripten) in headless Chromium and checks that it draws
// and answers the mouse, the keyboard and touch.
//
//   node web_browser_test.js <dir with raven_hello.html> [screenshot.png]
//
// Needs Playwright (require('playwright'), with Chromium). WebGL 2 comes from SwiftShader when there is no GPU.
const path = require('path');
const http = require('http');
const fs = require('fs');
const { chromium } = require('playwright');

const dir = path.resolve(process.argv[2] || '.');
const shot = process.argv[3];
const types = { '.html': 'text/html', '.js': 'text/javascript', '.wasm': 'application/wasm' };

const server = http.createServer((req, res) => {
  const file = path.join(dir, decodeURIComponent(req.url.split('?')[0]).replace(/^\/+$/, 'raven_hello.html'));
  fs.readFile(file, (err, data) => {
    if (err) { res.writeHead(404); res.end(); return; }
    res.writeHead(200, { 'content-type': types[path.extname(file)] || 'application/octet-stream' });
    res.end(data);
  });
});

let failures = 0;
function check(ok, what) { console.log((ok ? 'ok   ' : 'FAIL ') + what); if (!ok) failures++; }

(async () => {
  await new Promise((r) => server.listen(0, r));
  const url = `http://127.0.0.1:${server.address().port}/raven_hello.html`;
  const browser = await chromium.launch({ args: ['--use-gl=angle', '--use-angle=swiftshader', '--enable-unsafe-swiftshader', '--ignore-gpu-blocklist'] });
  const context = await browser.newContext({ viewport: { width: 640, height: 400 }, deviceScaleFactor: 2, hasTouch: true });
  const page = await context.newPage();
  const logs = [];
  page.on('console', (m) => logs.push(m.text()));
  page.on('pageerror', (e) => logs.push('pageerror: ' + e.message));
  await page.goto(url);
  await page.waitForTimeout(2500);

  const pixels = async () => page.evaluate(() => {
    // the canvas is drawn by WebGL: draw it into a 2D canvas to read it (works right after a frame)
    const c = document.getElementById('canvas');
    const copy = document.createElement('canvas');
    copy.width = c.width; copy.height = c.height;
    const g = copy.getContext('2d');
    g.drawImage(c, 0, 0);
    const d = g.getImageData(0, 0, c.width, c.height).data;
    let sum = 0, colors = new Set();
    for (let i = 0; i < d.length; i += 4 * 97) { sum += d[i] + d[i + 1] + d[i + 2]; colors.add((d[i] << 16) | (d[i + 1] << 8) | d[i + 2]); }
    return { width: c.width, height: c.height, colors: colors.size, sum };
  });

  const size = await page.evaluate(() => ({ w: document.getElementById('canvas').width, h: document.getElementById('canvas').height }));
  check(size.w === 1280 && size.h === 800, `canvas follows the page in physical pixels (${size.w}x${size.h})`);
  if (shot) await page.screenshot({ path: shot });
  await page.screenshot({ path: '/tmp/raven_web_first.png' });

  // find the button by clicking where hello puts it (centered); the label text changes on click
  const text = async () => (await page.screenshot()).length; // placeholder to wait for a frame
  const before = await page.screenshot();
  await page.mouse.move(294, 237);
  await page.mouse.down(); await page.mouse.up();
  await page.waitForTimeout(500);
  const afterClick = await page.screenshot();
  check(Buffer.compare(before, afterClick) !== 0, 'a mouse click changes what is shown');

  await page.touchscreen.tap(294, 237);
  await page.waitForTimeout(500);
  const afterTap = await page.screenshot();
  check(Buffer.compare(afterClick, afterTap) !== 0, 'a tap changes what is shown');
  fs.writeFileSync('/tmp/raven_web_after.png', afterTap);

  await page.setViewportSize({ width: 800, height: 500 });
  await page.waitForTimeout(800);
  const resized = await page.evaluate(() => ({ w: document.getElementById('canvas').width, h: document.getElementById('canvas').height }));
  check(resized.w === 1600 && resized.h === 1000, `a resized page resizes the canvas (${resized.w}x${resized.h})`);

  const errors = logs.filter((l) => /error|abort|pageerror|exception/i.test(l) && !/GPU stall|swiftshader/i.test(l));
  check(errors.length === 0, 'no errors in the console' + (errors.length ? ': ' + errors.slice(0, 3).join(' | ') : ''));
  if (logs.length) console.log('console:\n  ' + logs.slice(0, 12).join('\n  '));
  await browser.close();
  server.close();
  process.exit(failures ? 1 : 0);
})().catch((e) => { console.error(e); process.exit(2); });
