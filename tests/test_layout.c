/**
 * HUI tests - layout: the measure / arrange passes of every panel.
 * Sizes are checked through HUI_GetWidgetRect, in a 400 x 300 window.
 */

#include "test.h"

static HUI_id g_window;

/* A fixed-size block standing for any content. */
static HUI_id block(float width, float height) {
	HUI_id spacer = HUI_CreateWidget(HUI_WIDGET_SPACER);
	HUI_SetWidgetSize(spacer, width, height);
	return spacer;
}

static void test_vbox(void) {
	HUI_id box = HUI_CreateWidget(HUI_WIDGET_VBOX);
	HUI_id a = HUI_AddChild(box, block(50, 20));
	HUI_id b = HUI_AddChild(box, block(80, 30));
	HUI_id c = HUI_AddChild(box, block(10, 40));
	float w, h;
	HUI_SetWindowContent(g_window, box);

	/* auto slots stack with their desired height and fill the width */
	CHECK_RECT(a, 0, 0, 400, 20);
	CHECK_RECT(b, 0, 20, 400, 30);
	CHECK_RECT(c, 0, 50, 400, 40);
	HUI_GetWidgetDesiredSize(box, &w, &h);
	CHECK_NEAR(w, 80);
	CHECK_NEAR(h, 90);

	/* a fill slot takes what is left */
	HUI_SetSlotFill(b, 1.0f);
	CHECK_RECT(b, 0, 20, 400, 240);
	CHECK_RECT(c, 0, 260, 400, 40);

	/* two fill slots share in proportion to their coefficients */
	HUI_SetSlotFill(c, 3.0f);
	CHECK_RECT(b, 0, 20, 400, 70);
	CHECK_RECT(c, 0, 90, 400, 210);

	/* alignment across the axis, padding and spacing */
	HUI_SetSlotAuto(b);
	HUI_SetSlotAuto(c);
	HUI_SetSlotHAlign(a, HUI_HALIGN_CENTER);
	HUI_SetSlotHAlign(b, HUI_HALIGN_RIGHT);
	HUI_SetSlotPadding(c, 5, 6, 7, 8);
	HUI_SetPanelSpacing(box, 10);
	CHECK_RECT(a, 175, 0, 50, 20);
	CHECK_RECT(b, 320, 30, 80, 30);
	CHECK_RECT(c, 5, 76, 388, 40);

	/* a collapsed child takes no space, a hidden one keeps it */
	HUI_SetWidgetVisibility(a, HUI_COLLAPSED);
	CHECK_RECT(b, 320, 0, 80, 30);
	HUI_SetWidgetVisibility(a, HUI_HIDDEN);
	CHECK_RECT(b, 320, 30, 80, 30);

	/* the slot maximum caps a fill slot */
	HUI_SetWidgetVisibility(a, HUI_VISIBLE);
	HUI_SetSlotFill(b, 1.0f);
	HUI_SetSlotMaxSize(b, 100.0f);
	HUI_SetSlotHAlign(b, HUI_HALIGN_FILL);
	CHECK_RECT(b, 0, 30, 400, 100);
}

static void test_hbox(void) {
	HUI_id box = HUI_CreateWidget(HUI_WIDGET_HBOX);
	HUI_id a = HUI_AddChild(box, block(50, 20));
	HUI_id b = HUI_AddChild(box, block(80, 30));
	HUI_SetWindowContent(g_window, box);
	HUI_SetSlotFill(b, 1.0f);
	HUI_SetSlotVAlign(a, HUI_VALIGN_BOTTOM);
	CHECK_RECT(a, 0, 280, 50, 20);
	CHECK_RECT(b, 50, 0, 350, 300);
	HUI_SetSlotVAlign(a, HUI_VALIGN_CENTER);
	CHECK_RECT(a, 0, 140, 50, 20);
}

static void test_overlay_and_border(void) {
	HUI_id overlay = HUI_CreateWidget(HUI_WIDGET_OVERLAY);
	HUI_id back = HUI_AddChild(overlay, block(10, 10));
	HUI_id border = HUI_AddChild(overlay, HUI_CreateWidget(HUI_WIDGET_BORDER));
	HUI_id inside = HUI_AddChild(border, block(60, 40));
	HUI_id replaced;
	HUI_SetWindowContent(g_window, overlay);

	HUI_SetSlotHAlign(border, HUI_HALIGN_RIGHT);
	HUI_SetSlotVAlign(border, HUI_VALIGN_BOTTOM);
	HUI_SetSlotPadding(border, 0, 0, 10, 10);
	HUI_SetBorderPadding(border, 4, 4, 4, 4);
	CHECK_RECT(back, 0, 0, 400, 300);
	CHECK_RECT(border, 322, 242, 68, 48);
	CHECK_RECT(inside, 326, 246, 60, 40);

	/* a single-child panel replaces its child */
	replaced = HUI_AddChild(border, block(20, 20));
	CHECK(!HUI_IsValidWidget(inside));
	CHECK(HUI_GetChildCount(border) == 1);
	CHECK_RECT(replaced, 366, 266, 20, 20);
}

