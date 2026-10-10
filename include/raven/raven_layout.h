/**
 * Raven
 * raven_layout.h : panels (widgets that arrange children).
 *
 * Children are added with Rwidget_add_child and placed according to their slot
 * (Rslot_set_..., raven_widget.h). This header holds the settings that belong
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

#ifndef RAVEN_LAYOUT_H
#define RAVEN_LAYOUT_H

#include "raven_widget.h"

#ifdef __cplusplus
extern "C" {
#endif

	/* VBOX, HBOX, WRAP_BOX, GRID, UNIFORM_GRID, SCROLL_BOX */

	/** @brief Gap inserted between neighbouring children. */
	RAVEN_API void Rpanel_set_spacing(Rid panel, float spacing);

	/* GRID */

	/**
	 * @brief Lets a column / row grow to take a share of the free space
	 * (Slate FillColumn / FillRow). 0 sizes it to its content (default).
	 */
	RAVEN_API void Rgrid_set_column_fill(Rid grid, int column, float coefficient);
	RAVEN_API void Rgrid_set_row_fill(Rid grid, int row, float coefficient);

	/* UNIFORM_GRID */

	/**
	 * @brief Places children automatically, left to right then top to bottom,
	 * in the given number of columns. 0 uses the grid cell of each slot (default).
	 */
	RAVEN_API void Runiform_grid_set_columns(Rid grid, int columns);
	RAVEN_API void Runiform_grid_set_min_cell_size(Rid grid, float width, float height);

	/* BORDER */

	/** @brief Space between the outline and the child. */
	RAVEN_API void Rborder_set_padding(Rid border, float left, float top, float right, float bottom);
	/** @brief Background color (R_COLOR_PANEL of this widget). Alpha 0 draws no background. */
	RAVEN_API void Rborder_set_background(Rid border, float r, float g, float b, float a);
	/** @brief Outline width and color. Width 0 draws no outline (default). */
	RAVEN_API void Rborder_set_outline(Rid border, float width, float r, float g, float b, float a);
	RAVEN_API void Rborder_set_corner_radius(Rid border, float top_left, float top_right, float bottom_right, float bottom_left);
	/** @brief Soft drop shadow behind the border. Blur 0 disables it (default). */
	RAVEN_API void Rborder_set_shadow(Rid border, float blur, float offset_x, float offset_y);
	/**
	 * @brief What the border is made of. R_MATERIAL_GLASS turns it into a
	 * panel of glass tinted by its background color (the alpha of the color
	 * is the strength of the tint): floating tool bars, side bars, cards.
	 */
	RAVEN_API void Rborder_set_material(Rid border, RMaterial material);

	/* SIZE_BOX */

	/**
	 * @brief Keeps the child at a fixed width / height ratio, as large as fits.
	 * 0 disables it (default). The fixed, minimum and maximum sizes are set
	 * with Rwidget_set_size, Rwidget_set_min_size and Rwidget_set_max_size.
	 */
	RAVEN_API void Rsize_box_set_aspect_ratio(Rid size_box, float ratio);

	/* SPACER */

	RAVEN_API void Rspacer_set_size(Rid spacer, float width, float height);

	/* SCROLL_BOX */

	/** @brief Direction in which children are stacked and scrolled. Default: R_VERTICAL. */
	RAVEN_API void Rscroll_box_set_orientation(Rid scroll_box, ROrientation orientation);
	/** @brief R_TRUE also scrolls across the stacking direction when the content is too large. */
	RAVEN_API void Rscroll_box_set_scroll_both(Rid scroll_box, int enabled);
	RAVEN_API void Rscroll_box_set_offset(Rid scroll_box, float x, float y);
	RAVEN_API void Rscroll_box_get_offset(Rid scroll_box, float* x, float* y);
	RAVEN_API void Rscroll_box_scroll_to_end(Rid scroll_box);
	/** @brief Scrolls every ScrollBox above the widget so that the widget is visible. */
	RAVEN_API void Rwidget_scroll_into_view(Rid widget);

	/* SPLITTER */

	/** @brief R_HORIZONTAL puts the panes side by side (default), R_VERTICAL stacks them. */
	RAVEN_API void Rsplitter_set_orientation(Rid splitter, ROrientation orientation);
	RAVEN_API void Rsplitter_set_min_pane_size(Rid splitter, float size);

	/* SWITCHER */

	RAVEN_API void Rswitcher_set_index(Rid switcher, int index);
	RAVEN_API int Rswitcher_get_index(Rid switcher);

	/* TAB_VIEW */

	/**
	 * @brief Adds a page. Same as Rwidget_add_child followed by Rslot_set_title.
	 * @return The index of the new tab, or -1.
	 */
	RAVEN_API int Rtab_view_add_tab(Rid tab_view, const char* title, Rid content);
	RAVEN_API void Rtab_view_set_active(Rid tab_view, int index);
	RAVEN_API int Rtab_view_get_active(Rid tab_view);
	typedef void (*RTabChangedCallback)(Rid tab_view, int index, void* user_data);
	RAVEN_API void Rtab_view_set_changed_callback(Rid tab_view, RTabChangedCallback callback, void* user_data);

	/* EXPANDER */

	RAVEN_API void Rexpander_set_title(Rid expander, const char* title);
	RAVEN_API void Rexpander_set_expanded(Rid expander, int expanded);
	RAVEN_API int Rexpander_is_expanded(Rid expander);
	typedef void (*RExpanderChangedCallback)(Rid expander, int expanded, void* user_data);
	RAVEN_API void Rexpander_set_changed_callback(Rid expander, RExpanderChangedCallback callback, void* user_data);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* RAVEN_LAYOUT_H */
