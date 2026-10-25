/**
 * Raven - widget/style.c
 * The built-in themes, and how a widget finds its colors and metrics: its
 * own overrides, then the styles of its theme, then the theme. Named themes,
 * styles and theme files are in theme.c.
 */

#include "widget.h"
#include "../render/renderer.h"

#include <math.h>

#define RVN_COLOR_INDEX(role) ((int)(role) - (int)R_COLOR_WINDOW)
#define RVN_METRIC_INDEX(metric) ((int)(metric) - (int)R_METRIC_FONT_SIZE)

static int rvn_color_valid(RColorRole role) {
	return role >= R_COLOR_WINDOW && role < R_COLOR_LAST_;
}

static int rvn_metric_valid(RMetric metric) {
	return metric >= R_METRIC_FONT_SIZE && metric < R_METRIC_LAST_;
}

/* ---------------------------------------------------------------------------
 *  Built-in themes
 * ------------------------------------------------------------------------- */

typedef struct rvn_palette_entry_t {
	RColorRole role;
	uint32_t rgb;
	uint32_t alpha;
} rvn_palette_entry_t;

static const rvn_palette_entry_t g_dark_palette[] = {
	{ R_COLOR_WINDOW,           0x1E1E20, 255 },
	{ R_COLOR_PANEL,            0x27272A, 255 },
	{ R_COLOR_BORDER,           0x3F3F46, 255 },
	{ R_COLOR_TEXT,             0xD8D8DC, 255 },
	{ R_COLOR_TEXT_DISABLED,    0x6C6C74, 255 },
	{ R_COLOR_TEXT_PLACEHOLDER, 0x7C7C86, 255 },
	{ R_COLOR_ACCENT,           0x3B82F6, 255 },
	{ R_COLOR_ACCENT_HOVERED,   0x5A9AFA, 255 },
	{ R_COLOR_ACCENT_PRESSED,   0x2B6CD8, 255 },
	{ R_COLOR_ACCENT_TEXT,      0xFFFFFF, 255 },
	{ R_COLOR_CONTROL,          0x35353A, 255 },
	{ R_COLOR_CONTROL_HOVERED,  0x424248, 255 },
	{ R_COLOR_CONTROL_PRESSED,  0x2B2B30, 255 },
	{ R_COLOR_CONTROL_BORDER,   0x4C4C54, 255 },
	{ R_COLOR_INPUT,            0x161618, 255 },
	{ R_COLOR_FOCUS,            0x3B82F6, 255 },
	{ R_COLOR_SELECTION,        0x274B7A, 255 },
	{ R_COLOR_ROW_HOVERED,      0x2E2E33, 255 },
	{ R_COLOR_TRACK,            0x2C2C30, 255 },
	{ R_COLOR_HANDLE,           0x5C5C66, 255 },
	{ R_COLOR_HANDLE_HOVERED,   0x7C7C88, 255 },
	{ R_COLOR_SEPARATOR,        0x3F3F46, 255 },
	{ R_COLOR_HEADER,           0x2D2D31, 255 },
	{ R_COLOR_HEADER_HOVERED,   0x3A3A40, 255 },
	{ R_COLOR_TAB_ACTIVE,       0x1E1E20, 255 },
	{ R_COLOR_POPUP,            0x2B2B2F, 255 },
	{ R_COLOR_POPUP_BORDER,     0x4A4A52, 255 },
	{ R_COLOR_SHADOW,           0x000000, 120 },
	{ R_COLOR_TOOLTIP,          0x3C3C42, 255 },
	{ R_COLOR_TOOLTIP_TEXT,     0xE6E6EA, 255 },
	{ R_COLOR_SWITCH,           0x3B82F6, 255 },
	{ R_COLOR_SUCCESS,          0x3FB950, 255 },
	{ R_COLOR_WARNING,          0xD29922, 255 },
	{ R_COLOR_DANGER,           0xF85149, 255 },
	{ R_COLOR_LINK,             0x6CB0FF, 255 },
	{ R_COLOR_CODE,             0x141416, 255 }
};

