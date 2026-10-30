/**
 * Raven tests - fingers (Rwindow_inject_touch): a tap clicks, a pan scrolls and keeps going, a slider is dragged,
 * a long press is a right click, two fingers zoom. Headless (null platform, null backend).
 */

#include "test.h"

#include <time.h>

static Rid g_window, g_scroll, g_button, g_slider;
static int g_clicks, g_rights, g_wheels, g_ctrl_wheels, g_lefts_down;

static void on_click(Rid button, void* user_data) {
	(void)button;
	(void)user_data;
	g_clicks++;
}

static int on_event(Rid window, const REvent* event, void* user_data) {
	(void)window;
	(void)user_data;
	if (event->type == R_EVENT_MOUSE_DOWN && event->button == R_MOUSE_RIGHT) g_rights++;
	if (event->type == R_EVENT_MOUSE_DOWN && event->button == R_MOUSE_LEFT) g_lefts_down++;
	if (event->type == R_EVENT_MOUSE_WHEEL) {
		g_wheels++;
		if (event->modifiers & R_MOD_CTRL) g_ctrl_wheels++;
	}
	return R_FALSE;
}

static void pause_ms(int ms) {
	struct timespec t;
	t.tv_sec = ms / 1000;
	t.tv_nsec = (long)(ms % 1000) * 1000000L;
	nanosleep(&t, NULL);
}

static float scroll_y(void) {
	float x, y;
	Rscroll_box_get_offset(g_scroll, &x, &y);
	return y;
}

static void build(void) {
	Rid column = Rwidget_create(R_WIDGET_VBOX);
	Rid list = Rwidget_create(R_WIDGET_VBOX);
	int i;
	g_window = Rwindow_create("touch", 300, 400, R_WINDOW_DEFAULT);
	g_button = Rwidget_add_child(column, Rbutton_create("Tap me"));
	Rbutton_set_clicked_callback(g_button, on_click, NULL);
	g_slider = Rwidget_add_child(column, Rwidget_create(R_WIDGET_SLIDER));
	Rslider_set_range(g_slider, 0.0f, 100.0f);
	g_scroll = Rwidget_add_child(column, Rwidget_create(R_WIDGET_SCROLL_BOX));
	Rwidget_set_size(g_scroll, 280.0f, 220.0f);
	for (i = 0; i < 60; i++) {
		char text[32];
		snprintf(text, sizeof(text), "Row %d", i);
		Rwidget_add_child(list, Rlabel_create(text));
	}
	Rwidget_add_child(g_scroll, list);
	Rwindow_set_content(g_window, column);
	Rwindow_set_event_hook(g_window, on_event, NULL);
	Rdraw_frame();
}

static void center(Rid widget, float* x, float* y) {
	float rx, ry, w, h;
	Rwidget_get_rect(widget, &rx, &ry, &w, &h);
	*x = rx + w * 0.5f;
	*y = ry + h * 0.5f;
}

static void test_tap(void) {
	float x, y;
	center(g_button, &x, &y);
	g_clicks = 0;
	Rwindow_inject_touch(g_window, 1, R_TOUCH_BEGIN, x, y);
	CHECK(g_clicks == 0);
	Rwindow_inject_touch(g_window, 1, R_TOUCH_END, x, y);
	CHECK(g_clicks == 1);
	/* nothing scrolls under the finger, so it drags like a mouse: lifted away from the button, no click */
	Rwindow_inject_touch(g_window, 1, R_TOUCH_BEGIN, x, y);
	Rwindow_inject_touch(g_window, 1, R_TOUCH_MOVE, x, y + 90.0f);
	Rwindow_inject_touch(g_window, 1, R_TOUCH_END, x, y + 90.0f);
	CHECK(g_clicks == 1);
}

