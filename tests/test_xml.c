/**
 * HUI tests - XML: parsing, building windows and widgets, error reporting.
 */

#include "test.h"

static int g_actions;
static HUI_id g_sender;

static void on_action(HUI_id sender, const char* action, void* user_data) {
	(void)action;
	(void)user_data;
	g_actions++;
	g_sender = sender;
}

static HUI_id widget_from(const char* xml) {
	return HUI_CreateWidgetFromXml(xml, strlen(xml));
}

static void test_window(void) {
	static const char xml[] =
		"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
		"<!-- a small editor -->\n"
		"<Window Title=\"Editor &amp; tools\" Width=\"640\" Height=\"480\" ClearColor=\"#102030\">\n"
		"  <Shortcut Keys=\"Ctrl+S\" Action=\"file.save\"/>\n"
		"  <VBox Spacing=\"4\">\n"
		"    <MenuBar>\n"
		"      <Menu Text=\"File\">\n"
		"        <MenuItem Name=\"open\" Text=\"Open...\" Shortcut=\"Ctrl+O\" OnClicked=\"file.open\"/>\n"
		"        <Separator/>\n"
		"        <Menu Text=\"Recent\"><MenuItem Text=\"a.lynx\"/></Menu>\n"
		"      </Menu>\n"
		"    </MenuBar>\n"
		"    <HBox Slot.Fill=\"1\" Spacing=\"8\">\n"
		"      <Border Name=\"side\" Width=\"200\" Padding=\"8,4\" Background=\"#202020\" CornerRadius=\"6\">\n"
		"        <VBox>\n"
		"          <Label Name=\"title\">Scene &lt;1&gt;</Label>\n"
		"          <Button Name=\"save\" Text=\"Save\" Style=\"Primary\" OnClicked=\"file.save\" Slot.HAlign=\"Left\"/>\n"
		"          <Checkbox Name=\"grid\" Text=\"Grid\" Checked=\"true\"/>\n"
		"          <Slider Name=\"zoom\" Min=\"1\" Max=\"5\" Value=\"2.5\"/>\n"
		"          <ComboBox Name=\"quality\" Selected=\"1\"><Item Text=\"Low\"/><Item>High</Item></ComboBox>\n"
		"          <TreeView Name=\"tree\" Slot.Fill=\"1\">\n"
		"            <Item Text=\"World\" Expanded=\"true\"><Item Text=\"Player\"/></Item>\n"
		"          </TreeView>\n"
		"        </VBox>\n"
		"      </Border>\n"
		"      <Viewport Name=\"scene\" Slot.Fill=\"1\"/>\n"
		"    </HBox>\n"
		"  </VBox>\n"
		"</Window>\n";
	HUI_id window = HUI_CreateWindowFromXml(xml, sizeof(xml) - 1);
	HUI_id side, title, save, scene, tree, quality;
	int w = 0, h = 0, pw = 0, ph = 0;
	float x, y, fw, fh;

	CHECK(HUI_IsValidWindow(window));
	if (!HUI_IsValidWindow(window)) return;
	HUI_GetWindowSize(window, &w, &h);
	CHECK(w == 640 && h == 480);

	side = HUI_FindWidget(window, "side");
	title = HUI_FindWidget(window, "title");
	save = HUI_FindWidget(window, "save");
	scene = HUI_FindWidget(window, "scene");
	tree = HUI_FindWidget(window, "tree");
	quality = HUI_FindWidget(window, "quality");
	CHECK(HUI_GetWidgetType(side) == HUI_WIDGET_BORDER);
	CHECK(HUI_GetWidgetType(scene) == HUI_WIDGET_VIEWPORT);

	/* entities and text content */
	CHECK_STR(HUI_GetLabelText(title), "Scene <1>");
	CHECK(HUI_IsCheckboxChecked(HUI_FindWidget(window, "grid")));
	CHECK_NEAR(HUI_GetSliderValue(HUI_FindWidget(window, "zoom")) * 10, 25);
	CHECK(HUI_GetComboBoxItemCount(quality) == 2 && HUI_GetComboBoxSelected(quality) == 1);
	CHECK_STR(HUI_GetComboBoxItemText(quality, 1), "High");
	CHECK(HUI_GetTreeItemCount(tree) == 1);
	CHECK_STR(HUI_GetTreeItemText(HUI_GetTreeItem(tree, 0)), "World");
	CHECK(HUI_IsTreeItemExpanded(HUI_GetTreeItem(tree, 0)));
	CHECK_STR(HUI_GetTreeItemText(HUI_GetTreeItemChild(HUI_GetTreeItem(tree, 0), 0)), "Player");

	/* layout described by the attributes */
	HUI_GetWidgetRect(side, &x, &y, &fw, &fh);
	CHECK_NEAR(fw, 200);
	CHECK_NEAR(x, 0);
	HUI_GetWidgetRect(scene, &x, &y, &fw, &fh);
	CHECK_NEAR(x, 208);
	CHECK_NEAR(fw, 640 - 208);
	HUI_GetViewportPixelSize(scene, &pw, &ph);
	CHECK(pw == 640 - 208 && ph > 400);
	HUI_GetWidgetRect(title, &x, &y, &fw, &fh);
	CHECK_NEAR(x, 8);

	/* actions: button click, window shortcut, menu shortcut */
	HUI_BindAction("file.save", on_action, NULL);
	HUI_BindAction("file.open", on_action, NULL);
	g_actions = 0;
	test_click(window, save);
	CHECK(g_actions == 1 && g_sender == save);
	test_key(window, HUI_KEY_S, HUI_MOD_CTRL);
	CHECK(g_actions == 2 && g_sender == window);
	test_key(window, HUI_KEY_O, HUI_MOD_CTRL);
	CHECK(g_actions == 3 && g_sender == HUI_FindWidget(window, "open"));

	HUI_DeleteWindow(window);
	CHECK(!HUI_IsValidWindow(window));
	CHECK(!HUI_IsValidWidget(save));
}

