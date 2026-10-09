/**
 * HUI tests - threading: job system, tasks posted to the UI thread,
 * asynchronous textures, and parallel painting giving the same result as
 * serial painting.
 */

#include "test.h"

#include <stdlib.h>

#define ITEM_COUNT 100000

static int g_values[ITEM_COUNT];
static volatile int g_posted;
static HUI_id g_label;

static void fill_range(HUI_uint begin, HUI_uint end, void* user_data) {
	HUI_uint i;
	(void)user_data;
	for (i = begin; i < end; i++) g_values[i] += (int)i + 1;
}

/* Runs on the UI thread: it may touch widgets. */
static void posted_task(void* user_data) {
	HUI_SetLabelText(g_label, (const char*)user_data);
	g_posted++;
}

/* Runs on a worker thread: it must not touch widgets, only post. */
static void async_job(void* user_data) {
	/* the test counters are not thread-safe: report through the posted task instead of CHECK */
	HUI_PostTask(posted_task, HUI_IsUIThread() ? (void*)"job ran on the UI thread" : user_data);
}

static void wait_for_posted(int count) {
	double deadline = HUI_GetTime() + 5.0;
	while (g_posted < count && HUI_GetTime() < deadline) {
		HUI_WaitEvents(0.05);
		HUI_Update();
	}
}

static void test_jobs(void) {
	long long sum = 0;
	int i;
	CHECK(HUI_GetWorkerThreadCount() == 4);
	CHECK(HUI_IsUIThread());

	memset(g_values, 0, sizeof(g_values));
	HUI_ParallelFor(ITEM_COUNT, 512, fill_range, NULL);
	HUI_ParallelFor(ITEM_COUNT, 0, fill_range, NULL);
	for (i = 0; i < ITEM_COUNT; i++) sum += g_values[i];
	/* every item was visited exactly once per call */
	CHECK(sum == 2LL * ((long long)ITEM_COUNT * (ITEM_COUNT + 1) / 2));

	g_posted = 0;
	for (i = 0; i < 50; i++) HUI_RunAsync(async_job, "from a worker");
	wait_for_posted(50);
	CHECK(g_posted == 50);
	CHECK_STR(HUI_GetLabelText(g_label), "from a worker");

	/* widget functions refuse to run on another thread: tested by the worker above
	 * through HUI_IsUIThread; here the wake-up from a foreign thread */
	g_posted = 0;
	HUI_RunAsync(async_job, "again");
	HUI_WaitEvents(5.0);
	wait_for_posted(1);
	CHECK(g_posted == 1);
}

static void test_async_texture(void) {
	/* a 2 x 2 BMP, 24 bits, built by hand */
	static const unsigned char bmp[] = {
		'B', 'M', 70, 0, 0, 0, 0, 0, 0, 0, 54, 0, 0, 0, 40, 0, 0, 0, 2, 0, 0, 0, 2, 0, 0, 0, 1, 0, 24, 0,
		0, 0, 0, 0, 16, 0, 0, 0, 19, 11, 0, 0, 19, 11, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 255, 0, 255, 0, 0, 0, 255, 0, 0, 255, 255, 255, 0, 0
	};
	HUI_id texture = HUI_CreateTextureAsync((const char*)bmp, sizeof(bmp));
	HUI_id pixels;
	static const uint8_t rgba[16] = { 255, 0, 0, 255, 0, 255, 0, 255, 0, 0, 255, 255, 255, 255, 255, 255 };
	int w = 0, h = 0;

	CHECK(HUI_IsValidTexture(texture));
#if defined(HUI_TEST_NO_IMAGE_DECODER)
	/* built with HUI_WITH_STB_IMAGE=OFF: decoding fails, which must be reported and not hang */
	EXPECT_ERRORS_BEGIN();
	HUI_WaitForTexture(texture);
	EXPECT_ERRORS_END(1);
	CHECK(HUI_IsTextureReady(texture));
#else
	HUI_WaitForTexture(texture);
	CHECK(HUI_IsTextureReady(texture));
	HUI_GetTextureSize(texture, &w, &h);
	CHECK(w == 2 && h == 2);
#endif
	HUI_DeleteTexture(texture);
	CHECK(!HUI_IsValidTexture(texture));

	pixels = HUI_CreateTextureFromPixels(rgba, 2, 2);
	HUI_SetTextureFilter(pixels, HUI_FILTER_NEAREST);
	HUI_UpdateTexture(pixels, rgba, 2, 2);
	HUI_UpdateTexture(pixels, rgba, 4, 1);
	HUI_GetTextureSize(pixels, &w, &h);
	CHECK(w == 4 && h == 1);

	/* garbage is reported, not crashed on */
	EXPECT_ERRORS_BEGIN();
	CHECK(HUI_CreateTexture("not an image", 12) == HUI_INVALID_ID);
	texture = HUI_CreateTextureAsync("not an image", 12);
	HUI_WaitForTexture(texture);
	EXPECT_ERRORS_END(2);
	CHECK(HUI_IsTextureReady(texture));
	HUI_GetTextureSize(texture, &w, &h);
	CHECK(w == 0 && h == 0);
}

