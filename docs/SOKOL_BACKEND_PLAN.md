# Plan: a selectable Sokol rendering backend

Status: phase 0 (spike, section 10) and phase 1 (interface and build plumbing, section 11) are done; the offscreen core of the
backend (most of phase 2) already runs inside the library on GLES3.
Goal: add `R_BACKEND_SOKOL` next to `R_BACKEND_VULKAN`, selectable at `Rinit`.

**Why (the strategic reason):** Sokol is the better renderer for **macOS, iOS and the Web**.
- macOS: Raven draws today through Vulkan on MoltenVK, which the README makes the user
  install (`brew install molten-vk`). Native Metal removes that layer and that dependency.
- iOS: Vulkan would again mean MoltenVK shipped inside every app. Metal is native.
- Web: Vulkan cannot run in a browser. Sokol's WebGL2 (GLES3) and WebGPU backends can.

Platform order: **macOS (Metal) -> iOS (Metal) -> Web (WebGL2, then WebGPU) -> Linux (GL/EGL) -> Windows (D3D11).**
(Proposed after the iOS/Web priority; macOS-first, then Linux, then Windows was decided
earlier. Confirm where iOS and Web go, section 8.)
Vulkan stays the default on Linux and Windows until the Sokol backend reaches parity there.
It stays for Apple/Web only until Sokol replaces MoltenVK, which is a decision for phase 6.
Raven may change below the public widget/XML API (the render, backend-interface,
shader-pipeline and platform layers) wherever that makes the parts fit.

## 1. What we are building

```
 widgets / xml / widget tree                (unchanged)
 render/draw.c, font.c, texture.c           (unchanged except where noted in 3)
 ─────────────── RBackendVTable (v2) ───────────────
   vulkan/   (unchanged, keeps SPIR-V + Rvulkan_*)      sokol/   (new)
                                                          ├─ sg_backend.c    pipelines, buffers, textures, frame
                                                          ├─ sg_glass.c      backdrop capture + dual-filter blur
                                                          ├─ sg_shader.c     effect bundles
                                                          ├─ sg_readback.c   capture (per native API)
                                                          └─ present/        one file per native API:
                                                               metal (Cocoa, UIKit) · gles3/webgpu (Web) · egl (X11) · d3d11 (Win32) · offscreen

 platform/   cocoa (macOS) · x11 · win32 · null   +  NEW: uikit (iOS) · web (Emscripten)   (see 3.8)
```

Important: **a Sokol API is chosen when the library is compiled, not at run time**
(`SOKOL_METAL`, `SOKOL_GLCORE`, `SOKOL_GLES3`, `SOKOL_WGPU`, `SOKOL_D3D11`). So there is one
Sokol API per binary (Metal on Apple, GLES3 or WebGPU in a given wasm file). The run-time
choice is only **Vulkan vs Sokol**, which is exactly what `Rinit` selects.

Sokol (`sokol_gfx.h`) gives us the D3D11 / Metal / GL 4.1 / GLES3 drivers. It does
not give us window presentation for windows it does not own, so `present/` is
ours. That, plus effects and readback, is where the work is.

## 2. Findings that shape the plan

| Area | Fact | Consequence |
|---|---|---|
| Seam | `RBackendVTable` + `RDrawData` already isolate the renderer; `null` backend is a template | Sokol is a new vtable, not a rewrite |
| Rendering model | one instanced 4-vertex strip draw, SDF shading, no depth/MSAA, blend `(ONE, 1-SRC_A)` | Maps 1:1 to one sokol pipeline |
| Threads | every backend call is on one render thread | Matches sokol_gfx's single-thread rule |
| Presentation | sokol_gfx does not create swapchains for foreign windows | We write D3D11 DXGI, Metal layer, EGL/GLX surface code |
| Effects | `Rshader_create(spirv)`, `.spv` in `Effect=`, `raven_effect.glsl` assume SPIR-V at run time | Needs a new multi-target effect format (section 3.2) |
| Capture | `Rwindow_capture` and `tests/test_render.c` read pixels back; sokol has no readback (planned upstream) | Native readback per API, via offscreen target |
| Glyph atlas | `R_TEXTURE_ALPHA8` relies on a Vulkan view swizzle (1,1,1,a) | Expand on upload, or branch in shader (3.4) |
| Vertex data | `RDrawInstance` has `uint32` colors/flags | Confirmed in the spike: `UBYTE4N` colors and `SG_VERTEXFORMAT_UINT` flags work, no struct change |
| Vulkan public API | `raven_vulkan.h`, `RViewportDrawInfo.command_buffer`, shared textures | Stay Vulkan-only; add `raven_sokol.h` equivalent (3.5) |
| Upstream churn | sokol_gfx had several breaking API changes in 2026 | Vendor a pinned commit; upgrade deliberately |
| Linux | X11-only platform; Sokol means GL 4.1/GLES3 there | Vulkan remains the Linux default |
| Loop | The application owns the loop and may block: `Rwait_frame` sleeps in the window system | iOS and the browser own the loop. Needs an inverted-loop API (3.8) |
| Windows | Dialogs (`dialogs.c`), torn-off docks (`dock_area.c`) and popups are real OS windows (`Rwindow_create`); the platform table has move/resize/position/min-size hooks | iOS and the Web have one surface. Needs in-surface "virtual windows" (3.8) |
| Input | No touch or gesture events in `rvn_pevent_t`; no soft keyboard or safe-area concept | New event types and platform hooks (3.8) |
| Threads | Job system runs inline when it has 0 workers (`jobs.c`); `RInitDesc.disable_render_thread` exists | The Web can run single-threaded with no new code; pthreads there need cross-origin isolation |
| Files | `fs.c`, `fs_watch.c`, native file dialogs, hot reload of XML | Not available or different on iOS (sandbox) and the Web (virtual FS). Compile out or adapt |
| Not portable | `raven_web` drives an installed Chromium over a pipe | Unavailable on iOS and Web by design; the Web build uses the browser itself |
| Fonts | Only Latin subsets of Roboto are embedded; system fonts come from `font_system.c` | Web and iOS need either bundled fallback fonts or a system-font hook for CJK/Arabic/emoji |

