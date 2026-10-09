/**
 * HUI tests - controls driven by synthetic input (HUI_Inject...), focus,
 * popups, handles and deletion from inside callbacks.
 */

#include "test.h"

static HUI_id g_window;
static int g_clicks;
static int g_last_int;
static float g_last_float;
static double g_last_double;
static char g_last_text[128];
static HUI_id g_last_id;
static int g_actions;
static char g_last_action[64];

static void on_click(HUI_id button, void* user_data) {
	(void)button;
	(void)user_data;
	g_clicks++;
}

static void on_action(HUI_id sender, const char* action, void* user_data) {
	(void)user_data;
	g_actions++;
	g_last_id = sender;
	snprintf(g_last_action, sizeof(g_last_action), "%s", action);
}

static void on_check(HUI_id checkbox, int checked, void* user_data) {
	(void)checkbox;
	(void)user_data;
	g_last_int = checked;
	g_clicks++;
}

static void on_slider(HUI_id slider, float value, void* user_data) {
	(void)slider;
	(void)user_data;
	g_last_float = value;
}

static void on_text(HUI_id input, const char* text, void* user_data) {
	(void)input;
	(void)user_data;
	snprintf(g_last_text, sizeof(g_last_text), "%s", text);
	g_clicks++;
}

static void on_index(HUI_id widget, int index, void* user_data) {
	(void)widget;
	(void)user_data;
	g_last_int = index;
	g_clicks++;
}

static void on_spin(HUI_id widget, double value, void* user_data) {
	(void)widget;
	(void)user_data;
	g_last_double = value;
}

static void on_tree(HUI_id tree, HUI_id item, void* user_data) {
	(void)tree;
	(void)user_data;
	g_last_id = item;
	g_clicks++;
}

static void on_menu_item(HUI_id item, void* user_data) {
	(void)user_data;
	g_last_id = item;
	g_clicks++;
}

/* A column of widgets, each on its own row, aligned to the left. */
static HUI_id column(void) {
	HUI_id box = HUI_CreateWidget(HUI_WIDGET_VBOX);
	HUI_SetPanelSpacing(box, 4);
	HUI_SetWindowContent(g_window, box);
	return box;
}

static void test_button(void) {
	HUI_id box = column();
	HUI_id button = HUI_AddChild(box, HUI_CreateButton("Save"));
	HUI_id disabled = HUI_AddChild(box, HUI_CreateButton("Disabled"));
	float x, y, w, h;
	HUI_SetSlotHAlign(button, HUI_HALIGN_LEFT);
	HUI_SetButtonClickedCallback(button, on_click, NULL);
	HUI_SetButtonClickedCallback(disabled, on_click, NULL);
	HUI_SetWidgetAction(button, "file.save");
	HUI_SetWidgetEnabled(disabled, HUI_FALSE);
	HUI_BindAction("file.save", on_action, NULL);

	HUI_GetWidgetRect(button, &x, &y, &w, &h);
	CHECK(w > 30 && w < 120);
	CHECK_NEAR(h, HUI_GetThemeMetric(HUI_METRIC_CONTROL_HEIGHT));

	g_clicks = g_actions = 0;
	test_click(g_window, button);
	CHECK(g_clicks == 1);
	CHECK(g_actions == 1);
	CHECK_STR(g_last_action, "file.save");
	CHECK(g_last_id == button);
	CHECK(HUI_IsWidgetHovered(button));
	CHECK(HUI_GetFocusedWidget(g_window) == button);

	/* press on the button, release outside: no click */
	HUI_InjectMouseMove(g_window, x + 5, y + 5);
	HUI_InjectMouseButton(g_window, HUI_MOUSE_LEFT, HUI_TRUE);
	HUI_InjectMouseMove(g_window, 390, 290);
	HUI_InjectMouseButton(g_window, HUI_MOUSE_LEFT, HUI_FALSE);
	CHECK(g_clicks == 1);
	CHECK(!HUI_IsWidgetHovered(button));

	/* a disabled button ignores clicks */
	test_click(g_window, disabled);
	CHECK(g_clicks == 1);

	/* keyboard: the focused button is clicked with Enter and Space */
	HUI_FocusWidget(button);
	test_key(g_window, HUI_KEY_ENTER, 0);
	test_key(g_window, HUI_KEY_SPACE, 0);
	CHECK(g_clicks == 3);

	/* an unbound action goes to the default handler */
	HUI_UnbindAction("file.save");
	HUI_SetDefaultActionHandler(on_action, NULL);
	g_actions = 0;
	test_click(g_window, button);
	CHECK(g_actions == 1);
	HUI_SetDefaultActionHandler(NULL, NULL);
}