/* High contrast: every pair of text and background above 7:1 (WCAG AAA). */
static const rvn_palette_entry_t g_hc_dark_palette[] = {
	{ R_COLOR_WINDOW,           0x000000, 255 },
	{ R_COLOR_PANEL,            0x000000, 255 },
	{ R_COLOR_BORDER,           0xFFFFFF, 255 },
	{ R_COLOR_TEXT,             0xFFFFFF, 255 },
	{ R_COLOR_TEXT_DISABLED,    0x3FF23F, 255 },
	{ R_COLOR_TEXT_PLACEHOLDER, 0xC0C0C0, 255 },
	{ R_COLOR_ACCENT,           0x1AEBFF, 255 },
	{ R_COLOR_ACCENT_HOVERED,   0x7DF3FF, 255 },
	{ R_COLOR_ACCENT_PRESSED,   0x00B3C4, 255 },
	{ R_COLOR_ACCENT_TEXT,      0x000000, 255 },
	{ R_COLOR_CONTROL,          0x000000, 255 },
	{ R_COLOR_CONTROL_HOVERED,  0x1A1A1A, 255 },
	{ R_COLOR_CONTROL_PRESSED,  0x333333, 255 },
	{ R_COLOR_CONTROL_BORDER,   0xFFFFFF, 255 },
	{ R_COLOR_INPUT,            0x000000, 255 },
	{ R_COLOR_FOCUS,            0xFFFF00, 255 },
	{ R_COLOR_SELECTION,        0x1AEBFF, 140 },
	{ R_COLOR_ROW_HOVERED,      0x262626, 255 },
	{ R_COLOR_TRACK,            0x000000, 255 },
	{ R_COLOR_HANDLE,           0xFFFFFF, 255 },
	{ R_COLOR_HANDLE_HOVERED,   0xFFFF00, 255 },
	{ R_COLOR_SEPARATOR,        0xFFFFFF, 255 },
	{ R_COLOR_HEADER,           0x000000, 255 },
	{ R_COLOR_HEADER_HOVERED,   0x1A1A1A, 255 },
	{ R_COLOR_TAB_ACTIVE,       0x1AEBFF, 255 },
	{ R_COLOR_POPUP,            0x000000, 255 },
	{ R_COLOR_POPUP_BORDER,     0xFFFFFF, 255 },
	{ R_COLOR_SHADOW,           0x000000, 0 },
	{ R_COLOR_TOOLTIP,          0x000000, 255 },
	{ R_COLOR_TOOLTIP_TEXT,     0xFFFFFF, 255 },
	{ R_COLOR_SWITCH,           0x1AEBFF, 255 },
	{ R_COLOR_SUCCESS,          0x3FF23F, 255 },
	{ R_COLOR_WARNING,          0xFFFF00, 255 },
	{ R_COLOR_DANGER,           0xFF6B6B, 255 },
	{ R_COLOR_LINK,             0xFFFF00, 255 },
	{ R_COLOR_CODE,             0x000000, 255 }
};

static const rvn_palette_entry_t g_hc_light_palette[] = {
	{ R_COLOR_WINDOW,           0xFFFFFF, 255 },
	{ R_COLOR_PANEL,            0xFFFFFF, 255 },
	{ R_COLOR_BORDER,           0x000000, 255 },
	{ R_COLOR_TEXT,             0x000000, 255 },
	{ R_COLOR_TEXT_DISABLED,    0x600000, 255 },
	{ R_COLOR_TEXT_PLACEHOLDER, 0x404040, 255 },
	{ R_COLOR_ACCENT,           0x1C00A8, 255 },
	{ R_COLOR_ACCENT_HOVERED,   0x2D14D0, 255 },
	{ R_COLOR_ACCENT_PRESSED,   0x10006B, 255 },
	{ R_COLOR_ACCENT_TEXT,      0xFFFFFF, 255 },
	{ R_COLOR_CONTROL,          0xFFFFFF, 255 },
	{ R_COLOR_CONTROL_HOVERED,  0xE6E6E6, 255 },
	{ R_COLOR_CONTROL_PRESSED,  0xCCCCCC, 255 },
	{ R_COLOR_CONTROL_BORDER,   0x000000, 255 },
	{ R_COLOR_INPUT,            0xFFFFFF, 255 },
	{ R_COLOR_FOCUS,            0x1C00A8, 255 },
	{ R_COLOR_SELECTION,        0x37006E, 90 },
	{ R_COLOR_ROW_HOVERED,      0xE6E6E6, 255 },
	{ R_COLOR_TRACK,            0xFFFFFF, 255 },
	{ R_COLOR_HANDLE,           0x000000, 255 },
	{ R_COLOR_HANDLE_HOVERED,   0x1C00A8, 255 },
	{ R_COLOR_SEPARATOR,        0x000000, 255 },
	{ R_COLOR_HEADER,           0xFFFFFF, 255 },
	{ R_COLOR_HEADER_HOVERED,   0xE6E6E6, 255 },
	{ R_COLOR_TAB_ACTIVE,       0x1C00A8, 255 },
	{ R_COLOR_POPUP,            0xFFFFFF, 255 },
	{ R_COLOR_POPUP_BORDER,     0x000000, 255 },
	{ R_COLOR_SHADOW,           0x000000, 0 },
	{ R_COLOR_TOOLTIP,          0xFFFFFF, 255 },
	{ R_COLOR_TOOLTIP_TEXT,     0x000000, 255 },
	{ R_COLOR_SWITCH,           0x1C00A8, 255 },
	{ R_COLOR_SUCCESS,          0x005A00, 255 },
	{ R_COLOR_WARNING,          0x6B4A00, 255 },
	{ R_COLOR_DANGER,           0xA30000, 255 },
	{ R_COLOR_LINK,             0x00009F, 255 },
	{ R_COLOR_CODE,             0xFFFFFF, 255 }
};