/* A deep, wide tree: 40 groups of 60 rows, each row a border holding a label and a button. */
static HUI_id build_big_tree(void) {
	HUI_id scroll = HUI_CreateWidget(HUI_WIDGET_SCROLL_BOX);
	HUI_id columns = HUI_AddChild(scroll, HUI_CreateWidget(HUI_WIDGET_HBOX));
	int group, row;
	for (group = 0; group < 40; group++) {
		HUI_id box = HUI_AddChild(columns, HUI_CreateWidget(HUI_WIDGET_VBOX));
		for (row = 0; row < 60; row++) {
			char text[32];
			HUI_id border = HUI_AddChild(box, HUI_CreateWidget(HUI_WIDGET_BORDER));
			HUI_id line = HUI_AddChild(border, HUI_CreateWidget(HUI_WIDGET_HBOX));
			snprintf(text, sizeof(text), "Item %d.%d", group, row);
			HUI_AddChild(line, HUI_CreateLabel(text));
			HUI_AddChild(line, HUI_CreateButton("x"));
			HUI_SetBorderOutline(border, 1, 1, 1, 1, 0.2f);
		}
	}
	return scroll;
}

static HUI_FrameStats paint_big_tree(int workers) {
	HUI_FrameStats stats;
	HUI_id window;
	int frame;
	memset(&stats, 0, sizeof(stats));
	if (!test_init(workers)) return stats;
	/* large enough for every widget to be visible, so that nothing is culled */
	window = HUI_CreateWindow("big", 6000, 2000, HUI_WINDOW_DEFAULT);
	HUI_SetWindowContent(window, build_big_tree());
	/* the first frame also rasterizes the glyphs: measure a later one */
	for (frame = 0; frame < 5; frame++) {
		HUI_RequestWindowRedraw(window);
		HUI_Update();
	}
	HUI_GetFrameStats(window, &stats);
	HUI_Shutdown();
	return stats;
}

static void test_parallel_paint(void) {
	HUI_FrameStats serial = paint_big_tree(-1);
	HUI_FrameStats parallel = paint_big_tree(4);

	CHECK(serial.widget_count > 9000);
	CHECK(serial.paint_job_count == 0);
	CHECK(parallel.paint_job_count > 0);
	/* the same widgets produce the same primitives and the same batches */
	CHECK(parallel.widget_count == serial.widget_count);
	CHECK(parallel.instance_count == serial.instance_count);
	CHECK(parallel.draw_call_count == serial.draw_call_count);
	CHECK(serial.instance_count > 20000);
	/* text and shapes share one batch: thousands of widgets, a handful of draw calls */
	CHECK(serial.draw_call_count <= 2);
	printf("  %u widgets, %u instances, %u draw call(s); paint %.2f ms serial, %.2f ms with %u jobs\n",
		serial.widget_count, serial.instance_count, serial.draw_call_count, serial.paint_ms, parallel.paint_ms,
		parallel.paint_job_count);
}

int main(void) {
	HUI_id window;
	if (!test_init(4)) {
		printf("cannot initialize HUI\n");
		return 1;
	}
	window = HUI_CreateWindow("threads", 400, 300, HUI_WINDOW_DEFAULT);
	g_label = HUI_CreateLabel("start");
	HUI_SetWindowContent(window, g_label);

	RUN(test_jobs);
	RUN(test_async_texture);
	HUI_Shutdown();

	RUN(test_parallel_paint);
	return test_report("test_threads");
}
