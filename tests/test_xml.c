/**
 * Raven tests - XML: parsing, building windows and widgets, error reporting.
 */

#include "test.h"

static int g_actions;
static Rid g_sender;

static void on_action(Rid sender, const char* action, void* user_data) {
	(void)action;
	(void)user_data;
	g_actions++;
	g_sender = sender;
}

static Rid widget_from(const char* xml) {
	return Rwidget_create_from_xml(xml, strlen(xml));
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
	Rid window = Rwindow_create_from_xml(xml, sizeof(xml) - 1);
	Rid side, title, save, scene, tree, quality;
	int w = 0, h = 0, pw = 0, ph = 0;
	float x, y, fw, fh;

	CHECK(Rwindow_is_valid(window));
	if (!Rwindow_is_valid(window)) return;
	Rwindow_get_size(window, &w, &h);
	CHECK(w == 640 && h == 480);

	side = Rwidget_find(window, "side");
	title = Rwidget_find(window, "title");
	save = Rwidget_find(window, "save");
	scene = Rwidget_find(window, "scene");
	tree = Rwidget_find(window, "tree");
	quality = Rwidget_find(window, "quality");
	CHECK(Rwidget_get_type(side) == R_WIDGET_BORDER);
	CHECK(Rwidget_get_type(scene) == R_WIDGET_VIEWPORT);

	/* entities and text content */
	CHECK_STR(Rlabel_get_text(title), "Scene <1>");
	CHECK(Rcheckbox_is_checked(Rwidget_find(window, "grid")));
	CHECK_NEAR(Rslider_get_value(Rwidget_find(window, "zoom")) * 10, 25);
	CHECK(Rcombo_box_get_item_count(quality) == 2 && Rcombo_box_get_selected(quality) == 1);
	CHECK_STR(Rcombo_box_get_item_text(quality, 1), "High");
	CHECK(Rtree_view_get_item_count(tree) == 1);
	CHECK_STR(Rtree_item_get_text(Rtree_view_get_item(tree, 0)), "World");
	CHECK(Rtree_item_is_expanded(Rtree_view_get_item(tree, 0)));
	CHECK_STR(Rtree_item_get_text(Rtree_item_get_child(Rtree_view_get_item(tree, 0), 0)), "Player");

	/* layout described by the attributes */
	Rwidget_get_rect(side, &x, &y, &fw, &fh);
	CHECK_NEAR(fw, 200);
	CHECK_NEAR(x, 0);
	Rwidget_get_rect(scene, &x, &y, &fw, &fh);
	CHECK_NEAR(x, 208);
	CHECK_NEAR(fw, 640 - 208);
	Rviewport_get_pixel_size(scene, &pw, &ph);
	CHECK(pw == 640 - 208 && ph > 400);
	Rwidget_get_rect(title, &x, &y, &fw, &fh);
	CHECK_NEAR(x, 8);

	/* actions: button click, window shortcut, menu shortcut */
	Raction_bind("file.save", on_action, NULL);
	Raction_bind("file.open", on_action, NULL);
	g_actions = 0;
	test_click(window, save);
	CHECK(g_actions == 1 && g_sender == save);
	test_key(window, R_KEY_S, R_MOD_CTRL);
	CHECK(g_actions == 2 && g_sender == window);
	test_key(window, R_KEY_O, R_MOD_CTRL);
	CHECK(g_actions == 3 && g_sender == Rwidget_find(window, "open"));

	Rwindow_destroy(window);
	CHECK(!Rwindow_is_valid(window));
	CHECK(!Rwidget_is_valid(save));
}