## 3. Intermediate-layer changes

### 3.1 Backend interface v2 (`include/raven/raven_backend.h`, `render/renderer.*`, `core/context.c`)

- **Done:** a trailing optional `caps` member in `RBackendVTable` (`R_BACKEND_CAP_GLASS`, `ERASE`, `SHADERS`,
  `READBACK`, `CALLBACK`, `PRESENT`) and `Rbackend_get_caps()`. `Rbackend_register` still requires only the original
  members. Caveat: a custom backend compiled against the old header and registered with a new library
  reads `caps` past the end of its table; the library is at 0.1 and its API is declared unstable, so no
  version field was added. Tests use the caps to skip what a backend does not offer yet.
- Later: `BUNDLE_EFFECTS`, `SHARED_TEXTURE` caps with the features they describe.
- ~~Add `CreateShaderEx(Rid, const RShaderBlob*)` (3.2).~~ Deferred to phase 4 with the bundle format, so that no
  unused API is published. `CreateShader(spirv)` stays.
- **Done:** `R_BACKEND_SOKOL` (last value of `RBackend`, so existing values do not move), `Rget_backend()`, and
  `RAVEN_BACKEND=vulkan|sokol|null`, which applies when `RInitDesc.backend` is 0 (the default), so an
  application that asks for a backend explicitly keeps it. Bindings regenerated. The Sokol API (Metal, GLES3, WebGPU, GL, D3D11) is a
  build option, not a run-time one; `Rsokol_get_api()` reports which one was built.
- `RBackendInitInfo` gains a headless flag and a "single surface, owned by the platform" flag.

### 3.2 Effect shaders (`render/shader.c`, `include/raven/raven_shader.h`, `ravenc`)

Decision (recommended): **effect bundles, not run-time SPIR-V translation.**

- New `.rfx` file: a small container holding the effect compiled for each target
  (SPIR-V, GLSL 410, GLSL ES 300, HLSL bytecode, MSL) plus flags
  (`BACKDROP`, `ANIMATED`) and a name.
- New tool mode (`ravenc fx` or a standalone `raven_fxc`) that builds a bundle from one
  `.frag` using `raven_effect.glsl`. It shells out to sokol-shdc or glslang + SPIRV-Cross
  at *build* time; neither becomes a run-time dependency.
- `Rshader_create_from_bundle`, and `Effect="x.rfx"` in XML. `.spv` keeps working on
  Vulkan; on Sokol it logs the existing "cannot use this effect" error and draws the
  rounded rectangle (current fallback behavior, so no new failure mode).
- Spike findings for the translation step: GLES 3.0 / WebGL2 link varyings *by name*, so the tool
  renames the fragment inputs by location to the names of the UI vertex shader
  (`spirv-cross --rename-interface-variable`); and it records which of the three samplers a variant
  really uses (the GL compiler drops unused ones and sokol warns). Verified on all 8 built-ins and
  the two user effects of `examples/effects`.
- `rvn_shader_t` stores the blob; the renderer hands it to whichever backend is running.
- Optional later: `RAVEN_WITH_SPIRV_CROSS=ON` translates `.spv` at run time for users who
  accept a C++ dependency. Off by default.
- The 8 built-in effects are compiled into bundles at build time and embedded like the
  current `*_spv.h`.

### 3.3 One source of truth for the UI shaders (`render/vulkan/shaders` → `render/shaders`)