static void test_checkbox_and_radio(void) {
	HUI_id box = column();
	HUI_id check = HUI_AddChild(box, HUI_CreateCheckbox("Shadows"));
	HUI_id radio_a = HUI_AddChild(box, HUI_CreateCheckbox("Low"));
	HUI_id radio_b = HUI_AddChild(box, HUI_CreateCheckbox("High"));
	HUI_SetCheckboxStyle(radio_a, HUI_CHECKBOX_RADIO);
	HUI_SetCheckboxStyle(radio_b, HUI_CHECKBOX_RADIO);
	HUI_SetCheckboxChangedCallback(check, on_check, NULL);

	g_clicks = 0;
	test_click(g_window, check);
	CHECK(HUI_IsCheckboxChecked(check));
	CHECK(g_clicks == 1 && g_last_int == 1);
	test_click(g_window, check);
	CHECK(!HUI_IsCheckboxChecked(check));

	/* setting the value from code does not call back */
	HUI_SetCheckboxChecked(check, HUI_TRUE);
	CHECK(g_clicks == 2);

	/* radio boxes of one parent are exclusive and cannot be unchecked by a click */
	test_click(g_window, radio_a);
	CHECK(HUI_IsCheckboxChecked(radio_a) && !HUI_IsCheckboxChecked(radio_b));
	test_click(g_window, radio_b);
	CHECK(!HUI_IsCheckboxChecked(radio_a) && HUI_IsCheckboxChecked(radio_b));
	test_click(g_window, radio_b);
	CHECK(HUI_IsCheckboxChecked(radio_b));
}

static void test_slider_and_spin_box(void) {
	HUI_id box = column();
	HUI_id slider = HUI_AddChild(box, HUI_CreateWidget(HUI_WIDGET_SLIDER));
	HUI_id spin = HUI_AddChild(box, HUI_CreateWidget(HUI_WIDGET_SPIN_BOX));
	float x, y, w, h;
	HUI_SetSliderRange(slider, 0, 100);
	HUI_SetSliderChangedCallback(slider, on_slider, NULL);
	HUI_GetWidgetRect(slider, &x, &y, &w, &h);

	/* clicking in the middle of the track gives the middle of the range */
	test_click(g_window, slider);
	CHECK_NEAR(HUI_GetSliderValue(slider), 50);
	CHECK_NEAR(g_last_float, 50);

	/* dragging past the end clamps to the maximum */
	HUI_InjectMouseButton(g_window, HUI_MOUSE_LEFT, HUI_TRUE);
	HUI_InjectMouseMove(g_window, x + w + 200, y);
	HUI_InjectMouseButton(g_window, HUI_MOUSE_LEFT, HUI_FALSE);
	CHECK_NEAR(HUI_GetSliderValue(slider), 100);

	HUI_SetSliderStep(slider, 25);
	HUI_SetSliderValue(slider, 40);
	CHECK_NEAR(HUI_GetSliderValue(slider), 50);
	test_key(g_window, HUI_KEY_LEFT, 0);
	CHECK_NEAR(HUI_GetSliderValue(slider), 25);

	/* spin box: arrows step, typing replaces the value, the range clamps */
	HUI_SetSpinBoxRange(spin, 0, 10);
	HUI_SetSpinBoxStep(spin, 0.5);
	HUI_SetSpinBoxValue(spin, 3);
	HUI_SetSpinBoxChangedCallback(spin, on_spin, NULL);
	HUI_FocusWidget(spin);
	test_key(g_window, HUI_KEY_UP, 0);
	CHECK_NEAR(HUI_GetSpinBoxValue(spin) * 10, 35);
	CHECK_NEAR(g_last_double * 10, 35);
	HUI_InjectText(g_window, "7.25");
	test_key(g_window, HUI_KEY_ENTER, 0);
	CHECK_NEAR(HUI_GetSpinBoxValue(spin) * 100, 725);
	HUI_InjectText(g_window, "99");
	test_key(g_window, HUI_KEY_ENTER, 0);
	CHECK_NEAR(HUI_GetSpinBoxValue(spin), 10);
	/* Escape cancels an edit */
	HUI_InjectText(g_window, "1");
	test_key(g_window, HUI_KEY_ESCAPE, 0);
	CHECK_NEAR(HUI_GetSpinBoxValue(spin), 10);
}

