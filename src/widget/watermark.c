/**
 * Raven - widget/watermark.c
 * The watermark of the free edition.
 *
 * Raven is built in two editions, chosen when the library is compiled:
 *   - the free edition (the default, RAVEN_PAID=OFF) shows a small note in
 *     the bottom right corner of every window;
 *   - the paid edition (-DRAVEN_PAID=ON) shows nothing.
 * Nothing else differs, and what you make with either one is yours.
 *
 * The note is painted over everything (popups and tooltips included) and
 * never takes the mouse: what is under it still gets the clicks.
 */

#include "widget.h"
#include "../widgets/common.h"
#include "../render/font.h"

#include <string.h>

#define RVN_WATERMARK_URL "oscar-soirey.github.io/Raven"

static const char* const g_rvn_watermark_full[2] = {
	"Made with Raven \xC2\xB7 free edition",
	"The paid edition has no watermark \xC2\xB7 " RVN_WATERMARK_URL
};
static const char* const g_rvn_watermark_short[2] = {
	"Raven free edition",
	RVN_WATERMARK_URL
};

typedef struct rvn_watermark_layout_t {
	rvn_rect_t rect;
	const char* const* lines;
	float font_size;
	float line_height;
	float pad_x, pad_y;
} rvn_watermark_layout_t;

int rvn_watermark_enabled(void) {
#if defined(RAVEN_PAID)
	return R_FALSE;
#else
	return R_TRUE;
#endif
}

/* Where the note goes in a window of that size: R_FALSE when there is none. */
static int rvn_watermark_layout(const rvn_window_t* window, rvn_watermark_layout_t* out) {
	rvn_vec2_t size = rvn_window_size(window);
	rvn_font_t* font = rvn_font_resolve(R_INVALID_ID);
	rvn_font_metrics_t metrics;
	float base, width = 0.0f, margin = 8.0f;
	int i, attempt;

	memset(out, 0, sizeof(*out));
	if (!rvn_watermark_enabled() || !font || size.x < 40.0f || size.y < 24.0f) return R_FALSE;
	base = rvn_clampf(RVN_THEME_METRIC(R_METRIC_FONT_SIZE) * 0.85f, 9.0f, 13.0f);

	/* the long text when it fits, else the short one, made smaller if it must */
	for (attempt = 0; attempt < 2; attempt++) {
		const char* const* lines = attempt == 0 ? g_rvn_watermark_full : g_rvn_watermark_short;
		width = 0.0f;
		for (i = 0; i < 2; i++) width = rvn_maxf(width, rvn_text_width(font, base, lines[i], -1));
		out->lines = lines;
		if (width + 20.0f + margin * 2.0f <= size.x) break;
	}
	out->font_size = base;
	if (width + 20.0f + margin * 2.0f > size.x) {
		float fit = base * (size.x - 20.0f - margin * 2.0f) / rvn_maxf(width, 1.0f);
		out->font_size = rvn_maxf(4.0f, fit);
		width = width * out->font_size / base;
	}
	rvn_font_metrics(font, out->font_size, &metrics);
	out->line_height = metrics.line_height;
	out->pad_x = 10.0f;
	out->pad_y = 5.0f;
	out->rect.w = width + out->pad_x * 2.0f;
	out->rect.h = out->line_height * 2.0f + out->pad_y * 2.0f;
	out->rect.x = rvn_maxf(0.0f, size.x - out->rect.w - margin);
	out->rect.y = rvn_maxf(0.0f, size.y - out->rect.h - margin);
	return R_TRUE;
}

void rvn_window_paint_watermark(rvn_window_t* window, RPainter* painter) {
	rvn_watermark_layout_t layout;
	float opacity = painter->opacity, y;
	int i;
	if (!rvn_watermark_layout(window, &layout)) return;
	painter->opacity = 1.0f;
	rvn_paint_box(painter, layout.rect, rvn_minf(layout.rect.h * 0.5f, 10.0f), rvn_rgba(0.06f, 0.06f, 0.08f, 0.72f),
		1.0f, rvn_rgba(1.0f, 1.0f, 1.0f, 0.22f));
	y = layout.rect.y + layout.pad_y;
	for (i = 0; i < 2; i++) {
		rvn_color_t color = i == 0 ? rvn_rgba(1.0f, 1.0f, 1.0f, 0.95f) : rvn_rgba(0.80f, 0.86f, 1.0f, 0.90f);
		rvn_paint_text(painter, R_INVALID_ID, layout.font_size, layout.rect.x + layout.pad_x, y, layout.lines[i], -1, color);
		y += layout.line_height;
	}
	painter->opacity = opacity;
}

/* ---------------------------------------------------------------------------
 *  Public API
 * ------------------------------------------------------------------------- */

int Ris_free_edition(void) {
	return rvn_watermark_enabled();
}

int Rwindow_get_watermark_rect(Rid id, float* x, float* y, float* width, float* height) {
	rvn_watermark_layout_t layout;
	rvn_window_t* window;
	if (x) *x = 0.0f;
	if (y) *y = 0.0f;
	if (width) *width = 0.0f;
	if (height) *height = 0.0f;
	window = rvn_window_check(id, __func__);
	if (!window || !rvn_watermark_layout(window, &layout)) return R_FALSE;
	if (x) *x = layout.rect.x;
	if (y) *y = layout.rect.y;
	if (width) *width = layout.rect.w;
	if (height) *height = layout.rect.h;
	return R_TRUE;
}
