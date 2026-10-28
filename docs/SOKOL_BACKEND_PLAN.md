# Plan: a selectable Sokol rendering backend

Status: proposal. Based on a read of the code (no prototype yet).
Goal: add `R_BACKEND_SOKOL` next to `R_BACKEND_VULKAN`, selectable at `Rinit`.
Platform order (decided): **macOS (Metal) first, then Linux (GL/EGL), then Windows (D3D11).**
Vulkan stays, and stays the default, until the Sokol backend reaches parity.
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
                                                               d3d11 (Win32) · metal (Cocoa) · egl/glx (X11) · offscreen
```

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

## 3. Intermediate-layer changes

### 3.1 Backend interface v2 (`include/raven/raven_backend.h`, `render/renderer.*`, `core/context.c`)

- Add a leading `size`/version field and trailing optional members to `RBackendVTable`;
  `Rbackend_register` keeps requiring only the original members, so existing custom
  backends still work.
- Add `caps` flags the core can query: `GLASS`, `ERASE`, `SPIRV_EFFECTS`,
  `BUNDLE_EFFECTS`, `READBACK`, `NATIVE_CALLBACK`, `SHARED_TEXTURE`. The core uses them to
  degrade gracefully (today it assumes "NULL entry = no effect").
- Add `CreateShaderEx(Rid, const RShaderBlob*)` (3.2). `CreateShader(spirv)` stays.
- Add `R_BACKEND_SOKOL` to `RBackend`; `Rinit` honors `RAVEN_BACKEND=vulkan|sokol` and
  `RAVEN_SOKOL_API=d3d11|metal|gl|gles` (env overrides, for tests and users).
- `RBackendInitInfo` gains the requested sokol API and a headless flag.

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
| 3a | Presentation: macOS | `present/metal` on the existing `CAMetalLayer`; resize, vsync, DPI, occluded windows, many windows (popups/tool windows) | Examples `hello`, `editor`, `workspace` run interactively on macOS | 1–1.5 wk |
| 3b | Presentation: Linux | `present/egl` (X11 visual from `RNativeWindow`), one shared context, make-current per window; same checks | Same examples on X11 (Mesa and one vendor driver) | 1 wk |
| 3c | Presentation: Windows | `present/d3d11` (DXGI flip swapchain), resize, device loss; opaque windows only | Same examples on Windows | 1 wk |
| 4 | Glass + effects | 3.2 bundles + tool; `sg_glass.c` (scene target, capture, down/up chain, scissor re-capture); BLUR, CURVE, SHADER primitives | `glass`, `effects` examples match Vulkan visually; 8 built-ins work | 1.5–2 wk |
| 5 | Transparency + interop | Metal/X11 alpha first, DirectComposition path last, `raven_sokol.h`, viewport callback with native handles | `R_WINDOW_TRANSPARENT` and a viewport example work on each API | 1–1.5 wk |
| 6 | Hardening | Perf vs Vulkan (`raven_benchmark`), docs (ARCHITECTURE, README, reference), bindings regen, CI matrix | Parity matrix (section 6) reviewed; decide default per platform | 1–2 wk |

Metal + GL + D3D11 parity: roughly 8–11 weeks. Because macOS comes first, the first
usable Sokol build (Metal, no transparency) should land after phase 3a, about 5–6 weeks in.
Linux stays on Vulkan by default until phase 6 even though GL support is built second.

## 5. Build, test and CI

- CMake: `RAVEN_BACKEND_SOKOL` plus `RAVEN_SOKOL_D3D11|METAL|GL|GLES` selected by
  platform; Windows links d3d11/dxgi/dcomp, macOS links Metal/QuartzCore (sokol impl in a
  `.m` unit), Linux loads libEGL dynamically so nothing new is linked (same stance as Vulkan).
- Compile `sokol_gfx.h` into one translation unit with `SOKOL_API_DECL static` or a symbol
  prefix, so an application using its own sokol cannot clash.
- `test_render`: parametrize by `RAVEN_BACKEND`; golden comparison with a small perceptual
  tolerance (APIs differ in rounding). Report skipped when the API is unavailable.
- CI additions, in platform order: macOS (Metal), Linux (Mesa llvmpipe under Xvfb or
  surfaceless EGL), Windows (D3D11 WARP). Existing headless tests are unaffected.

## 6. Parity matrix (the acceptance checklist)

RECT/outline/texture · GLYPH · SHADOW · LINE · TRIANGLE · CURVE · GLASS · BLUR · SHADER
· `R_DRAW_ERASE` · `R_DRAW_BACKDROP` partial re-capture · viewport texture · viewport
draw callback · native child viewport · capture · transparent window · vsync on/off ·
resize/minimize · multi-window · hot-plug/device lost · HiDPI scale change.

## 7. Risks

| Risk | Impact | Mitigation |
|---|---|---|
| Win32 transparency needs DirectComposition and creation-time window flags | Low now (Windows is last) | Phase 3c ships opaque; transparency in phase 5; 3.6 hint added early |
| Glass in sokol passes (mid-frame pass break, scissored recapture) harder than expected | High for look | Prototype in phase 0/2 on GL before D3D11/Metal |
| Effect format change annoys existing `.spv` users | Medium | `.spv` stays valid on Vulkan; clear error + fallback on Sokol; migration note in `docs/shaders.html` |
| No upstream readback | Medium | Native readback per API; isolated in `sg_readback.c` |
| Sokol API churn | Medium | Pin commit, upgrade as a dedicated task |
| Linux quality on GL 4.1 / driver variance | Medium | Vulkan stays default on Linux |
| Metal/D3D11 can't be tested in this Linux dev environment | Medium | Phases 0–2 on Linux GL; Metal (first target) verified on your Mac and in CI; D3D11 later |

## 8. Decisions

1. **Effect format:** `.rfx` bundles, with optional SPIRV-Cross later. *Decided: yes.*
2. **Platform priority:** macOS (Metal), then Linux (GL/EGL), then Windows (D3D11). *Decided.*
3. **HRL / Vulkan device sharing:** *Decided: not a requirement.* (HRL appears only as an
   example in the comments of `raven_vulkan.h` and `raven_backend.h`: an external renderer
   that could hand Raven its Vulkan device. Nothing in Raven depends on it.) The Vulkan
   interop API (`Rvulkan_set_external_device`, wrapped image views, shared textures) stays
   Vulkan-only and is not reproduced for Sokol.
4. **Transparency:** *Decided: phase 5 is fine.* The first Sokol builds are opaque-only.

## 9. First step

Phase 0 on Linux (Mesa llvmpipe, headless). It is small, runs in this environment, and
answers the three open technical questions (instance layout, shader cross-compile, glass
pass structure) before any interface change is committed. The result carries over to
Metal, since sokol-shdc emits MSL from the same shader source.
