# Plan: a selectable Sokol rendering backend

Status: proposal. Based on a read of the code (no prototype yet).
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
| Vertex data | `RDrawInstance` has `uint32` colors/flags | Declare as `UBYTE4(N)` and rebuild in shader; verify UINT support before relying on it |
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

- Add a leading `size`/version field and trailing optional members to `RBackendVTable`;
  `Rbackend_register` keeps requiring only the original members, so existing custom
  backends still work.
- Add `caps` flags the core can query: `GLASS`, `ERASE`, `SPIRV_EFFECTS`,
  `BUNDLE_EFFECTS`, `READBACK`, `NATIVE_CALLBACK`, `SHARED_TEXTURE`. The core uses them to
  degrade gracefully (today it assumes "NULL entry = no effect").
- Add `CreateShaderEx(Rid, const RShaderBlob*)` (3.2). `CreateShader(spirv)` stays.
- Add `R_BACKEND_SOKOL` to `RBackend`; `Rinit` honors `RAVEN_BACKEND=vulkan|sokol`
  (env override, for tests and users). The Sokol API (Metal, GLES3, WebGPU, GL, D3D11) is a
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
- `rvn_shader_t` stores the blob; the renderer hands it to whichever backend is running.
- Optional later: `RAVEN_WITH_SPIRV_CROSS=ON` translates `.spv` at run time for users who
  accept a C++ dependency. Off by default.
- The 8 built-in effects are compiled into bundles at build time and embedded like the
  current `*_spv.h`.

### 3.3 One source of truth for the UI shaders (`render/vulkan/shaders` → `render/shaders`)

Move `ui.vert`, `ui.frag`, `blur.vert/frag` and the effect shaders to a neutral
`render/shaders/` folder with a thin wrapper per compiler (glslang for Vulkan, sokol-shdc
`@vs/@fs` for Sokol). Logic is shared, so glass/blur/curve look the same on both.
Generated outputs are committed (as `generated/*_spv.h` is today) plus
`render/sokol/generated/*.h`; `cmake/RavenSokolShaders.cmake` regenerates them only when
`RAVEN_REGENERATE_SHADERS=ON` and sokol-shdc is found.

### 3.4 Glyph atlas

Spike default: the Sokol backend stores `R_TEXTURE_ALPHA8` as RGBA8 (1,1,1,a) by expanding
on upload (4 MB per 1024² page, trivial). Optimize later by sampling `.r` for
`R_PRIM_GLYPH` in the Sokol shader variant. No change to `font.c`.

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
3. **Touch and soft keyboard.** New `rvn_pevent_t` types: touch begin/move/end/cancel,
   and pinch/pan/long-press as an optional gesture layer. The core maps one finger to the
   existing mouse path by default, so existing widgets work with the mouse model first,
   and gets real touch behaviour (scroll inertia, larger hit targets, long-press = right
   click) in a later step. Add `window_show_keyboard(bool)`, `window_get_safe_area()`,
   and a text-input field rect so the OS can scroll/avoid it. IME composition needs a
   `RVN_PEVENT_TEXT_COMPOSE` event (marked text) on both platforms.
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
7. **Product question (see section 8):** desktop-style Raven on iPad with keyboard/pointer is
   a small step; a touch-first phone UI (menus, docks, hover states) is a design project on
   top of all of the above.

### 3.7 Frame/capture contract

`RenderFrame(data, capture)` is unchanged. The Sokol backend renders into an offscreen
color target whenever `capture != NULL` or the surface is headless
(`R_WINDOW_SYSTEM_NONE`), reads it back natively, then (when not headless) blits to the
swapchain. This keeps `Rwindow_capture` and `test_render` working on every API.

## 4. Phases, exit criteria and estimates

Estimates are for one developer who knows the code; treat as ±40%.

