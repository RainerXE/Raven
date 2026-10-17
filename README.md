# Raven

A lightweight, multithreaded GUI library in C for editors and tools.

- **C11, no dependency.** Raven opens its own native windows (Win32, X11, Cocoa on macOS), loads
  Vulkan at run time and has its own memory allocator. A stripped shared
  library is about 1.3 MB, two fonts included.
- **Slate-style layout.** Boxes, grid, overlay, border, canvas, scroll box,
  splitter, wrap box, switcher, tabs, expander. Each child carries a *slot*
  (padding, alignment, auto or fill size) read by its parent.
- **Windows written in XML**, or built with the C API, or both. A document
  can declare **its own tags** (`<Widget Name="Card">`), with parameters.
- **Everything moves.** Animations are springs, as on iOS: buttons squash and
  bounce back, popups pop out of what opened them, panels fly to their place,
  lists stretch at their ends. Widgets arrive with transitions (`Appear="Drop"`).
- **What a tool needs**: a multi-line **text editor** with languages (colors,
  proposals, brackets, indentation), a **header bar** that replaces the title
  bar of the system, **docking**, the **file dialogs** of the system and a
  file browser widget, **drag and drop** from other applications.
- **Materials.** Solid, or three kinds of **glass** (clear, frosted, tinted)
  that blur and bend what is behind them, for one widget or for everything;
  panels that only **blur** what is behind; windows that are **seen through**,
  with the blur of the system behind them; an **island** that unfolds at the
  top of the window.
- **Tools of the trade.** A **graph of nodes** whose wires hang and swing, a
  **timeline** (tracks, clips, keys, markers), a complete **color picker**.
- **Themes you write.** Theme files, styles for a type of widget or for a
  class, a theme for a part of a window. **Markdown** in labels and in the
  text editor. Interface and theme files are **loaded again while the
  application runs** when they are saved.
- **Web pages**, in a separate and optional library (`raven_web`) that drives
  a browser already installed.
- **Viewports** for content rendered by the application: a texture (zero-copy
  Vulkan image, shared memory for OpenGL, or CPU pixels), a draw callback
  inside the render pass of Raven, or a native child window for any API.
- **Multithreaded.** A render thread owns the GPU, a job system paints large
  trees in parallel and runs your asynchronous work.
- **Your loop.** Raven draws one frame per call: `while (Rdraw_frame()) { ... }`.
- **Vulkan renderer behind an interface**: another backend is a table of functions.
- **Opaque handles** in the style of HRL: every object is a `Rid`,
  validated on every call, with errors reported through a callback.
- **A designer** (`raven_designer`) that edits the XML visually, and
  **bindings** for C++, Python, Rust, Go, C#, Zig and LuaJIT.
- **One header**: `#include <Raven.h>`.

