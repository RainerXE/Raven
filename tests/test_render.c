/**
 * HUI tests - rendering with the Vulkan backend.
 *
 * Frames are rendered for real and read back with HUI_CaptureWindow, then a
 * few well-chosen pixels are compared with the expected colors. The test
 * first tries to run without a window system (VK_EXT_headless_surface), then
 * with a real window. Without a usable Vulkan driver it is skipped (exit
 * code 77), so that the test suite still passes on machines without a GPU.
 */

#include "test.h"

#include <hui/hui_vulkan.h>

#include <stdlib.h>

#if !defined(_WIN32)
#include <unistd.h>
#endif

#define SKIP_CODE 77

static uint8_t* g_pixels;
static int g_width, g_height;
static float g_scale = 1.0f;

/* Renders the window and keeps its pixels for the CHECK_PIXEL that follow. */
static void capture(HUI_id window) {
	HUI_FreePixels(g_pixels);
	HUI_Update();
	g_pixels = HUI_CaptureWindow(window, &g_width, &g_height);
	g_scale = HUI_GetWindowScale(window);
	CHECK(g_pixels != NULL);
}

static const uint8_t* pixel_at(float x, float y) {
	static const uint8_t none[4] = { 0, 0, 0, 0 };
	int px = (int)(x * g_scale), py = (int)(y * g_scale);
	if (!g_pixels || px < 0 || py < 0 || px >= g_width || py >= g_height) return none;
	return g_pixels + ((size_t)py * (size_t)g_width + (size_t)px) * 4;
}

/* Position in logical units, expected color, with a tolerance for rounding in the blending. */
#define CHECK_PIXEL(x, y, er, eg, eb) do { \
	const uint8_t* test_p_ = pixel_at((float)(x), (float)(y)); \
	g_test_checks++; \
	if (abs((int)test_p_[0] - (er)) > 3 || abs((int)test_p_[1] - (eg)) > 3 || abs((int)test_p_[2] - (eb)) > 3) { \
		g_test_failures++; \
		printf("  FAIL %s:%d: pixel (%g, %g) = (%d, %d, %d), expected (%d, %d, %d)\n", __FILE__, __LINE__, \
			(double)(x), (double)(y), test_p_[0], test_p_[1], test_p_[2], (er), (eg), (eb)); \
	} \
} while (0)

static HUI_id window_from(const char* xml) {
	HUI_id window = HUI_CreateWindowFromXml(xml, strlen(xml));
	CHECK(HUI_IsValidWindow(window));
	return window;
}

/* ---------------------------------------------------------------------------
 *  Tests
 * ------------------------------------------------------------------------- */

static void test_shapes(void) {
	HUI_id window = window_from(
		"<Window Width=\"400\" Height=\"300\" ClearColor=\"#336699\">"
		"  <Canvas>"
		/* plain rectangle */
		"    <Border Slot.Position=\"10,10\" Slot.Size=\"100,60\" Background=\"#FF0000\" CornerRadius=\"0\" BorderWidth=\"0\"/>"
		/* disc: a square with a radius of half its side */
		"    <Border Slot.Position=\"150,10\" Slot.Size=\"100,100\" Background=\"#00FF00\" CornerRadius=\"50\" BorderWidth=\"0\"/>"
		/* outline */
		"    <Border Slot.Position=\"10,150\" Slot.Size=\"100,100\" Background=\"#000000\" CornerRadius=\"0\" BorderWidth=\"10\" BorderColor=\"#FFFFFF\"/>"
		/* half-transparent white over black */
		"    <Border Slot.Position=\"150,150\" Slot.Size=\"100,100\" Background=\"#000000\" CornerRadius=\"0\" BorderWidth=\"0\">"
		"      <Border Background=\"#FFFFFF\" Opacity=\"0.5\" CornerRadius=\"0\" BorderWidth=\"0\"/>"
		"    </Border>"
		"  </Canvas>"
		"</Window>");
	capture(window);
	CHECK(g_width == (int)(400 * g_scale) && g_height == (int)(300 * g_scale));

	/* clear color */
	CHECK_PIXEL(390, 290, 0x33, 0x66, 0x99);
	CHECK_PIXEL(5, 5, 0x33, 0x66, 0x99);
	/* rectangle: inside, just inside each edge, just outside */
	CHECK_PIXEL(60, 40, 255, 0, 0);
	CHECK_PIXEL(10.5, 10.5, 255, 0, 0);
	CHECK_PIXEL(109.5, 69.5, 255, 0, 0);
	CHECK_PIXEL(9.5, 40, 0x33, 0x66, 0x99);
	CHECK_PIXEL(110.5, 40, 0x33, 0x66, 0x99);
	CHECK_PIXEL(60, 70.5, 0x33, 0x66, 0x99);
	/* disc: center filled, corners of its square untouched */
	CHECK_PIXEL(200, 60, 0, 255, 0);
	CHECK_PIXEL(200, 12, 0, 255, 0);
	CHECK_PIXEL(153, 13, 0x33, 0x66, 0x99);
	CHECK_PIXEL(247, 107, 0x33, 0x66, 0x99);
	/* outline: white band of 10 units, black inside */
	CHECK_PIXEL(15, 200, 255, 255, 255);
	CHECK_PIXEL(60, 245, 255, 255, 255);
	CHECK_PIXEL(60, 200, 0, 0, 0);
	CHECK_PIXEL(22, 162, 0, 0, 0);
	/* opacity */
	CHECK_PIXEL(200, 200, 128, 128, 128);

	HUI_DeleteWindow(window);
}