static const rvn_palette_entry_t g_light_palette[] = {
	{ R_COLOR_WINDOW,           0xF3F3F5, 255 },
	{ R_COLOR_PANEL,            0xFFFFFF, 255 },
	{ R_COLOR_BORDER,           0xD0D0D6, 255 },
	{ R_COLOR_TEXT,             0x1F1F23, 255 },
	{ R_COLOR_TEXT_DISABLED,    0xA0A0A8, 255 },
	{ R_COLOR_TEXT_PLACEHOLDER, 0x90909A, 255 },
	{ R_COLOR_ACCENT,           0x2563EB, 255 },
	{ R_COLOR_ACCENT_HOVERED,   0x3B7BF5, 255 },
	{ R_COLOR_ACCENT_PRESSED,   0x1D4FC4, 255 },
	{ R_COLOR_ACCENT_TEXT,      0xFFFFFF, 255 },
	{ R_COLOR_CONTROL,          0xE7E7EB, 255 },
	{ R_COLOR_CONTROL_HOVERED,  0xDBDBE1, 255 },
	{ R_COLOR_CONTROL_PRESSED,  0xCBCBD3, 255 },
	{ R_COLOR_CONTROL_BORDER,   0xC2C2CA, 255 },
	{ R_COLOR_INPUT,            0xFFFFFF, 255 },
	{ R_COLOR_FOCUS,            0x2563EB, 255 },
	{ R_COLOR_SELECTION,        0xBBD4FB, 255 },
	{ R_COLOR_ROW_HOVERED,      0xE9EEF6, 255 },
	{ R_COLOR_TRACK,            0xDDDDE2, 255 },
	{ R_COLOR_HANDLE,           0xA9A9B2, 255 },
	{ R_COLOR_HANDLE_HOVERED,   0x8A8A94, 255 },
	{ R_COLOR_SEPARATOR,        0xD0D0D6, 255 },
	{ R_COLOR_HEADER,           0xE9E9ED, 255 },
	{ R_COLOR_HEADER_HOVERED,   0xDDDDE3, 255 },
	{ R_COLOR_TAB_ACTIVE,       0xF3F3F5, 255 },
	{ R_COLOR_POPUP,            0xFFFFFF, 255 },
	{ R_COLOR_POPUP_BORDER,     0xC8C8D0, 255 },
	{ R_COLOR_SHADOW,           0x000000, 60 },
	{ R_COLOR_TOOLTIP,          0x2B2B30, 255 },
	{ R_COLOR_TOOLTIP_TEXT,     0xF2F2F5, 255 },
	{ R_COLOR_SWITCH,           0x2563EB, 255 },
	{ R_COLOR_SUCCESS,          0x1A7F37, 255 },
	{ R_COLOR_WARNING,          0x9A6700, 255 },
	{ R_COLOR_DANGER,           0xCF222E, 255 },
	{ R_COLOR_LINK,             0x0B62D6, 255 },
	{ R_COLOR_CODE,             0xE6E6EB, 255 }
};

/*
 * Glass themes. Most surfaces are translucent: the alpha of a color is how
 * much of it is laid over what shows through. Glass surfaces use these colors
 * as their tint (CONTROL, ACCENT, INPUT, PANEL, POPUP: see widgets/common.c).
 */
static const rvn_palette_entry_t g_glass_dark_palette[] = {
	{ R_COLOR_WINDOW,           0x2A2A2E, 255 },
	{ R_COLOR_PANEL,            0x3A3A3F, 110 },
	{ R_COLOR_BORDER,           0xFFFFFF, 34 },
	{ R_COLOR_TEXT,             0xF5F5F7, 255 },
	{ R_COLOR_TEXT_DISABLED,    0xEBEBF5, 80 },
	{ R_COLOR_TEXT_PLACEHOLDER, 0xEBEBF5, 120 },
	{ R_COLOR_ACCENT,           0x0A84FF, 255 },
	{ R_COLOR_ACCENT_HOVERED,   0x409CFF, 255 },
	{ R_COLOR_ACCENT_PRESSED,   0x0A6FDB, 255 },
	{ R_COLOR_ACCENT_TEXT,      0xFFFFFF, 255 },
	{ R_COLOR_CONTROL,          0xFFFFFF, 28 },
	{ R_COLOR_CONTROL_HOVERED,  0xFFFFFF, 46 },
	{ R_COLOR_CONTROL_PRESSED,  0xFFFFFF, 66 },
	{ R_COLOR_CONTROL_BORDER,   0xFFFFFF, 60 },
	{ R_COLOR_INPUT,            0x1A1A1D, 90 },
	{ R_COLOR_FOCUS,            0x0A84FF, 210 },
	{ R_COLOR_SELECTION,        0x0A84FF, 130 },
	{ R_COLOR_ROW_HOVERED,      0xFFFFFF, 20 },
	{ R_COLOR_TRACK,            0xFFFFFF, 40 },
	{ R_COLOR_HANDLE,           0xFFFFFF, 90 },
	{ R_COLOR_HANDLE_HOVERED,   0xFFFFFF, 140 },
	{ R_COLOR_SEPARATOR,        0xFFFFFF, 30 },
	{ R_COLOR_HEADER,           0xFFFFFF, 16 },
	{ R_COLOR_HEADER_HOVERED,   0xFFFFFF, 30 },
	{ R_COLOR_TAB_ACTIVE,       0xFFFFFF, 52 },
	{ R_COLOR_POPUP,            0x333337, 190 },
	{ R_COLOR_POPUP_BORDER,     0xFFFFFF, 40 },
	{ R_COLOR_SHADOW,           0x000000, 110 },
	{ R_COLOR_TOOLTIP,          0x333337, 200 },
	{ R_COLOR_TOOLTIP_TEXT,     0xF5F5F7, 255 },
	{ R_COLOR_SWITCH,           0x30D158, 255 },
	{ R_COLOR_SUCCESS,          0x30D158, 255 },
	{ R_COLOR_WARNING,          0xFFD60A, 255 },
	{ R_COLOR_DANGER,           0xFF453A, 255 },
	{ R_COLOR_LINK,             0x6CB6FF, 255 },
	{ R_COLOR_CODE,             0x1A1A1D, 110 }
};

