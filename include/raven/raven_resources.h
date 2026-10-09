/**
 * Raven
 * raven_resources.h : fonts and textures.
 */

#ifndef RAVEN_RESOURCES_H
#define RAVEN_RESOURCES_H

#include "raven_core.h"


typedef enum RFilterType{
	R_FILTER_LINEAR = 0x02C0,
	/** No smoothing: keeps pixel art sharp. */
	R_FILTER_NEAREST
}RFilterType;


#ifdef __cplusplus
extern "C" {
#endif

	/* ============================================================================
	 *  FONTS
	 * ============================================================================ */

	/**
	 * @brief Creates a font from the content of a .ttf / .otf file.
	 * The buffer is copied. Glyphs are rasterized on demand, at the exact
	 * pixel size needed by each window scale, into a shared atlas.
	 * @return R_INVALID_ID on failure.
	 */
	RAVEN_API Rid Rfont_create(const char* data, size_t data_size);
	RAVEN_API Rid Rfont_create_from_file(const char* path);
	RAVEN_API void Rfont_destroy(Rid font);
	RAVEN_API int Rfont_is_valid(Rid font);

	/** @return The font embedded in the library (Roboto, Latin and Latin Extended-A). */
	RAVEN_API Rid Rfont_get_default(void);

	/**
	 * @brief Font used for the characters missing from `font` (symbols, icons,
	 * other scripts). Fallbacks can be chained.
	 */
	RAVEN_API void Rfont_set_fallback(Rid font, Rid fallback);

	/**
	 * @brief Measures a UTF-8 string, in logical units, at scale 1.
	 * @param font R_INVALID_ID uses the theme font.
	 * @param size Font size; 0 uses R_METRIC_FONT_SIZE.
	 */
	RAVEN_API void Rfont_measure_text(Rid font, float size, const char* text, float* width, float* height);


	/* ============================================================================
	 *  TEXTURES
	 * ============================================================================ */

	/**
	 * @brief Creates a texture from the content of an image file (PNG, JPEG,
	 * BMP, TGA, GIF, PSD). Decoding happens on the calling thread.
	 * @return R_INVALID_ID on failure.
	 */
	RAVEN_API Rid Rtexture_create(const char* data, size_t data_size);

	/**
	 * @brief Same as Rtexture_create, decoded on a worker thread. The handle
	 * is usable at once; widgets showing it draw nothing until it is ready.
	 * The buffer is copied.
	 */
	RAVEN_API Rid Rtexture_create_async(const char* data, size_t data_size);

	RAVEN_API Rid Rtexture_create_from_file(const char* path);
	/** @brief Reads and decodes the file on a worker thread. */
	RAVEN_API Rid Rtexture_create_from_file_async(const char* path);

	/** @brief Creates a texture from tightly packed RGBA8 pixels (straight alpha). */
	RAVEN_API Rid Rtexture_create_from_pixels(const uint8_t* rgba, int width, int height);

	/**
	 * @brief Replaces the pixels of a texture. The size may change. Use it to
	 * show frames produced on the CPU or read back from another renderer.
	 */
	RAVEN_API void Rtexture_update(Rid texture, const uint8_t* rgba, int width, int height);

	/** @return R_TRUE once an asynchronous texture is decoded (or failed). */
	RAVEN_API int Rtexture_is_ready(Rid texture);
	/** @brief Blocks until an asynchronous texture is ready. */
	RAVEN_API void Rtexture_wait(Rid texture);

	RAVEN_API void Rtexture_destroy(Rid texture);
	RAVEN_API int Rtexture_is_valid(Rid texture);
	RAVEN_API void Rtexture_get_size(Rid texture, int* width, int* height);
	RAVEN_API void Rtexture_set_filter(Rid texture, RFilterType filter);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* RAVEN_RESOURCES_H */
