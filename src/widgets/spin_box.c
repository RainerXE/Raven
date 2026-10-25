/**
 * Raven - widgets/spin_box.c
 * SpinBox (Slate SSpinBox): numeric field. Drag it sideways to scrub the
 * value, click it to type, use the arrow keys or the wheel to step.
 */

#include "common.h"

#define RVN_SPIN_DRAG_THRESHOLD 3.0f
#define RVN_SPIN_PIXELS_PER_STEP 4.0f

typedef struct rvn_spin_box_t {
	rvn_widget_t base;
	rvn_textedit_t edit;
	double value;
	double minimum;
	double maximum;
	double step;
	int decimals;
	int editing;
	int dragging;
	float press_x;
	double press_value;
	RSpinBoxChangedCallback callback;
	void* user_data;
} rvn_spin_box_t;

static void rvn_spin_box_init(rvn_widget_t* widget) {
	rvn_spin_box_t* spin = (rvn_spin_box_t*)widget;
	rvn_textedit_init(&spin->edit);
	spin->minimum = -1e30;
	spin->maximum = 1e30;
	spin->step = 1.0;
	spin->decimals = 2;
	widget->cursor = R_CURSOR_RESIZE_H;
}

static void rvn_spin_box_destroy(rvn_widget_t* widget) {
	rvn_textedit_free(&((rvn_spin_box_t*)widget)->edit);
}

static void rvn_spin_box_format(const rvn_spin_box_t* spin, char* buffer, size_t size) {
	snprintf(buffer, size, "%.*f", spin->decimals, spin->value);
}

static void rvn_spin_box_set(rvn_widget_t* widget, double value, int notify) {
	rvn_spin_box_t* spin = (rvn_spin_box_t*)widget;
	double low = spin->minimum < spin->maximum ? spin->minimum : spin->maximum;
	double high = spin->minimum < spin->maximum ? spin->maximum : spin->minimum;
	if (value != value) return; /* NaN */
	if (value < low) value = low;
	if (value > high) value = high;
	if (value == spin->value) return;
	spin->value = value;
	rvn_widget_invalidate_paint(widget);
	if (notify) {
		if (spin->callback) spin->callback(widget->id, value, spin->user_data);
		if (rvn_widget_alive(widget)) rvn_widget_fire_action(widget);
	}
}

static rvn_rect_t rvn_spin_box_area(const rvn_widget_t* widget) {
	float px = rvn_widget_metric(widget, R_METRIC_PADDING_X) * 0.7f;
	return rvn_rect(px, 0.0f, rvn_maxf(0.0f, widget->rect.w - px * 2.0f), widget->rect.h);
}

static void rvn_spin_box_begin_edit(rvn_widget_t* widget, int select_all) {
	rvn_spin_box_t* spin = (rvn_spin_box_t*)widget;
	char buffer[64];
	if (spin->editing) return;
	rvn_spin_box_format(spin, buffer, sizeof(buffer));
	rvn_textedit_set_text(&spin->edit, buffer);
	if (select_all) rvn_textedit_select_all(&spin->edit);
	else spin->edit.caret = spin->edit.anchor = 0;
	rvn_textedit_set_focused(&spin->edit, widget, R_TRUE);
	spin->editing = R_TRUE;
	widget->cursor = R_CURSOR_IBEAM;
}

static void rvn_spin_box_end_edit(rvn_widget_t* widget, int apply) {
	rvn_spin_box_t* spin = (rvn_spin_box_t*)widget;
	if (!spin->editing) return;
	spin->editing = R_FALSE;
	widget->cursor = R_CURSOR_RESIZE_H;
	rvn_textedit_set_focused(&spin->edit, widget, R_FALSE);
	if (apply) {
		char* end = NULL;
		double value;
		/* accept a decimal comma as well */
		char* text = rvn_strdup(spin->edit.text);
		char* comma = strchr(text, ',');
		if (comma) *comma = '.';
		value = strtod(text, &end);
		if (end != text) rvn_spin_box_set(widget, value, R_TRUE);
		rvn_free(text);
	}
	if (rvn_widget_alive(widget)) rvn_widget_invalidate_paint(widget);
}

static rvn_vec2_t rvn_spin_box_measure(rvn_widget_t* widget) {
	return rvn_vec2(80.0f, rvn_widget_metric(widget, R_METRIC_CONTROL_HEIGHT));
}