static void test_text_input(void) {
	HUI_id box = column();
	HUI_id input = HUI_AddChild(box, HUI_CreateWidget(HUI_WIDGET_TEXT_INPUT));
	HUI_id other = HUI_AddChild(box, HUI_CreateWidget(HUI_WIDGET_TEXT_INPUT));
	HUI_SetTextInputChangedCallback(input, on_text, NULL);
	HUI_SetTextInputCommittedCallback(input, on_text, NULL);

	test_click(g_window, input);
	CHECK(HUI_IsWidgetFocused(input));
	HUI_InjectText(g_window, "H\xC3\xA9llo");
	CHECK_STR(HUI_GetTextInputText(input), "H\xC3\xA9llo");
	CHECK_STR(g_last_text, "H\xC3\xA9llo");

	/* the caret moves by characters, not bytes */
	test_key(g_window, HUI_KEY_HOME, 0);
	test_key(g_window, HUI_KEY_RIGHT, 0);
	test_key(g_window, HUI_KEY_RIGHT, 0);
	test_key(g_window, HUI_KEY_BACKSPACE, 0);
	CHECK_STR(HUI_GetTextInputText(input), "Hllo");
	test_key(g_window, HUI_KEY_DELETE, 0);
	CHECK_STR(HUI_GetTextInputText(input), "Hlo");

	/* selection, clipboard */
	test_key(g_window, HUI_KEY_A, HUI_MOD_CTRL);
	test_key(g_window, HUI_KEY_C, HUI_MOD_CTRL);
	CHECK_STR(HUI_GetClipboardText(), "Hlo");
	test_key(g_window, HUI_KEY_END, 0);
	test_key(g_window, HUI_KEY_V, HUI_MOD_CTRL);
	CHECK_STR(HUI_GetTextInputText(input), "HloHlo");
	test_key(g_window, HUI_KEY_LEFT, HUI_MOD_SHIFT);
	test_key(g_window, HUI_KEY_LEFT, HUI_MOD_SHIFT);
	test_key(g_window, HUI_KEY_X, HUI_MOD_CTRL);
	CHECK_STR(HUI_GetTextInputText(input), "HloH");
	CHECK_STR(HUI_GetClipboardText(), "lo");

	/* Enter commits */
	g_clicks = 0;
	test_key(g_window, HUI_KEY_ENTER, 0);
	CHECK(g_clicks == 1);

	/* Tab moves the focus to the next focusable widget, Shift+Tab back */
	test_key(g_window, HUI_KEY_TAB, 0);
	CHECK(HUI_GetFocusedWidget(g_window) == other);
	test_key(g_window, HUI_KEY_TAB, HUI_MOD_SHIFT);
	CHECK(HUI_GetFocusedWidget(g_window) == input);

	/* limits and read-only */
	HUI_SetTextInputText(input, "");
	HUI_SetTextInputMaxLength(input, 3);
	HUI_InjectText(g_window, "abcdef");
	CHECK_STR(HUI_GetTextInputText(input), "abc");
	HUI_SetTextInputMaxLength(input, 0);
	HUI_SetTextInputReadOnly(input, HUI_TRUE);
	HUI_InjectText(g_window, "x");
	test_key(g_window, HUI_KEY_BACKSPACE, 0);
	CHECK_STR(HUI_GetTextInputText(input), "abc");
}

