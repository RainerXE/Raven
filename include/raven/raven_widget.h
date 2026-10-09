/**
 * Raven
 * raven_widget.h : widget tree, slots and properties shared by every widget.
 *
 * The layout model follows Unreal's Slate:
 *   - a widget computes a desired size from its content (bottom-up pass),
 *   - a panel then arranges its children inside the space it received
 *     (top-down pass),
 *   - how a child is placed inside its parent is described by its *slot*:
 *     padding, alignment, auto / fill sizing, grid cell, canvas anchors.
 *
 * A widget has at most one parent, so the slot is stored with the child and
 * is edited with Rslot_set_...(child, ...). Slot values a panel does not use
 * are ignored.
 */

#ifndef RAVEN_WIDGET_H
#define RAVEN_WIDGET_H

#include "raven_core.h"
#include "raven_window.h"
#include "raven_style.h"


typedef enum RWidgetType{
	/* ---- panels (raven_layout.h) ---- */
	/** Stacks children top to bottom. Slate: SVerticalBox. */
	R_WIDGET_VBOX = 0x0200,
	/** Stacks children left to right. Slate: SHorizontalBox. */
	R_WIDGET_HBOX,
	/** Rows and columns with spans and fill coefficients. Slate: SGridPanel. */
	R_WIDGET_GRID,
	/** Grid whose cells all have the same size. Slate: SUniformGridPanel. */
	R_WIDGET_UNIFORM_GRID,
	/** Draws children on top of each other. Slate: SOverlay. */
	R_WIDGET_OVERLAY,
	/** One child with a background, an outline and inner padding. Slate: SBorder. */
	R_WIDGET_BORDER,
	/** One child with size constraints or a fixed aspect ratio. Slate: SBox. */
	R_WIDGET_SIZE_BOX,
	/** Empty space. Slate: SSpacer. */
	R_WIDGET_SPACER,
	/** Scrollable stack of children. Slate: SScrollBox. */
	R_WIDGET_SCROLL_BOX,
	/** Children separated by draggable handles. Slate: SSplitter. */
	R_WIDGET_SPLITTER,
	/** Flows children and wraps to the next line. Slate: SWrapBox. */
	R_WIDGET_WRAP_BOX,
	/** Children placed with anchors and offsets. Slate: SConstraintCanvas. */
	R_WIDGET_CANVAS,
	/** Shows one child at a time. Slate: SWidgetSwitcher. */
	R_WIDGET_SWITCHER,
	/** Tab bar + one page per child. */
	R_WIDGET_TAB_VIEW,
	/** Collapsible header + one child. Slate: SExpandableArea. */
	R_WIDGET_EXPANDER,

	/* ---- controls (raven_controls.h) ---- */
	R_WIDGET_LABEL,
	R_WIDGET_BUTTON,
	R_WIDGET_CHECKBOX,
	R_WIDGET_SLIDER,
	R_WIDGET_PROGRESSBAR,
	R_WIDGET_IMAGE,
	R_WIDGET_TEXT_INPUT,
	R_WIDGET_SPIN_BOX,
	R_WIDGET_COMBO_BOX,
	R_WIDGET_SEPARATOR,
	R_WIDGET_LIST_VIEW,
	R_WIDGET_TREE_VIEW,
	R_WIDGET_MENU_BAR,
	R_WIDGET_MENU,
	R_WIDGET_MENU_ITEM,

	/* ---- raven_viewport.h / raven_draw.h ---- */
	/** Area showing content rendered by the application. */
	R_WIDGET_VIEWPORT,
	/** Widget painted and measured by application callbacks. */
	R_WIDGET_CUSTOM,

	R_WIDGET_LAST_
}RWidgetType;

typedef enum RHAlign{
	R_HALIGN_FILL = 0x0240,
	R_HALIGN_LEFT,
	R_HALIGN_CENTER,
	R_HALIGN_RIGHT
}RHAlign;

typedef enum RVAlign{
	R_VALIGN_FILL = 0x0250,
	R_VALIGN_TOP,
	R_VALIGN_CENTER,
	R_VALIGN_BOTTOM
}RVAlign;

typedef enum RVisibility{
	/** Drawn and laid out. */
	R_VISIBLE = 0x0260,
	/** Not drawn, but still takes its space in the layout. */
	R_HIDDEN,
	/** Not drawn and takes no space. */
	R_COLLAPSED
}RVisibility;

