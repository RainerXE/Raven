/**
 * HUI - Horizon UI
 * hui_layout.h : panels (widgets that arrange children).
 *
 * Children are added with HUI_AddChild and placed according to their slot
 * (HUI_SetSlot..., hui_widget.h). This header holds the settings that belong
 * to the panel itself.
 *
 *   Panel          Slot values it reads
 *   -------------  ----------------------------------------------------------
 *   VBox / HBox    padding, halign, valign, auto / fill, max size
 *   Grid           padding, halign, valign, grid cell, grid span
 *   UniformGrid    padding, halign, valign, grid cell
 *   Overlay        padding, halign, valign
 *   Border         padding, halign, valign                   (single child)
 *   SizeBox        padding, halign, valign                   (single child)
 *   ScrollBox      padding, halign / valign across the scroll direction
 *   Splitter       fill coefficient = relative pane size
 *   WrapBox        padding
 *   Canvas         anchors, position, size, alignment
 *   Switcher       padding, halign, valign
 *   TabView        title, padding
 *   Expander       padding                                    (single child)
 */

#ifndef HUI_LAYOUT_H
#define HUI_LAYOUT_H

#include "hui_widget.h"

#ifdef __cplusplus
extern "C" {
#endif

	/* VBOX, HBOX, WRAP_BOX, GRID, UNIFORM_GRID, SCROLL_BOX */

	/** @brief Gap inserted between neighbouring children. */
	HUI_API void HUI_SetPanelSpacing(HUI_id panel, float spacing);

	/* GRID */

	/**
	 * @brief Lets a column / row grow to take a share of the free space
	 * (Slate FillColumn / FillRow). 0 sizes it to its content (default).
	 */
	HUI_API void HUI_SetGridColumnFill(HUI_id grid, int column, float coefficient);
	HUI_API void HUI_SetGridRowFill(HUI_id grid, int row, float coefficient);

	/* UNIFORM_GRID */

	/**
	 * @brief Places children automatically, left to right then top to bottom,
	 * in the given number of columns. 0 uses the grid cell of each slot (default).
	 */
	HUI_API void HUI_SetUniformGridColumns(HUI_id grid, int columns);
	HUI_API void HUI_SetUniformGridMinCellSize(HUI_id grid, float width, float height);

	/* BORDER */

	/** @brief Space between the outline and the child. */
	HUI_API void HUI_SetBorderPadding(HUI_id border, float left, float top, float right, float bottom);
	/** @brief Background color (HUI_COLOR_PANEL of this widget). Alpha 0 draws no background. */
	HUI_API void HUI_SetBorderBackground(HUI_id border, float r, float g, float b, float a);
	/** @brief Outline width and color. Width 0 draws no outline (default). */
	HUI_API void HUI_SetBorderOutline(HUI_id border, float width, float r, float g, float b, float a);
	HUI_API void HUI_SetBorderCornerRadius(HUI_id border, float top_left, float top_right, float bottom_right, float bottom_left);
	/** @brief Soft drop shadow behind the border. Blur 0 disables it (default). */
	HUI_API void HUI_SetBorderShadow(HUI_id border, float blur, float offset_x, float offset_y);

	/* SIZE_BOX */

	/**
	 * @brief Keeps the child at a fixed width / height ratio, as large as fits.
	 * 0 disables it (default). The fixed, minimum and maximum sizes are set
	 * with HUI_SetWidgetSize, HUI_SetWidgetMinSize and HUI_SetWidgetMaxSize.
	 */
	HUI_API void HUI_SetSizeBoxAspectRatio(HUI_id size_box, float ratio);

	/* SPACER */

	HUI_API void HUI_SetSpacerSize(HUI_id spacer, float width, float height);

	/* SCROLL_BOX */

	/** @brief Direction in which children are stacked and scrolled. Default: HUI_VERTICAL. */
	HUI_API void HUI_SetScrollBoxOrientation(HUI_id scroll_box, HUI_EOrientation orientation);
	/** @brief HUI_TRUE also scrolls across the stacking direction when the content is too large. */
	HUI_API void HUI_SetScrollBoxScrollBoth(HUI_id scroll_box, int enabled);
	HUI_API void HUI_SetScrollOffset(HUI_id scroll_box, float x, float y);
	HUI_API void HUI_GetScrollOffset(HUI_id scroll_box, float* x, float* y);
	HUI_API void HUI_ScrollToEnd(HUI_id scroll_box);
	/** @brief Scrolls every ScrollBox above the widget so that the widget is visible. */
	HUI_API void HUI_ScrollIntoView(HUI_id widget);

	/* SPLITTER */

	/** @brief HUI_HORIZONTAL puts the panes side by side (default), HUI_VERTICAL stacks them. */
	HUI_API void HUI_SetSplitterOrientation(HUI_id splitter, HUI_EOrientation orientation);
	HUI_API void HUI_SetSplitterMinPaneSize(HUI_id splitter, float size);

	/* SWITCHER */

	HUI_API void HUI_SetSwitcherIndex(HUI_id switcher, int index);
	HUI_API int HUI_GetSwitcherIndex(HUI_id switcher);

	/* TAB_VIEW */

	/**
	 * @brief Adds a page. Same as HUI_AddChild followed by HUI_SetSlotTitle.
	 * @return The index of the new tab, or -1.
	 */
	HUI_API int HUI_AddTab(HUI_id tab_view, const char* title, HUI_id content);
	HUI_API void HUI_SetActiveTab(HUI_id tab_view, int index);
	HUI_API int HUI_GetActiveTab(HUI_id tab_view);
	typedef void (*HUI_CTabChanged)(HUI_id tab_view, int index, void* user_data);
	HUI_API void HUI_SetTabChangedCallback(HUI_id tab_view, HUI_CTabChanged callback, void* user_data);

	/* EXPANDER */

	HUI_API void HUI_SetExpanderTitle(HUI_id expander, const char* title);
	HUI_API void HUI_SetExpanderExpanded(HUI_id expander, int expanded);
	HUI_API int HUI_IsExpanderExpanded(HUI_id expander);
	typedef void (*HUI_CExpanderChanged)(HUI_id expander, int expanded, void* user_data);
	HUI_API void HUI_SetExpanderChangedCallback(HUI_id expander, HUI_CExpanderChanged callback, void* user_data);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* HUI_LAYOUT_H */