static void test_combo_box(void) {
	HUI_id box = column();
	HUI_id combo = HUI_AddChild(box, HUI_CreateWidget(HUI_WIDGET_COMBO_BOX));
	float x, y, w, h, row = HUI_GetThemeMetric(HUI_METRIC_ROW_HEIGHT);
	HUI_SetSlotHAlign(combo, HUI_HALIGN_LEFT);
	HUI_AddComboBoxItem(combo, "Low");
	HUI_AddComboBoxItem(combo, "Medium");
	CHECK(HUI_AddComboBoxItem(combo, "High") == 2);
	HUI_SetComboBoxSelected(combo, 0);
	HUI_SetComboBoxChangedCallback(combo, on_index, NULL);
	CHECK(HUI_GetComboBoxItemCount(combo) == 3);
	CHECK_STR(HUI_GetComboBoxItemText(combo, 1), "Medium");

	/* a click opens the list below the box; a click on the third row selects it */
	HUI_GetWidgetRect(combo, &x, &y, &w, &h);
	g_clicks = 0;
	test_click(g_window, combo);
	test_click_at(g_window, x + 10, y + h + 2 + 1 + row * 2.5f);
	CHECK(HUI_GetComboBoxSelected(combo) == 2);
	CHECK(g_clicks == 1 && g_last_int == 2);

	/* a click outside closes the list without selecting and without reaching what is below */
	test_click(g_window, combo);
	test_click_at(g_window, 380, 280);
	CHECK(HUI_GetComboBoxSelected(combo) == 2);
	CHECK(g_clicks == 1);

	/* keyboard in the open list */
	test_click(g_window, combo);
	test_key(g_window, HUI_KEY_UP, 0);
	test_key(g_window, HUI_KEY_ENTER, 0);
	CHECK(HUI_GetComboBoxSelected(combo) == 1);
	test_click(g_window, combo);
	test_key(g_window, HUI_KEY_ESCAPE, 0);
	CHECK(HUI_GetComboBoxSelected(combo) == 1);
}

static const char* virtual_row(HUI_id list, int index, void* user_data) {
	static char buffer[32];
	(void)list;
	(void)user_data;
	snprintf(buffer, sizeof(buffer), "Row %d", index);
	return buffer;
}