static void test_clipping(void) {
	/* the red child is larger than its clipping parent: only the part inside is drawn */
	HUI_id window = window_from(
		"<Window Width=\"300\" Height=\"200\" ClearColor=\"#000000\">"
		"  <Canvas>"
		"    <Canvas Slot.Position=\"50,50\" Slot.Size=\"100,100\" Clip=\"true\">"
		"      <Border Slot.Position=\"-40,-40\" Slot.Size=\"300,300\" Background=\"#FF0000\" CornerRadius=\"0\" BorderWidth=\"0\"/>"
		"    </Canvas>"
		"  </Canvas>"
		"</Window>");
	capture(window);
	CHECK_PIXEL(100, 100, 255, 0, 0);
	CHECK_PIXEL(51, 51, 255, 0, 0);
	CHECK_PIXEL(149, 149, 255, 0, 0);
	CHECK_PIXEL(49, 100, 0, 0, 0);
	CHECK_PIXEL(151, 100, 0, 0, 0);
	CHECK_PIXEL(100, 49, 0, 0, 0);
	CHECK_PIXEL(100, 151, 0, 0, 0);
	CHECK_PIXEL(20, 20, 0, 0, 0);
	HUI_DeleteWindow(window);
}

static void test_textures(void) {
	/* 2 x 2 texels: red, green / blue, half-transparent white */
	static const uint8_t texels[16] = {
		255, 0, 0, 255,   0, 255, 0, 255,
		0, 0, 255, 255,   255, 255, 255, 128
	};
	static const uint8_t yellow[16] = {
		255, 255, 0, 255,  255, 255, 0, 255,  255, 255, 0, 255,  255, 255, 0, 255
	};
	HUI_id window = window_from(
		"<Window Width=\"300\" Height=\"200\" ClearColor=\"#000000\">"
		"  <Canvas><Image Name=\"image\" Slot.Position=\"20,20\" Slot.Size=\"100,100\"/></Canvas>"
		"</Window>");
	HUI_id image = HUI_FindWidget(window, "image");
	HUI_id texture = HUI_CreateTextureFromPixels(texels, 2, 2);
	int width = 0, height = 0;

	CHECK(HUI_IsValidTexture(texture) && HUI_IsTextureReady(texture));
	HUI_GetTextureSize(texture, &width, &height);
	CHECK(width == 2 && height == 2);
	/* no smoothing: each texel is a flat quarter of the image */
	HUI_SetTextureFilter(texture, HUI_FILTER_NEAREST);
	HUI_SetImageTexture(image, texture);
	HUI_SetImageStretch(image, HUI_STRETCH_FILL);
	capture(window);
	CHECK_PIXEL(45, 45, 255, 0, 0);
	CHECK_PIXEL(95, 45, 0, 255, 0);
	CHECK_PIXEL(45, 95, 0, 0, 255);
	/* the alpha of the texture blends with what is behind (black) */
	CHECK_PIXEL(95, 95, 128, 128, 128);
	CHECK_PIXEL(130, 60, 0, 0, 0);

	/* the tint multiplies the texture */
	HUI_SetImageTintColor(image, 1.0f, 0.0f, 0.0f, 1.0f);
	capture(window);
	CHECK_PIXEL(45, 45, 255, 0, 0);
	CHECK_PIXEL(95, 45, 0, 0, 0);
	HUI_SetImageTintColor(image, 1.0f, 1.0f, 1.0f, 1.0f);

	/* new pixels, same size */
	HUI_UpdateTexture(texture, yellow, 2, 2);
	capture(window);
	CHECK_PIXEL(45, 45, 255, 255, 0);
	CHECK_PIXEL(95, 95, 255, 255, 0);

	/* a deleted texture is simply not drawn */
	HUI_DeleteTexture(texture);
	CHECK(!HUI_IsValidTexture(texture));
	capture(window);
	CHECK_PIXEL(45, 45, 0, 0, 0);

	HUI_DeleteWindow(window);
}

