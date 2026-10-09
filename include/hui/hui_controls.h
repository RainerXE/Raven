/**
 * HUI - Horizon UI
 * hui_controls.h : interactive widgets and their settings.
 *
 * Every control is created with HUI_CreateWidget(type). Its colors and sizes
 * come from the theme and can be overridden per widget (HUI_SetWidgetColor,
 * HUI_SetWidgetMetric). Its main event can be received either through the
 * typed callback declared here, or by name through an action
 * (HUI_SetWidgetAction + HUI_BindAction).
 */

#ifndef HUI_CONTROLS_H
#define HUI_CONTROLS_H

#include "hui_widget.h"


typedef enum HUI_ETextAlign{
	HUI_TEXT_ALIGN_LEFT = 0x0280,
	HUI_TEXT_ALIGN_CENTER,
	HUI_TEXT_ALIGN_RIGHT
}HUI_ETextAlign;

typedef enum HUI_EButtonStyle{
	/** Filled with HUI_COLOR_CONTROL. */
	HUI_BUTTON_DEFAULT = 0x0290,
	/** Filled with HUI_COLOR_ACCENT. */
	HUI_BUTTON_PRIMARY,
	/** No background until hovered (toolbars). */
	HUI_BUTTON_FLAT
}HUI_EButtonStyle;

typedef enum HUI_ECheckboxStyle{
	HUI_CHECKBOX_CHECK = 0x02A0,
	/** Round mark. Checking one unchecks the other radio boxes of the same parent. */
	HUI_CHECKBOX_RADIO,
	/** On / off switch. */
	HUI_CHECKBOX_SWITCH
}HUI_ECheckboxStyle;

typedef enum HUI_EStretch{
	/** Stretches the image to the widget, ignoring its aspect ratio. */
	HUI_STRETCH_FILL = 0x02B0,
	/** Largest size that fits entirely, keeping the aspect ratio. */
	HUI_STRETCH_FIT,
	/** Smallest size that covers the widget, keeping the aspect ratio (cropped). */
	HUI_STRETCH_COVER,
	/** Original size, centered. */
	HUI_STRETCH_NONE
}HUI_EStretch;


