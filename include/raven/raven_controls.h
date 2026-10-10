/**
 * Raven
 * raven_controls.h : interactive widgets and their settings.
 *
 * Every control is created with Rwidget_create(type). Its colors and sizes
 * come from the theme and can be overridden per widget (Rwidget_set_color,
 * Rwidget_set_metric). Its main event can be received either through the
 * typed callback declared here, or by name through an action
 * (Rwidget_set_action + Raction_bind).
 */

#ifndef RAVEN_CONTROLS_H
#define RAVEN_CONTROLS_H

#include "raven_widget.h"


typedef enum RTextAlign{
	R_TEXT_ALIGN_LEFT = 0x0280,
	R_TEXT_ALIGN_CENTER,
	R_TEXT_ALIGN_RIGHT
}RTextAlign;

typedef enum RButtonStyle{
	/** Filled with R_COLOR_CONTROL. */
	R_BUTTON_DEFAULT = 0x0290,
	/** Filled with R_COLOR_ACCENT. */
	R_BUTTON_PRIMARY,
	/** No background until hovered (toolbars). */
	R_BUTTON_FLAT
}RButtonStyle;

typedef enum RCheckboxStyle{
	R_CHECKBOX_CHECK = 0x02A0,
	/** Round mark. Checking one unchecks the other radio boxes of the same parent. */
	R_CHECKBOX_RADIO,
	/** On / off switch. */
	R_CHECKBOX_SWITCH
}RCheckboxStyle;

typedef enum RStretch{
	/** Stretches the image to the widget, ignoring its aspect ratio. */
	R_STRETCH_FILL = 0x02B0,
	/** Largest size that fits entirely, keeping the aspect ratio. */
	R_STRETCH_FIT,
	/** Smallest size that covers the widget, keeping the aspect ratio (cropped). */
	R_STRETCH_COVER,
	/** Original size, centered. */
	R_STRETCH_NONE
}RStretch;