static void test_text(void) {
	HUI_id window = window_from(
		"<Window Width=\"300\" Height=\"120\" ClearColor=\"#000000\">"
		"  <Canvas><Label Name=\"label\" Slot.Position=\"20,20\" TextSize=\"40\" Color=\"#FFFFFF\">HUI</Label></Canvas>"
		"</Window>");
	float x, y, w, h;
	int lit_inside = 0, lit_outside = 0, px, py;

	capture(window);
	HUI_GetWidgetRect(HUI_FindWidget(window, "label"), &x, &y, &w, &h);
	CHECK(w > 40 && h > 30);
	for (py = 0; py < g_height; py++) {
		for (px = 0; px < g_width; px++) {
			const uint8_t* p = g_pixels + ((size_t)py * (size_t)g_width + (size_t)px) * 4;
			float lx = (float)px / g_scale, ly = (float)py / g_scale;
			int inside = lx >= x - 1 && ly >= y - 1 && lx < x + w + 1 && ly < y + h + 1;
			if (p[0] > 128) {
				if (inside) lit_inside++;
				else lit_outside++;
			}
		}
	}
	/* three capital letters of 40 units cover a few hundred pixels, all inside the label */
	CHECK(lit_inside > 300);
	CHECK(lit_outside == 0);
	HUI_DeleteWindow(window);
}

static void paint_custom(HUI_id widget, HUI_Painter* painter, float width, float height, void* user_data) {
	(void)widget;
	(void)width;
	(void)height;
	(void)user_data;
	HUI_DrawTriangle(painter, 10, 10, 90, 10, 50, 90, 1, 0, 0, 1);
	HUI_DrawLine(painter, 110, 50, 190, 50, 10, 0, 1, 0, 1);
	HUI_DrawCircle(painter, 250, 50, 40, 0, 0, 1, 1);
	HUI_DrawRectOutline(painter, 10, 110, 80, 60, 0, 4, 1, 1, 1, 1);
	HUI_PushClipRect(painter, 110, 110, 40, 60);
	HUI_DrawRect(painter, 110, 110, 80, 60, 0, 1, 1, 0, 1);
	HUI_PopClipRect(painter);
}

static void test_custom_drawing(void) {
	HUI_id window = window_from(
		"<Window Width=\"300\" Height=\"200\" ClearColor=\"#000000\"><Custom Name=\"custom\"/></Window>");
	HUI_SetCustomPaintCallback(HUI_FindWidget(window, "custom"), paint_custom, NULL);
	capture(window);
	/* triangle */
	CHECK_PIXEL(50, 30, 255, 0, 0);
	CHECK_PIXEL(50, 80, 255, 0, 0);
	CHECK_PIXEL(15, 80, 0, 0, 0);
	CHECK_PIXEL(85, 80, 0, 0, 0);
	/* line of thickness 10 with round caps */
	CHECK_PIXEL(150, 50, 0, 255, 0);
	CHECK_PIXEL(150, 46, 0, 255, 0);
	CHECK_PIXEL(150, 58, 0, 0, 0);
	/* circle */
	CHECK_PIXEL(250, 50, 0, 0, 255);
	CHECK_PIXEL(250, 15, 0, 0, 255);
	CHECK_PIXEL(215, 15, 0, 0, 0);
	/* outline */
	CHECK_PIXEL(12, 140, 255, 255, 255);
	CHECK_PIXEL(50, 140, 0, 0, 0);
	/* clip rectangle: only the left half of the yellow rectangle */
	CHECK_PIXEL(130, 140, 255, 255, 0);
	CHECK_PIXEL(170, 140, 0, 0, 0);
	HUI_DeleteWindow(window);
}

