/**
 * Raven - render/font_raster.c
 * The only file that includes stb_truetype. The implementation is compiled
 * as static functions so that it never clashes with another copy of stb
 * linked into the application (HRL embeds one too).
 */

#include "font.h"

#define STBTT_STATIC
#define STBTT_malloc(size, user) ((void)(user), rvn_alloc(size))
#define STBTT_free(ptr, user) ((void)(user), rvn_free(ptr))
#define STB_TRUETYPE_IMPLEMENTATION

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wcast-qual"
#elif defined(_MSC_VER)
#pragma warning(push, 0)
#endif

#include "stb/stb_truetype.h"

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#pragma warning(pop)
#endif

struct rvn_ttf_t {
	stbtt_fontinfo info;
	float units_per_em;
	/* color glyphs (COLR / CPAL version 0): offsets in the data, 0 when the font has none */
	uint32_t colr, cpal;
};

static uint16_t rvn_ttf_u16(const uint8_t* p) { return (uint16_t)((p[0] << 8) | p[1]); }
static uint32_t rvn_ttf_u32(const uint8_t* p) { return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | p[3]; }

int rvn_ttf_has_color(rvn_ttf_t* ttf) {
	return ttf->colr != 0 && ttf->cpal != 0;
}

/*
 * The layers of a color glyph (an emoji of Segoe UI Emoji, of Noto's COLR
 * fonts...): each is an ordinary outline glyph of the font, drawn in a color
 * of the first palette, the first layer at the bottom. A color of 0 means
 * "the color of the text". Returns the number of layers, 0 for a plain glyph.
 */
int rvn_ttf_color_layers(rvn_ttf_t* ttf, int glyph, int* glyphs, uint32_t* colors, int max) {
	const uint8_t* data = ttf->info.data;
	const uint8_t* colr;
	const uint8_t* cpal;
	uint32_t base_offset, layer_offset, color_offset;
	uint16_t base_count, entries, first_index;
	int lo, hi;
	if (!ttf->colr || !ttf->cpal) return 0;
	colr = data + ttf->colr;
	cpal = data + ttf->cpal;
	if (rvn_ttf_u16(colr) != 0 && rvn_ttf_u16(colr) != 1) return 0;
	base_count = rvn_ttf_u16(colr + 2);
	base_offset = rvn_ttf_u32(colr + 4);
	layer_offset = rvn_ttf_u32(colr + 8);
	entries = rvn_ttf_u16(cpal + 2);
	color_offset = rvn_ttf_u32(cpal + 8);
	first_index = rvn_ttf_u16(cpal + 12);
	/* the base glyph records are sorted by glyph */
	lo = 0;
	hi = (int)base_count - 1;
	while (lo <= hi) {
		int mid = (lo + hi) / 2;
		const uint8_t* record = colr + base_offset + (size_t)mid * 6;
		int id = rvn_ttf_u16(record);
		if (id < glyph) lo = mid + 1;
		else if (id > glyph) hi = mid - 1;
		else {
			int first = rvn_ttf_u16(record + 2), count = rvn_ttf_u16(record + 4), i;
			if (count > max) count = max;
			for (i = 0; i < count; i++) {
				const uint8_t* layer = colr + layer_offset + (size_t)(first + i) * 4;
				uint16_t palette_index = rvn_ttf_u16(layer + 2);
				glyphs[i] = rvn_ttf_u16(layer);
				if (palette_index == 0xFFFF || palette_index >= entries) {
					colors[i] = 0;
				} else {
					/* CPAL colors are B, G, R, A */
					const uint8_t* c = cpal + color_offset + (size_t)(first_index + palette_index) * 4;
					colors[i] = (uint32_t)c[2] | ((uint32_t)c[1] << 8) | ((uint32_t)c[0] << 16) | ((uint32_t)c[3] << 24);
					if (!colors[i]) colors[i] = 1; /* (transparent black is not "the text color") */
				}
			}
			return count;
		}
	}
	return 0;
}

