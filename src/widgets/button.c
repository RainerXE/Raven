/**
 * Raven - widgets/button.c
 * Button (Slate SButton): a clickable frame around one child, usually a label.
 */

#include "common.h"

typedef struct rvn_button_t {
	rvn_widget_t base;
	RButtonStyle style;
	/** While pressed: R_TRUE when the cursor is still over the button. */
	int armed;
	RButtonClickedCallback callback;
	void* user_data;
} rvn_button_t;

static void rvn_button_init(rvn_widget_t* widget) {
	((rvn_button_t*)widget)->style = R_BUTTON_DEFAULT;
}

static rvn_margin_t rvn_button_padding(const rvn_widget_t* widget) {
	float px = rvn_widget_metric(widget, R_METRIC_PADDING_X), py = rvn_widget_metric(widget, R_METRIC_PADDING_Y);
	return rvn_margin(px, py, px, py);
}

static rvn_vec2_t rvn_button_measure(rvn_widget_t* widget) {
	rvn_margin_t padding = rvn_button_padding(widget);
	rvn_vec2_t desired = rvn_vec2(0.0f, 0.0f);
	if (widget->child_count > 0) desired = rvn_widget_measure_slot(widget->children[0]);
	desired.x += padding.l + padding.r;
	desired.y = rvn_maxf(desired.y + padding.t + padding.b, rvn_widget_metric(widget, R_METRIC_CONTROL_HEIGHT));
	return desired;
}

static void rvn_button_arrange(rvn_widget_t* widget) {
	if (widget->child_count > 0 && !rvn_widget_collapsed(widget->children[0])) {
		rvn_widget_arrange_slot(widget->children[0],
			rvn_rect_inset(rvn_rect(0.0f, 0.0f, widget->rect.w, widget->rect.h), rvn_button_padding(widget)));
	}
}

static void rvn_button_paint(rvn_widget_t* widget, RPainter* painter) {
	rvn_button_t* button = (rvn_button_t*)widget;
	rvn_rect_t rect = rvn_rect(0.0f, 0.0f, widget->rect.w, widget->rect.h);
	float radius = rvn_widget_metric(widget, R_METRIC_CORNER_RADIUS);
	int pressed = (widget->flags & RVN_WF_PRESSED) && button->armed;
	int hovered = (widget->flags & RVN_WF_HOVERED) && painter->enabled;
	rvn_color_t fill, outline = 0;
	float hover, press;

	/* a button is soft: it flattens under the finger and springs back past its shape */
	hover = rvn_widget_hover(widget, painter, hovered);
	press = rvn_widget_press(widget, painter, pressed);
	rvn_widget_squash(widget, press, rvn_widget_glass(widget) ? 0.13f : 0.09f);

	if (rvn_widget_glass(widget)) {
		/* a capsule of glass that lights up under the pointer */
		float opacity = painter->opacity;
		float lift = rvn_clampf(hover, 0.0f, 1.0f) * 0.6f;
		rect = rvn_rect(-lift, -lift, rect.w + lift * 2.0f, rect.h + lift * 2.0f);
		radius = rvn_control_radius(widget, rect.h);
		if (button->style == R_BUTTON_PRIMARY) {
			rvn_paint_surface(painter, widget, rect, radius, RVN_SURFACE_ACCENT, hover, press);
			if (painter->enabled) painter->text_color = rvn_widget_color(widget, R_COLOR_ACCENT_TEXT);
		} else if (button->style == R_BUTTON_FLAT) {
			/* flat buttons only show their glass while they are used */
			float shown = rvn_clampf(rvn_maxf(hover, press), 0.0f, 1.0f);
			if (shown > 0.01f) {
				painter->opacity *= shown;
				rvn_paint_surface(painter, widget, rect, radius, RVN_SURFACE_CONTROL, hover, press);
				painter->opacity = opacity;
			}
		} else {
			rvn_paint_surface(painter, widget, rect, radius, RVN_SURFACE_CONTROL, hover, press);
		}
		return;
	}

	/* flat themes: the colors of the three states blend with the same springs */
	hover = rvn_clampf(hover, 0.0f, 1.0f);
	press = rvn_clampf(press, 0.0f, 1.0f);
	if (button->style == R_BUTTON_PRIMARY) {
		fill = rvn_color_mix(rvn_color_mix(rvn_widget_color(widget, R_COLOR_ACCENT), rvn_widget_color(widget, R_COLOR_ACCENT_HOVERED), hover),
			rvn_widget_color(widget, R_COLOR_ACCENT_PRESSED), press);
		if (!painter->enabled) fill = rvn_color_fade(fill, 0.4f);
		/* the content of a primary button is drawn in the accent text color */
		if (painter->enabled) painter->text_color = rvn_widget_color(widget, R_COLOR_ACCENT_TEXT);
	} else if (button->style == R_BUTTON_FLAT) {
		fill = rvn_color_mix(rvn_color_fade(rvn_widget_color(widget, R_COLOR_CONTROL_HOVERED), hover),
			rvn_widget_color(widget, R_COLOR_CONTROL_PRESSED), press);
	} else {
		fill = rvn_color_mix(rvn_color_mix(rvn_widget_color(widget, R_COLOR_CONTROL), rvn_widget_color(widget, R_COLOR_CONTROL_HOVERED), hover),
			rvn_widget_color(widget, R_COLOR_CONTROL_PRESSED), press);
		outline = rvn_widget_color(widget, R_COLOR_CONTROL_BORDER);
		if (!painter->enabled) {
			fill = rvn_color_fade(fill, 0.5f);
			outline = rvn_color_fade(outline, 0.5f);
		}
	}
	rvn_paint_box(painter, rect, radius, fill, outline ? rvn_widget_metric(widget, R_METRIC_BORDER_WIDTH) : 0.0f, outline);
}

