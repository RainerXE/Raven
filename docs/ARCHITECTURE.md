# HUI architecture

This document explains how the library is organized and why, for someone who
wants to read, debug or extend it.

## Layers

```
            application
                │  hui.h (handles, callbacks)
┌───────────────┴────────────────────────────────────────────┐
│ xml/        parser, attributes, loader                     │
│ widgets/    one file per widget type                       │
│ widget/     tree, layout, events, painting, windows, style │
├──────────────────────────────┬─────────────────────────────┤
│ render/  draw lists, fonts,  │ platform/  windows, input,  │
│          textures, renderer  │            clipboard        │
│   ├─ vulkan/                 │   ├─ win32/                 │
│   └─ null/                   │   ├─ x11/                   │
│                              │   └─ null/                  │
├──────────────────────────────┴─────────────────────────────┤
│ core/   memory, handles, threads, jobs, lock-free map      │
└────────────────────────────────────────────────────────────┘
```

A layer only uses the layers below it. `platform/` and `render/` do not know
each other: the core passes the native window handle from one to the other.
Both are tables of functions (`hui_platform_t`, `HUI_BackendVTable`), chosen
at `HUI_Init`.

Naming: `HUI_` is the public API, `hui_` is internal. One global, `g_hui`
(`core/context.h`), holds the state of the library.

## Handles

`core/handle.h`. A handle is 32 bits: `[ type : 4 | generation : 8 | index : 20 ]`.
Each object type has a pool mapping the index to a pointer. Deleting an
object increments the generation of its slot, so a stale handle no longer
matches and the call reports `HUI_ERROR_INVALID_ID`. `0` and `HUI_INVALID_ID`
are never valid. Every public function starts by resolving its handles
(`HUI_WIDGET(widget, id, type)` in `widget/widget.h`), which also checks the
widget type.

## Threads

```
 UI thread                         worker threads              render thread
 (the one that called HUI_Init)    (job system)
 ─────────────────────────────     ───────────────────────     ─────────────────────
 events, callbacks                 paint jobs of large trees   every backend call:
 widget tree, layout               HUI_RunAsync                surfaces, textures,
 paint -> frame packet ──────────────────────────────────────▶ frames, present
 HUI_PostTask tasks                HUI_ParallelFor             viewport draw callbacks
                                   texture decoding
```

**UI thread.** Owns the widget tree. Nothing else reads or writes widgets,
except paint jobs, which only read and only run while the UI thread waits
for them.

**Render thread** (`render/renderer.c`). Every backend call becomes a command
in one ordered queue: create a surface, upload a texture, render a frame. A
frame is a packet (`hui_frame_t`) holding its own copy of the instances, so
the UI thread goes on while the frame is rendered. The UI thread only blocks
when two frames of a window are already queued; that back-pressure is what
paces a UI that redraws continuously. `HUI_InitDesc::disable_render_thread`
runs the commands inline instead.

**Job system** (`core/jobs.c`). A fixed pool of workers and one queue. A
thread waiting for a group of jobs runs jobs itself instead of sleeping, so
nested parallel loops cannot deadlock and no core idles. By default there is
one worker per core left after the UI and render threads.

**Parallel painting** (`widget/paint.c`). A widget whose subtree holds at
least 512 widgets splits its children into chunks; each chunk is painted
into its own draw list by a job, and the lists are concatenated in child
order, which gives exactly the result of a serial paint. Widgets whose
painting calls application code (custom widgets) are always painted on the
UI thread.

**Fonts** are the one structure paint jobs write to: a glyph that is not in
the atlas yet must be rasterized. Lookups go through a hash map that is
lock-free for readers (`core/lfmap.c`); only a miss takes a mutex.

**Deferred destruction.** Deleting a widget or a window from inside one of
its own callbacks is allowed: while an event is being dispatched
(`g_hui.dispatch_depth > 0`), deleted objects are marked dead and freed when
the dispatch ends.

## A frame

`HUI_Update` runs, for each window that needs it:

1. **Tasks and timers**, then the frame callback of the application.
2. **Layout** (`widget/layout.c`, only if something changed):
   - *measure*, bottom-up: each widget computes its desired size from its
     children. The result is cached; a change marks the widget and its
     ancestors dirty, nothing else.
   - *arrange*, top-down: each panel gives a rectangle to its children
     according to their slots. A widget whose rectangle and content did not
     change is not arranged again.
3. **Paint** (`widget/paint.c`): the tree is walked and each widget appends
   *instances* to a draw list through the painter (`render/draw.c`), which
   converts logical units to pixels, snaps to the pixel grid and applies
   clipping and opacity. Subtrees outside the clip rectangle are skipped.
4. **Submit**: the frame packet goes to the render thread.

A window is only laid out or painted when something asked for it (a property
changed, an event, a realtime viewport). `HUI_Run` sleeps in the window
system until an event, a timer, a posted task or `HUI_WakeUp`.

## Rendering model

`include/hui/hui_backend.h`. The whole UI is a flat array of
`HUI_DrawInstance` (80 bytes) and a short list of commands.