Move `ui.vert`, `ui.frag`, `blur.vert/frag` and the effect shaders to a neutral
`render/shaders/` folder with a thin wrapper per compiler (glslang for Vulkan, sokol-shdc
`@vs/@fs` for Sokol). Logic is shared, so glass/blur/curve look the same on both.
Phase 1 did not move the shader sources: `src/render/sokol/gen_shaders.py` reads them from
`src/render/vulkan/shaders` (the folder name is the only thing that is Vulkan-specific; it moves when a
second compiler wrapper is needed, for example for Metal). `blur.frag` needs one change for the GL targets: its push constants mix a `vec2` and an `int`, which
SPIRV-Cross cannot flatten, so they become one `vec4` (the spike does exactly this). The UI shaders
`ui.vert`/`ui.frag` compile unmodified through glslang -> SPIR-V -> SPIRV-Cross for ES 3.00 and GL 4.10.
Generated outputs are committed (as `generated/*_spv.h` is today) plus
`render/sokol/generated/*.h`; `cmake/RavenSokolShaders.cmake` regenerates them only when
`RAVEN_REGENERATE_SHADERS=ON` and sokol-shdc is found.

### 3.4 Glyph atlas

Verified in the spike: the Sokol backend stores `R_TEXTURE_ALPHA8` as RGBA8 (1,1,1,a) by expanding
on upload (4 MB per 1024² page, trivial); text renders identically. Optimize later by sampling `.r`
for `R_PRIM_GLYPH` in the Sokol shader variant. No change to `font.c`.

### 3.5 Native interop (new `include/raven/raven_sokol.h`)

- `RViewportDrawInfo`: add `native_device`, `native_context` / `native_encoder`, and
  `backend = R_BACKEND_SOKOL`. The callback runs on the render thread inside the
  window's pass; Raven calls `sg_reset_state_cache()` afterwards (same contract as the
  Vulkan "state restored" promise).
- `Rsokol_get_api()`, `Rsokol_get_native_device()`, `Rsokol_texture_wrap_native(...)`
  (D3D11 SRV / MTLTexture / GL texture id).
- Zero-copy shared textures (`Rvulkan_texture_create_shared`) remain Vulkan-only.
  Documented alternative: `Rtexture_update`, or wrap a native texture.
- Bindings: `tools/bindgen` regenerates them from the headers; add the new header to its list.

### 3.6 Platform layer (small, targeted)

- **Win32 (last):** transparent windows need `WS_EX_NOREDIRECTIONBITMAP` and a DirectComposition
  swapchain; this must be decided when the HWND is created. Add a `backend_hints` argument
  to platform window creation (Rinit knows the backend). Opaque windows need no change.
- **X11:** EGL surface creation needs the window's visual; `RNativeWindow.visual` already
  carries it. Verify GLX fallback is not needed.
- **Cocoa:** already installs a `CAMetalLayer`. Set `framebufferOnly = NO` only when a
  capture is requested; map `vsync` to `displaySyncEnabled`.
- Add a "present context" notion for GL: one shared context, `make current` per window on
  the render thread.

### 3.10 Backend implementation rules found in the spike

- **Y convention.** Keep Raven's "pixel row 0 = top, NDC y down" convention on every API. On GL
  this stores the UI top in framebuffer row 0, so offscreen targets, backdrop sampling,
  `gl_FragCoord` and read-back need no flip; only presenting to a GL default framebuffer does.
  Metal, D3D11 and WebGPU are top-left natively. Viewport/scissor rectangles are passed with
  `origin_top_left = sg_query_features().origin_top_left`, so they stay in memory-row space.
- **Uploads.** All `sg_write_buffer_transient` writes of a frame must come before the first copy
  from that staging buffer. Texture and atlas updates are queued and flushed together at the start
  of the next frame, outside a pass (`sg_copy_buffer_to_image`, `bytes_per_slice` set).
- **Instances.** One `write_transient` vertex buffer per backend, grown by powers of two; ES 3.0 has
  no base instance, so `first_instance` becomes a vertex-buffer offset in `sg_bindings`.
- **Passes.** Frame, scene and backdrop targets are plain `color_attachment` images with a
  color-attachment view and a texture view. A backdrop capture ends the scene pass, filters with
  `LOAD` (partial) or `DONTCARE` (full) and scissor, then resumes the scene with `LOAD`.
- **Frames.** One `sg_commit()` per `RenderFrame`; read-back through the native API of the backend,
  then `sg_reset_state_cache()`.

### 3.8 Single-surface platforms: iOS and Web (new platform layer work)

This is the largest new item and the least certain one. The Sokol backend alone does not
make Raven run on iOS or in a browser; the platform layer and a few core assumptions must
change. All of it is below the public widget/XML API; the only new public API is additive.

