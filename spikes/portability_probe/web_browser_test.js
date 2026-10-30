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
const types = { '.html': 'text/html', '.js': 'text/javascript', '.wasm': 'application/wasm', '.data': 'application/octet-stream' };

const server = http.createServer((req, res) => {
  const file = path.join(dir, decodeURIComponent(req.url.split('?')[0]));
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
  const base = `http://127.0.0.1:${server.address().port}`;
  const url = `${base}/raven_hello.html`;
  const browser = await chromium.launch({ args: ['--use-gl=angle', '--use-angle=swiftshader', '--enable-unsafe-swiftshader', '--ignore-gpu-blocklist'] });
  const context = await browser.newContext({ viewport: { width: 640, height: 400 }, deviceScaleFactor: 2, hasTouch: true });
  const page = await context.newPage();
  const logs = [];
  page.on('console', (m) => logs.push(m.text()));
  page.on('pageerror', (e) => logs.push('pageerror: ' + e.message));
  await page.goto(url);
  await page.waitForTimeout(2500);

  const size = await page.evaluate(() => ({ w: document.getElementById('canvas').width, h: document.getElementById('canvas').height }));
  check(size.w === 1280 && size.h === 800, `canvas follows the page in physical pixels (${size.w}x${size.h})`);
  if (shot) await page.screenshot({ path: shot });
  await page.screenshot({ path: '/tmp/raven_web_first.png' });

  // find the button by clicking where hello puts it (centered); the label text changes on click
  const text = async () => (await page.screenshot()).length; // placeholder to wait for a frame
  const before = await page.screenshot();
  await page.mouse.move(294, 271);
  await page.mouse.down(); await page.mouse.up();
  await page.waitForTimeout(500);
  const afterClick = await page.screenshot();
  check(Buffer.compare(before, afterClick) !== 0, 'a mouse click changes what is shown');

  await page.touchscreen.tap(294, 271);
  await page.waitForTimeout(500);
  const afterTap = await page.screenshot();
  check(Buffer.compare(afterClick, afterTap) !== 0, 'a tap changes what is shown');
  fs.writeFileSync('/tmp/raven_web_after.png', afterTap);

  // text: a tap on the text field raises the (hidden) field that gets the soft keyboard; what is typed, accents too, and pasted arrives
  const focused = () => page.evaluate(() => (document.activeElement && document.activeElement.tagName) || '');
  const beforeTyping = await page.screenshot();
  await page.touchscreen.tap(320, 233);
  await page.waitForTimeout(400);
  check((await focused()) === 'TEXTAREA', 'a tap on a text field focuses the hidden input field (soft keyboard)');
  await page.keyboard.type('Héllo €');
  await page.waitForTimeout(500);
  const typed = await page.screenshot();
  check(Buffer.compare(beforeTyping, typed) !== 0, 'typed text, accents included, appears');
  await context.grantPermissions(['clipboard-read', 'clipboard-write']);
  await page.evaluate(() => navigator.clipboard.writeText('PASTED'));
  await page.keyboard.press('Control+V');
  await page.waitForTimeout(500);
  const pasted = await page.screenshot();
  check(Buffer.compare(typed, pasted) !== 0, 'Ctrl+V pastes the clipboard');
  await page.touchscreen.tap(320, 100);
  await page.waitForTimeout(400);
  check((await focused()) !== 'TEXTAREA', 'a tap elsewhere releases the field (keyboard hides)');

  await page.setViewportSize({ width: 800, height: 500 });
  await page.waitForTimeout(800);
  const resized = await page.evaluate(() => ({ w: document.getElementById('canvas').width, h: document.getElementById('canvas').height }));
  check(resized.w === 1600 && resized.h === 1000, `a resized page resizes the canvas (${resized.w}x${resized.h})`);

  // the glass example: the blur and refraction passes draw, and a swipe of a finger scrolls the page under the glass
  if (fs.existsSync(path.join(dir, 'raven_glass.html'))) {
    const glass = await context.newPage();
    const glassLogs = [];
    glass.on('console', (m) => glassLogs.push(m.text()));
    glass.on('pageerror', (e) => glassLogs.push('pageerror: ' + e.message));
    await glass.setViewportSize({ width: 900, height: 600 });
    await glass.goto(`${base}/raven_glass.html`);
    await glass.waitForTimeout(3500);
    const first = await glass.screenshot();
    // a flat page compresses to a few KB; the colored cards, text and glass do not
    check(first.length > 30000, `glass example draws its colored cards and glass (screenshot of ${first.length} bytes)`);
    const cdp = await context.newCDPSession(glass);
    const touch = (type, y) => cdp.send('Input.dispatchTouchEvent', { type, touchPoints: type === 'touchEnd' ? [] : [{ x: 560, y, id: 1 }] });
    await touch('touchStart', 420);
    for (let y = 420; y >= 200; y -= 20) { await touch('touchMove', y); await glass.waitForTimeout(16); }
    await touch('touchEnd', 200);
    await glass.waitForTimeout(600);
    const scrolled = await glass.screenshot();
    check(Buffer.compare(first, scrolled) !== 0, 'a swipe of a finger scrolls the page');
    const glassErrors = glassLogs.filter((l) => /error|abort|pageerror|exception/i.test(l) && !/GPU stall|swiftshader/i.test(l));
    check(glassErrors.length === 0, 'no errors in the console (glass)' + (glassErrors.length ? ': ' + glassErrors.slice(0, 3).join(' | ') : ''));
    fs.writeFileSync('/tmp/raven_web_glass.png', scrolled);
  }

  const errors = logs.filter((l) => /error|abort|pageerror|exception/i.test(l) && !/GPU stall|swiftshader/i.test(l));
  check(errors.length === 0, 'no errors in the console' + (errors.length ? ': ' + errors.slice(0, 3).join(' | ') : ''));
  if (logs.length) console.log('console:\n  ' + logs.slice(0, 12).join('\n  '));
  await browser.close();
  server.close();
  process.exit(failures ? 1 : 0);
})().catch((e) => { console.error(e); process.exit(2); });