static const rvn_palette_entry_t g_glass_light_palette[] = {
	{ R_COLOR_WINDOW,           0xFFFFFF, 255 },
	{ R_COLOR_PANEL,            0xFFFFFF, 105 },
	{ R_COLOR_BORDER,           0x3C3C43, 40 },
	{ R_COLOR_TEXT,             0x1C1C1E, 255 },
	{ R_COLOR_TEXT_DISABLED,    0x3C3C43, 90 },
	{ R_COLOR_TEXT_PLACEHOLDER, 0x3C3C43, 130 },
	{ R_COLOR_ACCENT,           0x007AFF, 255 },
	{ R_COLOR_ACCENT_HOVERED,   0x2E90FF, 255 },
	{ R_COLOR_ACCENT_PRESSED,   0x0066D6, 255 },
	{ R_COLOR_ACCENT_TEXT,      0xFFFFFF, 255 },
	{ R_COLOR_CONTROL,          0xFFFFFF, 120 },
	{ R_COLOR_CONTROL_HOVERED,  0xFFFFFF, 165 },
	{ R_COLOR_CONTROL_PRESSED,  0xFFFFFF, 205 },
	{ R_COLOR_CONTROL_BORDER,   0x3C3C43, 46 },
	{ R_COLOR_INPUT,            0xFFFFFF, 110 },
	{ R_COLOR_FOCUS,            0x007AFF, 210 },
	{ R_COLOR_SELECTION,        0x007AFF, 80 },
	{ R_COLOR_ROW_HOVERED,      0x3C3C43, 16 },
	{ R_COLOR_TRACK,            0x787880, 52 },
	{ R_COLOR_HANDLE,           0x3C3C43, 80 },
	{ R_COLOR_HANDLE_HOVERED,   0x3C3C43, 130 },
	{ R_COLOR_SEPARATOR,        0x3C3C43, 46 },
	{ R_COLOR_HEADER,           0x3C3C43, 12 },
	{ R_COLOR_HEADER_HOVERED,   0x3C3C43, 26 },
	{ R_COLOR_TAB_ACTIVE,       0xFFFFFF, 215 },
	{ R_COLOR_POPUP,            0xFAFAFA, 160 },
	{ R_COLOR_POPUP_BORDER,     0x3C3C43, 30 },
	{ R_COLOR_SHADOW,           0x000000, 46 },
	{ R_COLOR_TOOLTIP,          0xFAFAFA, 185 },
	{ R_COLOR_TOOLTIP_TEXT,     0x1C1C1E, 255 },
	{ R_COLOR_SWITCH,           0x34C759, 255 },
	{ R_COLOR_SUCCESS,          0x248A3D, 255 },
	{ R_COLOR_WARNING,          0xB25000, 255 },
	{ R_COLOR_DANGER,           0xD70015, 255 },
	{ R_COLOR_LINK,             0x0066CC, 255 },
	{ R_COLOR_CODE,             0x3C3C43, 24 }
};

/* Same order as RMetric. */
static const float g_flat_metrics[R_METRIC_COUNT] = {
	13.0f, /* font size */
	4.0f,  /* corner radius */
	1.0f,  /* border width */
	26.0f, /* control height */
	10.0f, /* padding x */
	4.0f,  /* padding y */
	6.0f,  /* spacing */
	10.0f, /* scrollbar size */
	22.0f, /* row height */
	16.0f, /* indent */
	5.0f,  /* splitter size */
	16.0f, /* check size */
	14.0f, /* slider handle size */
	6.0f,  /* popup radius */
	1.0f, 1.0f, 1.0f, 1.0f, 1.0f, /* glass: frost, refraction, saturation, rim, shadow */
	18.0f  /* blur radius */
};

