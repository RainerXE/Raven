/**
 * HUI - benchmark: cost of a large widget tree.
 *
 *   hui_benchmark [columns] [worker threads] [vulkan]
 *
 * Builds `columns` x 60 rows (5 widgets per row), then measures the first
 * layout, a full repaint (serial or split between the worker threads), and
 * the layout after changing a single label. By default nothing is rendered
 * (null backend): the numbers are the CPU cost of HUI itself. With "vulkan",
 * frames are also sent to the GPU and the time of the render thread is shown.
 */

#include <hui/hui.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ROWS 60

static HUI_id build_tree(int columns) {
	HUI_id scroll = HUI_CreateWidget(HUI_WIDGET_SCROLL_BOX);
	HUI_id row_of_columns = HUI_AddChild(scroll, HUI_CreateWidget(HUI_WIDGET_HBOX));
	int column, row;
	for (column = 0; column < columns; column++) {
		HUI_id box = HUI_AddChild(row_of_columns, HUI_CreateWidget(HUI_WIDGET_VBOX));
		for (row = 0; row < ROWS; row++) {
			char text[32];
			HUI_id border = HUI_AddChild(box, HUI_CreateWidget(HUI_WIDGET_BORDER));
			HUI_id line = HUI_AddChild(border, HUI_CreateWidget(HUI_WIDGET_HBOX));
			snprintf(text, sizeof(text), "Item %d.%d", column, row);
			HUI_AddChild(line, HUI_CreateLabel(text));
			HUI_AddChild(line, HUI_CreateButton("x"));
		}
	}
	return scroll;
}

int main(int argc, char** argv) {
	int columns = argc > 1 ? atoi(argv[1]) : 40;
	int workers = argc > 2 ? atoi(argv[2]) : 0;
	int vulkan = argc > 3 && strcmp(argv[3], "vulkan") == 0;
	const int frames = 30;
	HUI_InitDesc desc;
	HUI_FrameStats stats;
	HUI_id window, label;
	double start, build_ms, best = 1e9, sum = 0.0, render = 0.0;
	int i;

	if (columns < 4) columns = 4;
	memset(&desc, 0, sizeof(desc));
	desc.backend = vulkan ? HUI_BACKEND_VULKAN : HUI_BACKEND_NULL;
	desc.headless = !vulkan;
	desc.worker_threads = workers;
	desc.disable_vsync = HUI_TRUE;
	if (!HUI_InitEx(&desc)) {
		fprintf(stderr, "hui_benchmark: cannot start HUI\n");
		return 1;
	}

	/* large enough for every widget to be visible: nothing is culled */
	window = HUI_CreateWindow("HUI benchmark", vulkan ? 1600 : 150 * columns, vulkan ? 900 : 2000, HUI_WINDOW_DEFAULT);
	start = HUI_GetTime();
	HUI_SetWindowContent(window, build_tree(columns));
	build_ms = (HUI_GetTime() - start) * 1000.0;

	HUI_Update();
	HUI_GetFrameStats(window, &stats);
	printf("%u widgets, %d worker thread(s)\n", stats.widget_count, HUI_GetWorkerThreadCount());
	printf("  build         %8.2f ms\n", build_ms);
	printf("  first layout  %8.2f ms\n", stats.layout_ms);
	printf("  first paint   %8.2f ms   (rasterizes the glyphs)\n", stats.paint_ms);

	for (i = 0; i < frames; i++) {
		HUI_RequestWindowRedraw(window);
		HUI_Update();
		HUI_GetFrameStats(window, &stats);
		if (stats.paint_ms < best) best = stats.paint_ms;
		sum += stats.paint_ms;
		render += stats.render_ms;
	}
	printf("  repaint       %8.2f ms   best of %d (average %.2f ms): %u primitives, %u draw call(s), %u paint job(s)\n",
		best, frames, sum / frames, stats.instance_count, stats.draw_call_count, stats.paint_job_count);
	if (vulkan) printf("  render thread %8.2f ms   average per frame (upload, record, submit, present)\n", render / frames);

	/* one label changes: only its ancestors are measured and arranged again */
	label = HUI_GetChild(HUI_GetChild(HUI_GetChild(HUI_GetChild(HUI_GetChild(HUI_GetWindowContent(window), 0), 3), 7), 0), 0);
	HUI_SetLabelText(label, "a longer text than before");
	HUI_Update();
	HUI_GetFrameStats(window, &stats);
	printf("  relayout      %8.3f ms   after changing one label\n", stats.layout_ms);

	HUI_Shutdown();
	return 0;
}