1. **Inverted loop.** Add `Rrun(RFrameCallback, void* user)`, plus a matching platform entry
   `run(frame)`. On desktop it wraps the existing `Rwait_frame` loop. On iOS the callback is
   driven by the display link; on the Web by `requestAnimationFrame`. `Rdraw_frame` /
   `Rwait_frame` stay for desktop; on these platforms they return an error that points to `Rrun`.
2. **Virtual windows.** When a platform reports `single_surface`, `Rwindow_create` for
   popups, dialogs and torn-off docks creates an in-surface window: same API, same widget
   tree, but drawn as a clipped layer (with its own draw list) inside the one native surface.
   `window_set_position`, `begin_move`, `min_size` are emulated by the core. Where a feature
   makes no sense (tear-off docks) it is refused with a clear error and the dock stays docked.
   This touches `widget/window.c` and `dock_area.c`, not the widgets themselves.
3. **Touch (required, see 3.9) and soft keyboard.** New `rvn_pevent_t` types: touch
   begin/move/end/cancel with an id and position. Add `window_show_keyboard(bool)`,
   `window_get_safe_area()`, and a text-input field rect so the OS can scroll/avoid it.
   IME composition needs a `RVN_PEVENT_TEXT_COMPOSE` event (marked text) on both platforms.
4. **Platform implementations.** Two choices; recommendation is the first:
   - **A. `sokol_app` as the platform for iOS and Web** (single window, owns the
     swapchain/canvas, runs the loop, delivers touch/key events). Fastest route, and it
     already hands back a Sokol-compatible swapchain. Risks: composition/IME, accessibility
     and clipboard are thin; we can replace pieces with our own UIKit / DOM code later.
   - **B. Own `platform/uikit` and `platform/web`** (UIKit view + `CAMetalLayer`; Emscripten
     canvas + DOM events + a hidden `<textarea>` for IME). More control, more code.
   Cocoa on macOS keeps the existing multi-window platform; `sokol_app` is not used there.
5. **Web build specifics.**
   - Emscripten, `RInitDesc.disable_render_thread = true`, `worker_threads = 0` by default;
     an opt-in threaded build (pthreads + SharedArrayBuffer) needs COOP/COEP headers.
   - Graphics: WebGL2 via Sokol GLES3 first (widest support), WebGPU via Sokol WGPU as a
     second wasm. They are separate compiles (3.1 note). Load whichever the browser supports.
   - Virtual FS (MEMFS/IDBFS) behind `Rfile_*`; `fs_watch`, native file dialogs and XML hot
     reload compiled out; file open/save through the browser's pickers is a follow-up.
   - Clipboard is asynchronous in browsers: add an async path to `clipboard_get`.
   - No `raven_web`, no AT-SPI/UIA. Accessibility via ARIA is a separate, later project.
6. **iOS build specifics.** CMake toolchain for `iphoneos`/`iphonesimulator`; an Xcode project
   or `xcodegen` template for the app; render thread stays enabled (Metal handles it);
   safe-area insets, rotation, keyboard avoidance; `raven_web`, `fs_watch`, file dialogs
   compiled out; sandbox paths for `Rfile_*`; `UIDocumentPicker` for dialogs later.
7. **Touch-first is required** (decided). It is planned work, not a follow-up: see 3.9.

### 3.9 Touch support: what Sokol gives and what Raven must build

What `sokol_app` provides (from its header, first part read; re-verify against the pinned
version): raw multitouch events (began/moved/ended/cancelled, up to 8 points, id + position)
on iOS, Android and the browser; `sapp_show_keyboard()` on iOS; character events as UTF-32;
a main loop that works on iOS and Emscripten. What it does **not** provide, as far as I found:
gesture recognition (pinch, pan, long-press), pressure/radius, IME composition (marked text),
safe-area insets (we can get them from UIKit/CSS directly), and on the web the soft keyboard
and clipboard are limited (clipboard only inside short-lived event handlers). So Raven builds:

1. **Touch event path in the core.** Platform-independent, fed by any platform (`sokol_app`
   now, native touchscreens on Windows/Linux later).
2. **Gesture recognizer** (`widget/gesture.c`): tap, double-tap, long-press, pan with
   velocity (kinetic scroll), pinch/zoom, two-finger pan, with the usual slop/timing rules
   and arbitration between competing widgets (scroll box inside a list, slider inside a scroll box).
3. **Widget behaviour:** touch-sized hit targets (themeable minimum 44 pt), no hover
   dependence (hover-only affordances get a touch equivalent), long-press = context menu,
   kinetic scrolling in `scroll_box`/`list_view`/`tree_view`/`text_editor`, drag handles for
   `slider`/`splitter`/`dock_area`, text selection handles and magnifier in `text_input`/`text_editor`,
   popups anchored to touch points, pinch zoom in `viewport`/`node_graph`/`timeline`.
4. **Touch theme:** a spacing/size theme variant (`R_THEME_TOUCH` or a density setting) so
   the same XML works at touch density.