/* High contrast: the flat metrics with thick borders. */
static const float g_hc_metrics[R_METRIC_COUNT] = {
	15.0f, /* font size */
	3.0f,  /* corner radius */
	2.0f,  /* border width */
	30.0f, /* control height */
	10.0f, /* padding x */
	5.0f,  /* padding y */
	6.0f,  /* spacing */
	14.0f, /* scrollbar size */
	26.0f, /* row height */
	18.0f, /* indent */
	6.0f,  /* splitter size */
	18.0f, /* check size */
	18.0f, /* slider handle size */
	3.0f,  /* popup radius */
	1.0f, 1.0f, 1.0f, 1.0f, 1.0f, /* glass: frost, refraction, saturation, rim, shadow */
	18.0f  /* blur radius */
};

/* Larger, rounder, with more air: controls are capsules. */
static const float g_glass_metrics[R_METRIC_COUNT] = {
	14.0f, /* font size */
	12.0f, /* corner radius */
	1.0f,  /* border width */
	32.0f, /* control height */
	15.0f, /* padding x */
	6.0f,  /* padding y */
	8.0f,  /* spacing */
	10.0f, /* scrollbar size */
	30.0f, /* row height */
	18.0f, /* indent */
	7.0f,  /* splitter size */
	22.0f, /* check size */
	22.0f, /* slider handle size */
	16.0f, /* popup radius */
	1.0f, 1.0f, 1.0f, 1.0f, 1.0f, /* glass: frost, refraction, saturation, rim, shadow */
	18.0f  /* blur radius */
};

/* ---------------------------------------------------------------------------
 *  Wallpaper of the glass themes
 * ------------------------------------------------------------------------- */

#define RVN_WALLPAPER_SIZE 256

typedef struct rvn_blob_t {
	float x, y, radius;
	float r, g, b;
} rvn_blob_t;

static Rid rvn_wallpaper_render(const float base[3], const rvn_blob_t* blobs, int count) {
	uint8_t* pixels = (uint8_t*)rvn_alloc((size_t)RVN_WALLPAPER_SIZE * RVN_WALLPAPER_SIZE * 4);
	int x, y, i;

	for (y = 0; y < RVN_WALLPAPER_SIZE; y++) {
		for (x = 0; x < RVN_WALLPAPER_SIZE; x++) {
			float u = ((float)x + 0.5f) / RVN_WALLPAPER_SIZE, v = ((float)y + 0.5f) / RVN_WALLPAPER_SIZE;
			float color[3];
			uint8_t* out = pixels + ((size_t)y * RVN_WALLPAPER_SIZE + (size_t)x) * 4;
			color[0] = base[0];
			color[1] = base[1];
			color[2] = base[2];
			for (i = 0; i < count; i++) {
				float dx = u - blobs[i].x, dy = v - blobs[i].y;
				float weight = expf(-(dx * dx + dy * dy) / (blobs[i].radius * blobs[i].radius * 0.5f)) * 0.9f;
				color[0] += (blobs[i].r - color[0]) * weight;
				color[1] += (blobs[i].g - color[1]) * weight;
				color[2] += (blobs[i].b - color[2]) * weight;
			}
			out[0] = (uint8_t)(rvn_clampf(color[0], 0.0f, 1.0f) * 255.0f + 0.5f);
			out[1] = (uint8_t)(rvn_clampf(color[1], 0.0f, 1.0f) * 255.0f + 0.5f);
			out[2] = (uint8_t)(rvn_clampf(color[2], 0.0f, 1.0f) * 255.0f + 0.5f);
			out[3] = 255;
		}
	}
	/* the texture takes ownership of the pixels */
	return rvn_texture_create_raw(RVN_WALLPAPER_SIZE, RVN_WALLPAPER_SIZE, R_TEXTURE_RGBA8, R_FILTER_LINEAR, pixels);
}

/* Where the blobs lie, and how large they are: the same for every wallpaper. */
static const rvn_blob_t g_blob_places[5] = {
	{ 0.15f, 0.20f, 0.55f, 0.0f, 0.0f, 0.0f },
	{ 0.88f, 0.12f, 0.50f, 0.0f, 0.0f, 0.0f },
	{ 0.80f, 0.86f, 0.58f, 0.0f, 0.0f, 0.0f },
	{ 0.12f, 0.92f, 0.46f, 0.0f, 0.0f, 0.0f },
	{ 0.50f, 0.50f, 0.34f, 0.0f, 0.0f, 0.0f }
};