static void rvn_spin_box_paint(rvn_widget_t* widget, RPainter* painter) {
	rvn_spin_box_t* spin = (rvn_spin_box_t*)widget;
	rvn_rect_t rect = rvn_rect(0.0f, 0.0f, widget->rect.w, widget->rect.h);
	int focused = (widget->flags & RVN_WF_FOCUSED) != 0;
	rvn_color_t fill = rvn_widget_color(widget, R_COLOR_INPUT);
	rvn_color_t text = rvn_widget_text_color(widget, painter);
	char buffer[64];

	if (!painter->enabled) fill = rvn_color_fade(fill, 0.5f);
	if (rvn_widget_glass(widget)) {
		float radius = rvn_control_radius(widget, rect.h);
		rvn_paint_surface(painter, widget, rect, radius, RVN_SURFACE_INPUT, 0.0f, 0.0f);
		if (focused) rvn_paint_box(painter, rect, radius, 0, 1.5f, rvn_widget_color(widget, R_COLOR_FOCUS));
	} else {
		rvn_paint_box(painter, rect, rvn_widget_metric(widget, R_METRIC_CORNER_RADIUS), fill,
			rvn_widget_metric(widget, R_METRIC_BORDER_WIDTH), rvn_widget_color(widget, focused ? R_COLOR_FOCUS : R_COLOR_BORDER));
	}
	if (spin->editing) {
		rvn_textedit_paint(&spin->edit, widget, painter, rvn_spin_box_area(widget), R_INVALID_ID, 0.0f, text, R_TRUE);
		return;
	}
	rvn_spin_box_format(spin, buffer, sizeof(buffer));
	rvn_paint_push_clip(painter, rect);
	rvn_paint_text_in(painter, R_INVALID_ID, rvn_widget_font_size(widget, 0.0f), rect, 0.5f, buffer, -1, text);
	rvn_paint_pop_clip(painter);
}

static int rvn_spin_box_event(rvn_widget_t* widget, const REvent* event) {
	rvn_spin_box_t* spin = (rvn_spin_box_t*)widget;
	rvn_rect_t area = rvn_spin_box_area(widget);
	int flags;

	switch (event->type) {
		case R_EVENT_FOCUS_GAINED:
			/* reaching the field with Tab starts typing right away */
			if (widget->window && widget->window->focus_visible) rvn_spin_box_begin_edit(widget, R_TRUE);
			return R_TRUE;
		case R_EVENT_FOCUS_LOST:
			rvn_spin_box_end_edit(widget, R_TRUE);
			return R_TRUE;
		case R_EVENT_MOUSE_DOWN:
			if (event->button != R_MOUSE_LEFT) return R_FALSE;
			rvn_widget_capture_mouse(widget);
			if (spin->editing) {
				rvn_textedit_mouse_down(&spin->edit, widget, R_INVALID_ID, 0.0f, event->local_x - area.x,
					event->clicks, (event->modifiers & R_MOD_SHIFT) != 0);
			} else {
				widget->flags |= RVN_WF_PRESSED;
				spin->dragging = R_FALSE;
				spin->press_x = event->x;
				spin->press_value = spin->value;
			}
			rvn_widget_invalidate_paint(widget);
			return R_TRUE;
		case R_EVENT_MOUSE_MOVE:
			if (spin->editing) {
				if (!spin->edit.mouse_selecting) return R_FALSE;
				rvn_textedit_mouse_drag(&spin->edit, widget, R_INVALID_ID, 0.0f, event->local_x - area.x);
				rvn_widget_invalidate_paint(widget);
				return R_TRUE;
			}
			if (!(widget->flags & RVN_WF_PRESSED)) return R_FALSE;
			if (!spin->dragging && fabsf(event->x - spin->press_x) > RVN_SPIN_DRAG_THRESHOLD) spin->dragging = R_TRUE;
			if (spin->dragging) {
				/* Shift scrubs ten times finer */
				double scale = (event->modifiers & R_MOD_SHIFT) ? 0.1 : 1.0;
				double steps = floor((double)(event->x - spin->press_x) / RVN_SPIN_PIXELS_PER_STEP);
				rvn_spin_box_set(widget, spin->press_value + steps * spin->step * scale, R_TRUE);
			}
			return R_TRUE;
		case R_EVENT_MOUSE_UP:
			if (event->button != R_MOUSE_LEFT) return R_FALSE;
			rvn_widget_release_mouse(widget);
			if (spin->editing) {
				spin->edit.mouse_selecting = R_FALSE;
				return R_TRUE;
			}
			if (!(widget->flags & RVN_WF_PRESSED)) return R_FALSE;
			widget->flags &= ~RVN_WF_PRESSED;
			/* a click without a drag switches to typing */
			if (!spin->dragging) rvn_spin_box_begin_edit(widget, R_TRUE);
			spin->dragging = R_FALSE;
			rvn_widget_invalidate_paint(widget);
			return R_TRUE;
		case R_EVENT_MOUSE_WHEEL:
			/* only a focused field steps with the wheel: it must not hijack the scrolling of a panel */
			if (!(widget->flags & RVN_WF_FOCUSED) || spin->editing || event->wheel_y == 0.0f) return R_FALSE;
			rvn_spin_box_set(widget, spin->value + (event->wheel_y > 0.0f ? spin->step : -spin->step), R_TRUE);
			return R_TRUE;
		case R_EVENT_KEY_DOWN:
			if (spin->editing) {
				if (event->key == R_KEY_ESCAPE) {
					rvn_spin_box_end_edit(widget, R_FALSE);
					return R_TRUE;
				}
				flags = rvn_textedit_key(&spin->edit, event);
				if (flags & RVN_TEXTEDIT_COMMITTED) rvn_spin_box_end_edit(widget, R_TRUE);
				else if (flags) rvn_widget_invalidate_paint(widget);
				return flags != 0;
			}
			switch (event->key) {
				case R_KEY_UP:
				case R_KEY_RIGHT:
					rvn_spin_box_set(widget, spin->value + spin->step, R_TRUE);
					return R_TRUE;
				case R_KEY_DOWN:
				case R_KEY_LEFT:
					rvn_spin_box_set(widget, spin->value - spin->step, R_TRUE);
					return R_TRUE;
				case R_KEY_ENTER:
				case R_KEY_KP_ENTER:
					rvn_spin_box_begin_edit(widget, R_TRUE);
					return R_TRUE;
				default:
					return R_FALSE;
			}
		case R_EVENT_TEXT:
			/* typing a number starts an edit that replaces the value */
			if (!spin->editing) {
				uint32_t c = event->codepoint;
				if (!((c >= '0' && c <= '9') || c == '-' || c == '.' || c == ',')) return R_FALSE;
				rvn_spin_box_begin_edit(widget, R_TRUE);
			}
			if (rvn_textedit_input(&spin->edit, event->codepoint)) rvn_widget_invalidate_paint(widget);
			return R_TRUE;
		default:
			return R_FALSE;
	}
}