static void test_pan(void) {
	float x, y, before, after, flung;
	int i;
	center(g_scroll, &x, &y);
	before = scroll_y();
	g_lefts_down = 0;
	Rwindow_inject_touch(g_window, 1, R_TOUCH_BEGIN, x, y + 60.0f);
	/* the content follows the finger: dragging up scrolls down */
	for (i = 1; i <= 6; i++) {
		pause_ms(10);
		Rwindow_inject_touch(g_window, 1, R_TOUCH_MOVE, x, y + 60.0f - 10.0f * (float)i);
	}
	after = scroll_y();
	CHECK(after > before + 20.0f);
	CHECK(g_lefts_down == 0);
	Rwindow_inject_touch(g_window, 1, R_TOUCH_END, x, y);
	/* inertia: the frames keep scrolling after the finger left, and it stops */
	for (i = 0; i < 20; i++) {
		pause_ms(20);
		Rdraw_frame();
	}
	flung = scroll_y();
	CHECK(flung > after + 5.0f);
	for (i = 0; i < 400 && Rwait_frame(0.0) && i < 200; i++) pause_ms(10);
	after = scroll_y();
	pause_ms(100);
	Rdraw_frame();
	CHECK_NEAR(scroll_y(), after);

	/* a finger on a moving list stops it */
	Rscroll_box_set_offset(g_scroll, 0.0f, 0.0f);
	Rwindow_inject_touch(g_window, 1, R_TOUCH_BEGIN, x, y + 80.0f);
	for (i = 1; i <= 6; i++) {
		pause_ms(5);
		Rwindow_inject_touch(g_window, 1, R_TOUCH_MOVE, x, y + 80.0f - 20.0f * (float)i);
	}
	Rwindow_inject_touch(g_window, 1, R_TOUCH_END, x, y - 40.0f);
	pause_ms(40);
	Rdraw_frame();
	Rwindow_inject_touch(g_window, 2, R_TOUCH_BEGIN, x, y);
	flung = scroll_y();
	pause_ms(100);
	Rdraw_frame();
	CHECK_NEAR(scroll_y(), flung);
	Rwindow_inject_touch(g_window, 2, R_TOUCH_END, x, y);
}

static void test_drag_slider(void) {
	float x, y, w, h, value;
	Rwidget_get_rect(g_slider, &x, &y, &w, &h);
	y += h * 0.5f;
	Rwindow_inject_touch(g_window, 1, R_TOUCH_BEGIN, x + 5.0f, y);
	Rwindow_inject_touch(g_window, 1, R_TOUCH_MOVE, x + w * 0.5f, y);
	Rwindow_inject_touch(g_window, 1, R_TOUCH_END, x + w * 0.5f, y);
	value = Rslider_get_value(g_slider);
	CHECK(value > 30.0f && value < 70.0f);
}

static void test_long_press(void) {
	float x, y;
	center(g_button, &x, &y);
	g_rights = g_clicks = 0;
	Rwindow_inject_touch(g_window, 1, R_TOUCH_BEGIN, x, y);
	pause_ms(650);
	Rdraw_frame();
	CHECK(g_rights == 1);
	Rwindow_inject_touch(g_window, 1, R_TOUCH_END, x, y);
	CHECK(g_clicks == 0);
}

static void test_pinch(void) {
	float x, y;
	center(g_scroll, &x, &y);
	g_ctrl_wheels = 0;
	Rwindow_inject_touch(g_window, 1, R_TOUCH_BEGIN, x - 20.0f, y);
	Rwindow_inject_touch(g_window, 2, R_TOUCH_BEGIN, x + 20.0f, y);
	Rwindow_inject_touch(g_window, 2, R_TOUCH_MOVE, x + 60.0f, y);
	CHECK(g_ctrl_wheels >= 1);
	Rwindow_inject_touch(g_window, 2, R_TOUCH_END, x + 60.0f, y);
	/* the finger left is spent: lifting it taps nothing */
	g_clicks = 0;
	Rwindow_inject_touch(g_window, 1, R_TOUCH_END, x - 20.0f, y);
	CHECK(g_clicks == 0);
	/* and the next touch works again */
	center(g_button, &x, &y);
	Rwindow_inject_touch(g_window, 1, R_TOUCH_BEGIN, x, y);
	Rwindow_inject_touch(g_window, 1, R_TOUCH_END, x, y);
	CHECK(g_clicks == 1);
}

static void test_cancel(void) {
	float x, y;
	center(g_slider, &x, &y);
	Rwindow_inject_touch(g_window, 1, R_TOUCH_BEGIN, x, y);
	Rwindow_inject_touch(g_window, 1, R_TOUCH_MOVE, x + 40.0f, y);
	Rwindow_inject_touch(g_window, 1, R_TOUCH_CANCEL, x + 40.0f, y);
	/* a new touch is not stuck behind the cancelled one */
	center(g_button, &x, &y);
	g_clicks = 0;
	Rwindow_inject_touch(g_window, 3, R_TOUCH_BEGIN, x, y);
	Rwindow_inject_touch(g_window, 3, R_TOUCH_END, x, y);
	CHECK(g_clicks == 1);
}

int main(void) {
	Rerror_set_callback(test_error_callback);
	CHECK(test_init(1));
	build();
	test_tap();
	test_pan();
	test_drag_slider();
	test_long_press();
	test_pinch();
	test_cancel();
	Rwindow_destroy(g_window);
	Rdraw_frame();
	Rshutdown();
	return test_report("test_touch");
}