Rid rvn_wallpaper_create_colors(uint32_t base, const uint32_t* colors, int count) {
	rvn_blob_t blobs[5];
	float base_color[3];
	int i;
	if (count > 5) count = 5;
	base_color[0] = (float)((base >> 16) & 0xFFu) / 255.0f;
	base_color[1] = (float)((base >> 8) & 0xFFu) / 255.0f;
	base_color[2] = (float)(base & 0xFFu) / 255.0f;
	for (i = 0; i < count; i++) {
		blobs[i] = g_blob_places[i];
		blobs[i].r = (float)((colors[i] >> 16) & 0xFFu) / 255.0f;
		blobs[i].g = (float)((colors[i] >> 8) & 0xFFu) / 255.0f;
		blobs[i].b = (float)(colors[i] & 0xFFu) / 255.0f;
	}
	return rvn_wallpaper_render(base_color, blobs, count);
}

/*
 * The background of the glass themes: plain black for the dark one, plain
 * white for the light one. It is still a wallpaper (a texture under the whole
 * window), so that Rwindow_set_background and the Wallpaper of a theme file
 * ("#101020, #3040C0, #C02080": colored blobs) replace it the same way.
 */
Rid rvn_theme_builtin_wallpaper(int dark) {
	Rid* wallpaper = &g_rvn.wallpapers[dark ? 0 : 1];
	if (!rvn_texture_get(*wallpaper)) {
		/* (dark: a graphite grey, not black: the glass reads as frosted, not as a hole) */
		*wallpaper = rvn_wallpaper_create_colors(dark ? 0x2A2A2E : 0xFFFFFF, NULL, 0);
	}
	return *wallpaper;
}

void rvn_theme_build(rvn_theme_t* theme, RTheme builtin) {
	const rvn_palette_entry_t* palette;
	size_t count, i;
	int glass = builtin == R_THEME_GLASS_DARK || builtin == R_THEME_GLASS_LIGHT;
	int contrast = builtin == R_THEME_HIGH_CONTRAST_DARK || builtin == R_THEME_HIGH_CONTRAST_LIGHT;
	int dark = builtin == R_THEME_DARK || builtin == R_THEME_GLASS_DARK || builtin == R_THEME_HIGH_CONTRAST_DARK;
	char* name = theme->name;
	char* path = theme->path;
	Rid font = theme->font;

	theme->name = NULL;
	theme->path = NULL;
	rvn_theme_clear(theme);
	theme->name = name;
	theme->path = path;
	/* (the font is not part of the built-in themes: the one in use stays) */
	theme->font = font;

	switch (builtin) {
		case R_THEME_LIGHT:
			palette = g_light_palette;
			count = RVN_COUNT_OF(g_light_palette);
			break;
		case R_THEME_GLASS_DARK:
			palette = g_glass_dark_palette;
			count = RVN_COUNT_OF(g_glass_dark_palette);
			break;
		case R_THEME_GLASS_LIGHT:
			palette = g_glass_light_palette;
			count = RVN_COUNT_OF(g_glass_light_palette);
			break;
		case R_THEME_HIGH_CONTRAST_DARK:
			palette = g_hc_dark_palette;
			count = RVN_COUNT_OF(g_hc_dark_palette);
			break;
		case R_THEME_HIGH_CONTRAST_LIGHT:
			palette = g_hc_light_palette;
			count = RVN_COUNT_OF(g_hc_light_palette);
			break;
		case R_THEME_DARK:
		default:
			palette = g_dark_palette;
			count = RVN_COUNT_OF(g_dark_palette);
			break;
	}
	for (i = 0; i < count; i++) theme->colors[RVN_COLOR_INDEX(palette[i].role)] = rvn_hex(palette[i].rgb, palette[i].alpha);
	memcpy(theme->metrics, glass ? g_glass_metrics : contrast ? g_hc_metrics : g_flat_metrics, sizeof(theme->metrics));

	theme->material = glass ? R_MATERIAL_FROSTED_GLASS : R_MATERIAL_SOLID;
	theme->glass = glass;
	theme->dark = dark;
	theme->wallpaper = glass ? rvn_theme_builtin_wallpaper(dark) : R_INVALID_ID;
}

void rvn_theme_apply(RTheme theme) {
	static const char* const names[] = { "Dark", "Light", "GlassDark", "GlassLight", "HighContrastDark", "HighContrastLight" };
	rvn_theme_build(&g_rvn.theme, theme);
	rvn_str_set(&g_rvn.theme_name, names[(int)theme - (int)R_THEME_DARK]);
	rvn_theme_changed();
}

/* ---------------------------------------------------------------------------
 *  Lookup
 * ------------------------------------------------------------------------- */

rvn_color_t rvn_widget_color(const rvn_widget_t* widget, RColorRole role) {
	int index = RVN_COLOR_INDEX(role);
	uint64_t bit = (uint64_t)1 << index;
	const rvn_style_match_t* match;
	if (widget->style && (widget->style->color_mask & bit)) return widget->style->colors[index];
	match = rvn_widget_match(widget);
	if (match && (match->style.color_mask & bit)) return match->style.colors[index];
	return rvn_widget_theme(widget)->colors[index];
}