static void rvn_button_paint_overlay(rvn_widget_t* widget, RPainter* painter) {
	rvn_paint_focus(widget, painter, rvn_control_radius(widget, widget->rect.h));
}

static void rvn_button_click(rvn_widget_t* widget) {
	rvn_button_t* button = (rvn_button_t*)widget;
	/* the release throws the button past its shape: it wobbles once before settling */
	rvn_spring_kick(&widget->press, -11.0f);
	if (button->callback) button->callback(widget->id, button->user_data);
	if (rvn_widget_alive(widget)) rvn_widget_fire_action(widget);
}

static int rvn_button_event(rvn_widget_t* widget, const REvent* event) {
	rvn_button_t* button = (rvn_button_t*)widget;
	int inside = event->local_x >= 0.0f && event->local_y >= 0.0f &&
		event->local_x < widget->rect.w && event->local_y < widget->rect.h;

	switch (event->type) {
		case R_EVENT_MOUSE_DOWN:
			if (event->button != R_MOUSE_LEFT) return R_FALSE;
			widget->flags |= RVN_WF_PRESSED;
			button->armed = R_TRUE;
			rvn_widget_capture_mouse(widget);
			rvn_widget_invalidate_paint(widget);
			return R_TRUE;
		case R_EVENT_MOUSE_MOVE:
			if (!(widget->flags & RVN_WF_PRESSED)) return R_FALSE;
			if (button->armed != inside) {
				button->armed = inside;
				rvn_widget_invalidate_paint(widget);
			}
			return R_TRUE;
		case R_EVENT_MOUSE_UP:
			if (event->button != R_MOUSE_LEFT || !(widget->flags & RVN_WF_PRESSED)) return R_FALSE;
			widget->flags &= ~RVN_WF_PRESSED;
			rvn_widget_release_mouse(widget);
			rvn_widget_invalidate_paint(widget);
			/* a click is a press and a release on the button */
			if (inside) rvn_button_click(widget);
			return R_TRUE;
		case R_EVENT_KEY_DOWN:
			if (!(widget->flags & RVN_WF_FOCUSED) || event->repeat) return R_FALSE;
			if (event->key == R_KEY_ENTER || event->key == R_KEY_KP_ENTER || event->key == R_KEY_SPACE) {
				rvn_button_click(widget);
				return R_TRUE;
			}
			return R_FALSE;
		default:
			return R_FALSE;
	}
}

