/**
 * HUI - Horizon UI
 * hui_draw.h : custom-painted widgets.
 *
 * A CUSTOM widget is measured and painted by the application. Coordinates are
 * in logical units, relative to the top-left corner of the widget; HUI applies
 * the window scale, pixel snapping, clipping and opacity.
 *
 * The paint callback runs on the UI thread and may only call HUI_Draw...
 * functions with the painter it received.
 */

#ifndef HUI_DRAW_H
#define HUI_DRAW_H

#include "hui_widget.h"

typedef struct HUI_Painter HUI_Painter;

#ifdef __cplusplus
extern "C" {
#endif

	typedef void (*HUI_CPaint)(HUI_id widget, HUI_Painter* painter, float width, float height, void* user_data);
	typedef void (*HUI_CMeasure)(HUI_id widget, float* width, float* height, void* user_data);

	HUI_API void HUI_SetCustomPaintCallback(HUI_id widget, HUI_CPaint callback, void* user_data);
	/** @brief Reports the desired size of the widget. Without it the widget asks for 0 x 0. */
	HUI_API void HUI_SetCustomMeasureCallback(HUI_id widget, HUI_CMeasure callback, void* user_data);

	/** @brief Filled rectangle with rounded corners (radius 0 = sharp). */
	HUI_API void HUI_DrawRect(HUI_Painter* painter, float x, float y, float width, float height,
		float radius, float r, float g, float b, float a);

	/** @brief Outline of a rounded rectangle, drawn inside its bounds. */
	HUI_API void HUI_DrawRectOutline(HUI_Painter* painter, float x, float y, float width, float height,
		float radius, float thickness, float r, float g, float b, float a);

	HUI_API void HUI_DrawCircle(HUI_Painter* painter, float center_x, float center_y, float radius,
		float r, float g, float b, float a);

	/** @brief Anti-aliased line with round caps. */
	HUI_API void HUI_DrawLine(HUI_Painter* painter, float x0, float y0, float x1, float y1,
		float thickness, float r, float g, float b, float a);

	HUI_API void HUI_DrawTriangle(HUI_Painter* painter, float x0, float y0, float x1, float y1,
		float x2, float y2, float r, float g, float b, float a);

	/** @brief Texture stretched to a rectangle and multiplied by a tint color. */
	HUI_API void HUI_DrawImage(HUI_Painter* painter, HUI_id texture, float x, float y, float width, float height,
		float r, float g, float b, float a);

	/**
	 * @brief Draws UTF-8 text; (x, y) is the top-left corner of the first line.
	 * @param font HUI_INVALID_ID uses the theme font.
	 * @param size 0 uses HUI_METRIC_FONT_SIZE.
	 */
	HUI_API void HUI_DrawText(HUI_Painter* painter, HUI_id font, float size, float x, float y,
		const char* text, float r, float g, float b, float a);

	/** @brief Restricts the following draws to a rectangle, intersected with the current one. */
	HUI_API void HUI_PushClipRect(HUI_Painter* painter, float x, float y, float width, float height);
	HUI_API void HUI_PopClipRect(HUI_Painter* painter);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* HUI_DRAW_H */