static int g_callback_calls;
static HUI_ViewportDrawInfo g_callback_info;

/* Render thread. */
static void on_viewport_draw(const HUI_ViewportDrawInfo* info, void* user_data) {
	(void)user_data;
	g_callback_info = *info;
	g_callback_calls++;
}

static void test_viewport(void) {
	static uint8_t pixels[8 * 8 * 4];
	HUI_id window = window_from(
		"<Window Width=\"300\" Height=\"200\" ClearColor=\"#000000\">"
		"  <Canvas>"
		"    <Viewport Name=\"texture\" Slot.Position=\"10,10\" Slot.Size=\"100,100\"/>"
		"    <Viewport Name=\"callback\" Slot.Position=\"150,20\" Slot.Size=\"120,80\"/>"
		"  </Canvas>"
		"</Window>");
	HUI_id texture_viewport = HUI_FindWidget(window, "texture");
	HUI_id texture;
	int width = 0, height = 0, i;

	/* texture mode */
	for (i = 0; i < 64; i++) {
		pixels[i * 4 + 0] = 0;
		pixels[i * 4 + 1] = 200;
		pixels[i * 4 + 2] = 100;
		pixels[i * 4 + 3] = 255;
	}
	texture = HUI_CreateTextureFromPixels(pixels, 8, 8);
	HUI_SetViewportTexture(texture_viewport, texture);
	/* callback mode */
	HUI_SetViewportDrawCallback(HUI_FindWidget(window, "callback"), on_viewport_draw, NULL);

	capture(window);
	CHECK_PIXEL(60, 60, 0, 200, 100);
	CHECK_PIXEL(120, 60, 0, 0, 0);
	HUI_GetViewportPixelSize(texture_viewport, &width, &height);
	CHECK(width == (int)(100 * g_scale) && height == (int)(100 * g_scale));

	/* the capture waited for the frame, so the callback already ran on the render thread */
	CHECK(g_callback_calls > 0);
	CHECK(g_callback_info.backend == HUI_BACKEND_VULKAN);
	CHECK(g_callback_info.window == window);
	CHECK(g_callback_info.command_buffer != NULL && g_callback_info.render_pass != NULL);
	CHECK(g_callback_info.x == (int)(150 * g_scale) && g_callback_info.y == (int)(20 * g_scale));
	CHECK(g_callback_info.width == (int)(120 * g_scale) && g_callback_info.height == (int)(80 * g_scale));
	CHECK(g_callback_info.clip_width == g_callback_info.width);

	HUI_DeleteTexture(texture);
	HUI_DeleteWindow(window);
}

static void test_windows(void) {
	HUI_id first = window_from("<Window Width=\"200\" Height=\"100\" ClearColor=\"#FF0000\"/>");
	HUI_id second = window_from("<Window Width=\"120\" Height=\"160\" ClearColor=\"#0000FF\"/>");
	int i;

	/* each window has its own swapchain */
	capture(second);
	CHECK(g_width == (int)(120 * g_scale) && g_height == (int)(160 * g_scale));
	CHECK_PIXEL(60, 80, 0, 0, 255);
	capture(first);
	CHECK_PIXEL(100, 50, 255, 0, 0);

	/* resizing recreates the swapchain; a real window system confirms the size with an event */
	HUI_SetWindowSize(first, 260, 140);
	for (i = 0; i < 50; i++) {
		int width = 0, height = 0;
		HUI_PollEvents();
		HUI_Update();
		HUI_GetWindowSize(first, &width, &height);
		if (width == 260 && height == 140) break;
#if !defined(_WIN32)
		usleep(10000);
#endif
	}
	capture(first);
	CHECK(g_width == (int)(260 * g_scale) && g_height == (int)(140 * g_scale));
	CHECK_PIXEL(250, 130, 255, 0, 0);

	HUI_SetWindowClearColor(first, 0.0f, 1.0f, 0.0f, 1.0f);
	capture(first);
	CHECK_PIXEL(10, 10, 0, 255, 0);

	HUI_DeleteWindow(second);
	capture(first);
	CHECK_PIXEL(10, 10, 0, 255, 0);
	HUI_DeleteWindow(first);
}

