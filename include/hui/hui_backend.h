/**
 * HUI - Horizon UI
 * hui_backend.h : rendering backend interface.
 *
 * Include this header only to write a backend (OpenGL, Direct3D, Metal, or on
 * top of another renderer such as HRL). Applications do not need it.
 *
 * HUI turns the widget tree into a flat list of *instances*. Every primitive
 * of the UI is one instance: a quad expanded by the vertex shader and shaded
 * analytically (signed distance) by the fragment shader. A backend only has to
 *   1. manage one presentation surface per window,
 *   2. manage textures,
 *   3. upload the instance array and issue one instanced draw per command.
 *
 * The reference shaders are src/render/vulkan/shaders/ui.vert and ui.frag.
 *
 * Threading: every function of the table is called from a single thread, the
 * render thread (or the UI thread when the render thread is disabled).
 */

#ifndef HUI_BACKEND_H
#define HUI_BACKEND_H

#include "hui_core.h"
#include "hui_window.h"
#include "hui_resources.h"
#include "hui_viewport.h"


/** Index part of a handle: unique among live objects of one type, < 2^20. */
#define HUI_ID_INDEX(id) ((HUI_uint)(id) & 0xFFFFFu)

/** Primitive kind, stored in the low bits of HUI_DrawInstance::flags. */
#define HUI_PRIM_RECT      0u /* rounded rectangle, optional outline, optional texture */
#define HUI_PRIM_GLYPH     1u /* textured quad without edge anti-aliasing (text) */
#define HUI_PRIM_SHADOW    2u /* blurred rounded rectangle */
#define HUI_PRIM_LINE      3u /* segment with round caps */
#define HUI_PRIM_TRIANGLE  4u /* filled triangle */
#define HUI_PRIM_MASK      0xFu
/** The instance samples the texture of its draw command. */
#define HUI_INSTANCE_TEXTURED (1u << 4)

/**
 * One primitive. All positions are in framebuffer pixels, origin top-left.
 * Colors are 4 bytes R, G, B, A in memory order, straight (not premultiplied).
 *
 *   kind      rect                 uv              radii              border_width
 *   RECT      x0, y0, x1, y1       u0, v0, u1, v1  tl, tr, br, bl     outline width
 *   GLYPH     x0, y0, x1, y1       u0, v0, u1, v1  -                  -
 *   SHADOW    x0, y0, x1, y1       -               tl, tr, br, bl     blur radius
 *   LINE      ax, ay, bx, by       -               [0] = thickness    -
 *   TRIANGLE  ax, ay, bx, by       -               [0], [1] = cx, cy  -
 *
 * Fragments outside `clip` (x0, y0, x1, y1) are discarded.
 */
typedef struct HUI_DrawInstance {
	float rect[4];
	float uv[4];
	float clip[4];
	float radii[4];
	uint32_t color;
	uint32_t border_color;
	float border_width;
	uint32_t flags;
} HUI_DrawInstance;

typedef enum HUI_EDrawCommandType{
	/** Draw `instance_count` instances starting at `first_instance`. */
	HUI_DRAW_INSTANCES = 0,
	/** Call the viewport callback (see hui_viewport.h), then restore the UI state. */
	HUI_DRAW_CALLBACK
}HUI_EDrawCommandType;

typedef struct HUI_DrawCommand {
	HUI_EDrawCommandType type;
	/** Texture sampled by the textured instances of the command, or HUI_INVALID_ID. */
	HUI_id texture;
	HUI_uint first_instance;
	HUI_uint instance_count;
	/* HUI_DRAW_CALLBACK */
	HUI_CViewportDraw callback;
	void* user_data;
	HUI_id viewport;
	/** x, y, width, height in pixels. */
	int rect[4];
	int clip[4];
} HUI_DrawCommand;

/** Everything needed to render one frame of one window. */
typedef struct HUI_DrawData {
	HUI_id window;
	const HUI_DrawInstance* instances;
	HUI_uint instance_count;
	const HUI_DrawCommand* commands;
	HUI_uint command_count;
	HUI_uint framebuffer_width;
	HUI_uint framebuffer_height;
	float clear_color[4];
	uint64_t frame_index;
} HUI_DrawData;

typedef enum HUI_ETextureFormat{
	/** 4 bytes per pixel: R, G, B, A. */
	HUI_TEXTURE_RGBA8 = 0x02D0,
	/** 1 byte per pixel, sampled as (1, 1, 1, value). Used by the glyph atlas. */
	HUI_TEXTURE_ALPHA8
}HUI_ETextureFormat;

typedef struct HUI_TextureDesc {
	HUI_uint width;
	HUI_uint height;
	HUI_ETextureFormat format;
	HUI_EFilterType filter;
} HUI_TextureDesc;

typedef struct HUI_BackendInitInfo {
	const char* app_name;
	int enable_validation;
	int vsync;
	HUI_EWindowSystem window_system;
	/** Display connection of the window system (X11 Display*), or NULL. */
	void* native_display;
} HUI_BackendInitInfo;

/** Filled by RenderFrame when a capture is requested. */
typedef struct HUI_Capture {
	/** RGBA8, allocated with HUI_AllocPixels. NULL when capture is unsupported. */
	uint8_t* pixels;
	int width;
	int height;
} HUI_Capture;

typedef struct HUI_BackendVTable {
	const char* name;

	int  (*Init)(const HUI_BackendInitInfo* info);
	void (*Shutdown)(void);

	/** Creates the presentation surface (swapchain) of a window. */
	int  (*CreateSurface)(HUI_id window, const HUI_NativeWindow* native, HUI_uint width, HUI_uint height);
	void (*DestroySurface)(HUI_id window);

	/** `pixels` may be NULL (content undefined until UpdateTexture). */
	int  (*CreateTexture)(HUI_id texture, const HUI_TextureDesc* desc, const void* pixels);
	void (*UpdateTexture)(HUI_id texture, HUI_uint x, HUI_uint y, HUI_uint width, HUI_uint height, const void* pixels);
	void (*SetTextureFilter)(HUI_id texture, HUI_EFilterType filter);
	void (*DestroyTexture)(HUI_id texture);

	/**
	 * Renders and presents one frame. The surface must follow
	 * data->framebuffer_width / height (recreate the swapchain when they change).
	 * @param capture NULL, or a structure to fill with the rendered pixels.
	 */
	void (*RenderFrame)(const HUI_DrawData* data, HUI_Capture* capture);

	/** Blocks until the GPU finished all submitted work. */
	void (*WaitIdle)(void);
} HUI_BackendVTable;


#ifdef __cplusplus
extern "C" {
#endif

	/**
	 * @brief Supplies the backend used by HUI_Init(HUI_BACKEND_CUSTOM).
	 * The table is copied. Must be called before HUI_Init.
	 */
	HUI_API void HUI_RegisterBackend(const HUI_BackendVTable* vtable);

	/** @brief Allocates a pixel buffer released by HUI_FreePixels. Thread-safe. */
	HUI_API uint8_t* HUI_AllocPixels(size_t size);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* HUI_BACKEND_H */
