/**
 * HUI - Horizon UI
 *
 * Lightweight, multithreaded, retained-mode GUI library in C with a Slate-like
 * layout model, a Vulkan renderer, native windowing and XML descriptions.
 *
 *    ,--.  ,--.,--. ,--.,--.
 *    |  '--'  ||  | |  ||  |
 *    |  .--.  ||  | |  ||  |
 *    |  |  |  |'  '-'  '|  |
 *    `--'  `--' `-----' `--'
 *
 * Include this single header from application code:
 *
 *     #include <hui/hui.h>
 *
 *     int main(void) {
 *         HUI_Init(HUI_BACKEND_VULKAN);
 *         HUI_id window = HUI_CreateWindow("Hello", 640, 480, HUI_WINDOW_DEFAULT);
 *         HUI_SetWindowContent(window, HUI_CreateLabel("Hello, world"));
 *         int code = HUI_Run();
 *         HUI_Shutdown();
 *         return code;
 *     }
 *
 * Optional headers, not included here:
 *     hui_vulkan.h   sharing a Vulkan device / images with the application
 *     hui_backend.h  writing a rendering backend
 */

#ifndef HUI_H
#define HUI_H

#include "hui_core.h"
#include "hui_window.h"
#include "hui_style.h"
#include "hui_resources.h"
#include "hui_widget.h"
#include "hui_layout.h"
#include "hui_controls.h"
#include "hui_viewport.h"
#include "hui_draw.h"
#include "hui_xml.h"

#endif /* HUI_H */
