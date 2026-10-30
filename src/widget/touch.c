/**
 * Raven - widget/touch.c
 * Fingers become the input the widgets already understand.
 *
 *   tap          mouse move, press, release (double taps count as double clicks)
 *   long press   right button press and release: the context menu
 *   pan          wheel events, followed by inertia; a widget that does not scroll under the finger makes it a drag
 *   drag         mouse move with the left button down: sliders, splitters, selections, drag and drop
 *   two fingers  the middle point pans (wheel), the distance zooms (wheel with Ctrl)
 *
 * Nothing is pressed until the finger lifts or moves further than the slop, so that a pan never presses (and
 * never clicks) what it starts on. A class with RVN_CF_TOUCH_DRAG drags at once, it does not try to scroll.
 */

#include "widget.h"

#include <math.h>

#define RVN_TOUCH_SLOP 8.0f
#define RVN_TOUCH_LONG_PRESS 0.5
/** Units of a pan that make one notch of a mouse wheel. */
#define RVN_TOUCH_NOTCH 40.0f
#define RVN_TOUCH_FLING_MIN 150.0f
#define RVN_TOUCH_FLING_STOP 30.0f
/** The inertia loses 1 - exp(-FRICTION) of its speed per second. */
#define RVN_TOUCH_FRICTION 3.0f
#define RVN_TOUCH_FRAME 0.016

enum {
	RVN_TOUCH_IDLE,
	/** One finger is down and has not decided yet. */
	RVN_TOUCH_PENDING,
	RVN_TOUCH_PANNING,
	RVN_TOUCH_DRAGGING,
	/** The long press already happened. */
	RVN_TOUCH_HELD,
	RVN_TOUCH_PINCH,
	/** Fingers that stay down after a pinch are ignored until all are up. */
	RVN_TOUCH_SPENT,
	RVN_TOUCH_FLING
};

static void rvn_touch_stop_timer(rvn_window_t* window) {
	rvn_touch_t* touch = &window->touch;
	if (touch->timer_active) Rtimer_destroy(touch->timer);
	touch->timer_active = R_FALSE;
}

void rvn_touch_release(rvn_window_t* window) {
	rvn_touch_stop_timer(window);
	window->touch.mode = RVN_TOUCH_IDLE;
}

static void rvn_touch_wheel(rvn_window_t* window, float dx, float dy, uint32_t modifiers_added, int* consumed) {
	uint32_t modifiers = window->modifiers;
	int taken;
	window->modifiers |= modifiers_added;
	taken = rvn_window_mouse_wheel(window, dx / RVN_TOUCH_NOTCH, dy / RVN_TOUCH_NOTCH);
	if (!window->dead) window->modifiers = modifiers;
	if (consumed) *consumed = taken;
}

static int rvn_touch_wants_drag(rvn_window_t* window, float x, float y) {
	rvn_widget_t* hit = rvn_window_hit(window, x, y, NULL);
	return hit && (hit->cls->flags & RVN_CF_TOUCH_DRAG);
}

static void rvn_touch_begin_drag(rvn_window_t* window) {
	rvn_touch_t* touch = &window->touch;
	touch->mode = RVN_TOUCH_DRAGGING;
	rvn_window_mouse_move(window, touch->x0, touch->y0);
	if (window->dead) return;
	rvn_window_mouse_button(window, R_MOUSE_LEFT, R_TRUE);
	if (!window->dead) rvn_window_mouse_move(window, touch->x, touch->y);
}

/* The long press: the right button. */
static void rvn_touch_long_press(Rid timer, void* user_data) {
	rvn_window_t* window = (rvn_window_t*)user_data;
	rvn_touch_t* touch = &window->touch;
	RVN_UNUSED(timer);
	touch->timer_active = R_FALSE;
	if (touch->mode != RVN_TOUCH_PENDING) return;
	touch->mode = RVN_TOUCH_HELD;
	rvn_dispatch_begin();
	rvn_window_mouse_move(window, touch->x0, touch->y0);
	if (!window->dead) rvn_window_mouse_button(window, R_MOUSE_RIGHT, R_TRUE);
	if (!window->dead) rvn_window_mouse_button(window, R_MOUSE_RIGHT, R_FALSE);
	rvn_dispatch_end();
}