static float rvn_widget_metric_raw(const rvn_widget_t* widget, RMetric metric) {
	int index = RVN_METRIC_INDEX(metric);
	uint32_t bit = 1u << index;
	const rvn_style_match_t* match;
	if (widget->style && (widget->style->metric_mask & bit)) return widget->style->metrics[index];
	match = rvn_widget_match(widget);
	if (match && (match->style.metric_mask & bit)) return match->style.metrics[index];
	return rvn_widget_theme(widget)->metrics[index];
}

float rvn_widget_metric(const rvn_widget_t* widget, RMetric metric) {
	float value = rvn_widget_metric_raw(widget, metric);
	float scale = rvn_a11y_text_scale();
	/* larger texts (Raccessibility_set_text_scale): the controls that hold a line of text grow with it */
	if (scale != 1.0f) {
		switch (metric) {
			case R_METRIC_CONTROL_HEIGHT: case R_METRIC_ROW_HEIGHT: case R_METRIC_CHECK_SIZE:
			case R_METRIC_SLIDER_HANDLE_SIZE: case R_METRIC_INDENT: case R_METRIC_PADDING_Y:
				value *= scale;
				break;
			default:
				break;
		}
	}
	return value;
}

int rvn_widget_has_color(const rvn_widget_t* widget, RColorRole role) {
	uint64_t bit = (uint64_t)1 << RVN_COLOR_INDEX(role);
	const rvn_style_match_t* match;
	if (widget->style && (widget->style->color_mask & bit)) return R_TRUE;
	match = rvn_widget_match(widget);
	return match && (match->style.color_mask & bit) != 0;
}

int rvn_widget_has_metric(const rvn_widget_t* widget, RMetric metric) {
	uint32_t bit = 1u << RVN_METRIC_INDEX(metric);
	const rvn_style_match_t* match;
	if (widget->style && (widget->style->metric_mask & bit)) return R_TRUE;
	match = rvn_widget_match(widget);
	return match && (match->style.metric_mask & bit) != 0;
}

static rvn_style_t* rvn_widget_style(rvn_widget_t* widget) {
	if (!widget->style) widget->style = (rvn_style_t*)rvn_calloc(sizeof(rvn_style_t));
	return widget->style;
}

void rvn_widget_set_color(rvn_widget_t* widget, RColorRole role, rvn_color_t color) {
	rvn_style_t* style = rvn_widget_style(widget);
	int index = RVN_COLOR_INDEX(role);
	style->colors[index] = color;
	style->color_mask |= (uint64_t)1 << index;
	rvn_widget_invalidate_paint(widget);
}

void rvn_widget_set_metric(rvn_widget_t* widget, RMetric metric, float value) {
	rvn_style_t* style = rvn_widget_style(widget);
	int index = RVN_METRIC_INDEX(metric);
	style->metrics[index] = value;
	style->metric_mask |= 1u << index;
	rvn_widget_invalidate_layout(widget);
}

rvn_color_t rvn_widget_text_color(const rvn_widget_t* widget, const RPainter* painter) {
	if (!painter->enabled) return rvn_widget_color(widget, R_COLOR_TEXT_DISABLED);
	/* the painter already carries the override of this widget or of an ancestor */
	return painter->text_color;
}

/* ---------------------------------------------------------------------------
 *  Public API
 * ------------------------------------------------------------------------- */

void Rtheme_apply(RTheme theme) {
	RVN_API_GUARD_VOID();
	if (theme < R_THEME_DARK || theme > R_THEME_HIGH_CONTRAST_LIGHT) {
		rvn_error(R_ERROR_INVALID_ENUM, R_SEVERITY_ERROR, "Rtheme_apply: invalid theme");
		return;
	}
	rvn_theme_apply(theme);
}

void Rtheme_set_color(RColorRole role, float r, float g, float b, float a) {
	RVN_API_GUARD_VOID();
	if (!rvn_color_valid(role)) {
		rvn_error(R_ERROR_INVALID_ENUM, R_SEVERITY_ERROR, "Rtheme_set_color: invalid color role");
		return;
	}
	RVN_THEME_COLOR(role) = rvn_rgba(r, g, b, a);
	rvn_theme_changed();
}

void Rtheme_get_color(RColorRole role, float* r, float* g, float* b, float* a) {
	if (!g_rvn.initialized || !rvn_color_valid(role)) {
		if (r) *r = 0.0f;
		if (g) *g = 0.0f;
		if (b) *b = 0.0f;
		if (a) *a = 0.0f;
		return;
	}
	rvn_color_to_floats(RVN_THEME_COLOR(role), r, g, b, a);
}

void Rtheme_set_metric(RMetric metric, float value) {
	RVN_API_GUARD_VOID();
	if (!rvn_metric_valid(metric)) {
		rvn_error(R_ERROR_INVALID_ENUM, R_SEVERITY_ERROR, "Rtheme_set_metric: invalid metric");
		return;
	}
	RVN_THEME_METRIC(metric) = value;
	rvn_theme_changed();
}