5. **Soft keyboard and text input:** keyboard avoidance (scroll the focused input into view
   above the keyboard), IME composition events, and on the web a hidden `<textarea>` owned by
   our platform code because `sokol_app` does not do it.
6. **Testing:** synthetic touch events in the headless tests; gesture recognizer unit tests;
   iOS simulator and a real iPad; Chrome DevTools touch emulation plus a real phone.

The recognizer and the widget behaviour are the large part; a first slice (tap, pan with
inertia, long-press, keyboard avoidance) covers most of the editor-style apps.

### 3.7 Frame/capture contract

`RenderFrame(data, capture)` is unchanged. The Sokol backend renders into an offscreen
color target whenever `capture != NULL` or the surface is headless
(`R_WINDOW_SYSTEM_NONE`), reads it back natively, then (when not headless) blits to the
swapchain. This keeps `Rwindow_capture` and `test_render` working on every API.

## 4. Phases, exit criteria and estimates

Estimates are for one developer who knows the code; treat as ±40%.

| # | Phase | Work | Exit criterion | Est. |
|---|---|---|---|---|
| 0 | Spike (Linux GL, headless) **DONE** | Vendored sokol_gfx; UI, blur and effect shaders through glslang + SPIRV-Cross; EGL surfaceless context; offscreen frames; glass, effects, atlas, erase; read-back | 16 frames match the Vulkan output (max difference 0, one scene 1/255). Instance layout, `gl_VertexIndex`, varying count and glass pass structure confirmed (section 10) | done (~1 day) |
| 1 | Interface + build plumbing **DONE** | 3.1, CMake option `RAVEN_BACKEND_SOKOL` (default OFF), vendored pinned `sokol_gfx.h` with symbol prefix, THIRD_PARTY, CI job; the spike backend promoted into `src/render/sokol/` | Vulkan build unchanged and green (27/27); Sokol builds; `test_render` passes on Sokol (section 11) | done |
| 2 | Core Sokol backend (offscreen) | Instance buffer, pipelines (UI + erase), textures incl. atlas, uniforms, readback, headless surface, `R_DRAW_CALLBACK` hook | `test_render` runs headless on Sokol/GL llvmpipe in CI; all non-glass cases pass | 1.5 wk |
| 3a | Presentation: macOS **DONE (verified on a Mac)** | `present/metal` on the existing `CAMetalLayer`; resize, vsync/ProMotion pacing, DPI, occluded windows, many windows (popups/tool windows) | Examples `hello`, `editor`, `workspace` run interactively on macOS without MoltenVK | 1–1.5 wk |
| 3b | Single-surface core work | 3.8 items 1–3: `Rrun`, virtual windows, touch/IME/safe-area events; exercised first on macOS with a "single surface" test mode and the mouse model | `hello` and `editor` run with one OS window only, popups and dialogs in-surface | 3–4 wk |
| 3c | Presentation: iOS | `sokol_app` platform (or `platform/uikit`), Metal swapchain, rotation, soft keyboard, simulator + device | `hello` and `editor` run on an iPad (simulator and device), with pointer/keyboard and basic touch | 2–3 wk |
| 3d | Presentation: Web | Emscripten build, `sokol_app` canvas platform, WebGL2 via GLES3, single-threaded; later WebGPU wasm | `hello`, `editor`, `glass` run in Chrome, Safari and Firefox from a static page | 3–4 wk |
| 3g | Touch UX | 3.9 items 1–5; first slice on iOS (tap, pan + inertia, long-press, text selection, keyboard avoidance), then pinch, then Web parity | `editor` and a touch-density `hello` are usable by finger on iPad and on a phone browser | 4–6 wk |
| 3e | Presentation: Linux **DONE (verified on real hardware)** | `present/egl` (X11 visual from `RNativeWindow`), one shared context, make-current per window | Same examples on X11 (Mesa and one vendor driver) | 1 wk |
| 3f | Presentation: Windows | `present/d3d11` (DXGI flip swapchain), resize, device loss; opaque windows only | Same examples on Windows | 1 wk |
| 4 | Glass + effects | 3.2 bundles + tool; `sg_glass.c` (scene target, capture, down/up chain, scissor re-capture); BLUR, CURVE, SHADER primitives | `glass`, `effects` examples match Vulkan visually; 8 built-ins work | 1.5–2 wk |
| 5 | Transparency + interop | Metal/X11 alpha first, DirectComposition path last, `raven_sokol.h`, viewport callback with native handles | `R_WINDOW_TRANSPARENT` and a viewport example work on each API | 1–1.5 wk |
| 6 | Hardening | Perf vs Vulkan (`raven_benchmark`), docs (ARCHITECTURE, README, reference), bindings regen, CI matrix | Parity matrix (section 6) reviewed; decide default per platform | 1–2 wk |