static void rvn_button_set_text(rvn_widget_t* widget, const char* text) {
	rvn_widget_t* label;
	/* reuse the label when the content already is one */
	if (widget->child_count > 0 && widget->children[0]->cls->type == R_WIDGET_LABEL) {
		rvn_widget_set_attribute(widget->children[0], "Text", text ? text : "");
		return;
	}
	label = rvn_widget_create(R_WIDGET_LABEL);
	if (!label) return;
	rvn_widget_set_attribute(label, "Text", text ? text : "");
	label->slot.halign = R_HALIGN_CENTER;
	label->slot.valign = R_VALIGN_CENTER;
	rvn_widget_add_child(widget, label, -1);
}

static int rvn_button_parse_style(const char* value, RButtonStyle* style) {
	if (rvn_stricmp(value, "Default") == 0) *style = R_BUTTON_DEFAULT;
	else if (rvn_stricmp(value, "Primary") == 0) *style = R_BUTTON_PRIMARY;
	else if (rvn_stricmp(value, "Flat") == 0) *style = R_BUTTON_FLAT;
	else return R_FALSE;
	return R_TRUE;
}

static int rvn_button_set_attribute(rvn_widget_t* widget, const char* name, const char* value) {
	rvn_button_t* button = (rvn_button_t*)widget;
	if (RVN_ATTR("Text")) {
		rvn_button_set_text(widget, value);
		return R_TRUE;
	}
	if (RVN_ATTR("Style")) {
		if (!rvn_button_parse_style(value, &button->style)) return R_FALSE;
		rvn_widget_invalidate_paint(widget);
		return R_TRUE;
	}
	return R_FALSE;
}

const rvn_widget_class_t rvn_class_button = {
	.type = R_WIDGET_BUTTON,
	.name = "Button",
	.size = sizeof(rvn_button_t),
	.flags = RVN_CF_SINGLE_CHILD | RVN_CF_FOCUSABLE | RVN_CF_SURFACE,
	.init = rvn_button_init,
	.measure = rvn_button_measure,
	.arrange = rvn_button_arrange,
	.paint = rvn_button_paint,
	.paint_overlay = rvn_button_paint_overlay,
	.event = rvn_button_event,
	.set_attribute = rvn_button_set_attribute
};

Rid Rbutton_create(const char* text) {
	rvn_widget_t* widget;
	RVN_API_GUARD(R_INVALID_ID);
	widget = rvn_widget_create(R_WIDGET_BUTTON);
	if (!widget) return R_INVALID_ID;
	if (text) rvn_button_set_text(widget, text);
	return widget->id;
}

void Rbutton_set_text(Rid id, const char* text) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_BUTTON);
	rvn_button_set_text(widget, text);
}

void Rbutton_set_style(Rid id, RButtonStyle style) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_BUTTON);
	if (style < R_BUTTON_DEFAULT || style > R_BUTTON_FLAT) {
		rvn_error(R_ERROR_INVALID_ENUM, R_SEVERITY_ERROR, "Rbutton_set_style: invalid style");
		return;
	}
	((rvn_button_t*)widget)->style = style;
	rvn_widget_invalidate_paint(widget);
}

void Rbutton_set_clicked_callback(Rid id, RButtonClickedCallback callback, void* user_data) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_BUTTON);
	((rvn_button_t*)widget)->callback = callback;
	((rvn_button_t*)widget)->user_data = user_data;
}
