/**
 * HUI - Horizon UI
 * hui_widget.h : widget tree, slots and properties shared by every widget.
 *
 * The layout model follows Unreal's Slate:
 *   - a widget computes a desired size from its content (bottom-up pass),
 *   - a panel then arranges its children inside the space it received
 *     (top-down pass),
 *   - how a child is placed inside its parent is described by its *slot*:
 *     padding, alignment, auto / fill sizing, grid cell, canvas anchors.
 *
 * A widget has at most one parent, so the slot is stored with the child and
 * is edited with HUI_SetSlot...(child, ...). Slot values a panel does not use
 * are ignored.
 */

#ifndef HUI_WIDGET_H
#define HUI_WIDGET_H

#include "hui_core.h"
#include "hui_window.h"
#include "hui_style.h"


typedef enum HUI_EWidgetType{
	/* ---- panels (hui_layout.h) ---- */
	/** Stacks children top to bottom. Slate: SVerticalBox. */
	HUI_WIDGET_VBOX = 0x0200,
	/** Stacks children left to right. Slate: SHorizontalBox. */
	HUI_WIDGET_HBOX,
	/** Rows and columns with spans and fill coefficients. Slate: SGridPanel. */
	HUI_WIDGET_GRID,
	/** Grid whose cells all have the same size. Slate: SUniformGridPanel. */
	HUI_WIDGET_UNIFORM_GRID,
	/** Draws children on top of each other. Slate: SOverlay. */
	HUI_WIDGET_OVERLAY,
	/** One child with a background, an outline and inner padding. Slate: SBorder. */
	HUI_WIDGET_BORDER,
	/** One child with size constraints or a fixed aspect ratio. Slate: SBox. */
	HUI_WIDGET_SIZE_BOX,
	/** Empty space. Slate: SSpacer. */
	HUI_WIDGET_SPACER,
	/** Scrollable stack of children. Slate: SScrollBox. */
	HUI_WIDGET_SCROLL_BOX,
	/** Children separated by draggable handles. Slate: SSplitter. */
	HUI_WIDGET_SPLITTER,
	/** Flows children and wraps to the next line. Slate: SWrapBox. */
	HUI_WIDGET_WRAP_BOX,
	/** Children placed with anchors and offsets. Slate: SConstraintCanvas. */
	HUI_WIDGET_CANVAS,
	/** Shows one child at a time. Slate: SWidgetSwitcher. */
	HUI_WIDGET_SWITCHER,
	/** Tab bar + one page per child. */
	HUI_WIDGET_TAB_VIEW,
	/** Collapsible header + one child. Slate: SExpandableArea. */
	HUI_WIDGET_EXPANDER,

	/* ---- controls (hui_controls.h) ---- */
	HUI_WIDGET_LABEL,
	HUI_WIDGET_BUTTON,
	HUI_WIDGET_CHECKBOX,
	HUI_WIDGET_SLIDER,
	HUI_WIDGET_PROGRESSBAR,
	HUI_WIDGET_IMAGE,
	HUI_WIDGET_TEXT_INPUT,
	HUI_WIDGET_SPIN_BOX,
	HUI_WIDGET_COMBO_BOX,
	HUI_WIDGET_SEPARATOR,
	HUI_WIDGET_LIST_VIEW,
	HUI_WIDGET_TREE_VIEW,
	HUI_WIDGET_MENU_BAR,
	HUI_WIDGET_MENU,
	HUI_WIDGET_MENU_ITEM,

	/* ---- hui_viewport.h / hui_draw.h ---- */
	/** Area showing content rendered by the application. */
	HUI_WIDGET_VIEWPORT,
	/** Widget painted and measured by application callbacks. */
	HUI_WIDGET_CUSTOM,

	HUI_WIDGET_LAST_
}HUI_EWidgetType;

typedef enum HUI_EHAlign{
	HUI_HALIGN_FILL = 0x0240,
	HUI_HALIGN_LEFT,
	HUI_HALIGN_CENTER,
	HUI_HALIGN_RIGHT
}HUI_EHAlign;

typedef enum HUI_EVAlign{
	HUI_VALIGN_FILL = 0x0250,
	HUI_VALIGN_TOP,
	HUI_VALIGN_CENTER,
	HUI_VALIGN_BOTTOM
}HUI_EVAlign;

typedef enum HUI_EVisibility{
	/** Drawn and laid out. */
	HUI_VISIBLE = 0x0260,
	/** Not drawn, but still takes its space in the layout. */
	HUI_HIDDEN,
	/** Not drawn and takes no space. */
	HUI_COLLAPSED
}HUI_EVisibility;

