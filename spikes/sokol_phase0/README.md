# Phase 0 spike: Raven on sokol_gfx (GLES 3.0, headless)

A standalone Raven rendering backend on `sokol_gfx` (pinned, see `third_party/README.md`), written against
the public `RBackendVTable` only. It renders headless through EGL "surfaceless" on Mesa llvmpipe and is
compared pixel for pixel with Raven's Vulkan backend (lavapipe) on the same scenes.
Goal: answer the open technical questions of `docs/SOKOL_BACKEND_PLAN.md` before touching the interface.

## Result

16 frames (8 scenes, 2 effect scenes, 6 stress frames), `compare.py` output:

```
scene           size        max    mean       >3   verdict
blur            320x120      0   0.000    0.00%   identical
clipping        300x200      0   0.000    0.00%   identical
controls_dark   460x360      0   0.000    0.00%   identical
controls_light  460x240      0   0.000    0.00%   identical
effects         520x260      1   0.000    0.00%   within tolerance   (8 built-in effects)
effects_user    300x130      0   0.000    0.00%   identical          (wave.spv, liquid.spv of examples/effects)
glass           400x200      0   0.000    0.00%   identical
glass_theme     420x320      0   0.000    0.00%   identical
shapes          400x300      0   0.000    0.00%   identical
stress_1..6                  0                    identical          (re-draw, partial re-capture, grow, shrink, 2nd window, destroy)
text            420x200      0   0.000    0.00%   identical
```

Both sides run on Mesa's llvmpipe, so the shader code generation is the same; on real GPUs expect +-1..2/255
(the tolerance `tests/test_render.c` already uses). What the zero difference does prove is that the
logic (shader translation, instance layout, blending, glass passes, atlas uploads) is equivalent.
The backend ran on Raven's render thread (the default), with the EGL context created there.

## Answers to the phase 0 questions

| Question | Answer |
|---|---|
| Instance layout | Works unchanged: 4x `FLOAT4`, 2x `UBYTE4N`, `FLOAT`, and `UINT` for `flags` (`SG_VERTEXFORMAT_UINT` exists in the pinned sokol_gfx). No change to `RDrawInstance`. |
| `gl_VertexIndex`, push constants | glslang -> SPIR-V -> SPIRV-Cross turns `gl_VertexIndex` into `gl_VertexID` and push constants into a flat `uniform vec4[]`, bound by name. The UI shaders compile **unmodified**. |
| Varying count (WebGL2) | The UI interface is 11 varyings, about 9 packed vec4 rows (counted, not measured on a WebGL2 device), under the 15 WebGL2 guarantees. |
| Glass pass structure | Works: scene target, end pass, scissored dual-filter chain (LOAD action keeps the rest), resume the scene pass with LOAD, copy to the frame. Identical to Vulkan, including partial re-captures. |
| Effects / custom shaders | Work through a precompiled GLES3 variant. Needs two fixes in the translation step (below). A backend cannot translate SPIR-V at run time here, which confirms the `.rfx` bundle decision. |
| Size | The whole offscreen backend (surfaces, textures, atlas, erase, glass, effects) is about 880 lines of C, against about 3,600 for the Vulkan backend. Presentation is not in it yet. |

## Findings that change the plan

1. **Y convention.** Raven draws in "Vulkan convention" (row 0 = top, NDC y down). On GL that puts the UI top in
   framebuffer row 0, so offscreen images, backdrop sampling and `glReadPixels` need **no flip**, and
   `gl_FragCoord` already matches the UI pixel space. Only presenting to a GL default framebuffer needs a flip.
   The `origin_top_left` argument of `sg_apply_viewport/scissor` must be `sg_query_features().origin_top_left`
   (false on GL, true on Metal/D3D11/WebGPU) so rectangles stay in memory-row space.
2. **Uploads.** `sg_write_buffer_transient` writes are only allowed until the first copy from that buffer in the
   same frame. Texture and atlas updates are therefore queued and flushed together at the start of the next
   frame (staging buffer, then `sg_copy_buffer_to_image`, outside a pass). `bytes_per_slice` must be set.
3. **No base-instance.** ES 3.0 lacks it: a command's `first_instance` becomes a vertex-buffer offset in `sg_bindings`.
4. **Push constants of mixed type.** `blur.frag` has a `vec2` and an `int`; SPIRV-Cross cannot flatten them for GL.
   The spike packs them into one `vec4`. The shared-source step (plan 3.3) must do the same.
5. **Effects.** GLES 3.0 / WebGL2 link varyings **by name**, and the effect fragment shader names differ from
   `ui.vert` (`rvn_v_*` vs `v_*`). The bundle tool must rename the fragment inputs by location
   (`spirv-cross --rename-interface-variable`). Samplers an effect does not use are dropped by the GL
   compiler and sokol warns; the bundle should record which samplers a variant really uses.
6. **ALPHA8 atlas.** Expanding to RGBA8 on upload (1,1,1,a) needs no shader change and rendered text identically.
7. **sokol-shdc.** Not used: downloading and running its prebuilt binary was declined by the sandbox policy, so
   SPIRV-Cross from the Ubuntu archive does the same translation. It remains the intended production tool
   once it is vendored or built from source in a way you approve.

## Not covered by this spike

Real Metal, D3D11, WebGL2 and WebGPU runs; presentation (swapchains, vsync, resize of a live window, GL flip on
present); `R_DRAW_CALLBACK` (viewport draw callback); transparent windows; effect bundle format; multi-context
GL for several windows.

## Reproduce

```sh
sudo apt install libegl-dev libgles-dev mesa-vulkan-drivers glslang-tools spirv-cross xvfb   # Ubuntu 24.04
cmake -S ../.. -B build-ref -G Ninja -DCMAKE_BUILD_TYPE=Release && ninja -C build-ref raven
./gen_shaders.sh                                   # SPIR-V -> GLSL ES 3.00 (UI, blur, effects)
./build.sh build-ref out                           # builds out/scenes
mkdir -p out/vk out/sg
VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/lvp_icd.json ./out/scenes vulkan out/vk ../../examples/effects
./out/scenes sokol out/sg ../../examples/effects
./compare.py out/vk out/sg --png out/cmp           # reference | candidate | difference x16
```
