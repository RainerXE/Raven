/**
 * Raven
 * raven_style.h : theme (colors, metrics, default font).
 *
 * Every widget reads its colors and sizes from the global theme. Any value can
 * be overridden for a single widget with Rwidget_set_color and
 * Rwidget_set_metric (raven_widget.h).
 */

#ifndef RAVEN_STYLE_H
#define RAVEN_STYLE_H

#include "raven_core.h"


typedef enum RTheme{
	/** Flat, dense themes for tools. */
	R_THEME_DARK = 0x00E0,
	R_THEME_LIGHT,
	/**
	 * Glass themes: controls are capsules of translucent glass that refract
	 * and blur what is behind them, catch the light on their rim and react
	 * to the pointer with short animations. Windows get a soft colored
	 * background unless they have their own (Rwindow_set_background,
	 * Rwindow_set_clear_color).
	 */
	R_THEME_GLASS_DARK,
	R_THEME_GLASS_LIGHT
}RTheme;

/** Color roles. The comment lists the widgets reading each role. */
typedef enum RColorRole{
	/** Window clear color. */
	R_COLOR_WINDOW = 0x0100,
	/** Border background. */
	R_COLOR_PANEL,
	/** Border outline, list and input outlines. */
	R_COLOR_BORDER,
	/** Text. Inherited by the children of the widget it is set on. */
	R_COLOR_TEXT,
	R_COLOR_TEXT_DISABLED,
	R_COLOR_TEXT_PLACEHOLDER,
	/** Primary buttons, checked boxes, slider fill, progress fill. */
	R_COLOR_ACCENT,
	R_COLOR_ACCENT_HOVERED,
	R_COLOR_ACCENT_PRESSED,
	/** Text and marks drawn on top of the accent color. */
	R_COLOR_ACCENT_TEXT,
	/** Button, combo box, check box background. */
	R_COLOR_CONTROL,
	R_COLOR_CONTROL_HOVERED,
	R_COLOR_CONTROL_PRESSED,
	R_COLOR_CONTROL_BORDER,
	/** Text input, spin box, list and tree background. */
	R_COLOR_INPUT,
	/** Keyboard focus outline. */
	R_COLOR_FOCUS,
	/** Selected rows and selected text. */
	R_COLOR_SELECTION,
	R_COLOR_ROW_HOVERED,
	/** Slider track, progress background, scrollbar track. */
	R_COLOR_TRACK,
	/** Scrollbar thumb, slider handle. */
	R_COLOR_HANDLE,
	R_COLOR_HANDLE_HOVERED,
	R_COLOR_SEPARATOR,
	/** Menu bar, tab bar, expander header. */
	R_COLOR_HEADER,
	R_COLOR_HEADER_HOVERED,
	/** Active tab. */
	R_COLOR_TAB_ACTIVE,
	/** Menus, combo box lists. */
	R_COLOR_POPUP,
	R_COLOR_POPUP_BORDER,
	R_COLOR_SHADOW,
	R_COLOR_TOOLTIP,
	R_COLOR_TOOLTIP_TEXT,
	/** Track of a switch that is on. */
	R_COLOR_SWITCH,
	R_COLOR_LAST_
}RColorRole;

#define R_COLOR_COUNT ((int)R_COLOR_LAST_ - (int)R_COLOR_WINDOW)

/** Sizes, in logical units. */
typedef enum RMetric{
	R_METRIC_FONT_SIZE = 0x0140,
	R_METRIC_CORNER_RADIUS,
	R_METRIC_BORDER_WIDTH,
	/** Height of buttons, inputs, combo boxes, tabs, menu bars. */
	R_METRIC_CONTROL_HEIGHT,
	/** Horizontal / vertical padding inside controls. */
	R_METRIC_PADDING_X,
	R_METRIC_PADDING_Y,
	/** Gap between an icon or a check mark and its text. */
	R_METRIC_SPACING,
	R_METRIC_SCROLLBAR_SIZE,
	/** Height of list, tree and menu rows. */
	R_METRIC_ROW_HEIGHT,
	/** Indentation per tree level. */
	R_METRIC_INDENT,
	R_METRIC_SPLITTER_SIZE,
	R_METRIC_CHECK_SIZE,
	R_METRIC_SLIDER_HANDLE_SIZE,
	/** Corner radius of menus, drop-down lists and tooltips. */
	R_METRIC_POPUP_RADIUS,
	R_METRIC_LAST_
}RMetric;

#define R_METRIC_COUNT ((int)R_METRIC_LAST_ - (int)R_METRIC_FONT_SIZE)


#ifdef __cplusplus
extern "C" {
#endif

	/**
	 * @brief Switches to a built-in theme: replaces every theme color and
	 * metric. Colors and metrics changed with Rtheme_set_color and
	 * Rtheme_set_metric are lost; per-widget overrides are kept.
	 */
	RAVEN_API void Rtheme_apply(RTheme theme);

	RAVEN_API void Rtheme_set_color(RColorRole role, float r, float g, float b, float a);
	RAVEN_API void Rtheme_get_color(RColorRole role, float* r, float* g, float* b, float* a);

	RAVEN_API void Rtheme_set_metric(RMetric metric, float value);
	RAVEN_API float Rtheme_get_metric(RMetric metric);

	/** @brief Font used by widgets that have no font of their own. */
	RAVEN_API void Rtheme_set_font(Rid font);
	RAVEN_API Rid Rtheme_get_font(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* RAVEN_STYLE_H */