#ifdef __cplusplus
extern "C" {
#endif

	/* ============================================================================
	 *  LABEL
	 * ============================================================================ */

	/** @brief Shortcut for Rwidget_create(R_WIDGET_LABEL) + Rlabel_set_text. */
	RAVEN_API Rid Rlabel_create(const char* text);
	/** @brief Sets the text. '\n' starts a new line. */
	RAVEN_API void Rlabel_set_text(Rid label, const char* text);
	RAVEN_API const char* Rlabel_get_text(Rid label);
	/** @brief R_INVALID_ID uses the theme font. */
	RAVEN_API void Rlabel_set_font(Rid label, Rid font);
	/** @brief Font size in logical units. 0 uses R_METRIC_FONT_SIZE. */
	RAVEN_API void Rlabel_set_text_size(Rid label, float size);
	RAVEN_API void Rlabel_set_text_align(Rid label, RTextAlign align);
	/** @brief R_TRUE wraps the text on the width given by the parent. */
	RAVEN_API void Rlabel_set_wrap(Rid label, int wrap);
	/** @brief Text color (R_COLOR_TEXT of this widget). */
	RAVEN_API void Rlabel_set_color(Rid label, float r, float g, float b, float a);

	/**
	 * @brief R_TRUE reads the text as Markdown and shows it formatted
	 * (Markdown="true" in XML):
	 *
	 *   # titles, **strong**, *emphasis*, `code`, ~~struck~~, [links](url),
	 *   lists and lists in lists, - [x] task lists, > quotes, --- rules,
	 *   | tables |, images alone on their line, and ``` blocks of code with
	 *   the colors of their language (```c, ```python...: the languages of
	 *   the text editor, raven_text_editor.h).
	 *
	 * The text wraps on the width of the label unless Rlabel_set_wrap said
	 * otherwise; put the label in a scroll box for a long document. Strong
	 * and emphasized text use the bold and italic of the font
	 * (Rfont_set_variant), which Raven derives from the font itself when it
	 * was given none. The colors come from the theme: R_COLOR_LINK,
	 * R_COLOR_CODE, R_COLOR_SEPARATOR, R_COLOR_ACCENT.
	 */
	RAVEN_API void Rlabel_set_markdown(Rid label, int enabled);
	RAVEN_API int Rlabel_get_markdown(Rid label);
	/**
	 * @brief Called when the user clicks a link of a Markdown label.
	 *
	 * Without a callback, a link written [Save](action:file.save) fires the
	 * action "file.save" (Raction_bind), and http, https and mailto links
	 * open in the browser or the mail program of the user (Ropen_url).
	 */
	typedef void (*RLabelLinkCallback)(Rid label, const char* url, void* user_data);
	RAVEN_API void Rlabel_set_link_callback(Rid label, RLabelLinkCallback callback, void* user_data);
	/** @brief The text of a Markdown label without its marks, as a reader sees it. Valid until the next call. */
	RAVEN_API const char* Rlabel_get_plain_text(Rid label);


	/* ============================================================================
	 *  BUTTON
	 * ============================================================================ */

	/** @brief Shortcut for Rwidget_create(R_WIDGET_BUTTON) + Rbutton_set_text. */
	RAVEN_API Rid Rbutton_create(const char* text);
	/**
	 * @brief Sets the content of the button to a label. A button can instead
	 * hold any single child widget (Rwidget_add_child), e.g. an HBox with an icon.
	 */
	RAVEN_API void Rbutton_set_text(Rid button, const char* text);
	RAVEN_API void Rbutton_set_style(Rid button, RButtonStyle style);
	typedef void (*RButtonClickedCallback)(Rid button, void* user_data);
	RAVEN_API void Rbutton_set_clicked_callback(Rid button, RButtonClickedCallback callback, void* user_data);


	/* ============================================================================
	 *  CHECKBOX
	 * ============================================================================ */

	RAVEN_API Rid Rcheckbox_create(const char* text);
	RAVEN_API void Rcheckbox_set_text(Rid checkbox, const char* text);
	RAVEN_API void Rcheckbox_set_checked(Rid checkbox, int checked);
	RAVEN_API int Rcheckbox_is_checked(Rid checkbox);
	RAVEN_API void Rcheckbox_set_style(Rid checkbox, RCheckboxStyle style);
	typedef void (*RCheckboxChangedCallback)(Rid checkbox, int checked, void* user_data);
	RAVEN_API void Rcheckbox_set_changed_callback(Rid checkbox, RCheckboxChangedCallback callback, void* user_data);


	/* ============================================================================
	 *  SLIDER
	 * ============================================================================ */

	RAVEN_API void Rslider_set_range(Rid slider, float minimum, float maximum);
	RAVEN_API void Rslider_set_value(Rid slider, float value);
	RAVEN_API float Rslider_get_value(Rid slider);
	/** @brief Snaps the value to multiples of `step`. 0 = continuous (default). */
	RAVEN_API void Rslider_set_step(Rid slider, float step);
	RAVEN_API void Rslider_set_orientation(Rid slider, ROrientation orientation);
	typedef void (*RSliderChangedCallback)(Rid slider, float value, void* user_data);
	RAVEN_API void Rslider_set_changed_callback(Rid slider, RSliderChangedCallback callback, void* user_data);


	/* ============================================================================
	 *  PROGRESS BAR
	 * ============================================================================ */

	/** @brief Progress between 0 and 1. */
	RAVEN_API void Rprogress_bar_set_value(Rid progress_bar, float value);
	RAVEN_API float Rprogress_bar_get_value(Rid progress_bar);


	/* ============================================================================
	 *  IMAGE
	 * ============================================================================ */

	RAVEN_API void Rimage_set_texture(Rid image, Rid texture);
	RAVEN_API void Rimage_set_tint_color(Rid image, float r, float g, float b, float a);
	RAVEN_API void Rimage_set_stretch(Rid image, RStretch stretch);
	/** @brief Part of the texture to show, in UV coordinates. Default: 0, 0, 1, 1. */
	RAVEN_API void Rimage_set_region(Rid image, float min_u, float min_v, float max_u, float max_v);
	RAVEN_API void Rimage_set_corner_radius(Rid image, float radius);


	/* ============================================================================
	 *  TEXT INPUT (single line)
	 * ============================================================================ */

	RAVEN_API void Rtext_input_set_text(Rid text_input, const char* text);
	RAVEN_API const char* Rtext_input_get_text(Rid text_input);
	/** @brief Dimmed text shown while the input is empty. */
	RAVEN_API void Rtext_input_set_placeholder(Rid text_input, const char* text);
	RAVEN_API void Rtext_input_set_password(Rid text_input, int password);
	RAVEN_API void Rtext_input_set_read_only(Rid text_input, int read_only);
	/** @brief Maximum number of characters. 0 = no limit (default). */
	RAVEN_API void Rtext_input_set_max_length(Rid text_input, int max_length);
	RAVEN_API void Rtext_input_select_all(Rid text_input);
	typedef void (*RTextChangedCallback)(Rid text_input, const char* text, void* user_data);
	/** @brief Called on every edit. */
	RAVEN_API void Rtext_input_set_changed_callback(Rid text_input, RTextChangedCallback callback, void* user_data);
	/** @brief Called when Enter is pressed. This also fires the action of the widget. */
	RAVEN_API void Rtext_input_set_committed_callback(Rid text_input, RTextChangedCallback callback, void* user_data);


	/* ============================================================================
	 *  SPIN BOX (numeric input: drag to scrub, click to type, arrows / wheel to step)
	 * ============================================================================ */

	RAVEN_API void Rspin_box_set_range(Rid spin_box, double minimum, double maximum);
	RAVEN_API void Rspin_box_set_value(Rid spin_box, double value);
	RAVEN_API double Rspin_box_get_value(Rid spin_box);
	RAVEN_API void Rspin_box_set_step(Rid spin_box, double step);
	/** @brief Digits shown after the decimal point. Default: 2. */
	RAVEN_API void Rspin_box_set_decimals(Rid spin_box, int decimals);
	typedef void (*RSpinBoxChangedCallback)(Rid spin_box, double value, void* user_data);
	RAVEN_API void Rspin_box_set_changed_callback(Rid spin_box, RSpinBoxChangedCallback callback, void* user_data);


	/* ============================================================================
	 *  COMBO BOX
	 * ============================================================================ */

	/** @return The index of the new item. */
	RAVEN_API int Rcombo_box_add_item(Rid combo_box, const char* text);
	RAVEN_API void Rcombo_box_clear_items(Rid combo_box);
	RAVEN_API int Rcombo_box_get_item_count(Rid combo_box);
	RAVEN_API const char* Rcombo_box_get_item_text(Rid combo_box, int index);
	/** @brief Selects an item. -1 selects nothing. */
	RAVEN_API void Rcombo_box_set_selected(Rid combo_box, int index);
	RAVEN_API int Rcombo_box_get_selected(Rid combo_box);
	typedef void (*RComboBoxChangedCallback)(Rid combo_box, int index, void* user_data);
	RAVEN_API void Rcombo_box_set_changed_callback(Rid combo_box, RComboBoxChangedCallback callback, void* user_data);


	/* ============================================================================
	 *  SEPARATOR
	 * ============================================================================ */

	/** @brief R_HORIZONTAL draws a horizontal line (default). */
	RAVEN_API void Rseparator_set_orientation(Rid separator, ROrientation orientation);


	/* ============================================================================
	 *  LIST VIEW (virtualized: only the visible rows are processed)
	 * ============================================================================ */

	/** @return The index of the new item. */
	RAVEN_API int Rlist_view_add_item(Rid list_view, const char* text);
	RAVEN_API void Rlist_view_set_item_text(Rid list_view, int index, const char* text);
	RAVEN_API const char* Rlist_view_get_item_text(Rid list_view, int index);
	RAVEN_API void Rlist_view_remove_item(Rid list_view, int index);
	RAVEN_API void Rlist_view_clear_items(Rid list_view);
	RAVEN_API int Rlist_view_get_item_count(Rid list_view);

	/**
	 * Virtual mode, for large or computed data: the list stores nothing and
	 * asks for the text of the rows it draws. The returned string must stay
	 * valid until the next call.
	 */
	typedef const char* (*RListItemTextCallback)(Rid list_view, int index, void* user_data);
	RAVEN_API void Rlist_view_set_virtual_items(Rid list_view, int count, RListItemTextCallback callback, void* user_data);

	/** @brief Selects a row. -1 selects nothing. */
	RAVEN_API void Rlist_view_set_selected(Rid list_view, int index);
	RAVEN_API int Rlist_view_get_selected(Rid list_view);
	RAVEN_API void Rlist_view_scroll_to_item(Rid list_view, int index);
	typedef void (*RListEventCallback)(Rid list_view, int index, void* user_data);
	RAVEN_API void Rlist_view_set_selection_changed_callback(Rid list_view, RListEventCallback callback, void* user_data);
	/** @brief Called on double click or Enter. This also fires the action of the widget. */
	RAVEN_API void Rlist_view_set_activated_callback(Rid list_view, RListEventCallback callback, void* user_data);


	/* ============================================================================
	 *  TREE VIEW (virtualized)
	 * ============================================================================ */

	/**
	 * @brief Adds an item.
	 * @param parent_item Item to add under, or R_INVALID_ID for a top-level item.
	 * @return The handle of the new item.
	 */
	RAVEN_API Rid Rtree_view_add_item(Rid tree_view, Rid parent_item, const char* text);
	/** @brief Deletes an item and its descendants. */
	RAVEN_API void Rtree_item_destroy(Rid item);
	RAVEN_API void Rtree_view_clear_items(Rid tree_view);
	RAVEN_API int Rtree_item_is_valid(Rid item);

	RAVEN_API void Rtree_item_set_text(Rid item, const char* text);
	RAVEN_API const char* Rtree_item_get_text(Rid item);
	RAVEN_API void Rtree_item_set_expanded(Rid item, int expanded);
	RAVEN_API int Rtree_item_is_expanded(Rid item);
	RAVEN_API void Rtree_item_set_user_handle(Rid item, void* handle);
	RAVEN_API void* Rtree_item_get_user_handle(Rid item);
	/** @brief Top-level items of a tree. */
	RAVEN_API int Rtree_view_get_item_count(Rid tree_view);
	RAVEN_API Rid Rtree_view_get_item(Rid tree_view, int index);
	/** @return The parent item, or R_INVALID_ID for a top-level item. */
	RAVEN_API Rid Rtree_item_get_parent(Rid item);
	RAVEN_API int Rtree_item_get_child_count(Rid item);
	RAVEN_API Rid Rtree_item_get_child(Rid item, int index);

	/** @brief Selects an item. R_INVALID_ID selects nothing. */
	RAVEN_API void Rtree_view_set_selected_item(Rid tree_view, Rid item);
	RAVEN_API Rid Rtree_view_get_selected_item(Rid tree_view);
	typedef void (*RTreeEventCallback)(Rid tree_view, Rid item, void* user_data);
	RAVEN_API void Rtree_view_set_selection_changed_callback(Rid tree_view, RTreeEventCallback callback, void* user_data);
	/** @brief Called on double click or Enter. This also fires the action of the widget. */
	RAVEN_API void Rtree_view_set_activated_callback(Rid tree_view, RTreeEventCallback callback, void* user_data);


	/* ============================================================================
	 *  MENUS
	 * ============================================================================ */

	/**
	 * A MENU_BAR holds MENUs; a MENU holds MENU_ITEMs and sub-MENUs. A MENU
	 * that has no parent can be opened anywhere as a context menu.
	 */

	/** @brief Creates a menu inside a menu bar or as a sub-menu of another menu. */
	RAVEN_API Rid Rmenu_add_menu(Rid menu_bar_or_menu, const char* title);
	RAVEN_API void Rmenu_set_title(Rid menu, const char* title);

	RAVEN_API Rid Rmenu_add_item(Rid menu, const char* text);
	RAVEN_API Rid Rmenu_add_separator(Rid menu);

	RAVEN_API void Rmenu_item_set_text(Rid item, const char* text);
	/**
	 * @brief Shows a shortcut next to the item and triggers the item when the
	 * keys are pressed in the window of its menu bar.
	 */
	RAVEN_API void Rmenu_item_set_shortcut(Rid item, RKey key, Ruint modifiers);
	RAVEN_API void Rmenu_item_set_checkable(Rid item, int checkable);
	RAVEN_API void Rmenu_item_set_checked(Rid item, int checked);
	RAVEN_API int Rmenu_item_is_checked(Rid item);
	typedef void (*RMenuItemClickedCallback)(Rid item, void* user_data);
	RAVEN_API void Rmenu_item_set_clicked_callback(Rid item, RMenuItemClickedCallback callback, void* user_data);

	/** @brief Opens a menu as a popup at a position of the window (logical units). */
	RAVEN_API void Rmenu_open(Rid menu, Rid window, float x, float y);
	/** @brief Closes every open menu and popup of the window. */
	RAVEN_API void Rwindow_close_popups(Rid window);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* RAVEN_CONTROLS_H */