Milestones (one developer; the backend phases are ±40%, the platform phases 3b–3d ±60%):
- **~5–6 weeks:** first usable Sokol build on macOS (Metal, no transparency, no MoltenVK).
- **~9–11 weeks:** iOS running (simulator + device) with the mouse/keyboard model.
- **~13–16 weeks:** Web (WebGL2) running with the mouse/keyboard model.
- **~17–22 weeks:** touch-first usable on iOS and Web (phase 3g runs partly in parallel).
  WebGPU and glass-on-web tuning are extra.
- **~21–26 weeks:** Linux GL and Windows D3D11 parity, hardening (phase 6).
Linux and Windows stay on Vulkan by default until phase 6.

## 5. Build, test and CI

- CMake: `RAVEN_BACKEND_SOKOL` plus `RAVEN_SOKOL_API=metal|gles3|wgpu|glcore|d3d11`,
  defaulted from the platform (a build option because Sokol fixes the API at compile time).
  macOS/iOS link Metal/QuartzCore (sokol impl in a `.m` unit); Windows links d3d11/dxgi/dcomp;
  Linux loads libEGL dynamically so nothing new is linked (same stance as Vulkan).
- New platform selections: `RAVEN_PLATFORM=uikit|web`, with an iOS toolchain file and an
  Emscripten toolchain preset in `CMakePresets.json`.
- Compile `sokol_gfx.h` into one translation unit with `SOKOL_API_DECL static` or a symbol
  prefix, so an application using its own sokol cannot clash.
- `test_render`: parametrize by `RAVEN_BACKEND`; golden comparison with a small perceptual
  tolerance (APIs differ in rounding). Report skipped when the API is unavailable.
- CI additions, in platform order: macOS (Metal), iOS simulator (build + headless run),
  Web (Emscripten build; run the headless tests in Node and the render tests in headless
  Chromium with SwiftShader/WebGL2), Linux (Mesa llvmpipe under Xvfb or surfaceless EGL),
  Windows (D3D11 WARP). Existing headless tests are unaffected.

## 6. Parity matrix (the acceptance checklist)

RECT/outline/texture · GLYPH · SHADOW · LINE · TRIANGLE · CURVE · GLASS · BLUR · SHADER
· `R_DRAW_ERASE` · `R_DRAW_BACKDROP` partial re-capture · viewport texture · viewport
draw callback · native child viewport · capture · transparent window · vsync on/off ·
resize/minimize · multi-window · hot-plug/device lost · HiDPI scale change.

## 7. Risks

| Risk | Impact | Mitigation |
|---|---|---|
| iOS/Web need single-surface support (virtual windows, inverted loop, touch) that the core never had | High: the largest unknown in the plan | Phase 0b builds the core for wasm and iOS with the null backend first; phase 3b lands on macOS where it can be tested quickly |
| Widgets are desktop-style (hover, menus, docks, small targets); touch-first UX is a design effort | Medium to High | Scope iOS/Web first as iPad/desktop-browser with pointer; treat touch-first as a separate follow-up |
| `sokol_app` is thin on IME composition, accessibility and clipboard | Medium | Start with it, keep the platform table so pieces can be replaced with UIKit/DOM code |
| WebGL2 limits (varying count, no readback, context loss) and WebGPU browser variance | Medium | Check varying count and the glass shader in the phase 0 spike (GLES3 profile); ship WebGL2 first, WebGPU as a second wasm |
| Wasm size and load time (Raven core + fonts + sokol) | Low to Medium | Measure early in 0b; fonts are already subset; strip unused widgets via build options if needed |
| Win32 transparency needs DirectComposition and creation-time window flags | Low now (Windows is last) | Phase 3c ships opaque; transparency in phase 5; 3.6 hint added early |
| Glass in sokol passes (mid-frame pass break, scissored recapture) harder than expected | Largely retired: identical to Vulkan on GL in the spike | Re-verify on Metal and WebGL2 in phases 3a/3d |
| Effect format change annoys existing `.spv` users | Medium | `.spv` stays valid on Vulkan; clear error + fallback on Sokol; migration note in `docs/shaders.html` |
| No upstream readback | Medium | Native readback per API; isolated in `sg_readback.c` |
| Sokol API churn | Medium | Pin commit, upgrade as a dedicated task |
| Linux quality on GL 4.1 / driver variance | Medium | Vulkan stays default on Linux |
| Metal/D3D11 can't be tested in this Linux dev environment | Medium | Phases 0–2 on Linux GL; Metal (first target) verified on your Mac and in CI; D3D11 later |

## 8. Decisions

Decided:
1. **Effect format:** `.rfx` bundles, with optional SPIRV-Cross later.
2. **HRL / Vulkan device sharing:** not a requirement. The Vulkan interop API stays
   Vulkan-only and is not reproduced for Sokol.
