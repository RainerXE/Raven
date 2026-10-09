/**
 * HUI examples - helpers shared by the examples (not part of the library).
 *
 *   example_init          starts HUI and prints its errors to stderr
 *   example_run           runs the event loop
 *
 * For automated runs, two environment variables make an example render a
 * fixed number of frames, optionally save the last one, and exit:
 *
 *   HUI_EXAMPLE_FRAMES=30 HUI_EXAMPLE_CAPTURE=shot.ppm ./hui_editor
 */

#ifndef HUI_EXAMPLE_H
#define HUI_EXAMPLE_H

#include <hui/hui.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void example_on_error(HUI_EError code, HUI_ESeverity severity, const char* detail) {
	fprintf(stderr, "[HUI] %s (%s): %s\n", HUI_ErrorEnumToString(code), HUI_SeverityEnumToString(severity), detail);
}

/** Starts HUI with the Vulkan backend. Returns HUI_FALSE on failure. */
static int example_init(const char* name) {
	HUI_InitDesc desc;
	memset(&desc, 0, sizeof(desc));
	desc.backend = HUI_BACKEND_VULKAN;
	desc.app_name = name;
#ifndef NDEBUG
	desc.enable_validation = HUI_TRUE;
#endif
	HUI_RegisterErrorCallback(example_on_error);
	if (!HUI_InitEx(&desc)) {
		fprintf(stderr, "%s: cannot start HUI (is a Vulkan driver installed?)\n", name);
		return HUI_FALSE;
	}
	return HUI_TRUE;
}

/** Saves a window as a binary PPM image. */
static void example_capture(HUI_id window, const char* path) {
	int width = 0, height = 0, i;
	uint8_t* pixels = HUI_CaptureWindow(window, &width, &height);
	FILE* file = pixels ? fopen(path, "wb") : NULL;
	if (file) {
		fprintf(file, "P6\n%d %d\n255\n", width, height);
		for (i = 0; i < width * height; i++) fwrite(pixels + i * 4, 1, 3, file);
		fclose(file);
	}
	HUI_FreePixels(pixels);
}

/** Runs until the last window is closed or HUI_Quit is called. */
static int example_run(HUI_id window) {
	const char* frames = getenv("HUI_EXAMPLE_FRAMES");
	const char* capture = getenv("HUI_EXAMPLE_CAPTURE");
	if (frames) {
		/* automated run: a fixed number of frames instead of waiting for the user */
		int count = atoi(frames), i;
		for (i = 0; i < count && HUI_IsRunning(); i++) {
			HUI_WaitEvents(0.01);
			HUI_RequestWindowRedraw(window);
			HUI_Update();
		}
		if (capture && HUI_IsValidWindow(window)) example_capture(window, capture);
		return 0;
	}
	return HUI_Run();
}

#endif /* HUI_EXAMPLE_H */