/* Inertia: the speed of the pan decays until it is not worth a frame. */
static void rvn_touch_fling_step(Rid timer, void* user_data) {
	rvn_window_t* window = (rvn_window_t*)user_data;
	rvn_touch_t* touch = &window->touch;
	float decay = expf(-RVN_TOUCH_FRICTION * (float)RVN_TOUCH_FRAME);
	int consumed = R_FALSE;
	RVN_UNUSED(timer);
	if (touch->mode != RVN_TOUCH_FLING) return;
	rvn_dispatch_begin();
	rvn_touch_wheel(window, touch->vx * (float)RVN_TOUCH_FRAME, touch->vy * (float)RVN_TOUCH_FRAME, 0, &consumed);
	touch->vx *= decay;
	touch->vy *= decay;
	if (!window->dead && (!consumed || (fabsf(touch->vx) < RVN_TOUCH_FLING_STOP && fabsf(touch->vy) < RVN_TOUCH_FLING_STOP))) {
		rvn_touch_stop_timer(window);
		touch->mode = RVN_TOUCH_IDLE;
		rvn_window_mouse_leave(window);
	}
	rvn_dispatch_end();
}

static void rvn_touch_start_fling(rvn_window_t* window) {
	rvn_touch_t* touch = &window->touch;
	if (hypotf(touch->vx, touch->vy) < RVN_TOUCH_FLING_MIN) {
		touch->mode = RVN_TOUCH_IDLE;
		rvn_window_mouse_leave(window);
		return;
	}
	touch->mode = RVN_TOUCH_FLING;
	touch->timer = Rtimer_create(RVN_TOUCH_FRAME, R_TRUE, rvn_touch_fling_step, window);
	touch->timer_active = touch->timer != R_INVALID_ID;
	if (!touch->timer_active) touch->mode = RVN_TOUCH_IDLE;
}

static void rvn_touch_track_speed(rvn_touch_t* touch, float dx, float dy) {
	double now = rvn_clock_seconds();
	double dt = now - touch->last_time;
	touch->last_time = now;
	if (dt < 0.001) dt = 0.001;
	/* a short average: the last movement counts most, a pause before the release gives no inertia */
	if (dt > 0.1) {
		touch->vx = touch->vy = 0.0f;
	} else {
		touch->vx = touch->vx * 0.4f + (dx / (float)dt) * 0.6f;
		touch->vy = touch->vy * 0.4f + (dy / (float)dt) * 0.6f;
	}
}

static float rvn_touch_distance(const rvn_touch_t* touch) {
	return hypotf(touch->x2 - touch->x, touch->y2 - touch->y);
}