static void test_widget_fragment(void) {
	Rid grid = widget_from(
		"<Grid Columns='auto,1' Rows='auto,auto' Spacing='2'>"
		"  <Label Text='Name' Slot.Column='0' Slot.Row='0'/>"
		"  <TextInput Name='name' Placeholder='type' Slot.Column='1' Slot.Row='0'/>"
		"  <ProgressBar Value='0.25' Slot.Column='0' Slot.Row='1' Slot.ColumnSpan='2'/>"
		"</Grid>");
	Rid tabs, canvas;
	CHECK(Rwidget_get_type(grid) == R_WIDGET_GRID);
	CHECK(Rwidget_get_child_count(grid) == 3);
	CHECK(Rwidget_get_type(Rwidget_find(grid, "name")) == R_WIDGET_TEXT_INPUT);
	CHECK_NEAR(Rprogress_bar_get_value(Rwidget_get_child(grid, 2)) * 100, 25);
	Rwidget_destroy(grid);

	/* tags and attribute names are not case sensitive; Slate names are accepted */
	tabs = widget_from(
		"<tabview activetab='1'>"
		"  <VerticalBox slot.title='General'><textblock text='a'/></VerticalBox>"
		"  <ScrollBox Slot.Title='Advanced' orientation='horizontal'><Spacer Width='10' Height='10'/></ScrollBox>"
		"</tabview>");
	CHECK(Rwidget_get_type(tabs) == R_WIDGET_TAB_VIEW);
	CHECK(Rtab_view_get_active(tabs) == 1);
	CHECK(Rwidget_get_type(Rwidget_get_child(tabs, 0)) == R_WIDGET_VBOX);
	Rwidget_destroy(tabs);

	canvas = widget_from(
		"<Canvas><Image Slot.Anchors='1,0' Slot.Alignment='1,0' Slot.Position='-4,4' Slot.Size='32,32'/>"
		"<![CDATA[ ignored <text> ]]></Canvas>");
	CHECK(Rwidget_get_child_count(canvas) == 1);
	Rwidget_destroy(canvas);
}

static void test_errors(void) {
	EXPECT_ERRORS_BEGIN();
	/* malformed documents */
	CHECK(widget_from("<VBox><Label></VBox>") == R_INVALID_ID);
	CHECK(widget_from("<VBox") == R_INVALID_ID);
	CHECK(widget_from("") == R_INVALID_ID);
	CHECK(widget_from("<VBox attr=unquoted/>") == R_INVALID_ID);
	CHECK(widget_from("<VBox/><VBox/>") == R_INVALID_ID);
	CHECK(widget_from("<VBox>&bogus;</VBox>") == R_INVALID_ID);
	/* unknown widget, child in a widget that takes none, wrong root */
	CHECK(widget_from("<VBox><Blink/></VBox>") == R_INVALID_ID);
	CHECK(widget_from("<Label><Button/></Label>") == R_INVALID_ID);
	CHECK(widget_from("<Window/>") == R_INVALID_ID);
	CHECK(Rwindow_create_from_xml("<VBox/>", 7) == R_INVALID_ID);
	CHECK(Rwindow_create_from_xml("<Window><VBox/><VBox/></Window>", 31) == R_INVALID_ID);
	CHECK(Rwidget_create_from_xml_file("/no/such/file.xml") == R_INVALID_ID);
	EXPECT_ERRORS_END(12);

	/* an unknown attribute is reported but does not stop the loading */
	{
		const char* detail = NULL;
		Rid label;
		EXPECT_ERRORS_BEGIN();
		label = widget_from("<VBox>\n  <Label Text=\"ok\" Colour=\"red\"/>\n</VBox>");
		EXPECT_ERRORS_END(1);
		CHECK(Rwidget_is_valid(label));
		CHECK(Rerror_get_last(&detail, NULL) == R_ERROR_XML);
		/* the message carries the position of the element */
		CHECK(detail && strstr(detail, "line 2") != NULL);
		CHECK(detail && strstr(detail, "Colour") != NULL);
		Rwidget_destroy(label);
	}
	/* nothing leaked from the failed documents: no widget is left */
	CHECK(Rerror_get_last(NULL, NULL) == R_NO_ERROR);
}

int main(void) {
	if (!test_init(0)) {
		printf("cannot initialize Raven\n");
		return 1;
	}
	RUN(test_window);
	RUN(test_widget_fragment);
	RUN(test_errors);
	Rshutdown();
	return test_report("test_xml");
}
