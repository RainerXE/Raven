/**
 * HUI - Horizon UI
 * hui_resources.h : fonts and textures.
 */

#ifndef HUI_RESOURCES_H
#define HUI_RESOURCES_H

#include "hui_core.h"


typedef enum HUI_EFilterType{
	HUI_FILTER_LINEAR = 0x02C0,
	/** No smoothing: keeps pixel art sharp. */
	HUI_FILTER_NEAREST
}HUI_EFilterType;


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
	 * @return HUI_INVALID_ID on failure.
	 */
	HUI_API HUI_id HUI_CreateFont(const char* data, size_t data_size);
	HUI_API HUI_id HUI_CreateFontFromFile(const char* path);
	HUI_API void HUI_DeleteFont(HUI_id font);
	HUI_API int HUI_IsValidFont(HUI_id font);

	/** @return The font embedded in the library (Roboto, Latin and Latin Extended-A). */
	HUI_API HUI_id HUI_GetDefaultFont(void);

	/**
	 * @brief Font used for the characters missing from `font` (symbols, icons,
	 * other scripts). Fallbacks can be chained.
	 */
	HUI_API void HUI_SetFontFallback(HUI_id font, HUI_id fallback);

	/**
	 * @brief Measures a UTF-8 string, in logical units, at scale 1.
	 * @param font HUI_INVALID_ID uses the theme font.
	 * @param size Font size; 0 uses HUI_METRIC_FONT_SIZE.
	 */
	HUI_API void HUI_MeasureText(HUI_id font, float size, const char* text, float* width, float* height);


	/* ============================================================================
	 *  TEXTURES
	 * ============================================================================ */

	/**
	 * @brief Creates a texture from the content of an image file (PNG, JPEG,
	 * BMP, TGA, GIF, PSD). Decoding happens on the calling thread.
	 * @return HUI_INVALID_ID on failure.
	 */
	HUI_API HUI_id HUI_CreateTexture(const char* data, size_t data_size);

	/**
	 * @brief Same as HUI_CreateTexture, decoded on a worker thread. The handle
	 * is usable at once; widgets showing it draw nothing until it is ready.
	 * The buffer is copied.
	 */
	HUI_API HUI_id HUI_CreateTextureAsync(const char* data, size_t data_size);

	HUI_API HUI_id HUI_CreateTextureFromFile(const char* path);
	/** @brief Reads and decodes the file on a worker thread. */
	HUI_API HUI_id HUI_CreateTextureFromFileAsync(const char* path);

	/** @brief Creates a texture from tightly packed RGBA8 pixels (straight alpha). */
	HUI_API HUI_id HUI_CreateTextureFromPixels(const uint8_t* rgba, int width, int height);

	/**
	 * @brief Replaces the pixels of a texture. The size may change. Use it to
	 * show frames produced on the CPU or read back from another renderer.
	 */
	HUI_API void HUI_UpdateTexture(HUI_id texture, const uint8_t* rgba, int width, int height);

	/** @return HUI_TRUE once an asynchronous texture is decoded (or failed). */
	HUI_API int HUI_IsTextureReady(HUI_id texture);
	/** @brief Blocks until an asynchronous texture is ready. */
	HUI_API void HUI_WaitForTexture(HUI_id texture);

	HUI_API void HUI_DeleteTexture(HUI_id texture);
	HUI_API int HUI_IsValidTexture(HUI_id texture);
	HUI_API void HUI_GetTextureSize(HUI_id texture, int* width, int* height);
	HUI_API void HUI_SetTextureFilter(HUI_id texture, HUI_EFilterType filter);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* HUI_RESOURCES_H */