| # | Phase | Work | Exit criterion | Est. |
|---|---|---|---|---|
| 0 | Spike (Linux GL, headless) | Vendor sokol_gfx; port `ui.vert/frag` via sokol-shdc; EGL surfaceless context; render a rect+text frame offscreen; read back | PNG matches Vulkan output of the same draw list within tolerance. Also confirm: UBYTE4 instance layout, `gl_VertexIndex` handling, varying count | 3–4 days |
| 1 | Interface + build plumbing | 3.1, 3.3, CMake option `RAVEN_BACKEND_SOKOL` (default OFF), THIRD_PARTY update | Vulkan build unchanged and green; Sokol builds | ~1 wk |
| 2 | Core Sokol backend (offscreen) | Instance buffer, pipelines (UI + erase), textures incl. atlas, uniforms, readback, headless surface, `R_DRAW_CALLBACK` hook | `test_render` runs headless on Sokol/GL llvmpipe in CI; all non-glass cases pass | 1.5 wk |
| 3a | Presentation: macOS | `present/metal` on the existing `CAMetalLayer`; resize, vsync/ProMotion pacing, DPI, occluded windows, many windows (popups/tool windows) | Examples `hello`, `editor`, `workspace` run interactively on macOS without MoltenVK | 1–1.5 wk |
| 3b | Single-surface core work | 3.8 items 1–3: `Rrun`, virtual windows, touch/IME/safe-area events; exercised first on macOS with a "single surface" test mode and the mouse model | `hello` and `editor` run with one OS window only, popups and dialogs in-surface | 3–4 wk |
| 3c | Presentation: iOS | `sokol_app` platform (or `platform/uikit`), Metal swapchain, rotation, soft keyboard, simulator + device | `hello` and `editor` run on an iPad (simulator and device), with pointer/keyboard and basic touch | 2–3 wk |
| 3d | Presentation: Web | Emscripten build, `sokol_app` canvas platform, WebGL2 via GLES3, single-threaded; later WebGPU wasm | `hello`, `editor`, `glass` run in Chrome, Safari and Firefox from a static page | 3–4 wk |
| 3e | Presentation: Linux | `present/egl` (X11 visual from `RNativeWindow`), one shared context, make-current per window | Same examples on X11 (Mesa and one vendor driver) | 1 wk |
| 3f | Presentation: Windows | `present/d3d11` (DXGI flip swapchain), resize, device loss; opaque windows only | Same examples on Windows | 1 wk |
| 4 | Glass + effects | 3.2 bundles + tool; `sg_glass.c` (scene target, capture, down/up chain, scissor re-capture); BLUR, CURVE, SHADER primitives | `glass`, `effects` examples match Vulkan visually; 8 built-ins work | 1.5–2 wk |
| 5 | Transparency + interop | Metal/X11 alpha first, DirectComposition path last, `raven_sokol.h`, viewport callback with native handles | `R_WINDOW_TRANSPARENT` and a viewport example work on each API | 1–1.5 wk |
| 6 | Hardening | Perf vs Vulkan (`raven_benchmark`), docs (ARCHITECTURE, README, reference), bindings regen, CI matrix | Parity matrix (section 6) reviewed; decide default per platform | 1–2 wk |

Milestones (one developer; the backend phases are ±40%, the platform phases 3b–3d ±60%):
- **~5–6 weeks:** first usable Sokol build on macOS (Metal, no transparency, no MoltenVK).
- **~9–11 weeks:** iOS running (simulator + device), mouse/keyboard model plus basic touch.
- **~13–16 weeks:** Web (WebGL2) running. WebGPU, glass-on-web tuning and real touch UX are extra.
- **~16–21 weeks:** Linux GL and Windows D3D11 parity, hardening (phase 6).
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
| Glass in sokol passes (mid-frame pass break, scissored recapture) harder than expected | High for look | Prototype in phase 0/2 on GL before D3D11/Metal |
| Effect format change annoys existing `.spv` users | Medium | `.spv` stays valid on Vulkan; clear error + fallback on Sokol; migration note in `docs/shaders.html` |
| No upstream readback | Medium | Native readback per API; isolated in `sg_readback.c` |
| Sokol API churn | Medium | Pin commit, upgrade as a dedicated task |
| Linux quality on GL 4.1 / driver variance | Medium | Vulkan stays default on Linux |
| Metal/D3D11 can't be tested in this Linux dev environment | Medium | Phases 0–2 on Linux GL; Metal (first target) verified on your Mac and in CI; D3D11 later |

## 8. Decisions

Decided:
1. **Effect format:** `.rfx` bundles, with optional SPIRV-Cross later.
2. **Platform order (earlier):** macOS, then Linux, then Windows.
3. **HRL / Vulkan device sharing:** not a requirement. (HRL appears only as an example in the
   comments of `raven_vulkan.h` and `raven_backend.h`.) The Vulkan interop API stays
   Vulkan-only and is not reproduced for Sokol.
4. **Transparency:** phase 5 is fine; first Sokol builds are opaque-only.
5. **Priority (new):** Sokol is wanted mainly for macOS, iOS and Web.

Open, with my recommendation:
6. **Order with iOS and Web:** macOS -> iOS -> Web -> Linux -> Windows. (This replaces the
   earlier macOS -> Linux -> Windows order. Linux/Windows move later because they already
   work on Vulkan.)
7. **iOS/Web target UX:** first iPad/desktop-browser with keyboard and pointer, touch-first
   phone UI later. Tell me if a phone-first UI is required for the first release.
8. **Platform layer for iOS/Web:** start with `sokol_app` (option A in 3.8), replace parts
   with own UIKit/DOM code only if IME, accessibility or clipboard force it.
9. **Web graphics:** WebGL2 first, WebGPU as a second wasm.
10. **Apple default:** once phase 3a passes, make Sokol/Metal the default on macOS and iOS
    (Vulkan/MoltenVK stays selectable) or only after phase 6? I would do it at 3a plus one
    release of soak time.

## 9. First steps

- **Phase 0 (Linux, headless GL / GLES3):** runs in this environment, answers the open
  technical questions (instance layout, shader cross-compile, glass pass structure, varying
  count under the WebGL2 limit). The result carries over to Metal, since sokol-shdc emits
  MSL from the same shader source, and to WebGL2.
- **Phase 0b (portability probe):** build the Raven core with `RAVEN_PLATFORM=null` and the
  null backend for Emscripten (run `test_layout`, `test_widgets` in Node) and for the iOS
  simulator. This costs a few days and tells us how much of the core already ports before we
  commit to 3b. Needs the Emscripten SDK, and Xcode for iOS.