- One instance is one primitive: rounded rectangle with optional outline and
  texture, glyph, soft shadow, line, triangle.
- The vertex shader expands each instance into a quad (`gl_VertexIndex`,
  no vertex or index buffer). The fragment shader evaluates the exact shape
  as a signed distance in pixels, so edges are anti-aliased at any size and
  rounded corners cost nothing more than a rectangle.
- Clipping is a rectangle stored in each instance and tested in the fragment
  shader, so a clip change never breaks a batch.
- Untextured instances do not care which texture is bound. A new command
  only starts when a textured instance needs another texture. Text uses one
  atlas texture, so a typical window is 1 to 3 draw calls.
- Colors are premultiplied in the shader; blending is `(ONE, ONE_MINUS_SRC_ALPHA)`.
  The UI is not color managed: values are written as they are to a UNORM
  surface.

**Text** (`render/font.c`). Glyphs are rasterized by stb_truetype at the
exact pixel size (logical size x window scale) and packed into 1024 x 1024
single-channel atlas pages shared by all fonts. Pages changed during a paint
are uploaded before the frame that uses them. Missing glyphs follow a chain
of fallback fonts.

## The Vulkan backend

`src/render/vulkan/`, about 2 800 lines.

- `vk_loader.c` opens the Vulkan library at run time and fills a table of
  function pointers (`hui_vk.CreateImage(...)`). HUI never links to Vulkan.
- `vk_device.c` creates the instance and device, or adopts the ones of the
  application (`HUI_Vulkan_SetExternalDevice`). Vulkan 1.0 is enough; 1.1 is
  only used for shared textures.
- `vk_surface.c` holds one surface and swapchain per window. The window
  system headers are not included: the create-info structures of the surface
  extensions are declared locally. X11 windows work with either
  `VK_KHR_xlib_surface` or `VK_KHR_xcb_surface`; `VK_EXT_headless_surface`
  renders without a window (tests).
- `vk_pipeline.c`: one pipeline per swapchain format, one descriptor set per
  texture.
- `vk_texture.c`: uploads are batched into one command buffer per frame.
  Textures can wrap an image view of the application or export their memory.
- `vk_frame.c`: two frames in flight per window, each with a persistently
  mapped instance buffer; viewport callbacks; capture.

Objects are destroyed after waiting for the frames that may use them
(`hui_vk_wait_frames`); this only happens on rare events (a texture is
deleted, a swapchain is resized), never in a steady frame.

## Platforms

`platform/platform.h` is the whole contract: create a window, change it,
pump events into one handler, clipboard, child windows, wake-up.

**Win32.** Messages are dispatched by `hui_win32_pump`. Two things are
specific to Windows:

- While the user moves or resizes a window, Windows runs its own message
  loop and `HUI_Update` is not called. A timer emits `HUI_PEVENT_MODAL_TICK`,
  on which the core produces a frame, so the content follows the resize.
- Graphics drivers send messages to the window from the thread that creates
  or resizes a swapchain. If the UI thread were blocked on the render thread
  at that moment, both would wait forever. Whenever the UI thread waits for
  the render thread, it therefore keeps answering *sent* messages
  (`hui_platform_t::wait_service`, `hui_renderer_wait`), without dispatching
  anything to the core.

**X11.** Xlib only. Text input goes through an X input context (dead keys,
compose). The scale comes from the `Xft.dpi` resource. A self-pipe wakes the
event loop from other threads.

## Adding things

**A widget.** Copy a small one (`widgets/progress_bar.c`): a structure that
starts with `hui_widget_t`, a `hui_widget_class_t` with the callbacks it
needs (`measure`, `arrange`, `paint`, `event`, `set_attribute`), and its
public functions. Then add the type to `HUI_EWidgetType`, the class to the
table in `widget/registry.c` and the file to `CMakeLists.txt`. The XML tag
is the class name.

**A window system.** Implement `hui_platform_t` in `platform/<name>/`, return
it from `hui_platform_select` (`platform/platform.c`), and add the matching
surface creation in `vk_surface.c`. `platform/null/null_platform.c` is the
smallest implementation.

**A backend.** Implement the ten functions of `HUI_BackendVTable`; an
application can do it without touching HUI (`HUI_RegisterBackend`, then
`HUI_Init(HUI_BACKEND_CUSTOM)`). The shaders to port are
`render/vulkan/shaders/ui.vert` and `ui.frag`.

## Tests

| | |
|---|---|
| `test_layout` | panels, slots, dirty propagation |
| `test_widgets` | controls driven by injected mouse and keyboard events |
| `test_xml` | parser, loader, attributes, error reporting |
| `test_threads` | jobs, posted tasks, asynchronous textures, parallel paint equal to serial paint |
| `test_render` | real frames through Vulkan, pixels compared; skipped without a Vulkan driver |

The first four run with the null platform and the null backend: no display,
no GPU. Input is injected with `HUI_InjectMouseMove`, `HUI_InjectKey`...,
which follow the same path as real events.