rvn_ttf_t* rvn_ttf_open(const uint8_t* data, size_t size) {
	rvn_ttf_t* ttf;
	int offset;
	if (!data || size < 12) return NULL;
	offset = stbtt_GetFontOffsetForIndex(data, 0);
	if (offset < 0) return NULL;
	ttf = (rvn_ttf_t*)rvn_calloc(sizeof(rvn_ttf_t));
	if (!stbtt_InitFont(&ttf->info, data, offset)) {
		rvn_free(ttf);
		return NULL;
	}
	/* stbtt_ScaleForMappingEmToPixels(1) is 1 / unitsPerEm */
	ttf->units_per_em = 1.0f / stbtt_ScaleForMappingEmToPixels(&ttf->info, 1.0f);
	ttf->colr = stbtt__find_table((stbtt_uint8*)(uintptr_t)data, (stbtt_uint32)offset, "COLR");
	ttf->cpal = stbtt__find_table((stbtt_uint8*)(uintptr_t)data, (stbtt_uint32)offset, "CPAL");
	if (ttf->colr && ((size_t)ttf->colr + 14 > size || rvn_ttf_u32(data + ttf->colr + 4) + (size_t)ttf->colr > size)) ttf->colr = 0;
	if (ttf->cpal && (size_t)ttf->cpal + 14 > size) ttf->cpal = 0;
	return ttf;
}

void rvn_ttf_close(rvn_ttf_t* ttf) {
	rvn_free(ttf);
}

int rvn_ttf_glyph_index(rvn_ttf_t* ttf, uint32_t codepoint) {
	return stbtt_FindGlyphIndex(&ttf->info, (int)codepoint);
}

float rvn_ttf_advance(rvn_ttf_t* ttf, int glyph) {
	int advance = 0, bearing = 0;
	stbtt_GetGlyphHMetrics(&ttf->info, glyph, &advance, &bearing);
	return (float)advance / ttf->units_per_em;
}

void rvn_ttf_vmetrics(rvn_ttf_t* ttf, float* ascent, float* descent, float* line_gap) {
	int a = 0, d = 0, g = 0;
	stbtt_GetFontVMetrics(&ttf->info, &a, &d, &g);
	*ascent = (float)a / ttf->units_per_em;
	*descent = (float)(-d) / ttf->units_per_em;
	*line_gap = (float)g / ttf->units_per_em;
}

void rvn_ttf_glyph_box(rvn_ttf_t* ttf, int glyph, float size_px, int* x0, int* y0, int* x1, int* y1) {
	float scale = stbtt_ScaleForMappingEmToPixels(&ttf->info, size_px);
	stbtt_GetGlyphBitmapBox(&ttf->info, glyph, scale, scale, x0, y0, x1, y1);
}

void rvn_ttf_render(rvn_ttf_t* ttf, int glyph, float size_px, uint8_t* out, int width, int height, int stride) {
	float scale = stbtt_ScaleForMappingEmToPixels(&ttf->info, size_px);
	stbtt_MakeGlyphBitmap(&ttf->info, out, width, height, stride, scale, scale, glyph);
}

void rvn_ttf_render_bold(rvn_ttf_t* ttf, int glyph, float size_px, float spread, uint8_t* out, int width, int height, int stride) {
	float scale = stbtt_ScaleForMappingEmToPixels(&ttf->info, size_px);
	int ax0, ay0, ax1, ay1, copies = spread > 1.0f ? (int)ceilf(spread) : 1, copy;
	stbtt_GetGlyphBitmapBox(&ttf->info, glyph, scale, scale, &ax0, &ay0, &ax1, &ay1);
	stbtt_MakeGlyphBitmap(&ttf->info, out, width, height, stride, scale, scale, glyph);
	/* the outline again, moved right by a part of a pixel (by steps of at most one), kept where it is the darker of the two */
	for (copy = 1; copy <= copies; copy++) {
		float shift = spread * (float)copy / (float)copies;
		float whole = floorf(shift);
		int bx0, by0, bx1, by1, w, h, x, y, dx;
		uint8_t* moved;
		stbtt_GetGlyphBitmapBoxSubpixel(&ttf->info, glyph, scale, scale, shift - whole, 0.0f, &bx0, &by0, &bx1, &by1);
		w = bx1 - bx0;
		h = by1 - by0;
		if (w <= 0 || h <= 0) continue;
		moved = (uint8_t*)rvn_calloc((size_t)w * (size_t)h);
		stbtt_MakeGlyphBitmapSubpixel(&ttf->info, moved, w, h, w, scale, scale, shift - whole, 0.0f, glyph);
		dx = bx0 - ax0 + (int)whole;
		for (y = 0; y < h; y++) {
			int oy = y + by0 - ay0;
			if (oy < 0 || oy >= height) continue;
			for (x = 0; x < w; x++) {
				int ox = x + dx;
				if (ox < 0 || ox >= width) continue;
				if (moved[(size_t)y * (size_t)w + (size_t)x] > out[(size_t)oy * (size_t)stride + (size_t)ox]) {
					out[(size_t)oy * (size_t)stride + (size_t)ox] = moved[(size_t)y * (size_t)w + (size_t)x];
				}
			}
		}
		rvn_free(moved);
	}
}