typedef enum ROrientation{
	R_HORIZONTAL = 0x0270,
	R_VERTICAL
}ROrientation;


#ifdef __cplusplus
extern "C" {
#endif

	/* ============================================================================
	 *  CREATION & TREE
	 * ============================================================================ */

	/**
	 * @brief Creates a widget. It is not shown until it is added to a parent
	 * that belongs to a window (Rwidget_add_child, Rwindow_set_content).
	 */
	RAVEN_API Rid Rwidget_create(RWidgetType type);

	/** @brief Deletes a widget and all its descendants. */
	RAVEN_API void Rwidget_destroy(Rid widget);
	RAVEN_API int Rwidget_is_valid(Rid widget);
	RAVEN_API RWidgetType Rwidget_get_type(Rid widget);

	/**
	 * @brief Appends a child to a panel. The parent takes ownership.
	 * A widget that already has a parent is moved.
	 * @return `child`, so calls can be nested:
	 *         Rid ok = Rwidget_add_child(row, Rbutton_create("OK"));
	 */
	RAVEN_API Rid Rwidget_add_child(Rid parent, Rid child);
	RAVEN_API Rid Rwidget_insert_child(Rid parent, Rid child, int index);

	/** @brief Detaches a child without deleting it. The caller owns it again. */
	RAVEN_API void Rwidget_remove_child(Rid parent, Rid child);

	/** @brief Deletes every child of a panel. */
	RAVEN_API void Rwidget_clear_children(Rid parent);

	RAVEN_API Rid Rwidget_get_parent(Rid widget);
	RAVEN_API int Rwidget_get_child_count(Rid widget);
	RAVEN_API Rid Rwidget_get_child(Rid widget, int index);
	/** @return The window showing the widget, or R_INVALID_ID. */
	RAVEN_API Rid Rwidget_get_window(Rid widget);

	RAVEN_API void Rwidget_set_name(Rid widget, const char* name);
	RAVEN_API const char* Rwidget_get_name(Rid widget);

	/**
	 * @brief Finds a widget by name.
	 * @param scope A window or a widget; the search covers its whole subtree.
	 * @return R_INVALID_ID when no widget has that name.
	 */
	RAVEN_API Rid Rwidget_find(Rid scope, const char* name);


	/* ============================================================================
	 *  SLOT (how a child is placed by its parent)
	 * ============================================================================ */

	/** @brief Space kept around the child, inside its slot. All panels. */
	RAVEN_API void Rslot_set_padding(Rid child, float left, float top, float right, float bottom);

	/** @brief Alignment of the child inside its slot. Default: fill. All panels. */
	RAVEN_API void Rslot_set_halign(Rid child, RHAlign align);
	RAVEN_API void Rslot_set_valign(Rid child, RVAlign align);

	/**
	 * @brief The slot takes the desired size of the child along the panel
	 * direction (Slate AutoWidth / AutoHeight). This is the default.
	 * VBox, HBox.
	 */
	RAVEN_API void Rslot_set_auto(Rid child);

	/**
	 * @brief The slot shares the space left by the auto slots, in proportion
	 * to `coefficient` (Slate FillWidth / FillHeight). VBox, HBox.
	 * In a Splitter, the coefficient is the relative size of the pane.
	 */
	RAVEN_API void Rslot_set_fill(Rid child, float coefficient);

	/** @brief Upper bound of the slot along the panel direction. 0 = none. VBox, HBox. */
	RAVEN_API void Rslot_set_max_size(Rid child, float max_size);

	/** @brief Cell of the child. Grid, UniformGrid. */
	RAVEN_API void Rslot_set_grid(Rid child, int column, int row);
	RAVEN_API void Rslot_set_grid_span(Rid child, int column_span, int row_span);

	/**
	 * Canvas placement. Anchors are fractions of the canvas size (0..1).
	 *   - anchors equal on an axis: the child is at anchor + position, with
	 *     the slot size (or its desired size when the slot size is <= 0);
	 *   - anchors different on an axis: the child stretches between them,
	 *     position and size are then the near and far margins.
	 * Alignment is the pivot of the child (0,0 = top-left, 0.5,0.5 = center).
	 */
	RAVEN_API void Rslot_set_anchors(Rid child, float min_x, float min_y, float max_x, float max_y);
	RAVEN_API void Rslot_set_position(Rid child, float x, float y);
	RAVEN_API void Rslot_set_size(Rid child, float width, float height);
	RAVEN_API void Rslot_set_alignment(Rid child, float x, float y);

	/** @brief Title of the tab showing the child. TabView. */
	RAVEN_API void Rslot_set_title(Rid child, const char* title);


	/* ============================================================================
	 *  COMMON PROPERTIES
	 * ============================================================================ */

	RAVEN_API void Rwidget_set_visibility(Rid widget, RVisibility visibility);
	RAVEN_API RVisibility Rwidget_get_visibility(Rid widget);

	/** @brief A disabled widget ignores input and is drawn dimmed, with its descendants. */
	RAVEN_API void Rwidget_set_enabled(Rid widget, int enabled);
	RAVEN_API int Rwidget_is_enabled(Rid widget);

	/** @brief R_FALSE lets mouse events pass through the widget (not its children). */
	RAVEN_API void Rwidget_set_hit_testable(Rid widget, int hit_testable);

	/** @brief Opacity multiplier applied to the widget and its descendants (0..1). */
	RAVEN_API void Rwidget_set_opacity(Rid widget, float opacity);

	/** @brief R_TRUE clips the descendants to the bounds of the widget. */
	RAVEN_API void Rwidget_set_clipping(Rid widget, int clip);

	/** @brief Text shown when the cursor rests on the widget. NULL removes it. */
	RAVEN_API void Rwidget_set_tooltip(Rid widget, const char* text);

	RAVEN_API void Rwidget_set_cursor(Rid widget, RCursor cursor);

	/**
	 * Size constraints, in logical units. They apply to the desired size the
	 * widget reports to its parent. A negative value removes the constraint
	 * on that axis.
	 */
	RAVEN_API void Rwidget_set_size(Rid widget, float width, float height);
	RAVEN_API void Rwidget_set_min_size(Rid widget, float width, float height);
	RAVEN_API void Rwidget_set_max_size(Rid widget, float width, float height);

	/** @brief Bounds of the widget in its window, in logical units, after the last layout. */
	RAVEN_API void Rwidget_get_rect(Rid widget, float* x, float* y, float* width, float* height);
	RAVEN_API void Rwidget_get_desired_size(Rid widget, float* width, float* height);

	RAVEN_API int Rwidget_is_hovered(Rid widget);

	RAVEN_API void Rwidget_set_focusable(Rid widget, int focusable);
	/** @brief Gives the keyboard focus to the widget. */
	RAVEN_API void Rwidget_focus(Rid widget);
	RAVEN_API int Rwidget_is_focused(Rid widget);

	RAVEN_API void Rwidget_set_user_handle(Rid widget, void* handle);
	RAVEN_API void* Rwidget_get_user_handle(Rid widget);

	/**
	 * @brief Receives the input events of a widget before the widget itself.
	 * Return R_TRUE to consume the event (the widget and its parents will
	 * not see it), R_FALSE to let it continue.
	 */
	typedef int (*RWidgetEventCallback)(Rid widget, const REvent* event, void* user_data);
	RAVEN_API void Rwidget_set_event_callback(Rid widget, RWidgetEventCallback callback, void* user_data);

	/**
	 * @brief Name of the action fired by the main event of the widget: button
	 * click, value change, text commit, item activation (see Raction_bind).
	 */
	RAVEN_API void Rwidget_set_action(Rid widget, const char* action);
	RAVEN_API const char* Rwidget_get_action(Rid widget);

	/** @brief Menu opened by a right click on the widget. The widget does not own it. */
	RAVEN_API void Rwidget_set_context_menu(Rid widget, Rid menu);

	/** @brief Asks for the widget to be laid out and painted again. */
	RAVEN_API void Rwidget_invalidate(Rid widget);


	/* ============================================================================
	 *  STYLE OVERRIDES
	 * ============================================================================ */

	/**
	 * @brief Overrides a theme color for one widget. R_COLOR_TEXT is also
	 * inherited by its descendants.
	 */
	RAVEN_API void Rwidget_set_color(Rid widget, RColorRole role, float r, float g, float b, float a);
	RAVEN_API void Rwidget_clear_color(Rid widget, RColorRole role);

	/** @brief Overrides a theme metric for one widget. */
	RAVEN_API void Rwidget_set_metric(Rid widget, RMetric metric, float value);
	RAVEN_API void Rwidget_clear_metric(Rid widget, RMetric metric);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* RAVEN_WIDGET_H */
