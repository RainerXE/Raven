/**
 * HUI - Horizon UI
 * hui_style.h : theme (colors, metrics, default font).
 *
 * Every widget reads its colors and sizes from the global theme. Any value can
 * be overridden for a single widget with HUI_SetWidgetColor and
 * HUI_SetWidgetMetric (hui_widget.h).
 */

#ifndef HUI_STYLE_H
#define HUI_STYLE_H

#include "hui_core.h"


typedef enum HUI_ETheme{
	HUI_THEME_DARK = 0x00E0,
	HUI_THEME_LIGHT
}HUI_ETheme;

/** Color roles. The comment lists the widgets reading each role. */
typedef enum HUI_EColor{
	/** Window clear color. */
	HUI_COLOR_WINDOW = 0x0100,
	/** Border background. */
	HUI_COLOR_PANEL,
	/** Border outline, list and input outlines. */
	HUI_COLOR_BORDER,
	/** Text. Inherited by the children of the widget it is set on. */
	HUI_COLOR_TEXT,
	HUI_COLOR_TEXT_DISABLED,
	HUI_COLOR_TEXT_PLACEHOLDER,
	/** Primary buttons, checked boxes, slider fill, progress fill. */
	HUI_COLOR_ACCENT,
	HUI_COLOR_ACCENT_HOVERED,
	HUI_COLOR_ACCENT_PRESSED,
	/** Text and marks drawn on top of the accent color. */
	HUI_COLOR_ACCENT_TEXT,
	/** Button, combo box, check box background. */
	HUI_COLOR_CONTROL,
	HUI_COLOR_CONTROL_HOVERED,
	HUI_COLOR_CONTROL_PRESSED,
	HUI_COLOR_CONTROL_BORDER,
	/** Text input, spin box, list and tree background. */
	HUI_COLOR_INPUT,
	/** Keyboard focus outline. */
	HUI_COLOR_FOCUS,
	/** Selected rows and selected text. */
	HUI_COLOR_SELECTION,
	HUI_COLOR_ROW_HOVERED,
	/** Slider track, progress background, scrollbar track. */
	HUI_COLOR_TRACK,
	/** Scrollbar thumb, slider handle. */
	HUI_COLOR_HANDLE,
	HUI_COLOR_HANDLE_HOVERED,
	HUI_COLOR_SEPARATOR,
	/** Menu bar, tab bar, expander header. */
	HUI_COLOR_HEADER,
	HUI_COLOR_HEADER_HOVERED,
	/** Active tab. */
	HUI_COLOR_TAB_ACTIVE,
	/** Menus, combo box lists. */
	HUI_COLOR_POPUP,
	HUI_COLOR_POPUP_BORDER,
	HUI_COLOR_SHADOW,
	HUI_COLOR_TOOLTIP,
	HUI_COLOR_TOOLTIP_TEXT,
	HUI_COLOR_LAST_
}HUI_EColor;

#define HUI_COLOR_COUNT ((int)HUI_COLOR_LAST_ - (int)HUI_COLOR_WINDOW)

/** Sizes, in logical units. */
typedef enum HUI_EMetric{
	HUI_METRIC_FONT_SIZE = 0x0140,
	HUI_METRIC_CORNER_RADIUS,
	HUI_METRIC_BORDER_WIDTH,
	/** Height of buttons, inputs, combo boxes, tabs, menu bars. */
	HUI_METRIC_CONTROL_HEIGHT,
	/** Horizontal / vertical padding inside controls. */
	HUI_METRIC_PADDING_X,
	HUI_METRIC_PADDING_Y,
	/** Gap between an icon or a check mark and its text. */
	HUI_METRIC_SPACING,
	HUI_METRIC_SCROLLBAR_SIZE,
	/** Height of list, tree and menu rows. */
	HUI_METRIC_ROW_HEIGHT,
	/** Indentation per tree level. */
	HUI_METRIC_INDENT,
	HUI_METRIC_SPLITTER_SIZE,
	HUI_METRIC_CHECK_SIZE,
	HUI_METRIC_SLIDER_HANDLE_SIZE,
	HUI_METRIC_LAST_
}HUI_EMetric;

#define HUI_METRIC_COUNT ((int)HUI_METRIC_LAST_ - (int)HUI_METRIC_FONT_SIZE)


#ifdef __cplusplus
extern "C" {
#endif

	/** @brief Replaces every theme color with a built-in palette. */
	HUI_API void HUI_ApplyTheme(HUI_ETheme theme);

	HUI_API void HUI_SetThemeColor(HUI_EColor role, float r, float g, float b, float a);
	HUI_API void HUI_GetThemeColor(HUI_EColor role, float* r, float* g, float* b, float* a);

	HUI_API void HUI_SetThemeMetric(HUI_EMetric metric, float value);
	HUI_API float HUI_GetThemeMetric(HUI_EMetric metric);

	/** @brief Font used by widgets that have no font of their own. */
	HUI_API void HUI_SetThemeFont(HUI_id font);
	HUI_API HUI_id HUI_GetThemeFont(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* HUI_STYLE_H */