#ifdef __cplusplus
extern "C" {
#endif

	/* ============================================================================
	 *  LABEL
	 * ============================================================================ */

	/** @brief Shortcut for HUI_CreateWidget(HUI_WIDGET_LABEL) + HUI_SetLabelText. */
	HUI_API HUI_id HUI_CreateLabel(const char* text);
	/** @brief Sets the text. '\n' starts a new line. */
	HUI_API void HUI_SetLabelText(HUI_id label, const char* text);
	HUI_API const char* HUI_GetLabelText(HUI_id label);
	/** @brief HUI_INVALID_ID uses the theme font. */
	HUI_API void HUI_SetLabelFont(HUI_id label, HUI_id font);
	/** @brief Font size in logical units. 0 uses HUI_METRIC_FONT_SIZE. */
	HUI_API void HUI_SetLabelTextSize(HUI_id label, float size);
	HUI_API void HUI_SetLabelTextAlign(HUI_id label, HUI_ETextAlign align);
	/** @brief HUI_TRUE wraps the text on the width given by the parent. */
	HUI_API void HUI_SetLabelWrap(HUI_id label, int wrap);
	/** @brief Text color (HUI_COLOR_TEXT of this widget). */
	HUI_API void HUI_SetLabelColor(HUI_id label, float r, float g, float b, float a);


	/* ============================================================================
	 *  BUTTON
	 * ============================================================================ */

	/** @brief Shortcut for HUI_CreateWidget(HUI_WIDGET_BUTTON) + HUI_SetButtonText. */
	HUI_API HUI_id HUI_CreateButton(const char* text);
	/**
	 * @brief Sets the content of the button to a label. A button can instead
	 * hold any single child widget (HUI_AddChild), e.g. an HBox with an icon.
	 */
	HUI_API void HUI_SetButtonText(HUI_id button, const char* text);
	HUI_API void HUI_SetButtonStyle(HUI_id button, HUI_EButtonStyle style);
	typedef void (*HUI_CButtonClicked)(HUI_id button, void* user_data);
	HUI_API void HUI_SetButtonClickedCallback(HUI_id button, HUI_CButtonClicked callback, void* user_data);


	/* ============================================================================
	 *  CHECKBOX
	 * ============================================================================ */

	HUI_API HUI_id HUI_CreateCheckbox(const char* text);
	HUI_API void HUI_SetCheckboxText(HUI_id checkbox, const char* text);
	HUI_API void HUI_SetCheckboxChecked(HUI_id checkbox, int checked);
	HUI_API int HUI_IsCheckboxChecked(HUI_id checkbox);
	HUI_API void HUI_SetCheckboxStyle(HUI_id checkbox, HUI_ECheckboxStyle style);
	typedef void (*HUI_CCheckboxChanged)(HUI_id checkbox, int checked, void* user_data);
	HUI_API void HUI_SetCheckboxChangedCallback(HUI_id checkbox, HUI_CCheckboxChanged callback, void* user_data);


	/* ============================================================================
	 *  SLIDER
	 * ============================================================================ */

	HUI_API void HUI_SetSliderRange(HUI_id slider, float minimum, float maximum);
	HUI_API void HUI_SetSliderValue(HUI_id slider, float value);
	HUI_API float HUI_GetSliderValue(HUI_id slider);
	/** @brief Snaps the value to multiples of `step`. 0 = continuous (default). */
	HUI_API void HUI_SetSliderStep(HUI_id slider, float step);
	HUI_API void HUI_SetSliderOrientation(HUI_id slider, HUI_EOrientation orientation);
	typedef void (*HUI_CSliderChanged)(HUI_id slider, float value, void* user_data);
	HUI_API void HUI_SetSliderChangedCallback(HUI_id slider, HUI_CSliderChanged callback, void* user_data);


	/* ============================================================================
	 *  PROGRESS BAR
	 * ============================================================================ */

	/** @brief Progress between 0 and 1. */
	HUI_API void HUI_SetProgressBarValue(HUI_id progress_bar, float value);
	HUI_API float HUI_GetProgressBarValue(HUI_id progress_bar);


	/* ============================================================================
	 *  IMAGE
	 * ============================================================================ */

	HUI_API void HUI_SetImageTexture(HUI_id image, HUI_id texture);
	HUI_API void HUI_SetImageTintColor(HUI_id image, float r, float g, float b, float a);
	HUI_API void HUI_SetImageStretch(HUI_id image, HUI_EStretch stretch);
	/** @brief Part of the texture to show, in UV coordinates. Default: 0, 0, 1, 1. */
	HUI_API void HUI_SetImageRegion(HUI_id image, float min_u, float min_v, float max_u, float max_v);
	HUI_API void HUI_SetImageCornerRadius(HUI_id image, float radius);


	/* ============================================================================
	 *  TEXT INPUT (single line)
	 * ============================================================================ */

	HUI_API void HUI_SetTextInputText(HUI_id text_input, const char* text);
	HUI_API const char* HUI_GetTextInputText(HUI_id text_input);
	/** @brief Dimmed text shown while the input is empty. */
	HUI_API void HUI_SetTextInputPlaceholder(HUI_id text_input, const char* text);
	HUI_API void HUI_SetTextInputPassword(HUI_id text_input, int password);
	HUI_API void HUI_SetTextInputReadOnly(HUI_id text_input, int read_only);
	/** @brief Maximum number of characters. 0 = no limit (default). */
	HUI_API void HUI_SetTextInputMaxLength(HUI_id text_input, int max_length);
	HUI_API void HUI_SelectAllText(HUI_id text_input);
	typedef void (*HUI_CTextChanged)(HUI_id text_input, const char* text, void* user_data);
	/** @brief Called on every edit. */
	HUI_API void HUI_SetTextInputChangedCallback(HUI_id text_input, HUI_CTextChanged callback, void* user_data);
	/** @brief Called when Enter is pressed. This also fires the action of the widget. */
	HUI_API void HUI_SetTextInputCommittedCallback(HUI_id text_input, HUI_CTextChanged callback, void* user_data);


	/* ============================================================================
	 *  SPIN BOX (numeric input: drag to scrub, click to type, arrows / wheel to step)
	 * ============================================================================ */

	HUI_API void HUI_SetSpinBoxRange(HUI_id spin_box, double minimum, double maximum);
	HUI_API void HUI_SetSpinBoxValue(HUI_id spin_box, double value);
	HUI_API double HUI_GetSpinBoxValue(HUI_id spin_box);
	HUI_API void HUI_SetSpinBoxStep(HUI_id spin_box, double step);
	/** @brief Digits shown after the decimal point. Default: 2. */
	HUI_API void HUI_SetSpinBoxDecimals(HUI_id spin_box, int decimals);
	typedef void (*HUI_CSpinBoxChanged)(HUI_id spin_box, double value, void* user_data);
	HUI_API void HUI_SetSpinBoxChangedCallback(HUI_id spin_box, HUI_CSpinBoxChanged callback, void* user_data);


	/* ============================================================================
	 *  COMBO BOX
	 * ============================================================================ */

	/** @return The index of the new item. */
	HUI_API int HUI_AddComboBoxItem(HUI_id combo_box, const char* text);
	HUI_API void HUI_ClearComboBoxItems(HUI_id combo_box);
	HUI_API int HUI_GetComboBoxItemCount(HUI_id combo_box);
	HUI_API const char* HUI_GetComboBoxItemText(HUI_id combo_box, int index);
	/** @brief Selects an item. -1 selects nothing. */
	HUI_API void HUI_SetComboBoxSelected(HUI_id combo_box, int index);
	HUI_API int HUI_GetComboBoxSelected(HUI_id combo_box);
	typedef void (*HUI_CComboBoxChanged)(HUI_id combo_box, int index, void* user_data);
	HUI_API void HUI_SetComboBoxChangedCallback(HUI_id combo_box, HUI_CComboBoxChanged callback, void* user_data);


	/* ============================================================================
	 *  SEPARATOR
	 * ============================================================================ */

	/** @brief HUI_HORIZONTAL draws a horizontal line (default). */
	HUI_API void HUI_SetSeparatorOrientation(HUI_id separator, HUI_EOrientation orientation);


	/* ============================================================================
	 *  LIST VIEW (virtualized: only the visible rows are processed)
	 * ============================================================================ */

	/** @return The index of the new item. */
	HUI_API int HUI_AddListItem(HUI_id list_view, const char* text);
	HUI_API void HUI_SetListItemText(HUI_id list_view, int index, const char* text);
	HUI_API const char* HUI_GetListItemText(HUI_id list_view, int index);
	HUI_API void HUI_RemoveListItem(HUI_id list_view, int index);
	HUI_API void HUI_ClearListItems(HUI_id list_view);
	HUI_API int HUI_GetListItemCount(HUI_id list_view);

	/**
	 * Virtual mode, for large or computed data: the list stores nothing and
	 * asks for the text of the rows it draws. The returned string must stay
	 * valid until the next call.
	 */
	typedef const char* (*HUI_CListItemText)(HUI_id list_view, int index, void* user_data);
	HUI_API void HUI_SetListVirtualItems(HUI_id list_view, int count, HUI_CListItemText callback, void* user_data);

	/** @brief Selects a row. -1 selects nothing. */
	HUI_API void HUI_SetListSelected(HUI_id list_view, int index);
	HUI_API int HUI_GetListSelected(HUI_id list_view);
	HUI_API void HUI_ScrollToListItem(HUI_id list_view, int index);
	typedef void (*HUI_CListEvent)(HUI_id list_view, int index, void* user_data);
	HUI_API void HUI_SetListSelectionChangedCallback(HUI_id list_view, HUI_CListEvent callback, void* user_data);
	/** @brief Called on double click or Enter. This also fires the action of the widget. */
	HUI_API void HUI_SetListActivatedCallback(HUI_id list_view, HUI_CListEvent callback, void* user_data);


	/* ============================================================================
	 *  TREE VIEW (virtualized)
	 * ============================================================================ */

	/**
	 * @brief Adds an item.
	 * @param parent_item Item to add under, or HUI_INVALID_ID for a top-level item.
	 * @return The handle of the new item.
	 */
	HUI_API HUI_id HUI_AddTreeItem(HUI_id tree_view, HUI_id parent_item, const char* text);
	/** @brief Deletes an item and its descendants. */
	HUI_API void HUI_DeleteTreeItem(HUI_id item);
	HUI_API void HUI_ClearTreeItems(HUI_id tree_view);
	HUI_API int HUI_IsValidTreeItem(HUI_id item);

	HUI_API void HUI_SetTreeItemText(HUI_id item, const char* text);
	HUI_API const char* HUI_GetTreeItemText(HUI_id item);
	HUI_API void HUI_SetTreeItemExpanded(HUI_id item, int expanded);
	HUI_API int HUI_IsTreeItemExpanded(HUI_id item);
	HUI_API void HUI_SetTreeItemUserHandle(HUI_id item, void* handle);
	HUI_API void* HUI_GetTreeItemUserHandle(HUI_id item);
	/** @brief Top-level items of a tree. */
	HUI_API int HUI_GetTreeItemCount(HUI_id tree_view);
	HUI_API HUI_id HUI_GetTreeItem(HUI_id tree_view, int index);
	/** @return The parent item, or HUI_INVALID_ID for a top-level item. */
	HUI_API HUI_id HUI_GetTreeItemParent(HUI_id item);
	HUI_API int HUI_GetTreeItemChildCount(HUI_id item);
	HUI_API HUI_id HUI_GetTreeItemChild(HUI_id item, int index);

	/** @brief Selects an item. HUI_INVALID_ID selects nothing. */
	HUI_API void HUI_SetTreeSelectedItem(HUI_id tree_view, HUI_id item);
	HUI_API HUI_id HUI_GetTreeSelectedItem(HUI_id tree_view);
	typedef void (*HUI_CTreeEvent)(HUI_id tree_view, HUI_id item, void* user_data);
	HUI_API void HUI_SetTreeSelectionChangedCallback(HUI_id tree_view, HUI_CTreeEvent callback, void* user_data);
	/** @brief Called on double click or Enter. This also fires the action of the widget. */
	HUI_API void HUI_SetTreeActivatedCallback(HUI_id tree_view, HUI_CTreeEvent callback, void* user_data);


	/* ============================================================================
	 *  MENUS
	 * ============================================================================ */

	/**
	 * A MENU_BAR holds MENUs; a MENU holds MENU_ITEMs and sub-MENUs. A MENU
	 * that has no parent can be opened anywhere as a context menu.
	 */

	/** @brief Creates a menu inside a menu bar or as a sub-menu of another menu. */
	HUI_API HUI_id HUI_AddMenu(HUI_id menu_bar_or_menu, const char* title);
	HUI_API void HUI_SetMenuTitle(HUI_id menu, const char* title);

	HUI_API HUI_id HUI_AddMenuItem(HUI_id menu, const char* text);
	HUI_API HUI_id HUI_AddMenuSeparator(HUI_id menu);

	HUI_API void HUI_SetMenuItemText(HUI_id item, const char* text);
	/**
	 * @brief Shows a shortcut next to the item and triggers the item when the
	 * keys are pressed in the window of its menu bar.
	 */
	HUI_API void HUI_SetMenuItemShortcut(HUI_id item, HUI_EKey key, HUI_uint modifiers);
	HUI_API void HUI_SetMenuItemCheckable(HUI_id item, int checkable);
	HUI_API void HUI_SetMenuItemChecked(HUI_id item, int checked);
	HUI_API int HUI_IsMenuItemChecked(HUI_id item);
	typedef void (*HUI_CMenuItemClicked)(HUI_id item, void* user_data);
	HUI_API void HUI_SetMenuItemClickedCallback(HUI_id item, HUI_CMenuItemClicked callback, void* user_data);

	/** @brief Opens a menu as a popup at a position of the window (logical units). */
	HUI_API void HUI_OpenMenu(HUI_id menu, HUI_id window, float x, float y);
	/** @brief Closes every open menu and popup of the window. */
	HUI_API void HUI_ClosePopups(HUI_id window);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* HUI_CONTROLS_H */