void rvn_window_touch(rvn_window_t* window, uint32_t id, int phase, float x, float y) {
	rvn_touch_t* touch = &window->touch;
	float dx, dy;

	if (phase == R_TOUCH_BEGIN) {
		if (touch->mode == RVN_TOUCH_FLING) {
			/* a finger stops the inertia, and is the start of something new */
			rvn_touch_stop_timer(window);
			touch->mode = RVN_TOUCH_IDLE;
		}
		if (touch->mode == RVN_TOUCH_IDLE) {
			memset(touch, 0, sizeof(*touch));
			touch->mode = RVN_TOUCH_PENDING;
			touch->id = id;
			touch->x0 = touch->x = x;
			touch->y0 = touch->y = y;
			touch->last_time = rvn_clock_seconds();
			rvn_window_mouse_move(window, x, y);
			if (window->dead) return;
			touch->timer = Rtimer_create(RVN_TOUCH_LONG_PRESS, R_FALSE, rvn_touch_long_press, window);
			touch->timer_active = touch->timer != R_INVALID_ID;
		} else if (touch->mode == RVN_TOUCH_PENDING || touch->mode == RVN_TOUCH_PANNING) {
			rvn_touch_stop_timer(window);
			touch->mode = RVN_TOUCH_PINCH;
			touch->id2 = id;
			touch->x2 = x;
			touch->y2 = y;
			touch->pinch_distance = rvn_touch_distance(touch);
			rvn_window_mouse_move(window, (touch->x + x) * 0.5f, (touch->y + y) * 0.5f);
		}
		return;
	}

	if (touch->mode == RVN_TOUCH_IDLE || touch->mode == RVN_TOUCH_FLING) return;
	if (id != touch->id && !(touch->mode == RVN_TOUCH_PINCH && id == touch->id2)) return;

	if (phase == R_TOUCH_MOVE) {
		switch (touch->mode) {
			case RVN_TOUCH_PENDING:
				touch->x = x;
				touch->y = y;
				if (hypotf(x - touch->x0, y - touch->y0) < RVN_TOUCH_SLOP) break;
				rvn_touch_stop_timer(window);
				if (rvn_touch_wants_drag(window, touch->x0, touch->y0)) {
					rvn_touch_begin_drag(window);
				} else {
					int consumed = R_FALSE;
					rvn_touch_wheel(window, x - touch->x0, y - touch->y0, 0, &consumed);
					if (window->dead) break;
					if (consumed) {
						touch->mode = RVN_TOUCH_PANNING;
						rvn_touch_track_speed(touch, x - touch->x0, y - touch->y0);
					} else {
						/* nothing under the finger scrolls: it drags (a selection, an item to drop) */
						rvn_touch_begin_drag(window);
					}
				}
				break;
			case RVN_TOUCH_PANNING:
				dx = x - touch->x;
				dy = y - touch->y;
				touch->x = x;
				touch->y = y;
				rvn_touch_track_speed(touch, dx, dy);
				rvn_touch_wheel(window, dx, dy, 0, NULL);
				break;
			case RVN_TOUCH_DRAGGING:
				touch->x = x;
				touch->y = y;
				rvn_window_mouse_move(window, x, y);
				break;
			case RVN_TOUCH_PINCH: {
				float before_x = (touch->x + touch->x2) * 0.5f, before_y = (touch->y + touch->y2) * 0.5f;
				float after_x, after_y, distance;
				if (id == touch->id) {
					touch->x = x;
					touch->y = y;
				} else {
					touch->x2 = x;
					touch->y2 = y;
				}
				after_x = (touch->x + touch->x2) * 0.5f;
				after_y = (touch->y + touch->y2) * 0.5f;
				distance = rvn_touch_distance(touch);
				rvn_window_mouse_move(window, after_x, after_y);
				if (window->dead) break;
				/* the zoom first, then the pan, each as the wheel of a mouse */
				if (fabsf(distance - touch->pinch_distance) > 0.5f) {
					rvn_touch_wheel(window, 0.0f, distance - touch->pinch_distance, R_MOD_CTRL, NULL);
					touch->pinch_distance = distance;
				}
				if (!window->dead && (after_x != before_x || after_y != before_y)) {
					rvn_touch_wheel(window, after_x - before_x, after_y - before_y, 0, NULL);
				}
				break;
			}
			default:
				break;
		}
		return;
	}

	/* the finger lifted, or the system took it away */
	rvn_touch_stop_timer(window);
	switch (touch->mode) {
		case RVN_TOUCH_PENDING:
			if (phase == R_TOUCH_END) {
				rvn_window_mouse_move(window, x, y);
				if (!window->dead) rvn_window_mouse_button(window, R_MOUSE_LEFT, R_TRUE);
				if (!window->dead) rvn_window_mouse_button(window, R_MOUSE_LEFT, R_FALSE);
			}
			touch->mode = RVN_TOUCH_IDLE;
			if (!window->dead) rvn_window_mouse_leave(window);
			break;
		case RVN_TOUCH_DRAGGING:
			if (phase == R_TOUCH_END) rvn_window_mouse_move(window, x, y);
			if (!window->dead) rvn_window_mouse_button(window, R_MOUSE_LEFT, R_FALSE);
			touch->mode = RVN_TOUCH_IDLE;
			if (!window->dead) rvn_window_mouse_leave(window);
			break;
		case RVN_TOUCH_PANNING:
			if (phase == R_TOUCH_END) rvn_touch_start_fling(window);
			else {
				touch->mode = RVN_TOUCH_IDLE;
				rvn_window_mouse_leave(window);
			}
			break;
		case RVN_TOUCH_PINCH:
			/* the finger that stays is spent: it does not tap or pan after a pinch */
			touch->mode = RVN_TOUCH_SPENT;
			touch->id = id == touch->id ? touch->id2 : touch->id;
			break;
		case RVN_TOUCH_SPENT:
		case RVN_TOUCH_HELD:
		default:
			touch->mode = RVN_TOUCH_IDLE;
			break;
	}
}
