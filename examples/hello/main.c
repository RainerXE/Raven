/**
 * HUI example - hello
 * The smallest program: a window built with the C API, a label and a button.
 *
 * Every object is an opaque handle (HUI_id). Layout follows Slate: a panel
 * arranges its children, and each child carries a *slot* that tells the panel
 * how to place it (padding, alignment, auto or fill size).
 */

#include "../common/example.h"

static HUI_id g_label;
static int g_clicks;

static void on_clicked(HUI_id button, void* user_data) {
	char text[64];
	(void)button;
	(void)user_data;
	g_clicks++;
	snprintf(text, sizeof(text), "Clicked %d time%s", g_clicks, g_clicks > 1 ? "s" : "");
	HUI_SetLabelText(g_label, text);
}

static void on_quit(HUI_id button, void* user_data) {
	(void)button;
	(void)user_data;
	HUI_Quit(0);
}

int main(void) {
	HUI_id window, box, title, row, button, quit;

	if (!example_init("hello")) return 1;

	window = HUI_CreateWindow("Hello HUI", 420, 220, HUI_WINDOW_DEFAULT);

	/* a vertical box centered in the window */
	box = HUI_CreateWidget(HUI_WIDGET_VBOX);
	HUI_SetPanelSpacing(box, 12.0f);
	HUI_SetSlotHAlign(box, HUI_HALIGN_CENTER);
	HUI_SetSlotVAlign(box, HUI_VALIGN_CENTER);
	HUI_SetWindowContent(window, box);

	title = HUI_AddChild(box, HUI_CreateLabel("Hello from HUI"));
	HUI_SetLabelTextSize(title, 24.0f);
	HUI_SetSlotHAlign(title, HUI_HALIGN_CENTER);

	g_label = HUI_AddChild(box, HUI_CreateLabel("Press the button"));
	HUI_SetSlotHAlign(g_label, HUI_HALIGN_CENTER);

	row = HUI_AddChild(box, HUI_CreateWidget(HUI_WIDGET_HBOX));
	HUI_SetPanelSpacing(row, 8.0f);
	HUI_SetSlotHAlign(row, HUI_HALIGN_CENTER);

	button = HUI_AddChild(row, HUI_CreateButton("Click me"));
	HUI_SetButtonStyle(button, HUI_BUTTON_PRIMARY);
	HUI_SetButtonClickedCallback(button, on_clicked, NULL);
	HUI_SetWidgetTooltip(button, "Counts the clicks");

	quit = HUI_AddChild(row, HUI_CreateButton("Quit"));
	HUI_SetButtonClickedCallback(quit, on_quit, NULL);

	example_run(window);

	/* releases every window, widget and resource that is still alive */
	HUI_Shutdown();
	return 0;
}