typedef enum HUI_EOrientation{
	HUI_HORIZONTAL = 0x0270,
	HUI_VERTICAL
}HUI_EOrientation;


#ifdef __cplusplus
extern "C" {
#endif

	/* ============================================================================
	 *  CREATION & TREE
	 * ============================================================================ */

	/**
	 * @brief Creates a widget. It is not shown until it is added to a parent
	 * that belongs to a window (HUI_AddChild, HUI_SetWindowContent).
	 */
	HUI_API HUI_id HUI_CreateWidget(HUI_EWidgetType type);

	/** @brief Deletes a widget and all its descendants. */
	HUI_API void HUI_DeleteWidget(HUI_id widget);
	HUI_API int HUI_IsValidWidget(HUI_id widget);
	HUI_API HUI_EWidgetType HUI_GetWidgetType(HUI_id widget);

	/**
	 * @brief Appends a child to a panel. The parent takes ownership.
	 * A widget that already has a parent is moved.
	 * @return `child`, so calls can be nested:
	 *         HUI_id ok = HUI_AddChild(row, HUI_CreateButton("OK"));
	 */
	HUI_API HUI_id HUI_AddChild(HUI_id parent, HUI_id child);
	HUI_API HUI_id HUI_InsertChild(HUI_id parent, HUI_id child, int index);

	/** @brief Detaches a child without deleting it. The caller owns it again. */
	HUI_API void HUI_RemoveChild(HUI_id parent, HUI_id child);

	/** @brief Deletes every child of a panel. */
	HUI_API void HUI_ClearChildren(HUI_id parent);

	HUI_API HUI_id HUI_GetParent(HUI_id widget);
	HUI_API int HUI_GetChildCount(HUI_id widget);
	HUI_API HUI_id HUI_GetChild(HUI_id widget, int index);
	/** @return The window showing the widget, or HUI_INVALID_ID. */
	HUI_API HUI_id HUI_GetWidgetWindow(HUI_id widget);

	HUI_API void HUI_SetWidgetName(HUI_id widget, const char* name);
	HUI_API const char* HUI_GetWidgetName(HUI_id widget);

	/**
	 * @brief Finds a widget by name.
	 * @param scope A window or a widget; the search covers its whole subtree.
	 * @return HUI_INVALID_ID when no widget has that name.
	 */
	HUI_API HUI_id HUI_FindWidget(HUI_id scope, const char* name);


	/* ============================================================================
	 *  SLOT (how a child is placed by its parent)
	 * ============================================================================ */

	/** @brief Space kept around the child, inside its slot. All panels. */
	HUI_API void HUI_SetSlotPadding(HUI_id child, float left, float top, float right, float bottom);

	/** @brief Alignment of the child inside its slot. Default: fill. All panels. */
	HUI_API void HUI_SetSlotHAlign(HUI_id child, HUI_EHAlign align);
	HUI_API void HUI_SetSlotVAlign(HUI_id child, HUI_EVAlign align);

	/**
	 * @brief The slot takes the desired size of the child along the panel
	 * direction (Slate AutoWidth / AutoHeight). This is the default.
	 * VBox, HBox.
	 */
	HUI_API void HUI_SetSlotAuto(HUI_id child);

	/**
	 * @brief The slot shares the space left by the auto slots, in proportion
	 * to `coefficient` (Slate FillWidth / FillHeight). VBox, HBox.
	 * In a Splitter, the coefficient is the relative size of the pane.
	 */
	HUI_API void HUI_SetSlotFill(HUI_id child, float coefficient);

	/** @brief Upper bound of the slot along the panel direction. 0 = none. VBox, HBox. */
	HUI_API void HUI_SetSlotMaxSize(HUI_id child, float max_size);

	/** @brief Cell of the child. Grid, UniformGrid. */
	HUI_API void HUI_SetSlotGrid(HUI_id child, int column, int row);
	HUI_API void HUI_SetSlotGridSpan(HUI_id child, int column_span, int row_span);

	/**
	 * Canvas placement. Anchors are fractions of the canvas size (0..1).
	 *   - anchors equal on an axis: the child is at anchor + position, with
	 *     the slot size (or its desired size when the slot size is <= 0);
	 *   - anchors different on an axis: the child stretches between them,
	 *     position and size are then the near and far margins.
	 * Alignment is the pivot of the child (0,0 = top-left, 0.5,0.5 = center).
	 */
	HUI_API void HUI_SetSlotAnchors(HUI_id child, float min_x, float min_y, float max_x, float max_y);
	HUI_API void HUI_SetSlotPosition(HUI_id child, float x, float y);
	HUI_API void HUI_SetSlotSize(HUI_id child, float width, float height);
	HUI_API void HUI_SetSlotAlignment(HUI_id child, float x, float y);

	/** @brief Title of the tab showing the child. TabView. */
	HUI_API void HUI_SetSlotTitle(HUI_id child, const char* title);


	/* ============================================================================
	 *  COMMON PROPERTIES
	 * ============================================================================ */

	HUI_API void HUI_SetWidgetVisibility(HUI_id widget, HUI_EVisibility visibility);
	HUI_API HUI_EVisibility HUI_GetWidgetVisibility(HUI_id widget);

	/** @brief A disabled widget ignores input and is drawn dimmed, with its descendants. */
	HUI_API void HUI_SetWidgetEnabled(HUI_id widget, int enabled);
	HUI_API int HUI_IsWidgetEnabled(HUI_id widget);

	/** @brief HUI_FALSE lets mouse events pass through the widget (not its children). */
	HUI_API void HUI_SetWidgetHitTestable(HUI_id widget, int hit_testable);

	/** @brief Opacity multiplier applied to the widget and its descendants (0..1). */
	HUI_API void HUI_SetWidgetOpacity(HUI_id widget, float opacity);

	/** @brief HUI_TRUE clips the descendants to the bounds of the widget. */
	HUI_API void HUI_SetWidgetClipping(HUI_id widget, int clip);

	/** @brief Text shown when the cursor rests on the widget. NULL removes it. */
	HUI_API void HUI_SetWidgetTooltip(HUI_id widget, const char* text);

	HUI_API void HUI_SetWidgetCursor(HUI_id widget, HUI_ECursor cursor);

	/**
	 * Size constraints, in logical units. They apply to the desired size the
	 * widget reports to its parent. A negative value removes the constraint
	 * on that axis.
	 */
	HUI_API void HUI_SetWidgetSize(HUI_id widget, float width, float height);
	HUI_API void HUI_SetWidgetMinSize(HUI_id widget, float width, float height);
	HUI_API void HUI_SetWidgetMaxSize(HUI_id widget, float width, float height);

	/** @brief Bounds of the widget in its window, in logical units, after the last layout. */
	HUI_API void HUI_GetWidgetRect(HUI_id widget, float* x, float* y, float* width, float* height);
	HUI_API void HUI_GetWidgetDesiredSize(HUI_id widget, float* width, float* height);

	HUI_API int HUI_IsWidgetHovered(HUI_id widget);

	HUI_API void HUI_SetWidgetFocusable(HUI_id widget, int focusable);
	/** @brief Gives the keyboard focus to the widget. */
	HUI_API void HUI_FocusWidget(HUI_id widget);
	HUI_API int HUI_IsWidgetFocused(HUI_id widget);

	HUI_API void HUI_SetWidgetUserHandle(HUI_id widget, void* handle);
	HUI_API void* HUI_GetWidgetUserHandle(HUI_id widget);

	/**
	 * @brief Receives the input events of a widget before the widget itself.
	 * Return HUI_TRUE to consume the event (the widget and its parents will
	 * not see it), HUI_FALSE to let it continue.
	 */
	typedef int (*HUI_CWidgetEvent)(HUI_id widget, const HUI_Event* event, void* user_data);
	HUI_API void HUI_SetWidgetEventCallback(HUI_id widget, HUI_CWidgetEvent callback, void* user_data);

	/**
	 * @brief Name of the action fired by the main event of the widget: button
	 * click, value change, text commit, item activation (see HUI_BindAction).
	 */
	HUI_API void HUI_SetWidgetAction(HUI_id widget, const char* action);
	HUI_API const char* HUI_GetWidgetAction(HUI_id widget);

	/** @brief Menu opened by a right click on the widget. The widget does not own it. */
	HUI_API void HUI_SetWidgetContextMenu(HUI_id widget, HUI_id menu);

	/** @brief Asks for the widget to be laid out and painted again. */
	HUI_API void HUI_InvalidateWidget(HUI_id widget);


	/* ============================================================================
	 *  STYLE OVERRIDES
	 * ============================================================================ */

	/**
	 * @brief Overrides a theme color for one widget. HUI_COLOR_TEXT is also
	 * inherited by its descendants.
	 */
	HUI_API void HUI_SetWidgetColor(HUI_id widget, HUI_EColor role, float r, float g, float b, float a);
	HUI_API void HUI_ClearWidgetColor(HUI_id widget, HUI_EColor role);

	/** @brief Overrides a theme metric for one widget. */
	HUI_API void HUI_SetWidgetMetric(HUI_id widget, HUI_EMetric metric, float value);
	HUI_API void HUI_ClearWidgetMetric(HUI_id widget, HUI_EMetric metric);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* HUI_WIDGET_H */