static void test_list_and_tree(void) {
	HUI_id box = column();
	HUI_id list = HUI_AddChild(box, HUI_CreateWidget(HUI_WIDGET_LIST_VIEW));
	HUI_id tree = HUI_AddChild(box, HUI_CreateWidget(HUI_WIDGET_TREE_VIEW));
	HUI_id root, child, leaf;
	float x, y, w, h, row = HUI_GetThemeMetric(HUI_METRIC_ROW_HEIGHT);
	HUI_SetSlotFill(tree, 1.0f);
	HUI_SetWidgetSize(list, -1, 100);

	/* a million virtual rows cost nothing */
	HUI_SetListVirtualItems(list, 1000000, virtual_row, NULL);
	HUI_SetListSelectionChangedCallback(list, on_index, NULL);
	CHECK(HUI_GetListItemCount(list) == 1000000);
	CHECK_STR(HUI_GetListItemText(list, 123456), "Row 123456");
	HUI_GetWidgetRect(list, &x, &y, &w, &h);
	test_click_at(g_window, x + 20, y + 1 + row * 1.5f);
	CHECK(HUI_GetListSelected(list) == 1);
	test_key(g_window, HUI_KEY_DOWN, 0);
	test_key(g_window, HUI_KEY_DOWN, 0);
	CHECK(HUI_GetListSelected(list) == 3);
	test_key(g_window, HUI_KEY_END, 0);
	CHECK(HUI_GetListSelected(list) == 999999 && g_last_int == 999999);
	/* the list scrolled to its end: clicking the last visible row still maps to the right index */
	test_click_at(g_window, x + 20, y + h - 3);
	CHECK(HUI_GetListSelected(list) == 999999);

	/* stored items */
	HUI_SetListVirtualItems(list, 0, NULL, NULL);
	HUI_AddListItem(list, "one");
	HUI_AddListItem(list, "two");
	HUI_AddListItem(list, "three");
	HUI_SetListSelected(list, 2);
	HUI_RemoveListItem(list, 0);
	CHECK(HUI_GetListItemCount(list) == 2);
	CHECK(HUI_GetListSelected(list) == 1);
	CHECK_STR(HUI_GetListItemText(list, 1), "three");

	/* tree */
	root = HUI_AddTreeItem(tree, HUI_INVALID_ID, "World");
	child = HUI_AddTreeItem(tree, root, "Player");
	leaf = HUI_AddTreeItem(tree, child, "Camera");
	HUI_AddTreeItem(tree, HUI_INVALID_ID, "Sky");
	HUI_SetTreeSelectionChangedCallback(tree, on_tree, NULL);
	CHECK(HUI_GetTreeItemParent(leaf) == child);
	CHECK(HUI_GetTreeItemParent(root) == HUI_INVALID_ID);
	CHECK(HUI_GetTreeItemChildCount(root) == 1);
	CHECK(HUI_GetTreeItemChild(root, 0) == child);

	/* only "World" and "Sky" are shown: the second row is "Sky" */
	HUI_GetWidgetRect(tree, &x, &y, &w, &h);
	test_click_at(g_window, x + 60, y + 1 + row * 1.5f);
	CHECK_STR(HUI_GetTreeItemText(HUI_GetTreeSelectedItem(tree)), "Sky");

	/* keyboard: up to "World", right opens it, right again enters it */
	test_key(g_window, HUI_KEY_UP, 0);
	CHECK(HUI_GetTreeSelectedItem(tree) == root);
	test_key(g_window, HUI_KEY_RIGHT, 0);
	CHECK(HUI_IsTreeItemExpanded(root));
	test_key(g_window, HUI_KEY_RIGHT, 0);
	CHECK(HUI_GetTreeSelectedItem(tree) == child && g_last_id == child);

	/* selecting a hidden item opens the branches above it */
	HUI_SetTreeItemExpanded(root, HUI_FALSE);
	CHECK(HUI_GetTreeSelectedItem(tree) == root);
	HUI_SetTreeSelectedItem(tree, leaf);
	CHECK(HUI_IsTreeItemExpanded(root) && HUI_IsTreeItemExpanded(child));

	/* deleting an item deletes its branch and clears the selection inside it */
	HUI_DeleteTreeItem(child);
	CHECK(!HUI_IsValidTreeItem(leaf));
	CHECK(HUI_GetTreeSelectedItem(tree) == HUI_INVALID_ID);
	CHECK(HUI_GetTreeItemChildCount(root) == 0);
}

