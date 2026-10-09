/**
 * HUI - Horizon UI
 * hui_viewport.h : embedding content rendered by the application.
 *
 * A VIEWPORT widget reserves an area of the layout and shows external content
 * in it. Three modes cover every graphics API:
 *
 *   1. Texture  (HUI_SetViewportTexture)
 *      The application renders to an image and HUI draws it like any widget:
 *      it is clipped, can have UI on top, and follows the layout exactly.
 *        - Vulkan : wrap your VkImageView with
 *                   HUI_Vulkan_CreateTextureFromImageView (hui_vulkan.h), zero copy.
 *        - OpenGL : share memory with HUI_Vulkan_CreateSharedTexture and
 *                   GL_EXT_memory_object (zero copy), or read pixels back and
 *                   call HUI_UpdateTexture.
 *        - Any    : HUI_UpdateTexture with CPU pixels.
 *
 *   2. Draw callback  (HUI_SetViewportDrawCallback)
 *      HUI calls the application while it records its own frame, with the
 *      viewport and scissor set to the widget. The application draws straight
 *      into the window: no intermediate image, UI can still be drawn on top.
 *      With the Vulkan backend the callback receives the VkCommandBuffer.
 *      The callback runs on the render thread.
 *
 *   3. Native child window  (HUI_SetViewportNative)
 *      HUI creates a child OS window (HWND / X11 Window) kept on the widget.
 *      The application attaches its own swapchain or context to it: OpenGL,
 *      Vulkan, Direct3D, anything. HUI cannot draw over or clip that area.
 *
 * Input: a viewport is focusable and forwards its mouse and keyboard events
 * through HUI_SetWidgetEventCallback.
 */

#ifndef HUI_VIEWPORT_H
#define HUI_VIEWPORT_H

#include "hui_widget.h"
#include "hui_controls.h"

/** Passed to the draw callback of a viewport. Sizes are in pixels. */
typedef struct HUI_ViewportDrawInfo {
	HUI_EBackend backend;
	HUI_id viewport;
	HUI_id window;
	/** Area of the viewport in the window framebuffer. */
	int x, y, width, height;
	/** Visible part of that area. Already set as the scissor rectangle. */
	int clip_x, clip_y, clip_width, clip_height;
	int framebuffer_width, framebuffer_height;
	uint64_t frame_index;
	/** Vulkan: VkCommandBuffer being recorded, inside the render pass. */
	void* command_buffer;
	/** Vulkan: VkRenderPass in use (one color attachment, no depth). */
	void* render_pass;
	/** Vulkan: VkFormat of the color attachment. */
	HUI_uint color_format;
} HUI_ViewportDrawInfo;


#ifdef __cplusplus
extern "C" {
#endif

	/* ---- mode 1: texture ---- */

	/** @brief Shows a texture. HUI_INVALID_ID shows nothing. */
	HUI_API void HUI_SetViewportTexture(HUI_id viewport, HUI_id texture);
	/** @brief How the texture is fitted in the widget. Default: HUI_STRETCH_FILL. */
	HUI_API void HUI_SetViewportStretch(HUI_id viewport, HUI_EStretch stretch);
	/** @brief HUI_TRUE flips the texture vertically (OpenGL framebuffers are bottom-up). */
	HUI_API void HUI_SetViewportFlipY(HUI_id viewport, int flip);

	/* ---- mode 2: draw callback ---- */

	/**
	 * @brief Called on the render thread each time the window is rendered.
	 * State you bind (pipeline, descriptor sets, viewport, scissor) does not
	 * need to be restored. NULL removes the callback.
	 */
	typedef void (*HUI_CViewportDraw)(const HUI_ViewportDrawInfo* info, void* user_data);
	HUI_API void HUI_SetViewportDrawCallback(HUI_id viewport, HUI_CViewportDraw callback, void* user_data);

	/* ---- mode 3: native child window ---- */

	/**
	 * @brief Creates (or destroys) the native child window of the viewport.
	 * The viewport must already be in a window.
	 * @return HUI_TRUE on success.
	 */
	HUI_API int HUI_SetViewportNative(HUI_id viewport, int enabled);
	/** @brief Fills `native` with the handles of the child window. */
	HUI_API int HUI_GetViewportNativeHandle(HUI_id viewport, HUI_NativeWindow* native);

	/* ---- common ---- */

	/** @brief Size of the viewport in pixels after the last layout. */
	HUI_API void HUI_GetViewportPixelSize(HUI_id viewport, int* width, int* height);

	/** @brief Called on the UI thread when the pixel size changes: resize your render target here. */
	typedef void (*HUI_CViewportResize)(HUI_id viewport, int width, int height, void* user_data);
	HUI_API void HUI_SetViewportResizeCallback(HUI_id viewport, HUI_CViewportResize callback, void* user_data);

	/**
	 * @brief HUI_TRUE redraws the window every frame while the viewport is
	 * visible (game view). HUI_FALSE redraws only on HUI_InvalidateWidget (default).
	 */
	HUI_API void HUI_SetViewportRealtime(HUI_id viewport, int realtime);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* HUI_VIEWPORT_H */