> **Raven is free — with a watermark.** Use it for anything; what you make
> with it is yours. A license removes the watermark: **$20 once** for
> individuals, **$200/month** for companies.
> [See the license page](https://oscar-soirey.github.io/Raven/license.html).

![The studio example](docs/images/studio_glass.png)

*`examples/studio`: a graph of nodes with widgets inside them, a color picker and a timeline, with the dark glass theme.*

![The designer](docs/images/designer.png)

*`raven_designer`: the widgets that can be added, the tree, the interface built for real, the attributes of the selected element, and the XML.*

![The workspace example](docs/images/workspace.png)

*`examples/workspace`: docked panels, a file browser and an island, with the dark glass theme and a custom header bar.*

![The glass example](docs/images/glass.png)

*`examples/glass`: a frosted side bar, a tinted tool bar and a clear player bar over a page that scrolls under them.*

```c
#include <Raven.h>

static void on_clicked(Rid button, void* user) { Rquit(0); }

int main(void) {
    Rinit(R_BACKEND_VULKAN);

    Rid window = Rwindow_create("Hello", 420, 220, R_WINDOW_DEFAULT);
    Rid box = Rwidget_create(R_WIDGET_VBOX);
    Rpanel_set_spacing(box, 12);
    Rslot_set_halign(box, R_HALIGN_CENTER);
    Rslot_set_valign(box, R_VALIGN_CENTER);
    Rwindow_set_content(window, box);

    Rwidget_add_child(box, Rlabel_create("Hello from Raven"));
    Rid button = Rwidget_add_child(box, Rbutton_create("Quit"));
    Rbutton_set_clicked_callback(button, on_clicked, NULL);

    while (Rwait_frame(-1.0)) {}     /* the loop is yours: see "The loop" */
    Rshutdown();
    return 0;
}
```

The same window in XML (`Rwindow_create_from_xml_file("hello.xml")`):

```xml
<Window Title="Hello" Width="420" Height="220">
    <VBox Spacing="12" Slot.HAlign="Center" Slot.VAlign="Center">
        <Label>Hello from Raven</Label>
        <Button Text="Quit" OnClicked="app.quit"/>
    </VBox>
</Window>
```

## License

**Raven is free to use, for anything.** Whatever you build with Raven is
yours, with a license or without one: sell it, ship it to clients, keep its
source closed, use it at work. No royalties, no revenue share, no permission to
ask.

The only difference is a watermark. The default build (the **license-free
edition**) draws a small note in the bottom right corner of every window:

```
Made with Raven · free edition
No watermark: license $20 once (individuals) · $200/month (companies) · oscar-soirey.github.io/Raven
```

It never takes the mouse (what is under it still gets the clicks), but it is
on every window, and a watermark on a tool you're proud of would be a shame.
A license gives you the **Release edition**, which is the same library without
the watermark: not one feature is held back in the free edition.

| | Price | For |
|---|---|---|
| License-free edition | free | everyone; every window shows the watermark |
| Personal license | **$20, once** | one person: freelancers, indie developers, students, hobbyists |
| Company license | **$200 / month** | a company and all its developers; cancel any time |

Buy a license on [the Raven site](https://oscar-soirey.github.io/Raven/license.html)
(payments by Stripe). For a company license, fill in the short form there
first (it is sent to oscarsoirey.contact@gmail.com), then subscribe. Questions:
oscarsoirey.contact@gmail.com.

### The two editions

The edition is chosen when Raven is built, with `RAVEN_EDITION` (not to be
confused with `CMAKE_BUILD_TYPE`: a Debug build can be either edition):

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release                          # license-free (watermark)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DRAVEN_EDITION=Release  # licensed: no watermark
```

```bat
generate-cmake.bat --edition release
build.bat --edition release
dist.bat --edition release
```

The presets have both too: `linux`, `msvc`, `mingw` (and `-shared`) are
license-free; `linux-release`, `msvc-release`, `mingw-release` (and
`-shared-release`) are the Release edition. A program can ask which one it runs
on with `Ris_license_free()`, and where the watermark is with
`Rwindow_get_watermark_rect()`.

## Status

Version 0.1. Raven was developed on one Linux machine without a GPU. What
has been verified there, and what has not been verified anywhere:

| | State |
|---|---|
| Layout, widgets, XML, text editor, dialogs, docking, memory, animations, themes and theme files, live reload, Markdown, color picker, timeline, node graph | 12 headless test programs (about 20 800 checks, the mouse and the keyboard injected as real events); no report from AddressSanitizer, UBSan or ThreadSanitizer in Raven code |
| Vulkan renderer, glass, blur, curves | pixel tests (`test_render`) and the examples, on a **software** Vulkan driver (SwiftShader). No real GPU, no validation layers: run the examples in a Debug build (it enables the validation layers) before trusting it |
| Windows that are seen through | **never seen composed on a screen**: the test machine has no compositor. On X11 the path was forced (`RAVEN_X11_COMPOSITED=1`) and the alpha of the rendered frame checked: empty where nothing is drawn, a hole under a `Native` blur. The blur of KWin and everything on Windows (DWM blur, acrylic, mica, opacity) was written from the documentation and never run |
| `raven_web` | on Linux with Chromium 141: `test_web` (pages, scripts, messages, clicks and typed text, history, two views; 30 runs in a row, clean under the sanitizers) and the `browser` example with the Vulkan renderer. **Its Windows part was never run**, and Edge and Chrome were not tried |
| Linux / X11 | run under Xvfb with real pointer and keyboard events: clipboard, resize, custom header (move, maximize, resize by the edges, without a window manager), files dropped by a test program speaking XDND, native child window, OpenGL (llvmpipe) |
| File dialogs on Linux | Raven starts `zenity` or `kdialog`; neither was installed, so that path was run with a script standing in for them. Without them Raven shows its own dialog (tested) |
| Windows / Win32 | every file passes clang's syntax and type checking against the mingw-w64 headers without a warning, but nothing was **linked or run** on Windows. This includes the custom header (`WM_NCHITTEST`), dropped files and the file dialogs. Expect to fix details on the first run; the code to look at is `src/platform/win32/win32_platform.c` and the Win32 parts of `src/widgets/dialogs.c` and `src/core/fs.c` |
| MSVC, MinGW gcc | not compiled. The code avoids GNU extensions and has an MSVC path for atomics, but `/W4` may report warnings |
| `build.bat`, `generate-cmake.bat`, `dist.bat` | written without a Windows machine: **never run** |
| `dist.sh`, CMake presets, install, `find_package` | run on Linux |
| The designer | run on Linux/X11; a scripted run is part of the tests |
| Bindings | C++, Python, Rust and Go are compiled and run by the tests. **C#, Zig and LuaJIT were generated but never compiled or run** |
| macOS / Cocoa | native Cocoa windows, drag-and-drop, clipboard and Metal surface for Vulkan/MoltenVK are implemented in this change, but **not built or run on macOS here** |
| Wayland | no native backend yet (XWayland works through the X11 platform) |

## Building

Requirements: CMake 3.16+, a C11 compiler. Linux also needs the X11
development package (`libx11-dev`). The Vulkan *headers* are needed to build;
they come from the Vulkan SDK when it is installed and are downloaded
otherwise. No Vulkan library is linked.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
cd build && ctest -C Release --output-on-failure
```

The programs are written to `build/bin` (`build\bin\Release` with Visual
Studio), next to `raven.dll` in a shared build.

With CMake 3.21 or newer, presets do the same: `cmake --preset linux` (or
`msvc`, `msvc-shared`, `mingw`, `mingw-shared`, `linux-shared`, `dev`, and
`linux-release`, `msvc-release`... for the Release edition), then
`cmake --build --preset linux` and `ctest --preset linux`.

On Windows, two scripts work from the folder of Raven:

```bat
generate-cmake.bat            :: creates build\raven.sln (the newest Visual Studio installed)
generate-cmake.bat --open     :: ... and opens it; F5 starts the editor example
build.bat                     :: Release build (generates the build tree if needed)
build.bat debug --test        :: Debug build, then the tests
build.bat --run glass         :: builds, then starts an example
build.bat --edition release   :: the Release edition (licensed, no watermark)
```

`generate-cmake.bat --help` and `build.bat --help` list their options
(`--shared`, `--generator Ninja`, `--build-dir`, `--fresh`, ...). CMake
options go after `--`: `generate-cmake.bat -- -DRAVEN_WARNINGS_AS_ERRORS=ON`.

| Option | Default | |
|---|---|---|
| `RAVEN_EDITION` | `LicenseFree` | `LicenseFree`: every window shows a watermark; `Release`: no watermark, with a license (see [License](#license)) |
| `RAVEN_BUILD_SHARED` | `OFF` | shared library instead of static |
| `RAVEN_BUILD_EXAMPLES` | `ON` when top level | |
| `RAVEN_BUILD_TESTS` | `ON` when top level | tests, tests of the bindings, `raven_benchmark` |
| `RAVEN_BUILD_TOOLS` | `ON` when top level | `raven_designer` |
| `RAVEN_BUILD_WEB` | `ON` when top level | `raven_web`, the optional library that shows web pages ([docs/WEB.md](docs/WEB.md)); it needs nothing more to build |
| `RAVEN_BACKEND_VULKAN` | `ON` | `OFF` builds without any GPU code (custom backend or headless) |
| `RAVEN_PLATFORM` | `auto` | `win32`, `x11`, `cocoa` (macOS) or `null` |
| `RAVEN_WITH_STB_IMAGE` | `ON` | PNG / JPEG / BMP / TGA decoding |
| `RAVEN_FETCH_VULKAN_HEADERS` | `ON` | download the Khronos headers when no SDK is found |
| `RAVEN_VULKAN_INCLUDE_DIR` | | folder containing `vulkan/vulkan.h`, to skip the search |
| `RAVEN_REBUILD_SHADERS` | `OFF` | recompile the GLSL (needs `glslangValidator` or `glslc`); the SPIR-V is committed |
| `RAVEN_MSVC_STATIC_RUNTIME` | `OFF` | Visual C++: `/MT` instead of `/MD` |
| `RAVEN_WARNINGS_AS_ERRORS` | `OFF` | |

Using it from another CMake project:

```cmake
add_subdirectory(raven)                       # or find_package(raven) after an install
target_link_libraries(app PRIVATE raven::raven)
```

### Packages to give away

`dist.bat` (Windows) and `dist.sh` (Linux) build what is given to people who
use Raven without building it: the static library and the shared one, the
headers, the designer, the bindings and the documentation, in one folder and
one archive per compiler.

```bat
dist.bat              :: Visual C++ and MinGW-w64, whichever are installed
dist.bat msvc         :: dist\raven-0.1.0-windows-x64-msvc-license-free.zip
dist.bat mingw        :: dist\raven-0.1.0-windows-x64-mingw-license-free.zip  (gcc and mingw32-make in the PATH)
dist.bat --edition release   :: ...-msvc-release.zip, ...-mingw-release.zip: no watermark (licensed)
```

```
raven-0.1.0-windows-x64-msvc-license-free\
    include\                 Raven.h, raven\*.h
    bin\raven.dll            and raven_designer.exe
    lib\raven.lib            to link with raven.dll        (MinGW: libraven.dll.a)
    lib\static\raven.lib     the static library            (MinGW: libraven.a)
    lib\cmake\raven          find_package(raven), DLL; lib\static\cmake\raven for the static library
    share\raven\             README.md, docs\, bindings\, licenses\
```

The libraries of one compiler are not for the other; `raven.dll` itself can be
loaded by anything. The MinGW DLL is linked with `-static-libgcc`, so that it
does not need the DLLs of the compiler beside it. `cmake --build build
--target package` makes a simpler archive (one library) with CPack.

## Examples

| | |
|---|---|
| `examples/hello` | a window with the C API |
| `examples/editor` | a scene editor described in `editor.xml`: menus, splitters, tree, tabs, property grid, a viewport rendered in software on the job system, a custom-drawn graph, a virtual list. Its View menu switches between the four themes |
| `examples/notepad` | a text editor: custom header bar, languages chosen from the name of the file, a language declared by the application, proposals from a callback, markers, the file dialogs of the system, files dropped on the window |
| `examples/animations` | transitions played on demand, four springs in a custom widget, cards that pop in and out, an island, speed and "reduce motion" |
| `examples/workspace` | panels that dock, a file browser that opens files in new panels, the three kinds of glass for everything or for one panel, saving and restoring the arrangement |
| `examples/components` | tags declared with `<Widget>`, `<Repeat>`, `<ForEach>`, `<Include>`, and a piece of interface built from C as a document |
| `examples/memory` | an allocator given by the application, the statistics of the library, the heap check |
| `examples/studio` | a graph of nodes (widgets inside the nodes, links the application can refuse, nodes added from code), a timeline that plays, a color picker that colors the selected nodes; its XML is loaded again when saved |
| `examples/themes` | two theme files, styles and classes, a card that keeps its own theme, Markdown in a label and in an editor; every file of the example is watched |
| `examples/overlay` | a window that is seen through, the backdrops of the system, and the four ways to blur what is behind a panel |
| `examples/browser` | web pages with `raven_web`: an address bar, the history, a color picker of Raven that drives the CSS of the page, messages from the page |
| `examples/glass` | the glass themes and the three glass materials. `raven_glass light` starts with the light glass |
| `examples/viewport_vulkan` | a Vulkan scene in the UI, twice: offscreen image wrapped as a texture, and drawn in the render pass of Raven. Needs the Vulkan library |
| `examples/viewport_opengl` | an OpenGL scene in a native child window (WGL / GLX) |

![The notepad example](docs/images/notepad.png)

*`examples/notepad`: the title bar is a widget; the lines that say TODO and FIXME are marked.*

![The animations example](docs/images/animations.png)

## The designer

`raven_designer [file.xml]` edits the XML of an interface. Its window shows
the same document five ways, kept in step:

- **Widgets**: what can be added, the tags of the library and those the
  document declares with `<Widget>`. A double click adds inside the selection.
- **Tree**: the elements. Move up, down, out, in; duplicate; delete; wrap in
  a panel or unwrap; copy and paste as XML.
- **Preview**: the interface, built for real. A click selects an element;
  with *Use the preview* on, it reacts as the interface will.
- **Properties**: every attribute of the selected element, by kind of value
  (a list for a choice, a swatch for a color), grouped: its own, those of
  every widget, its place in its parent, colors, metrics.
- **XML**: the text, colored; typing in it updates the rest, and a mistake is
  underlined and listed in **Problems** while the last good version stays shown.

Undo and redo cover everything; `F5` opens the interface in a window of its
own; the View menu switches the theme; files dropped on the window are opened.

The designer uses the public API only: the document is a `Rxml_document`, the
lists come from `Rxml_get_tag_name` and `Rxml_get_attribute_info`, the element
under the pointer from `Rwidget_pick` and `Rwidget_get_xml_node`. Its own
interface is XML with a declared tag. It is one file, `tools/designer/main.c`,
meant to be read.

## Other languages

`bindings/` holds bindings generated from the headers by `tools/bindgen`:
C++ (classes and lambdas), Python (`ctypes`), Rust (a crate), Go (cgo), C#
(P/Invoke), Zig and LuaJIT. C++, Python, Rust and Go are tested by the build;
**C#, Zig and LuaJIT were never compiled**. See
[bindings/README.md](bindings/README.md). They cover everything in
`Raven.h`; the optional `raven_web` is C only.

```python
import raven as r
r.Rinit(r.R_BACKEND_VULKAN)
window = r.Rwindow_create_from_xml('<Window Title="Hi"><Button Text="Quit" OnClicked="quit"/></Window>')
r.Raction_bind("quit", lambda sender, action, user: r.Rquit(0))
while r.Rwait_frame(-1.0):
    pass
r.Rshutdown()
```

## Concepts

### Names

`#include <Raven.h>` gives the whole API; the headers it includes
(`raven/raven_window.h`, `raven/raven_controls.h`...) can also be included
one by one.

| | | |
|---|---|---|
| functions | `R<object>_<action>` | `Rwindow_create`, `Rwindow_get_size`, `Rbutton_set_text`, `Rslot_set_padding` |
| library functions | `R<action>` | `Rinit`, `Rdraw_frame`, `Rwait_frame`, `Rquit`, `Rshutdown` |
| types | `R<Name>` | `Rid`, `REvent`, `RInitDesc`, `RPainter`; callbacks end with `Callback` |
| constants | `R_<NAME>` | `R_INVALID_ID`, `R_WIDGET_VBOX`, `R_KEY_ESCAPE`, `R_THEME_GLASS_DARK` |
| build options and macros | `RAVEN_<NAME>` | `RAVEN_BUILD_SHARED`, `RAVEN_API`, `RAVEN_VERSION_MAJOR` |

Names that do not start this way (`rvn_...`, `RVN_...`) are internal to the
library and are not exported.

### Handles and errors

Every object is a `Rid`. Handles carry a generation, so using a deleted
object is detected and reported instead of crashing. Deleting a widget
deletes its children; deleting a window deletes its content.

```c
static void on_error(RError code, RSeverity severity, const char* detail) {
    fprintf(stderr, "%s: %s\n", Rerror_to_string(code), detail);
}
Rerror_set_callback(on_error);
```

### The loop

Raven has no loop of its own. The application asks for one frame at a time:

```c
while (Rdraw_frame()) {            /* a game or an engine: never waits      */
    update_and_render_my_scene();
}

while (Rwait_frame(-1.0)) {}       /* a tool: sleeps until something happens */
```

`Rdraw_frame` processes the events, runs the timers and the posted tasks,
then lays out, paints and submits the windows that changed; a window in which
nothing changed is not drawn again. `Rwait_frame` first sleeps until there is
something to do (an event, a timer, an animation still moving). Both return
false once `Rquit` was called or the last window is closed.

### Layout

Layout is the two passes of Slate: each widget computes the size it would
like, bottom-up, then each panel gives a rectangle to its children, top-down.
How a child is placed is stored on the child, in its slot:

```c
Rid row = Rwidget_add_child(parent, Rwidget_create(R_WIDGET_HBOX));
Rid name = Rwidget_add_child(row, Rwidget_create(R_WIDGET_TEXT_INPUT));
Rslot_set_fill(name, 1.0f);                   /* takes the free width          */
Rid ok = Rwidget_add_child(row, Rbutton_create("OK"));
Rslot_set_valign(ok, R_VALIGN_CENTER);      /* its own height, centered      */
Rslot_set_padding(ok, 8, 0, 0, 0);
```

Sizes are logical units, multiplied by the scale of the monitor. Only the
part of the tree that changed is measured and arranged again.

### XML

Tags are widget types, attributes are properties, `Slot.*` attributes are
the slot. `On...` attributes name *actions*, bound from C with
`Raction_bind`. The full reference is [docs/XML.md](docs/XML.md).

A document can declare its own tags, with parameters:

```xml
<Widget Name="Field" Label="Name" Hint="" Wide="false">
    <HBox Spacing="8">
        <Label Text="{Label}:" Width="{Wide ? 140 : 90}"/>
        <TextInput Placeholder="{Hint}" Slot.Fill="1"/>
        <Children/>
    </HBox>
</Widget>

<Field Label="Email" Hint="you@example.com" Name="email"/>
<Field Label="Password" Wide="true"><Button Text="Show"/></Field>
```

The attributes of `<Widget>` other than `Name` are its parameters and their
default values; `{...}` is an expression of them (arithmetic, comparisons,
`a ? b : c`, a few functions); `<Children/>` is where the children given to
the tag go. `If="..."`, `<Repeat Count="3" Var="i">`, `<ForEach Items="a|b"
Var="x">` and `<Include Source="file.xml"/>` complete it. From C,
`Rxml_register_widgets` reads declarations ahead of the documents that use them.

For tools, `Rxml_document_parse` gives a document as a tree of nodes that can
be changed, written back and built, each widget knowing its node
(`raven_xml.h`); `Rxml_get_attribute_info` lists what can be written.

### Animations

Nothing has to be done for the controls: they move by themselves, with
springs. What an application decides:

```c
Rwidget_set_transition(card, R_TRANSITION_FLY_UP, 0.1f);   /* each time it appears; Appear="FlyUp" in XML */
Rwidget_play_transition(card, R_TRANSITION_DROP, 0.0f);    /* once, now                                  */

/* in the paint callback of a custom widget: a value pulled to its target */
float s = Rdraw_spring(painter, &state->size, hovered ? 1.0f : 0.0f, R_MOTION_BOUNCY);

Rset_animation_speed(0.25f);          /* slow motion        */
Rset_animations_enabled(R_FALSE);     /* "reduce motion"    */
```

Transitions: `Fade`, `Pop`, `Zoom`, `FlyUp/Down/Left/Right`, `Drop`,
`SlideLeft/Right`. Motions: `SNAPPY`, `SMOOTH`, `BOUNCY`, `WOBBLY`. While
something moves the window keeps drawing by itself; `Rwait_frame` sleeps
again when everything is at rest.

### Text editor

`R_WIDGET_TEXT_EDITOR` (`<TextEditor>`) edits text of any number of lines:
selection, clipboard, undo, line numbers, word wrap. With a language it
colors the text, proposes words while typing (those of the language and the
names already in the text; `Ctrl+Space` asks), shows which bracket closes
which, closes brackets and quotes, indents, and comments lines out (`Ctrl+/`).

```c
Rtext_editor_set_language(editor, Rlanguage_for_file(path));     /* "main.rs" -> "Rust" */
```

C, C++, C#, Java, JavaScript, TypeScript, Python, Rust, GLSL, JSON, CSS, XML
and HTML (with the JavaScript and the CSS inside it) are built in. Any of
them can be changed and others declared with `Rlanguage_register`: a language
is a description (its words, its comments, its quotes), not code.

This is what an editor knows from the text alone: it is **not a language
server**. What needs to understand a whole program comes from outside,
through two doors made for it: `Rtext_editor_set_completion_callback` (your
proposals, listed first) and `Rtext_editor_add_marker` (underline a part of
the text with a message: the errors of a compiler). An LSP client would plug
in there; Raven does not include one.

### Header bar

```xml
<Window Title="Tool" CustomHeader="true">
    <VBox>
        <HeaderBar>
            <MenuBar>...</MenuBar>
            <Button Text="Run" Slot.HAlign="Right"/>
        </HeaderBar>
        ...
```

`CustomHeader` (`R_WINDOW_CUSTOM_HEADER`) removes the title bar of the
system; the `HeaderBar` draws the title and the window buttons and holds any
widgets. Dragging it moves the window, a double click maximizes it, the edges
of the window resize it. On Windows this is done with the hit test of the
system, so snapping to the sides of the screen keeps working. Any widget can
play the same role (`Rwidget_set_window_drag`).

### Docking

A `DockArea` arranges its children as panels with tabs. The user drags a tab
onto another tab bar, onto a side of a panel or onto an edge of the area; the
panels fly to their new place. `Slot.Dock`, `Slot.DockTo` and `Slot.DockSize`
give the starting arrangement in XML; `Rdock_area_add_panel` adds panels from
C, and `Rdock_area_save_layout` / `Rdock_area_load_layout` turn the
arrangement into a short text and back. Panels stay inside their window:
tearing one off into a window of its own is not implemented.

### Dialogs and files

```c
RFileDialogDesc desc = { .parent = window, .title = "Open", .filters = filters, .filter_count = 2 };
const char* path = Rfile_dialog_open(&desc);          /* the dialog of the system; NULL when cancelled */

if (Rmessage_box(window, "Quit", "Discard the changes?", R_MESSAGE_OK_CANCEL) == R_MESSAGE_RESULT_OK) ...
```

File dialogs are those of the system (Windows: the common dialogs; Linux:
`zenity` or `kdialog` when installed), and otherwise a dialog of Raven built
on the `FileBrowser` widget, which can also be put in any window. Any widget
can be shown as a dialog of a window (`Rdialog_open`, `Rdialog_run`).
`Rfile_read` and `Rfile_write` take UTF-8 paths on every platform.

### Drag and drop

```c
static int on_drop(Rid target, const RDropEvent* event, void* user) {
    if (event->phase == R_DROP_DROPPED && event->path_count > 0) open(event->paths[0]);
    return R_TRUE;                    /* "I take it": the target is highlighted while it hovers */
}
Rwindow_set_drop_callback(window, on_drop, NULL);     /* or Rwidget_set_drop_callback for one widget */
```

Files and text dragged from other applications. Dragging *out* of a Raven
window to another application is not implemented.

### Materials and glass

A material is what the surface of a widget is made of:

| | |
|---|---|
| `Solid` | filled with the colors of the theme |
| `Glass` | clear glass: what is behind shows through, bent at the rim |
| `FrostedGlass` | blurred: the colors behind, without their shapes |
| `TintedGlass` | blurred and colored by the theme: the easiest to read on |

```c
Rtheme_apply(R_THEME_GLASS_DARK);                     /* a theme comes with its material          */
Rtheme_set_material(R_MATERIAL_FROSTED_GLASS);        /* every widget that was not given its own  */
Rwidget_set_material(panel, R_MATERIAL_GLASS);        /* this widget and what it contains         */
```

In XML: `Material="TintedGlass"` on any widget. Whatever the material, a
popup is more opaque than the panel it comes from, and a panel than the bar
above it, so that what is on top stays readable.

With glass, controls are capsules that show a blurred, more vivid version of
what is behind them; their rim bends it like a lens and catches a light
placed at the top left; the knob of a slider turns into a clear lens while it
is dragged. Glass needs something behind it: the glass themes give every
window a soft colored wallpaper, `Rwindow_set_background` replaces it with an
image. A custom widget draws glass with `Rdraw_glass`.

How it is rendered: a frame that contains glass is drawn into an offscreen
image; where glass begins, what has been drawn so far is copied and blurred,
and the glass primitives sample both the sharp and the blurred copy. Glass
over glass works. A frame without glass is drawn straight to the window: the
solid themes cost nothing more. [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)
has the details.

### Blur, and windows that are seen through

`BackgroundBlur` is a panel that shows what is behind it out of focus,
without the bending and the rim of glass. Four styles:

| | |
|---|---|
| `Plain` | only blurred |
| `Frosted` | blurred, a little veiled, with a fine grain (the default) |
| `Glass` | blurred and bent at its rim, like the glass material |
| `Native` | opens the window: the *system* blurs the desktop behind it |

```xml
<Window Transparent="true" Backdrop="Blur" Borderless="true">
    <BackgroundBlur Style="Native" CornerRadius="24" Padding="22" Tint="#10141C99">
        ...
    </BackgroundBlur>
</Window>
```

A window created with `Transparent="true"` (`R_WINDOW_TRANSPARENT`) is not
filled: where no widget draws, what is behind it on the screen shows.
`Backdrop` (`Rwindow_set_backdrop`) asks the system for what it puts there:
nothing, a blur, or the acrylic and mica materials of Windows 11;
`Rwindow_set_opacity` fades the whole window. All of it depends on the
window system (Windows 10 and 11; X11 with a compositor, and KWin for the
blur): `Rwindow_get_backdrop` tells what was obtained, and where nothing is
available the same interface shows on a plain background.

![The overlay example](docs/images/overlay.png)

*`examples/overlay` as the window system receives it, shown here over a
checkerboard (the machine it was captured on has no compositor): the panel is
translucent, its corners are empty, and the `Native` panel is a hole through
the stripes.*

### Color picker, timeline, node graph

`raven_tools.h`. Each holds a small model that the application fills and
reads, or writes in XML ([docs/XML.md](docs/XML.md#tools)).

```c
Rid picker = Rcolor_picker_create();                 /* square, hues, opacity, code, numbers, swatches */
Rcolor_picker_set_changed_callback(picker, on_color, NULL);
Rcolor_picker_set_popup(picker, R_TRUE);             /* a swatch that opens it */

Rid timeline = Rtimeline_create();
int track = Rtimeline_add_track(timeline, "Camera");
int clip  = Rtimeline_add_clip(timeline, track, 0.0, 3.5, "Wide shot");
Rtimeline_add_key(timeline, track, 6.0);
Rtimeline_set_time_callback(timeline, on_time, NULL);       /* scrubbed, or playing */
Rtimeline_set_changed_callback(timeline, on_item, NULL);    /* moved, stretched     */

Rid graph = Rnode_graph_create();
int noise = Rnode_graph_add_node(graph, "Noise", 40, 60);
int out   = Rnode_graph_add_output(graph, noise, "Value", "number");
int blend = Rnode_graph_add_node(graph, "Blend", 320, 90);
int in    = Rnode_graph_add_input(graph, blend, "A", "number");
Rnode_graph_connect(graph, out, in);
Rnode_graph_set_node_content(graph, noise, Rwidget_create(R_WIDGET_SLIDER));   /* any widget inside a node */
Rnode_graph_set_connect_callback(graph, may_link, NULL);                       /* refuse loops, apply rules */
```

The **timeline** snaps to frames, to the playhead and to the other items,
moves several items at once, stretches clips by their ends, selects with a
rectangle, zooms with Ctrl + wheel and plays. The **graph** pans (middle
button, or Alt + drag) and zooms around the pointer; pins have types, a
color per type, and only link to pins they fit; a wire is pulled from a pin,
is attracted by the pins it can go to, and can be dropped on a node; taken
by its end, a link is plugged elsewhere or let go. Wires hang like cables and
swing when their nodes move, nodes lift while they are carried, and pulses
run along the links while the pointer is over the graph
(`Rnode_graph_set_flow_visible`). Widgets inside nodes are ordinary widgets
and work at any zoom.

### Markdown

```xml
<Label Markdown="true" Wrap="true">
    ## Notes
    Some **strong** and *emphasized* text, `code`, a [link](https://example.org),
    - a list
    - [x] with a box
</Label>
<TextEditor Markdown="true"/>
```

On a label the text is rendered (headings, lists, quotes, code blocks, tables,
links that open or fire actions); on the text editor it stays as typed and is
styled where it stands.

### Files that are loaded again

An interface created from a file is watched while the application runs:
save `editor.xml` and the window follows, keeping the widgets the file did
not change (their handles, their callbacks, what was typed or scrolled).
Theme files and the files of declared tags are watched too.
`Rxml_set_reload_callback` tells when it happened;
`Rxml_set_live_reload(R_FALSE)` or `RAVEN_LIVE_RELOAD=0` turns it off.

### Web pages

```c
#include <raven_web.h>                      /* a separate library: target raven::web */
Rweb_register_xml();                        /* <WebView Url="https://..."/> */
Rid view = Rweb_view_create();
Rweb_view_load_url(view, "https://example.org");
Rweb_view_run_javascript(view, "document.title", on_result, NULL);
```

`raven_web` shows real web pages by driving a browser of the Chromium family
that is already installed (Edge on Windows): the page arrives as pictures in
a texture, the input goes back. It contains no browser and Raven does not
depend on it. What it can and cannot do is in [docs/WEB.md](docs/WEB.md).

![The browser example](docs/images/browser.png)

### Island

```xml
<Window ...>
    <Island>
        <Label Text="3 messages"/>                      <!-- folded   -->
        <VBox> ... anything ... </VBox>                 <!-- unfolded -->
    </Island>
    <VBox> ... the content of the window ... </VBox>
</Window>
```

A pill at the top of the window that unfolds under the pointer, on a click,
or from C (`Risland_set_expanded`, `Risland_expand_for(island, 2.0)` to say
something and fold back). Its two contents cross-fade while it changes size,
a click pins it open and a click elsewhere closes it. It is dark whatever
the theme, as the notch of a phone is (`Theme="Default"` gives it the theme
of the window). It is optional, and an `Island` can also be used as an
ordinary widget anywhere.

### Memory

Raven does not call `malloc`. It takes memory from the system by regions of
4 MB and hands it out with its own heap (TLSF: constant time, neighbours
merged when freed); the bundled font and image decoders use it too.

```c
RAllocator mine = { my_alloc, my_realloc, my_free, my_data };
Rset_allocator(&mine);                 /* before Rinit: every block goes through your functions */

RMemoryStats stats;
Rmemory_get_stats(&stats);             /* blocks, bytes in use, peak, taken from the system     */
```

`Rset_allocator(Rallocator_get_system())`, or the environment variable
`RAVEN_ALLOCATOR=system`, selects `malloc` (what Valgrind and heap profilers
follow; it is the default in a build with AddressSanitizer). `Rmemory_check`
verifies the heap, and the tests fail on a block left allocated after
`Rshutdown`.

### Threads

- The thread that calls `Rinit` is the **UI thread**. Every function is
  called from it, except the few documented as thread-safe (`Rtask_post`,
  `Rtask_run_async`, `Rwake_up`, `Rquit`, `Rwindow_request_redraw`,
  `Rget_time`, `Rmemory_alloc` / `Rmemory_realloc` / `Rmemory_free`).
- From another thread, change the UI by posting a task:
  `Rtask_post(function, data)` runs `function` on the UI thread.
- `Rtask_run_async` and `Rparallel_for` run your work on the worker threads.
- `Rtexture_create_from_file_async` decodes in the background; the widgets
  that use the texture update when it is ready.
- Viewport draw callbacks run on the **render thread**.

### Viewports

```c
/* 1. texture: any image, shown like a widget (clipped, UI on top) */
Rviewport_set_texture(viewport, texture);
/*    Vulkan, zero copy:   Rvulkan_texture_create_from_image_view(view, 0, w, h)
      OpenGL, zero copy:   Rvulkan_texture_create_shared + GL_EXT_memory_object
      anything, one copy:  Rtexture_update(texture, rgba, w, h)             */

/* 2. draw callback: your Vulkan draw calls in the render pass of the window */
Rviewport_set_draw_callback(viewport, on_draw, user);

/* 3. native child window: a HWND / X11 Window for OpenGL, Direct3D, anything */
Rviewport_set_native(viewport, R_TRUE);
Rviewport_get_native_handle(viewport, &native);
```

To render the UI with the device of an engine instead of a device created
by Raven, call `Rvulkan_set_external_device` (and
`Rvulkan_set_queue_lock_callbacks`) before `Rinit`: see `raven_vulkan.h`.

### Themes

`Rtheme_apply` chooses one of four themes: `R_THEME_DARK`, `R_THEME_LIGHT`,
`R_THEME_GLASS_DARK`, `R_THEME_GLASS_LIGHT`. From there:

```c
Rtheme_set_accent(0.13f, 0.83f, 0.93f);                                 /* one color, and the shades that follow */
Rtheme_set_color(R_COLOR_PANEL, 0.07f, 0.10f, 0.17f, 1.0f);             /* any color or metric of the theme      */
Rtheme_set_style_metric("Button", NULL, R_METRIC_CORNER_RADIUS, 16);    /* a style for a type of widget          */
Rtheme_set_style_color(NULL, "danger", R_COLOR_ACCENT, 1, 0.3f, 0.4f, 1); /* ... or for a class: Class="danger" */
Rtheme_load("themes/ocean.xml");  Rtheme_use("Ocean");                  /* a theme file                          */
```

```xml
<Theme Name="Ocean" Base="Dark" Accent="#22D3EE" CornerRadius="10">
    <Colors Panel="#111A2C" Text="#DCE7F5"/>
    <Style Widget="Button" CornerRadius="16"/>
    <Style Class="danger" Accent="#FB7185"/>
</Theme>
```

`Theme="Ocean"` (or `Theme="themes/ocean.xml"`) on a widget or on a window
gives a theme to that part of the interface only. Colors and metrics can
still be set on one widget (`Rwidget_set_color`, `Color.Accent="..."`).
[docs/THEMES.md](docs/THEMES.md) is the guide.

Fonts are TrueType files (`Rfont_create_from_file`, `Rtheme_set_font`,
`Rfont_set_fallback`, `Rfont_set_variant` for real bold and italic faces);
subsets of Roboto and of Roboto Mono covering Latin-1 and Latin Extended-A
are embedded.

### Custom widgets and custom backends

A `R_WIDGET_CUSTOM` widget is measured and painted by two callbacks using
the `Rdraw_...` functions (`raven_draw.h`), and receives its events through
`Rwidget_set_event_callback`. `Rxml_register_tag` gives it a tag of its own
in XML, with its attributes, and `Rwidget_set_destroy_callback` tells when
it is gone. A rendering backend is a `RBackendVTable`
(`raven_backend.h`) given to `Rbackend_register`;
`src/render/null/null_backend.c` is a template.

## Performance

`raven_benchmark` (built with the tests) measures a large tree without a GPU.
On the 2-core virtual machine used for development, release build:

| 12 000 widgets, 27 800 primitives | |
|---|---|
| first layout | 2 to 3 ms |
| full repaint | 1.3 to 2.3 ms, one draw call |
| layout after changing one label | 0.03 to 0.05 ms |

Parallel painting made little difference on that machine (two cores shared
with the render thread); measure on yours with `raven_benchmark 160 0` against
`raven_benchmark 160 -1`. Nothing is recomputed or drawn while nothing changes.

The glass materials were only run on a software Vulkan driver (SwiftShader,
two cores), where a frame of the `glass` example takes about 250 ms at
1100 x 720. That measures the CPU rasterizer, not the design: on a GPU the
cost is the blur passes, but it has not been measured. Read
`RFrameStats::render_ms` and `backdrop_count` on your machine.

## Layout of the repository

```
include/Raven.h      the whole public API in one include
include/raven/       ... one header per area
src/core/            allocator, handles, threads, job system, files, main loop
src/platform/        window systems behind one interface: win32/, x11/, null/
src/render/          draw lists, fonts and glyph atlas, textures, render thread
src/render/vulkan/   the Vulkan backend and its shaders (vk_glass.c: backdrop blur)
src/widget/          widget base: tree, layout, events, painting, animation, windows, style, themes
src/widgets/         one file per widget type (text_editor.c, markdown.c, node_graph.c, timeline.c, dialogs.c...)
src/xml/             parser, loader, declared widgets, documents, watched files, the list of tags and attributes
web/                 raven_web: the optional library that shows web pages
examples/            fourteen programs
tools/designer/      raven_designer
tools/bindgen/       writes bindings/ from the headers
bindings/            C++, Python, Rust, Go, C#, Zig, LuaJIT
tests/  docs/  cmake/  third_party/
build.bat  generate-cmake.bat  dist.bat  dist.sh  CMakePresets.json
```

[docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) explains how the pieces work
and how to add a widget, a window system or a backend.

## Not implemented yet

native Wayland; text kerning, right-to-left and complex scripts;
IME composition window placement; in the text editor: several cursors,
search and replace, code folding, a language server client; tearing a docked
panel off into its own window; dragging out of a window to another
application; mipmaps for scaled-down images; a GPU memory sub-allocator (each
texture has its own allocation); a depth buffer in the draw-callback viewport
(render to a texture when you need one); presenting several windows in one
call; recovery from a lost Vulkan device; glass over a native child window
(it is another window of the OS: use a texture or a draw-callback viewport
under glass). In the node graph: knots on links, groups and comments, a
minimap, undo (the application holds the model: it can keep the history). In
the timeline: tracks inside tracks, curves between keys. In Markdown: images
from the network, HTML inside the text, footnotes. `raven_web` shows pages
through an installed browser and has the limits listed in
[docs/WEB.md](docs/WEB.md); embedding the web view of the system itself
(WebView2, WebKitGTK) is not done. Watching files is a comparison of dates
five times a second, not a notification of the system.

## Third-party code

`stb_truetype` and `stb_image` (public domain / MIT, Sean Barrett), a subset
of the Roboto font (Apache 2.0, Google) and a subset of Roboto Mono (SIL Open
Font License 1.1) are included under `third_party/`; see
[THIRD_PARTY.md](THIRD_PARTY.md). The Vulkan headers (Apache 2.0 / MIT,
Khronos) are needed to build but are not part of this repository.