static void test_menus(void) {
	HUI_id box = column();
	HUI_id bar = HUI_AddChild(box, HUI_CreateWidget(HUI_WIDGET_MENU_BAR));
	HUI_id file = HUI_AddMenu(bar, "File");
	HUI_id edit = HUI_AddMenu(bar, "Edit");
	HUI_id open = HUI_AddMenuItem(file, "Open...");
	HUI_id recent = HUI_AddMenu(file, "Recent");
	HUI_id recent_a = HUI_AddMenuItem(recent, "scene.lynx");
	HUI_id grid_item, context, context_item, target;
	float row = HUI_GetThemeMetric(HUI_METRIC_ROW_HEIGHT) + 2, bar_h = HUI_GetThemeMetric(HUI_METRIC_CONTROL_HEIGHT);
	HUI_AddMenuSeparator(file);
	HUI_AddMenuItem(file, "Quit");
	grid_item = HUI_AddMenuItem(edit, "Show grid");
	HUI_SetMenuItemCheckable(grid_item, HUI_TRUE);
	HUI_SetMenuItemShortcut(grid_item, HUI_KEY_G, HUI_MOD_CTRL);
	HUI_SetMenuItemClickedCallback(open, on_menu_item, NULL);
	HUI_SetMenuItemClickedCallback(recent_a, on_menu_item, NULL);
	HUI_SetMenuItemClickedCallback(grid_item, on_menu_item, NULL);
	target = HUI_AddChild(box, HUI_CreateButton("Target"));
	HUI_SetButtonClickedCallback(target, on_click, NULL);

	/* click the "File" title, then its first entry */
	g_clicks = 0;
	test_click_at(g_window, 15, bar_h * 0.5f);
	test_click_at(g_window, 30, bar_h + 4 + row * 0.5f);
	CHECK(g_clicks == 1 && g_last_id == open);

	/* hovering "Recent" opens the sub-menu to the right; click its entry */
	test_click_at(g_window, 15, bar_h * 0.5f);
	HUI_InjectMouseMove(g_window, 30, bar_h + 4 + row * 1.5f);
	{
		float mx, my, mw, mh;
		HUI_GetWidgetRect(recent, &mx, &my, &mw, &mh);
		CHECK(mx > 40 && mw > 40);
		test_click_at(g_window, mx + 20, my + 4 + row * 0.5f);
	}
	CHECK(g_clicks == 2 && g_last_id == recent_a);

	/* with a menu open, a click elsewhere only closes it */
	test_click_at(g_window, 15, bar_h * 0.5f);
	test_click(g_window, target);
	CHECK(g_clicks == 2);
	test_click(g_window, target);
	CHECK(g_clicks == 3);

	/* keyboard navigation: Down skips to the first entry, Enter triggers it */
	test_click_at(g_window, 15, bar_h * 0.5f);
	test_key(g_window, HUI_KEY_DOWN, 0);
	test_key(g_window, HUI_KEY_ENTER, 0);
	CHECK(g_clicks == 4 && g_last_id == open);

	/* the shortcut of a menu item works without opening the menu, and toggles the check */
	test_key(g_window, HUI_KEY_G, HUI_MOD_CTRL);
	CHECK(g_clicks == 5 && g_last_id == grid_item);
	CHECK(HUI_IsMenuItemChecked(grid_item));
	HUI_SetWidgetEnabled(grid_item, HUI_FALSE);
	test_key(g_window, HUI_KEY_G, HUI_MOD_CTRL);
	CHECK(g_clicks == 5);

	/* context menu: a right click on the button opens it at the cursor */
	context = HUI_CreateWidget(HUI_WIDGET_MENU);
	context_item = HUI_AddMenuItem(context, "Rename");
	HUI_SetMenuItemClickedCallback(context_item, on_menu_item, NULL);
	HUI_SetWidgetContextMenu(target, context);
	{
		float x, y, w, h;
		HUI_GetWidgetRect(target, &x, &y, &w, &h);
		HUI_InjectMouseMove(g_window, x + 10, y + 5);
		HUI_InjectMouseButton(g_window, HUI_MOUSE_RIGHT, HUI_TRUE);
		HUI_InjectMouseButton(g_window, HUI_MOUSE_RIGHT, HUI_FALSE);
		test_click_at(g_window, x + 10 + 30, y + 5 + 4 + row * 0.5f);
	}
	CHECK(g_clicks == 6 && g_last_id == context_item);
	HUI_DeleteWidget(context);
}