3. **Transparency:** phase 5 is fine; first Sokol builds are opaque-only.
4. **Priority:** Sokol is wanted mainly for macOS, iOS and Web.
5. **Platform order:** macOS -> iOS -> Web -> Linux -> Windows.
6. **Touch-first is required** for iOS and Web. `sokol_app` supplies raw touches only;
   Raven builds the gesture layer and touch widget behaviour (3.9).
7. **Platform layer for iOS/Web:** start with `sokol_app`; replace parts with own
   UIKit/DOM code only if IME, accessibility, safe-area or clipboard force it.
8. **Web graphics:** WebGL2 (Sokol GLES3) first, WebGPU as a second wasm.
9. **Apple default:** after phase 3a plus one release of soak time, Sokol/Metal becomes the
   default on macOS and iOS. **Our Vulkan backend stays in the tree and selectable**
   (`RAVEN_BACKEND=vulkan`, `RInitDesc.backend`); MoltenVK remains an opt-in path.

Open: none blocking. New questions will come from phase 0.

## 9. Next steps

- **Phase 0b (portability probe):** build the Raven core with `RAVEN_PLATFORM=null` and the
  null backend for Emscripten (run `test_layout`, `test_widgets` in Node) and for the iOS
  simulator. A few days; tells us how much of the core already ports before we commit to 3b.
  Needs the Emscripten SDK, and Xcode for iOS.
- **Phase 1 (interface and build plumbing)** can start now: sections 3.1 and 3.3 and the CMake
  option, since the spike removed the main technical uncertainty of the backend itself.
- **Metal check:** run the spike's shaders and frames on a Mac early (sokol Metal backend takes
  MSL; the same SPIR-V goes through SPIRV-Cross `--msl`), before building phase 3a on assumptions.

## 10. Phase 0 results (spike)

`spikes/sokol_phase0`: a standalone sokol_gfx (GLES 3.0, headless EGL on Mesa llvmpipe) backend of
about 880 lines against the public `RBackendVTable`, compared with the Vulkan backend (lavapipe) on
16 frames: the shapes, clipping, text, light and dark controls, glass, glass theme, background blur,
the 8 built-in effects, two user-written effects, and a stress sequence (re-draw, partial
backdrop re-capture, grow, shrink, a second window). All identical except the built-in effects
scene (max 1/255 on a few pixels). It ran on Raven's render thread. Both sides use llvmpipe, so
real GPUs will add normal rounding differences, which `test_render` already tolerates (3/255).
Details, reproduction steps and the list of what is not covered: `spikes/sokol_phase0/README.md`.
Not covered: Metal, D3D11, WebGL2/WebGPU runs, presentation, viewport draw callback, transparency.

## 11. Phase 1 results (interface, plumbing, and the offscreen core in the library)

Implemented (commits on `claude/vibrant-tesla-tosmjt`):

- `include/raven/raven_core.h`: `R_BACKEND_SOKOL`, `Rget_backend()`, `RAVEN_BACKEND` environment override.
  `include/raven/raven_backend.h`: `R_BACKEND_CAP_*`, `RBackendVTable::caps`, `Rbackend_get_caps()`. Vulkan and the
  null backend report their caps. Bindings regenerated (`tools/bindgen`, `bindings_up_to_date` passes).
- `third_party/sokol/sokol_gfx.h` (pinned, unmodified) compiled once in `src/render/sokol/sg_impl.c` with every
  function renamed `rvn_sokol_*` (`gen_prefix.py`), so an application's own sokol cannot clash.
- `src/render/sokol/`: the backend, split by layer: `sg_egl.c` (native API: GLES3 context, read-back),
  `sg_texture.c`, `sg_pipeline.c`, `sg_glass.c`, `sg_backend.c`; shaders generated by `gen_shaders.py`.
- CMake: `RAVEN_BACKEND_SOKOL` (OFF), `RAVEN_SOKOL_API` (`gles3`); builds with Vulkan on or off.
- Tests: `test_render` runs on the backend named by `RAVEN_BACKEND`; ctest has `test_render_sokol` in a build with both
  backends. CI: a `sokol` job (Linux, Mesa software GLES3 + lavapipe) runs the whole suite; it has not run on
  GitHub yet, only its commands locally.

Verified here (Mesa llvmpipe / lavapipe, Linux):

| Configuration | Result |
|---|---|
| Default (Vulkan only, Sokol OFF) | ctest 27/27, `test_render` 193 checks |
| Vulkan + Sokol | ctest 28/28; `test_render` 193 (Vulkan), `test_render_sokol` 175 (headless and with an Xvfb window, which is rendered offscreen) |
| Sokol only (Vulkan OFF) | ctest 27/27, `test_render` 175 on Sokol |
| Spike scenes, in-library backend vs Vulkan | 15 of 16 frames identical (effects within 1/255); the 16th uses user-written SPIR-V effects, which are refused (drawn as rectangles) until effect bundles exist |

