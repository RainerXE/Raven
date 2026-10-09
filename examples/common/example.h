/**
 * Raven examples - helpers shared by the examples (not part of the library).
 *
 *   example_init          starts Raven and prints its errors to stderr
 *   example_run           runs the event loop
 *
 * For automated runs, two environment variables make an example render a
 * fixed number of frames, optionally save the last one, and exit:
 *
 *   RAVEN_EXAMPLE_FRAMES=30 RAVEN_EXAMPLE_CAPTURE=shot.ppm ./raven_editor
 */

#ifndef RVN_EXAMPLE_H
#define RVN_EXAMPLE_H

#include <Raven.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void example_on_error(RError code, RSeverity severity, const char* detail) {
	fprintf(stderr, "[Raven] %s (%s): %s\n", Rerror_to_string(code), Rseverity_to_string(severity), detail);
}

/** Starts Raven with the Vulkan backend. Returns R_FALSE on failure. */
static int example_init(const char* name) {
	RInitDesc desc;
	memset(&desc, 0, sizeof(desc));
	desc.backend = R_BACKEND_VULKAN;
	desc.app_name = name;
#ifndef NDEBUG
	desc.enable_validation = R_TRUE;
#endif
	Rerror_set_callback(example_on_error);
	if (!Rinit_ex(&desc)) {
		fprintf(stderr, "%s: cannot start Raven (is a Vulkan driver installed?)\n", name);
		return R_FALSE;
	}
	return R_TRUE;
}

/** Saves a window as a binary PPM image. */
static void example_capture(Rid window, const char* path) {
	int width = 0, height = 0, i;
	uint8_t* pixels = Rwindow_capture(window, &width, &height);
	FILE* file = pixels ? fopen(path, "wb") : NULL;
	if (file) {
		fprintf(file, "P6\n%d %d\n255\n", width, height);
		for (i = 0; i < width * height; i++) fwrite(pixels + i * 4, 1, 3, file);
		fclose(file);
	}
	Rpixels_free(pixels);
}

/** Runs until the last window is closed or Rquit is called. */
static int example_run(Rid window) {
	const char* frames = getenv("RAVEN_EXAMPLE_FRAMES");
	const char* capture = getenv("RAVEN_EXAMPLE_CAPTURE");
	if (frames) {
		/* automated run: a fixed number of frames instead of waiting for the user */
		int count = atoi(frames), i;
		for (i = 0; i < count && Ris_running(); i++) {
			Rwait_events(0.01);
			Rwindow_request_redraw(window);
			Rupdate();
		}
		if (capture && Rwindow_is_valid(window)) example_capture(window, capture);
		if (Rwindow_is_valid(window)) {
			RFrameStats stats;
			Rwindow_get_frame_stats(window, &stats);
			printf("last frame: %u widgets, %u primitives, %u draw calls, %u backdrop captures, paint %.2f ms, render %.2f ms\n",
				stats.widget_count, stats.instance_count, stats.draw_call_count, stats.backdrop_count,
				stats.paint_ms, stats.render_ms);
		}
		return 0;
	}
	return Rrun();
}

#endif /* RVN_EXAMPLE_H */