static void test_tabs_and_expander(void) {
	HUI_id box = column();
	HUI_id tabs = HUI_AddChild(box, HUI_CreateWidget(HUI_WIDGET_TAB_VIEW));
	HUI_id page_a = HUI_CreateButton("A");
	HUI_id page_b = HUI_CreateButton("B");
	HUI_id expander = HUI_AddChild(box, HUI_CreateWidget(HUI_WIDGET_EXPANDER));
	HUI_id content = HUI_AddChild(expander, HUI_CreateButton("Inside"));
	float x, y, w, h, ex, ey, ew, eh;
	CHECK(HUI_AddTab(tabs, "First", page_a) == 0);
	CHECK(HUI_AddTab(tabs, "Second", page_b) == 1);
	HUI_SetTabChangedCallback(tabs, on_index, NULL);
	HUI_SetButtonClickedCallback(page_a, on_click, NULL);
	HUI_SetButtonClickedCallback(content, on_click, NULL);
	HUI_SetExpanderTitle(expander, "Transform");

	/* the hidden page cannot be clicked */
	HUI_GetWidgetRect(page_a, &x, &y, &w, &h);
	g_clicks = 0;
	test_click(g_window, page_a);
	CHECK(g_clicks == 1);
	HUI_SetActiveTab(tabs, 1);
	test_click_at(g_window, x + w * 0.5f, y + h * 0.5f);
	CHECK(g_clicks == 1);

	/* clicking the first tab title comes back to it */
	HUI_GetWidgetRect(tabs, &x, &y, &w, &h);
	test_click_at(g_window, x + 10, y + 8);
	CHECK(HUI_GetActiveTab(tabs) == 0 && g_last_int == 0);

	/* collapsing the expander removes its content from the layout and from hit testing */
	HUI_GetWidgetRect(expander, &ex, &ey, &ew, &eh);
	g_clicks = 0;
	test_click(g_window, content);
	CHECK(g_clicks == 1);
	test_click_at(g_window, ex + 30, ey + 5);
	CHECK(!HUI_IsExpanderExpanded(expander));
	HUI_GetWidgetRect(expander, &x, &y, &w, &h);
	CHECK(h < eh);
	test_click_at(g_window, ex + ew * 0.5f, ey + eh - 5);
	CHECK(g_clicks == 1);
}

/* Deleting widgets from their own callbacks must be safe. */
static void delete_self(HUI_id button, void* user_data) {
	(void)user_data;
	HUI_DeleteWidget(button);
	g_clicks++;
}

static void delete_parent(HUI_id button, void* user_data) {
	(void)user_data;
	HUI_DeleteWidget(HUI_GetParent(button));
	g_clicks++;
}

static void replace_content(HUI_id button, void* user_data) {
	(void)button;
	(void)user_data;
	HUI_SetWindowContent(g_window, HUI_CreateLabel("replaced"));
	g_clicks++;
}

static int consume_event(HUI_id widget, const HUI_Event* event, void* user_data) {
	(void)widget;
	(void)user_data;
	return event->type == HUI_EVENT_MOUSE_DOWN;
}

static void test_deletion_and_events(void) {
	HUI_id box = column();
	HUI_id first = HUI_AddChild(box, HUI_CreateButton("delete me"));
	HUI_id inner_box = HUI_AddChild(box, HUI_CreateWidget(HUI_WIDGET_HBOX));
	HUI_id second = HUI_AddChild(inner_box, HUI_CreateButton("delete my parent"));
	HUI_id third = HUI_AddChild(box, HUI_CreateButton("replace everything"));
	HUI_id blocked;
	HUI_SetButtonClickedCallback(first, delete_self, NULL);
	HUI_SetButtonClickedCallback(second, delete_parent, NULL);
	HUI_SetButtonClickedCallback(third, replace_content, NULL);

	g_clicks = 0;
	test_click(g_window, first);
	CHECK(g_clicks == 1 && !HUI_IsValidWidget(first));
	test_click(g_window, second);
	CHECK(g_clicks == 2 && !HUI_IsValidWidget(inner_box) && !HUI_IsValidWidget(second));
	test_click(g_window, third);
	CHECK(g_clicks == 3 && !HUI_IsValidWidget(box) && !HUI_IsValidWidget(third));
	CHECK(HUI_GetWidgetType(HUI_GetWindowContent(g_window)) == HUI_WIDGET_LABEL);

	/* an event callback that consumes the press prevents the click */
	box = column();
	blocked = HUI_AddChild(box, HUI_CreateButton("blocked"));
	HUI_SetButtonClickedCallback(blocked, on_click, NULL);
	HUI_SetWidgetEventCallback(blocked, consume_event, NULL);
	g_clicks = 0;
	test_click(g_window, blocked);
	CHECK(g_clicks == 0);

	/* hit testing can be switched off: the click goes through */
	HUI_SetWidgetEventCallback(blocked, NULL, NULL);
	HUI_SetWidgetHitTestable(blocked, HUI_FALSE);
	test_click(g_window, blocked);
	CHECK(g_clicks == 0);
	HUI_SetWidgetHitTestable(blocked, HUI_TRUE);
	test_click(g_window, blocked);
	CHECK(g_clicks == 1);
}