The 18 checks that only the Vulkan run has are the viewport draw callback (`R_BACKEND_CAP_CALLBACK`) and the
`Rvulkan_*` API; both are gated by the caps, not removed.

Known limits (not regressions): no presentation to a window (frames are offscreen; `Rwindow_capture` works),
no viewport draw callbacks, only the built-in effects, GLES3/EGL only, EGL linked rather than loaded at run time.

## 12. Metal path (phase 3a, first part): written without a Mac

`RAVEN_SOKOL_API=metal` (the default on Apple) builds the same backend on Metal:

- `gen_shaders.py` emits MSL next to GLSL ES 3.00 (SPIRV-Cross `--msl`). Checked by reading the output: attribute
  slots 0-7, push constants at `[[buffer(0)]]`, `[[texture(0..2)]]` and `[[sampler(0..2)]]`, entry `main0`, vertex
  outputs and fragment inputs matching by `[[user(locnN)]]`.
- Adjustments the Metal convention needs (origin top left, NDC y up, the reverse of Vulkan): the transform flips y;
  `ui.vert` takes `abs()` of the scale for the backdrop coordinates and `blur.vert` reads its source with
  `v = 1 - y` (patches in `gen_shaders.py`, only in the MSL target); descriptor bindings are remapped so the three
  textures get distinct slots. GL is unchanged (the Vulkan convention already lands right there).
- `sg_metal.m`: device (`MTLCreateSystemDefaultDevice`), read-back (blit to a shared buffer on sokol's command
  queue), and an autorelease pool around every entry of the backend table (the render thread has no run loop).
- `sg_impl.m` compiles `sokol_gfx.h` as Objective-C; CMake links Foundation, Metal and QuartzCore and builds the
  two `.m` files with ARC. Three samplers are bound for every API now (one per texture), which GL accepts.

**Verified on the maintainer's Mac (first run, no fixes needed): `RAVEN_BACKEND=sokol ./build/bin/test_render` passes 175 checks, 0
failures** (the same 175 as on Linux GLES3): the MSL compiles, the pipelines are created and the frames, glass and blur
included, match the expected pixels, so the y-flip handling is right. Verified here before that: the Linux GLES3 build is unchanged (28/28); `sg_metal.m` passes `clang -fsyntax-only -Wall`
against stub Metal headers; the shared C files compile with the Metal macro. **Not yet verified:** a frame-for-frame comparison with Vulkan on the Mac (`spikes/sokol_phase0`, needs MoltenVK for the
Vulkan side), the effect scenes beyond what `test_render` checks, the `sokol-macos` CI job (Actions is not enabled on the
fork), and anything about presenting to a window (not implemented).

## 13. Presentation (phase 3a for macOS, 3e for Linux)

Windows are now shown, not only captured. Common design (`sg_backend.c`): the frame is still rendered offscreen, then one
more pass (`present_pipeline`, the blur shader in copy mode, whose fourth parameter flips the image) draws it into the
swapchain of the window. The platform-specific part is four functions per native API (`present_create`, `begin`,
`end`, `destroy`).

- **Linux (`sg_egl.c`)**: the EGL display is made from the X11 connection of the platform (`RBackendInitInfo::native_display`);
  one window surface per window, with the configuration whose native visual is the visual of the window; the context is
  created without a configuration (`EGL_KHR_no_config_context`) so it fits all of them; the surface is made current for the
  present pass, the swap interval follows `Rset_vsync`, `eglSwapBuffers` follows `sg_commit()`. A window that cannot be
  presented is still rendered and captured (a warning says so).
- **macOS (`sg_metal.m`)**: the layer is the `CAMetalLayer` that the Cocoa platform installs; `drawableSize` follows the frame,
  `displaySyncEnabled` follows vsync, and `nextDrawable` gives the drawable that `sokol_gfx` presents at `sg_commit()`. The
  swapchain format is BGRA8 (the frame stays RGBA8).
- Examples now honor `RAVEN_BACKEND` (they forced Vulkan): `RAVEN_BACKEND=sokol ./bin/raven_hello`.

Verified on Linux, here, by looking and by comparing: `hello` shown upright through the real X window; the `glass` and
`workspace` (several windows, docks) examples are **pixel-identical** to Vulkan in an Xvfb screenshot; the eight built-in
effects render in `effects`; `test_render` with real windows (created, resized and destroyed while presenting) passes 175/175.
The harmless log lines of `sokol_gfx` (information, unused sampler names) are no longer reported as warnings.
**Confirmed by the maintainer on real hardware:** the examples run and show windows on the Sokol backend on a Mac (Metal) and on a Linux
machine (EGL, real GPU), after building from the branch. Metal presentation was written without a Mac and worked on its first run.
**Not verified:** transparent windows and applications' own effects (neither is implemented), pixel-for-pixel comparison with Vulkan on
real GPUs.