static void test_grid(void) {
	HUI_id grid = HUI_CreateWidget(HUI_WIDGET_GRID);
	HUI_id a = HUI_AddChild(grid, block(40, 20));
	HUI_id b = HUI_AddChild(grid, block(60, 30));
	HUI_id c = HUI_AddChild(grid, block(100, 10));
	HUI_id d = HUI_AddChild(grid, block(20, 50));
	float w, h;
	HUI_SetWindowContent(g_window, grid);
	HUI_SetSlotGrid(a, 0, 0);
	HUI_SetSlotGrid(b, 1, 0);
	HUI_SetSlotGrid(c, 0, 1);
	HUI_SetSlotGrid(d, 1, 1);

	/* columns: 100, 60 - rows: 30, 50 */
	HUI_GetWidgetDesiredSize(grid, &w, &h);
	CHECK_NEAR(w, 160);
	CHECK_NEAR(h, 80);
	CHECK_RECT(a, 0, 0, 100, 30);
	CHECK_RECT(b, 100, 0, 60, 30);
	CHECK_RECT(d, 100, 30, 60, 50);

	/* a fill column takes the free width, a fill row the free height */
	HUI_SetGridColumnFill(grid, 1, 1.0f);
	HUI_SetGridRowFill(grid, 0, 1.0f);
	CHECK_RECT(b, 100, 0, 300, 250);
	CHECK_RECT(c, 0, 250, 100, 50);

	/* a spanning cell covers both columns */
	HUI_SetSlotGridSpan(c, 2, 1);
	HUI_SetWidgetVisibility(d, HUI_COLLAPSED);
	CHECK_RECT(c, 0, 290, 400, 10);

	/* the first column is now only as wide as `a`; gaps of 8 between tracks */
	HUI_SetPanelSpacing(grid, 8);
	CHECK_RECT(b, 48, 0, 352, 282);
}

static void test_uniform_grid(void) {
	HUI_id grid = HUI_CreateWidget(HUI_WIDGET_UNIFORM_GRID);
	HUI_id cells[5];
	int i;
	for (i = 0; i < 5; i++) cells[i] = HUI_AddChild(grid, block(10, 10));
	HUI_SetWindowContent(g_window, grid);
	HUI_SetUniformGridColumns(grid, 2);
	/* 2 columns x 3 rows sharing 400 x 300 */
	CHECK_RECT(cells[0], 0, 0, 200, 100);
	CHECK_RECT(cells[1], 200, 0, 200, 100);
	CHECK_RECT(cells[4], 0, 200, 200, 100);
}

static void test_size_box_and_canvas(void) {
	HUI_id box = HUI_CreateWidget(HUI_WIDGET_SIZE_BOX);
	HUI_id content = HUI_AddChild(box, block(10, 10));
	HUI_id canvas, pinned, stretched;
	HUI_SetWindowContent(g_window, box);

	/* 2:1 inside 400 x 300 is 400 x 200, centered */
	HUI_SetSizeBoxAspectRatio(box, 2.0f);
	CHECK_RECT(content, 0, 50, 400, 200);
	HUI_SetSizeBoxAspectRatio(box, 0.5f);
	CHECK_RECT(content, 125, 0, 150, 300);

	canvas = HUI_CreateWidget(HUI_WIDGET_CANVAS);
	pinned = HUI_AddChild(canvas, block(40, 20));
	stretched = HUI_AddChild(canvas, block(1, 1));
	HUI_SetWindowContent(g_window, canvas);

	/* pinned to the bottom-right corner, pivot on its own bottom-right corner */
	HUI_SetSlotAnchors(pinned, 1, 1, 1, 1);
	HUI_SetSlotAlignment(pinned, 1, 1);
	HUI_SetSlotPosition(pinned, -10, -10);
	CHECK_RECT(pinned, 350, 270, 40, 20);

	/* stretched over the whole canvas with a 20 unit margin */
	HUI_SetSlotAnchors(stretched, 0, 0, 1, 1);
	HUI_SetSlotPosition(stretched, 20, 20);
	HUI_SetSlotSize(stretched, 20, 20);
	CHECK_RECT(stretched, 20, 20, 360, 260);
}