static int rvn_spin_box_set_attribute(rvn_widget_t* widget, const char* name, const char* value) {
	rvn_spin_box_t* spin = (rvn_spin_box_t*)widget;
	float number;
	if (!rvn_parse_float(value, &number)) return R_FALSE;
	if (RVN_ATTR("Min")) spin->minimum = number;
	else if (RVN_ATTR("Max")) spin->maximum = number;
	else if (RVN_ATTR("Step")) spin->step = number;
	else if (RVN_ATTR("Decimals")) spin->decimals = rvn_clampi((int)number, 0, 9);
	else if (RVN_ATTR("Value")) {
		rvn_spin_box_set(widget, number, R_FALSE);
		return R_TRUE;
	} else {
		return R_FALSE;
	}
	rvn_spin_box_set(widget, spin->value, R_FALSE);
	rvn_widget_invalidate_paint(widget);
	return R_TRUE;
}

const rvn_widget_class_t rvn_class_spin_box = {
	.type = R_WIDGET_SPIN_BOX,
	.name = "SpinBox",
	.size = sizeof(rvn_spin_box_t),
	.flags = RVN_CF_NO_CHILDREN | RVN_CF_FOCUSABLE | RVN_CF_SURFACE,
	.init = rvn_spin_box_init,
	.destroy = rvn_spin_box_destroy,
	.measure = rvn_spin_box_measure,
	.paint = rvn_spin_box_paint,
	.event = rvn_spin_box_event,
	.set_attribute = rvn_spin_box_set_attribute
};

void Rspin_box_set_range(Rid id, double minimum, double maximum) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_SPIN_BOX);
	rvn_spin_box_t* spin = (rvn_spin_box_t*)widget;
	spin->minimum = minimum;
	spin->maximum = maximum;
	rvn_spin_box_set(widget, spin->value, R_FALSE);
}

void Rspin_box_set_value(Rid id, double value) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_SPIN_BOX);
	rvn_spin_box_set(widget, value, R_FALSE);
}

double Rspin_box_get_value(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_SPIN_BOX, 0.0);
	return ((rvn_spin_box_t*)widget)->value;
}

void Rspin_box_get_range(Rid id, double* minimum, double* maximum, double* step) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_SPIN_BOX);
	if (minimum) *minimum = ((rvn_spin_box_t*)widget)->minimum;
	if (maximum) *maximum = ((rvn_spin_box_t*)widget)->maximum;
	if (step) *step = ((rvn_spin_box_t*)widget)->step;
}

void Rspin_box_set_step(Rid id, double step) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_SPIN_BOX);
	((rvn_spin_box_t*)widget)->step = step;
}

void Rspin_box_set_decimals(Rid id, int decimals) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_SPIN_BOX);
	((rvn_spin_box_t*)widget)->decimals = rvn_clampi(decimals, 0, 9);
	rvn_widget_invalidate_paint(widget);
}

void Rspin_box_set_changed_callback(Rid id, RSpinBoxChangedCallback callback, void* user_data) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_SPIN_BOX);
	((rvn_spin_box_t*)widget)->callback = callback;
	((rvn_spin_box_t*)widget)->user_data = user_data;
}