static void test_widget_fragment(void) {
	HUI_id grid = widget_from(
		"<Grid Columns='auto,1' Rows='auto,auto' Spacing='2'>"
		"  <Label Text='Name' Slot.Column='0' Slot.Row='0'/>"
		"  <TextInput Name='name' Placeholder='type' Slot.Column='1' Slot.Row='0'/>"
		"  <ProgressBar Value='0.25' Slot.Column='0' Slot.Row='1' Slot.ColumnSpan='2'/>"
		"</Grid>");
	HUI_id tabs, canvas;
	CHECK(HUI_GetWidgetType(grid) == HUI_WIDGET_GRID);
	CHECK(HUI_GetChildCount(grid) == 3);
	CHECK(HUI_GetWidgetType(HUI_FindWidget(grid, "name")) == HUI_WIDGET_TEXT_INPUT);
	CHECK_NEAR(HUI_GetProgressBarValue(HUI_GetChild(grid, 2)) * 100, 25);
	HUI_DeleteWidget(grid);

	/* tags and attribute names are not case sensitive; Slate names are accepted */
	tabs = widget_from(
		"<tabview activetab='1'>"
		"  <VerticalBox slot.title='General'><textblock text='a'/></VerticalBox>"
		"  <ScrollBox Slot.Title='Advanced' orientation='horizontal'><Spacer Width='10' Height='10'/></ScrollBox>"
		"</tabview>");
	CHECK(HUI_GetWidgetType(tabs) == HUI_WIDGET_TAB_VIEW);
	CHECK(HUI_GetActiveTab(tabs) == 1);
	CHECK(HUI_GetWidgetType(HUI_GetChild(tabs, 0)) == HUI_WIDGET_VBOX);
	HUI_DeleteWidget(tabs);

	canvas = widget_from(
		"<Canvas><Image Slot.Anchors='1,0' Slot.Alignment='1,0' Slot.Position='-4,4' Slot.Size='32,32'/>"
		"<![CDATA[ ignored <text> ]]></Canvas>");
	CHECK(HUI_GetChildCount(canvas) == 1);
	HUI_DeleteWidget(canvas);
}

static void test_errors(void) {
	EXPECT_ERRORS_BEGIN();
	/* malformed documents */
	CHECK(widget_from("<VBox><Label></VBox>") == HUI_INVALID_ID);
	CHECK(widget_from("<VBox") == HUI_INVALID_ID);
	CHECK(widget_from("") == HUI_INVALID_ID);
	CHECK(widget_from("<VBox attr=unquoted/>") == HUI_INVALID_ID);
	CHECK(widget_from("<VBox/><VBox/>") == HUI_INVALID_ID);
	CHECK(widget_from("<VBox>&bogus;</VBox>") == HUI_INVALID_ID);
	/* unknown widget, child in a widget that takes none, wrong root */
	CHECK(widget_from("<VBox><Blink/></VBox>") == HUI_INVALID_ID);
	CHECK(widget_from("<Label><Button/></Label>") == HUI_INVALID_ID);
	CHECK(widget_from("<Window/>") == HUI_INVALID_ID);
	CHECK(HUI_CreateWindowFromXml("<VBox/>", 7) == HUI_INVALID_ID);
	CHECK(HUI_CreateWindowFromXml("<Window><VBox/><VBox/></Window>", 31) == HUI_INVALID_ID);
	CHECK(HUI_CreateWidgetFromXmlFile("/no/such/file.xml") == HUI_INVALID_ID);
	EXPECT_ERRORS_END(12);

	/* an unknown attribute is reported but does not stop the loading */
	{
		const char* detail = NULL;
		HUI_id label;
		EXPECT_ERRORS_BEGIN();
		label = widget_from("<VBox>\n  <Label Text=\"ok\" Colour=\"red\"/>\n</VBox>");
		EXPECT_ERRORS_END(1);
		CHECK(HUI_IsValidWidget(label));
		CHECK(HUI_GetLastError(&detail, NULL) == HUI_ERROR_XML);
		/* the message carries the position of the element */
		CHECK(detail && strstr(detail, "line 2") != NULL);
		CHECK(detail && strstr(detail, "Colour") != NULL);
		HUI_DeleteWidget(label);
	}
	/* nothing leaked from the failed documents: no widget is left */
	CHECK(HUI_GetLastError(NULL, NULL) == HUI_NO_ERROR);
}

int main(void) {
	if (!test_init(0)) {
		printf("cannot initialize HUI\n");
		return 1;
	}
	RUN(test_window);
	RUN(test_widget_fragment);
	RUN(test_errors);
	HUI_Shutdown();
	return test_report("test_xml");
}
