/**
 * Raven
 *
 * Lightweight, multithreaded, retained-mode GUI library in C with a Slate-like
 * layout model, a Vulkan renderer, native windowing and XML descriptions.
 *
 * This single header includes the whole API:
 *
 *     #include <Raven.h>
 *
 *     int main(void) {
 *         Rinit(R_BACKEND_VULKAN);
 *         Rid window = Rwindow_create("Hello", 640, 480, R_WINDOW_DEFAULT);
 *         Rwindow_set_content(window, Rlabel_create("Hello, world"));
 *         int code = Rrun();
 *         Rshutdown();
 *         return code;
 *     }
 *
 * The headers can also be included one by one:
 *
 *     raven/raven_core.h       lifecycle, main loop, threads, errors
 *     raven/raven_window.h     windows, input events
 *     raven/raven_style.h      themes, colors, metrics
 *     raven/raven_resources.h  fonts, textures
 *     raven/raven_widget.h     widget tree, slots, common properties
 *     raven/raven_layout.h     panels
 *     raven/raven_controls.h   labels, buttons, inputs, lists, menus...
 *     raven/raven_viewport.h   content rendered by the application
 *     raven/raven_draw.h       custom widgets
 *     raven/raven_xml.h        windows described in XML
 *     raven/raven_vulkan.h     sharing a Vulkan device or images (no Vulkan header needed)
 *     raven/raven_backend.h    writing a rendering backend
 */

#ifndef RAVEN_H
#define RAVEN_H

#include "raven/raven_core.h"
#include "raven/raven_window.h"
#include "raven/raven_style.h"
#include "raven/raven_resources.h"
#include "raven/raven_widget.h"
#include "raven/raven_layout.h"
#include "raven/raven_controls.h"
#include "raven/raven_viewport.h"
#include "raven/raven_draw.h"
#include "raven/raven_xml.h"
#include "raven/raven_vulkan.h"
#include "raven/raven_backend.h"

#endif /* RAVEN_H */