float Rtheme_get_metric(RMetric metric) {
	if (!g_rvn.initialized || !rvn_metric_valid(metric)) return 0.0f;
	return RVN_THEME_METRIC(metric);
}

void Rtheme_set_material(RMaterial material) {
	RVN_API_GUARD_VOID();
	if (material < R_MATERIAL_SOLID || material > R_MATERIAL_TINTED_GLASS) {
		rvn_error(R_ERROR_INVALID_ENUM, R_SEVERITY_ERROR, "Rtheme_set_material: invalid material");
		return;
	}
	g_rvn.theme.material = material;
	g_rvn.theme.glass = material != R_MATERIAL_SOLID;
	/* (the sizes of some controls depend on it) */
	rvn_theme_changed();
}

RMaterial Rtheme_get_material(void) {
	return g_rvn.initialized ? g_rvn.theme.material : R_MATERIAL_SOLID;
}

void Rwidget_set_material(Rid id, RMaterial material) {
	RVN_WIDGET_VOID(widget, id, 0);
	if (material < R_MATERIAL_SOLID || material > R_MATERIAL_DEFAULT) {
		rvn_error(R_ERROR_INVALID_ENUM, R_SEVERITY_ERROR, "Rwidget_set_material: invalid material");
		return;
	}
	widget->material = material == R_MATERIAL_DEFAULT ? (RMaterial)0 : material;
	rvn_widget_invalidate_layout(widget);
}

RMaterial Rwidget_get_material(Rid id) {
	RVN_WIDGET(widget, id, 0, R_MATERIAL_SOLID);
	return rvn_widget_material(widget);
}

void Rtheme_set_font(Rid font) {
	RVN_API_GUARD_VOID();
	if (font != R_INVALID_ID && !rvn_pool_get(&g_rvn.fonts, font)) {
		rvn_error(R_ERROR_INVALID_ID, R_SEVERITY_ERROR, "Rtheme_set_font: invalid font");
		return;
	}
	g_rvn.theme.font = font == R_INVALID_ID ? g_rvn.default_font : font;
	rvn_theme_changed();
}

Rid Rtheme_get_font(void) {
	return g_rvn.initialized ? g_rvn.theme.font : R_INVALID_ID;
}

void Rwidget_set_color(Rid id, RColorRole role, float r, float g, float b, float a) {
	RVN_WIDGET_VOID(widget, id, 0);
	if (!rvn_color_valid(role)) {
		rvn_error(R_ERROR_INVALID_ENUM, R_SEVERITY_ERROR, "Rwidget_set_color: invalid color role");
		return;
	}
	rvn_widget_set_color(widget, role, rvn_rgba(r, g, b, a));
}

void Rwidget_clear_color(Rid id, RColorRole role) {
	RVN_WIDGET_VOID(widget, id, 0);
	if (!rvn_color_valid(role)) {
		rvn_error(R_ERROR_INVALID_ENUM, R_SEVERITY_ERROR, "Rwidget_clear_color: invalid color role");
		return;
	}
	if (widget->style) widget->style->color_mask &= ~((uint64_t)1 << RVN_COLOR_INDEX(role));
	rvn_widget_invalidate_paint(widget);
}

void Rwidget_set_metric(Rid id, RMetric metric, float value) {
	RVN_WIDGET_VOID(widget, id, 0);
	if (!rvn_metric_valid(metric)) {
		rvn_error(R_ERROR_INVALID_ENUM, R_SEVERITY_ERROR, "Rwidget_set_metric: invalid metric");
		return;
	}
	rvn_widget_set_metric(widget, metric, value);
}

void Rwidget_clear_metric(Rid id, RMetric metric) {
	RVN_WIDGET_VOID(widget, id, 0);
	if (!rvn_metric_valid(metric)) {
		rvn_error(R_ERROR_INVALID_ENUM, R_SEVERITY_ERROR, "Rwidget_clear_metric: invalid metric");
		return;
	}
	if (widget->style) widget->style->metric_mask &= ~(1u << RVN_METRIC_INDEX(metric));
	rvn_widget_invalidate_layout(widget);
}

void Rwidget_get_color(Rid id, RColorRole role, float* r, float* g, float* b, float* a) {
	rvn_widget_t* widget = rvn_widget_check(id, 0, __func__);
	rvn_color_t color = 0;
	if (widget && !rvn_color_valid(role)) {
		rvn_error(R_ERROR_INVALID_ENUM, R_SEVERITY_ERROR, "Rwidget_get_color: invalid color role");
	} else if (widget) {
		color = rvn_widget_color(widget, role);
	}
	rvn_color_to_floats(color, r, g, b, a);
}

float Rwidget_get_metric(Rid id, RMetric metric) {
	RVN_WIDGET(widget, id, 0, 0.0f);
	if (!rvn_metric_valid(metric)) {
		rvn_error(R_ERROR_INVALID_ENUM, R_SEVERITY_ERROR, "Rwidget_get_metric: invalid metric");
		return 0.0f;
	}
	return rvn_widget_metric(widget, metric);
}
