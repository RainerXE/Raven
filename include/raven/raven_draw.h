/**
 * Raven
 * raven_draw.h : custom-painted widgets.
 *
 * A CUSTOM widget is measured and painted by the application. Coordinates are
 * in logical units, relative to the top-left corner of the widget; Raven applies
 * the window scale, pixel snapping, clipping and opacity.
 *
 * The paint callback runs on the UI thread and may only call Rdraw_...
 * functions with the painter it received.
 */

#ifndef RAVEN_DRAW_H
#define RAVEN_DRAW_H

#include "raven_widget.h"

typedef struct RPainter RPainter;

#ifdef __cplusplus
extern "C" {
#endif

	typedef void (*RPaintCallback)(Rid widget, RPainter* painter, float width, float height, void* user_data);
	typedef void (*RMeasureCallback)(Rid widget, float* width, float* height, void* user_data);

	RAVEN_API void Rcustom_set_paint_callback(Rid widget, RPaintCallback callback, void* user_data);
	/** @brief Reports the desired size of the widget. Without it the widget asks for 0 x 0. */
	RAVEN_API void Rcustom_set_measure_callback(Rid widget, RMeasureCallback callback, void* user_data);

	/** @brief Filled rectangle with rounded corners (radius 0 = sharp). */
	RAVEN_API void Rdraw_rect(RPainter* painter, float x, float y, float width, float height,
		float radius, float r, float g, float b, float a);

	/** @brief Outline of a rounded rectangle, drawn inside its bounds. */
	RAVEN_API void Rdraw_rect_outline(RPainter* painter, float x, float y, float width, float height,
		float radius, float thickness, float r, float g, float b, float a);

	RAVEN_API void Rdraw_circle(RPainter* painter, float center_x, float center_y, float radius,
		float r, float g, float b, float a);

	/** @brief Anti-aliased line with round caps. */
	RAVEN_API void Rdraw_line(RPainter* painter, float x0, float y0, float x1, float y1,
		float thickness, float r, float g, float b, float a);

	RAVEN_API void Rdraw_triangle(RPainter* painter, float x0, float y0, float x1, float y1,
		float x2, float y2, float r, float g, float b, float a);

	/** @brief Texture stretched to a rectangle and multiplied by a tint color. */
	RAVEN_API void Rdraw_image(RPainter* painter, Rid texture, float x, float y, float width, float height,
		float r, float g, float b, float a);

	/**
	 * @brief Draws UTF-8 text; (x, y) is the top-left corner of the first line.
	 * @param font R_INVALID_ID uses the theme font.
	 * @param size 0 uses R_METRIC_FONT_SIZE.
	 */
	RAVEN_API void Rdraw_text(RPainter* painter, Rid font, float size, float x, float y,
		const char* text, float r, float g, float b, float a);

	/**
	 * @brief Rounded rectangle of glass. It shows what was drawn before it,
	 * frosted and bent along its rim, under a tint: (r, g, b) is the color of
	 * the tint and `a` its strength (0 = clear glass, 1 = opaque color).
	 * Works with any theme. Needs a backend that implements glass (the Vulkan
	 * backend does).
	 */
	RAVEN_API void Rdraw_glass(RPainter* painter, float x, float y, float width, float height,
		float radius, float r, float g, float b, float a);

	/** @brief Restricts the following draws to a rectangle, intersected with the current one. */
	RAVEN_API void Rdraw_push_clip_rect(RPainter* painter, float x, float y, float width, float height);
	RAVEN_API void Rdraw_pop_clip_rect(RPainter* painter);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* RAVEN_DRAW_H */