static void on_timer(HUI_id timer, void* user_data) {
	(void)timer;
	(*(int*)user_data)++;
}

static void test_timers_theme_and_attributes(void) {
	HUI_id box = column();
	HUI_id label = HUI_AddChild(box, HUI_CreateLabel("text"));
	HUI_id timer;
	int fired = 0, i;
	float r, g, b, a, before, after, w;

	timer = HUI_CreateTimer(0.0, HUI_FALSE, on_timer, &fired);
	CHECK(HUI_IsValidTimer(timer));
	for (i = 0; i < 3; i++) HUI_Update();
	CHECK(fired == 1);
	CHECK(!HUI_IsValidTimer(timer));

	/* a theme change is seen by widgets that were already measured */
	HUI_GetWidgetDesiredSize(label, &w, &before);
	HUI_SetThemeMetric(HUI_METRIC_FONT_SIZE, 26);
	HUI_GetWidgetDesiredSize(label, &w, &after);
	CHECK(after > before * 1.5f);
	HUI_SetThemeMetric(HUI_METRIC_FONT_SIZE, 13);

	HUI_ApplyTheme(HUI_THEME_LIGHT);
	HUI_GetThemeColor(HUI_COLOR_WINDOW, &r, &g, &b, &a);
	CHECK(r > 0.8f && a > 0.99f);
	HUI_ApplyTheme(HUI_THEME_DARK);
	HUI_GetThemeColor(HUI_COLOR_WINDOW, &r, &g, &b, &a);
	CHECK(r < 0.3f);

	/* string attributes: the names used in XML */
	CHECK(HUI_SetWidgetAttribute(label, "Text", "changed"));
	CHECK_STR(HUI_GetLabelText(label), "changed");
	CHECK(HUI_SetWidgetAttribute(label, "Slot.HAlign", "Center"));
	CHECK(HUI_SetWidgetAttribute(label, "Color.Text", "#FF8000"));
	CHECK(HUI_SetWidgetAttribute(label, "Visibility", "Collapsed"));
	CHECK(HUI_GetWidgetVisibility(label) == HUI_COLLAPSED);
	CHECK(!HUI_SetWidgetAttribute(label, "NoSuchAttribute", "1"));
	CHECK(!HUI_SetWidgetAttribute(label, "Opacity", "not a number"));
}

int main(void) {
	if (!test_init(0)) {
		printf("cannot initialize HUI\n");
		return 1;
	}
	g_window = HUI_CreateWindow("widgets", 400, 300, HUI_WINDOW_DEFAULT);

	RUN(test_button);
	RUN(test_checkbox_and_radio);
	RUN(test_slider_and_spin_box);
	RUN(test_text_input);
	RUN(test_combo_box);
	RUN(test_list_and_tree);
	RUN(test_menus);
	RUN(test_tabs_and_expander);
	RUN(test_deletion_and_events);
	RUN(test_timers_theme_and_attributes);

	/* everything still paints */
	HUI_Update();
	HUI_Shutdown();
	return test_report("test_widgets");
}