static void test_vulkan_api(void) {
	HUI_uint count = 0;
	const char* const* names;
	HUI_id shared;

	CHECK(HUI_Vulkan_GetInstance() != NULL);
	CHECK(HUI_Vulkan_GetPhysicalDevice() != NULL);
	CHECK(HUI_Vulkan_GetDevice() != NULL);
	CHECK(HUI_Vulkan_GetGraphicsQueue() != NULL);
	names = HUI_Vulkan_GetRequiredInstanceExtensions(&count);
	CHECK(names != NULL && count >= 2 && strcmp(names[0], "VK_KHR_surface") == 0);
	names = HUI_Vulkan_GetRequiredDeviceExtensions(&count);
	CHECK(names != NULL && count == 1 && strcmp(names[0], "VK_KHR_swapchain") == 0);

	/* optional feature: not every device can export memory */
	EXPECT_ERRORS_BEGIN();
	shared = HUI_Vulkan_CreateSharedTexture(64, 32);
	EXPECT_ERRORS_END(0);
	if (shared != HUI_INVALID_ID) {
		HUI_SharedTextureInfo info;
		memset(&info, 0, sizeof(info));
		CHECK(HUI_Vulkan_GetSharedTextureInfo(shared, &info));
		CHECK(info.width == 64 && info.height == 32);
		CHECK(info.allocation_size >= 64 * 32 * 4);
#if !defined(_WIN32)
		CHECK(info.handle >= 0);
		if (info.handle >= 0) close((int)info.handle);
#endif
		HUI_DeleteTexture(shared);
		printf("  shared textures: supported\n");
	} else {
		printf("  shared textures: not supported by this device\n");
	}
}

/* ---------------------------------------------------------------------------
 *  Start-up
 * ------------------------------------------------------------------------- */

/* Starts the Vulkan backend and checks that a window can really be rendered. */
static int start(int headless) {
	HUI_InitDesc desc;
	HUI_id probe;
	uint8_t* pixels;
	int width = 0, height = 0;

	memset(&desc, 0, sizeof(desc));
	desc.backend = HUI_BACKEND_VULKAN;
	desc.headless = headless;
	desc.app_name = "test_render";
	HUI_RegisterErrorCallback(test_error_callback);
	if (!HUI_InitEx(&desc)) return 0;
	probe = HUI_CreateWindow("probe", 64, 64, HUI_WINDOW_DEFAULT);
	pixels = probe != HUI_INVALID_ID ? HUI_CaptureWindow(probe, &width, &height) : NULL;
	HUI_FreePixels(pixels);
	if (probe != HUI_INVALID_ID) HUI_DeleteWindow(probe);
	if (!pixels) {
		HUI_Shutdown();
		return 0;
	}
	return 1;
}

int main(void) {
	/* failures to start are expected on machines without Vulkan: stay quiet */
	/* HUI_TEST_WINDOWED=1 forces a real window, to test the window system path too */
	int windowed = getenv("HUI_TEST_WINDOWED") != NULL;
	EXPECT_ERRORS_BEGIN();
	if (!windowed && start(HUI_TRUE)) {
		printf("test_render: Vulkan without a window system (headless surface)\n");
	} else if (start(HUI_FALSE)) {
		printf("test_render: Vulkan with a real window\n");
	} else {
		printf("test_render: SKIPPED, no usable Vulkan driver or display\n");
		return SKIP_CODE;
	}
	g_test_expect_errors = 0;
	g_test_checks = 0;
	g_test_failures = 0;

	RUN(test_shapes);
	RUN(test_clipping);
	RUN(test_textures);
	RUN(test_text);
	RUN(test_custom_drawing);
	RUN(test_viewport);
	RUN(test_windows);
	RUN(test_vulkan_api);

	HUI_FreePixels(g_pixels);
	HUI_Shutdown();
	return test_report("test_render");
}