static void test_switcher_and_constraints(void) {
	HUI_id switcher = HUI_CreateWidget(HUI_WIDGET_SWITCHER);
	HUI_id first = HUI_AddChild(switcher, block(10, 10));
	HUI_id second = HUI_AddChild(switcher, block(10, 10));
	HUI_id box, limited;
	float w, h;
	HUI_SetWindowContent(g_window, switcher);
	CHECK_RECT(first, 0, 0, 400, 300);
	HUI_SetSwitcherIndex(switcher, 1);
	CHECK(HUI_GetSwitcherIndex(switcher) == 1);
	CHECK_RECT(second, 0, 0, 400, 300);

	box = HUI_CreateWidget(HUI_WIDGET_VBOX);
	limited = HUI_AddChild(box, block(500, 5));
	HUI_SetWindowContent(g_window, box);
	HUI_SetSlotHAlign(limited, HUI_HALIGN_LEFT);
	HUI_SetWidgetMaxSize(limited, 120, -1);
	HUI_SetWidgetMinSize(limited, -1, 33);
	HUI_GetWidgetDesiredSize(limited, &w, &h);
	CHECK_NEAR(w, 120);
	CHECK_NEAR(h, 33);
	CHECK_RECT(limited, 0, 0, 120, 33);
}

static void test_scroll_box(void) {
	HUI_id scroll = HUI_CreateWidget(HUI_WIDGET_SCROLL_BOX);
	HUI_id rows[10];
	float x, y, bar = HUI_GetThemeMetric(HUI_METRIC_SCROLLBAR_SIZE);
	int i;
	for (i = 0; i < 10; i++) rows[i] = HUI_AddChild(scroll, block(50, 100));
	HUI_SetWindowContent(g_window, scroll);

	/* 1000 units of content in a 300 unit view: the scrollbar takes its width */
	CHECK_RECT(rows[0], 0, 0, 400 - bar, 100);
	HUI_SetScrollOffset(scroll, 0, 250);
	CHECK_RECT(rows[0], 0, -250, 400 - bar, 100);
	CHECK_RECT(rows[3], 0, 50, 400 - bar, 100);

	/* offsets are clamped to the content */
	HUI_SetScrollOffset(scroll, 0, 5000);
	HUI_GetScrollOffset(scroll, &x, &y);
	CHECK_NEAR(y, 700);
	HUI_ScrollToEnd(scroll);
	CHECK_RECT(rows[9], 0, 200, 400 - bar, 100);

	HUI_SetScrollOffset(scroll, 0, 0);
	HUI_ScrollIntoView(rows[5]);
	HUI_GetScrollOffset(scroll, &x, &y);
	CHECK_NEAR(y, 300);

	/* the mouse wheel scrolls, and widgets scrolled out of view cannot be hit */
	HUI_SetScrollOffset(scroll, 0, 0);
	HUI_InjectMouseMove(g_window, 100, 100);
	HUI_InjectMouseWheel(g_window, 0, -1);
	HUI_GetScrollOffset(scroll, &x, &y);
	CHECK(y > 0);
}

static void test_splitter_and_wrap(void) {
	HUI_id splitter = HUI_CreateWidget(HUI_WIDGET_SPLITTER);
	HUI_id left = HUI_AddChild(splitter, block(10, 10));
	HUI_id right = HUI_AddChild(splitter, block(10, 10));
	float handle = HUI_GetThemeMetric(HUI_METRIC_SPLITTER_SIZE);
	float x, y, w, h;
	HUI_id wrap, items[5];
	int i;
	HUI_SetWindowContent(g_window, splitter);

	HUI_SetSlotFill(left, 1.0f);
	HUI_SetSlotFill(right, 3.0f);
	HUI_GetWidgetRect(left, &x, &y, &w, &h);
	CHECK_NEAR(w, (400 - handle) * 0.25f);
	CHECK_NEAR(h, 300);

	/* dragging the handle 50 units to the right grows the left pane by 50 */
	HUI_InjectMouseMove(g_window, w + handle * 0.5f, 150);
	HUI_InjectMouseButton(g_window, HUI_MOUSE_LEFT, HUI_TRUE);
	HUI_InjectMouseMove(g_window, w + handle * 0.5f + 50, 150);
	HUI_InjectMouseButton(g_window, HUI_MOUSE_LEFT, HUI_FALSE);
	CHECK_RECT(left, 0, 0, w + 50, 300);

	wrap = HUI_CreateWidget(HUI_WIDGET_WRAP_BOX);
	for (i = 0; i < 5; i++) items[i] = HUI_AddChild(wrap, block(150, 40));
	HUI_SetWindowContent(g_window, wrap);
	/* two blocks of 150 fit in 400, the third wraps */
	CHECK_RECT(items[1], 150, 0, 150, 40);
	CHECK_RECT(items[2], 0, 40, 150, 40);
	CHECK_RECT(items[4], 0, 80, 150, 40);
	HUI_GetWidgetDesiredSize(wrap, &w, &h);
	CHECK_NEAR(h, 120);
}

