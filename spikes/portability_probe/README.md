# Phase 0b: does the Raven core port to WebAssembly and iOS?

The core (widgets, layout, XML, fonts, text editor, themes, the job system and threads) built for WebAssembly with the null
platform and no GPU backend, and its headless tests were run in Node. The iOS side is a script for a Mac (not run: there
is no Xcode here). Results feed `docs/SOKOL_BACKEND_PLAN.md` (section 14).

## Web (WebAssembly), tested

Toolchain: Emscripten 3.1.6 (Ubuntu 24.04 package, old) and Node 22.

| Question | Answer |
|---|---|
| Does the library compile? | Yes, all 88 source files. One change was needed: the URL download starts `curl` with `fork`/`exec` (`src/core/fs.c`); a build for platforms without subprocesses (`RVN_NO_SUBPROCESS`, set for Emscripten and iOS) now stubs it. `Ropen_url` uses `window.open` on the web; the native file dialogs (`widgets/dialogs.c`) are stubbed the same way. |
| Do the tests pass? | **With threads: 15 of 15 programs, 0 failures** (about 24,500 checks: layout, widgets, XML, text editor, themes, markdown, tools, dialogs, shell, memory, animation, locale, dock tearing, and `test_threads`: render thread, worker pool, parallel painting). |
| Without threads? | Everything passes except the checks that need a thread: the render thread cannot start (Raven falls back to rendering on the UI thread, which is by design, and says so in a warning the test harness counts), the worker count, and the parallel paint jobs. |
| Speed (12,042 widgets, `raven_benchmark`) | repaint: **1.70 ms** threaded wasm, 2.09 ms single-threaded wasm, 1.59 ms native; first layout 5.0 / 3.9 / 2.4 ms, first paint 12.2 / 10.7 / 4.4 ms (glyph rasterization). About native speed in steady state, 2 to 3 times slower at start-up. |
| Does the Sokol GLES3 backend compile for the web? | Yes, unchanged (`-DRAVEN_SOKOL_GLES3`). Its native part (`sg_egl.c`, a context and a window surface) does not apply: a browser needs its own (a WebGL context of the canvas). Not run: there is no browser here. |

Not part of this probe, still to do for the web: the browser-owned frame loop (`Rrun`), in-surface windows, touch, the DOM text
input, a canvas platform, asynchronous download, and a GPU backend running in a real browser. Threads in a browser need the page
to be cross-origin isolated (COOP/COEP headers); the single-threaded build has no such requirement.

Toolchain notes: this old Emscripten does not link a `main(int, char**)` (it finds no `main`, and the program does nothing):
`tests/test_dock_tearing.c` and `tests/benchmark.c` need a `main(void)` copy (the script does this). Node 22 needs
`-sWASM_ASYNC_COMPILATION=0` with it. A current emsdk very probably has neither problem (not tried). `ravenc` runs on the host in
a cross build, so `test_xmlc` / `test_xmlcpp` are not built here.

```sh
./probe_wasm.sh threads        # or: nothreads
```

## iOS, not run here

A static scan found the places that cannot compile for iOS (its SDK marks `fork` and `exec*` unavailable): `Ropen_url`,
the `curl` download and the native file dialogs. They are now behind `RVN_NO_SUBPROCESS`; forcing the macro on in a Linux build
(`-DCMAKE_C_FLAGS=-DRVN_NO_SUBPROCESS`) passes the whole suite (28 of 28). Nothing else the scan looked for turned up
(`system`, `popen`, `dlopen`, `posix_spawn`, `waitpid` outside the platform folders).

`probe_ios.sh` builds the library for the iOS simulator and runs the same headless tests in a booted simulator. Run it on a Mac:

```sh
xcrun simctl boot "iPhone 15"      # once
./probe_ios.sh
```
