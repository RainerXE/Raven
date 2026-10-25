/**
 * Raven - widgets/progress_bar.c
 * ProgressBar (Slate SProgressBar) and Separator (SSeparator): two widgets
 * that only draw.
 */

#include "common.h"

/* ===========================================================================
 *  ProgressBar
 * ========================================================================= */

typedef struct rvn_progress_bar_t {
	rvn_widget_t base;
	float value;
	/** The value as drawn: it glides to the real one. */
	RSpring shown;
} rvn_progress_bar_t;

static rvn_vec2_t rvn_progress_bar_measure(rvn_widget_t* widget) {
	RVN_UNUSED(widget);
	return rvn_vec2(120.0f, 8.0f);
}

static void rvn_progress_bar_paint(rvn_widget_t* widget, RPainter* painter) {
	rvn_progress_bar_t* bar = (rvn_progress_bar_t*)widget;
	rvn_rect_t rect = rvn_rect(0.0f, 0.0f, widget->rect.w, widget->rect.h);
	float radius = rvn_minf(widget->rect.h * 0.5f, rvn_widget_metric(widget, R_METRIC_CORNER_RADIUS));
	float shown = rvn_paint_spring(painter, &bar->shown, rvn_clampf(bar->value, 0.0f, 1.0f), R_MOTION_SMOOTH);
	float filled = floorf(widget->rect.w * rvn_clampf(shown, 0.0f, 1.0f) + 0.5f);
	rvn_color_t fill = rvn_widget_color(widget, R_COLOR_ACCENT);

	if (!painter->enabled) fill = rvn_color_fade(fill, 0.4f);
	rvn_paint_box(painter, rect, radius, rvn_widget_color(widget, R_COLOR_TRACK), 0.0f, 0);
	if (filled > 0.0f) {
		/* clipping the full-size shape keeps the rounded ends at any progress */
		rvn_paint_push_clip(painter, rvn_rect(0.0f, 0.0f, filled, widget->rect.h));
		rvn_paint_box(painter, rect, radius, fill, 0.0f, 0);
		rvn_paint_pop_clip(painter);
	}
}

static int rvn_progress_bar_set_attribute(rvn_widget_t* widget, const char* name, const char* value) {
	if (RVN_ATTR("Value")) {
		if (!rvn_parse_float(value, &((rvn_progress_bar_t*)widget)->value)) return R_FALSE;
		rvn_widget_invalidate_paint(widget);
		return R_TRUE;
	}
	return R_FALSE;
}

const rvn_widget_class_t rvn_class_progress_bar = {
	.type = R_WIDGET_PROGRESSBAR,
	.name = "ProgressBar",
	.size = sizeof(rvn_progress_bar_t),
	.flags = RVN_CF_NO_CHILDREN | RVN_CF_NO_HIT | RVN_CF_SURFACE,
	.measure = rvn_progress_bar_measure,
	.paint = rvn_progress_bar_paint,
	.set_attribute = rvn_progress_bar_set_attribute
};

void Rprogress_bar_set_value(Rid id, float value) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_PROGRESSBAR);
	value = rvn_clampf(value, 0.0f, 1.0f);
	if (((rvn_progress_bar_t*)widget)->value == value) return;
	((rvn_progress_bar_t*)widget)->value = value;
	rvn_widget_invalidate_paint(widget);
}

float Rprogress_bar_get_value(Rid id) {
	RVN_WIDGET(widget, id, R_WIDGET_PROGRESSBAR, 0.0f);
	return ((rvn_progress_bar_t*)widget)->value;
}

/* ===========================================================================
 *  Separator
 * ========================================================================= */

typedef struct rvn_separator_t {
	rvn_widget_t base;
	ROrientation orientation;
} rvn_separator_t;

static void rvn_separator_init(rvn_widget_t* widget) {
	((rvn_separator_t*)widget)->orientation = R_HORIZONTAL;
}

static rvn_vec2_t rvn_separator_measure(rvn_widget_t* widget) {
	RVN_UNUSED(widget);
	/* one unit thick in both cases: it stretches along the other axis by its fill alignment */
	return rvn_vec2(1.0f, 1.0f);
}

static void rvn_separator_paint(rvn_widget_t* widget, RPainter* painter) {
	rvn_separator_t* separator = (rvn_separator_t*)widget;
	rvn_color_t color = rvn_widget_color(widget, R_COLOR_SEPARATOR);
	if (separator->orientation == R_HORIZONTAL) {
		rvn_paint_rect(painter, rvn_rect(0.0f, floorf((widget->rect.h - 1.0f) * 0.5f), widget->rect.w, 1.0f), color);
	} else {
		rvn_paint_rect(painter, rvn_rect(floorf((widget->rect.w - 1.0f) * 0.5f), 0.0f, 1.0f, widget->rect.h), color);
	}
}

static int rvn_separator_set_attribute(rvn_widget_t* widget, const char* name, const char* value) {
	if (RVN_ATTR("Orientation")) {
		if (!rvn_parse_orientation(value, &((rvn_separator_t*)widget)->orientation)) return R_FALSE;
		rvn_widget_invalidate_paint(widget);
		return R_TRUE;
	}
	return R_FALSE;
}

const rvn_widget_class_t rvn_class_separator = {
	.type = R_WIDGET_SEPARATOR,
	.name = "Separator",
	.size = sizeof(rvn_separator_t),
	.flags = RVN_CF_NO_CHILDREN | RVN_CF_NO_HIT | RVN_CF_SURFACE,
	.init = rvn_separator_init,
	.measure = rvn_separator_measure,
	.paint = rvn_separator_paint,
	.set_attribute = rvn_separator_set_attribute
};

void Rseparator_set_orientation(Rid id, ROrientation orientation) {
	RVN_WIDGET_VOID(widget, id, R_WIDGET_SEPARATOR);
	if (orientation != R_HORIZONTAL && orientation != R_VERTICAL) {
		rvn_error(R_ERROR_INVALID_ENUM, R_SEVERITY_ERROR, "Rseparator_set_orientation: invalid orientation");
		return;
	}
	((rvn_separator_t*)widget)->orientation = orientation;
	rvn_widget_invalidate_paint(widget);
}