static void test_window_resize_and_text(void) {
	HUI_id box = HUI_CreateWidget(HUI_WIDGET_VBOX);
	HUI_id label = HUI_AddChild(box, HUI_CreateLabel("The quick brown fox jumps over the lazy dog, again and again and again."));
	HUI_id filler = HUI_AddChild(box, block(10, 10));
	float single_w, single_h, w, h, text_w, text_h;
	HUI_SetWindowContent(g_window, box);
	HUI_SetSlotFill(filler, 1.0f);

	HUI_GetWidgetDesiredSize(label, &single_w, &single_h);
	CHECK(single_w > 300 && single_h > 8);
	HUI_MeasureText(HUI_INVALID_ID, 0, "The quick brown fox", &text_w, &text_h);
	CHECK(text_w > 80 && text_w < single_w);
	/* the label rounds its desired size up to whole units */
	CHECK(text_h <= single_h && text_h > single_h - 1.0f);

	/* a wrapped label gets taller when the window gets narrower */
	HUI_SetLabelWrap(label, HUI_TRUE);
	HUI_SetWindowSize(g_window, 150, 300);
	HUI_GetWidgetRect(label, NULL, NULL, &w, &h);
	CHECK_NEAR(w, 150);
	CHECK(h >= single_h * 3 - 1);
	CHECK_RECT(filler, 0, h, 150, 300 - h);

	HUI_SetWindowSize(g_window, 400, 300);
	HUI_GetWidgetRect(label, NULL, NULL, &w, &h);
	CHECK_NEAR(w, 400);
	CHECK(h < single_h * 3);
}

static void test_tree_api(void) {
	HUI_id box = HUI_CreateWidget(HUI_WIDGET_VBOX);
	HUI_id a = HUI_AddChild(box, block(10, 10));
	HUI_id b = HUI_AddChild(box, block(10, 10));
	HUI_id other = HUI_CreateWidget(HUI_WIDGET_HBOX);
	HUI_SetWindowContent(g_window, box);

	HUI_SetWidgetName(b, "second");
	CHECK(HUI_FindWidget(g_window, "second") == b);
	CHECK(HUI_FindWidget(box, "missing") == HUI_INVALID_ID);
	CHECK(HUI_GetParent(a) == box);
	CHECK(HUI_GetChild(box, 1) == b);
	CHECK(HUI_GetWidgetWindow(a) == g_window);
	CHECK(HUI_GetWidgetType(box) == HUI_WIDGET_VBOX);

	/* moving a widget to another parent */
	HUI_InsertChild(other, a, 0);
	CHECK(HUI_GetChildCount(box) == 1);
	CHECK(HUI_GetParent(a) == other);
	CHECK(HUI_GetWidgetWindow(a) == HUI_INVALID_ID);

	/* deleting a parent deletes its children; stale handles are detected */
	HUI_DeleteWidget(other);
	CHECK(!HUI_IsValidWidget(a));
	EXPECT_ERRORS_BEGIN();
	HUI_SetWidgetName(a, "ghost");
	CHECK(HUI_AddChild(a, b) == HUI_INVALID_ID);
	CHECK(HUI_AddChild(box, box) == HUI_INVALID_ID);
	CHECK(HUI_AddChild(b, box) == HUI_INVALID_ID); /* a spacer has no children */
	HUI_SetGridColumnFill(box, 0, 1.0f);             /* wrong widget type */
	EXPECT_ERRORS_END(5);
}

int main(void) {
	if (!test_init(0)) {
		printf("cannot initialize HUI\n");
		return 1;
	}
	g_window = HUI_CreateWindow("layout", 400, 300, HUI_WINDOW_DEFAULT);
	CHECK(HUI_IsValidWindow(g_window));

	RUN(test_vbox);
	RUN(test_hbox);
	RUN(test_overlay_and_border);
	RUN(test_grid);
	RUN(test_uniform_grid);
	RUN(test_size_box_and_canvas);
	RUN(test_switcher_and_constraints);
	RUN(test_scroll_box);
	RUN(test_splitter_and_wrap);
	RUN(test_window_resize_and_text);
	RUN(test_tree_api);

	HUI_Shutdown();
	return test_report("test_layout");
}
