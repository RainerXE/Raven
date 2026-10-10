/**
 * Raven Designer - edits the XML of an interface, visually.
 *
 *   raven_designer [file.xml]
 *
 * The window shows the same document five ways, all kept in step:
 *
 *   Widgets      what can be added: the tags of the library and those the
 *                document declares with <Widget>
 *   Tree         the elements; arrows and buttons move, wrap, copy, delete
 *   Preview      the interface, built for real; a click selects an element
 *   Properties   the attributes of the selected element, by kind of value
 *   XML          the text itself, colored, with the mistakes underlined
 *
 * Everything here uses the public API of Raven only: the document is a
 * Rxml_document, the lists of tags and attributes come from Rxml_get_tag_...
 * and Rxml_get_attribute_..., and the preview finds the element under the
 * pointer with Rwidget_pick and Rwidget_get_xml_node. The interface of the
 * designer is itself described in XML (DESIGNER_XML below).
 *
 * For automated runs (the tests of the build use them):
 *
 *   --null             no window system and no GPU
 *   --headless         offscreen rendering
 *   --frames N         stops after N frames
 *   --script "..."     commands run before the frames: see designer_script
 *   --capture x.ppm    saves the last frame
 */

#include <Raven.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---------------------------------------------------------------------------
 *  The interface of the designer
 * ------------------------------------------------------------------------- */

/* (in pieces: a C compiler is only required to accept texts of 4095 characters) */
static const char* const DESIGNER_XML[] = {
"<Window Title=\"Raven Designer\" Width=\"1380\" Height=\"860\" MinWidth=\"900\" MinHeight=\"560\" CustomHeader=\"true\">"
"  <Widget Name=\"ToolButton\" Text=\"\" Tip=\"\" Do=\"\">"
"    <Button Text=\"{Text}\" Style=\"Flat\" Tooltip=\"{Tip}\" OnClicked=\"{Do}\" Metric.PaddingX=\"8\"/>"
"  </Widget>"
"  <VBox>"
"    <HeaderBar Name=\"header\" Title=\"Raven Designer\">"
"      <MenuBar>"
"        <Menu Text=\"File\">"
"          <Menu Text=\"New\">"
"            <MenuItem Text=\"Window\" Shortcut=\"Ctrl+N\" OnClicked=\"file.new.window\"/>"
"            <MenuItem Text=\"Panel\" OnClicked=\"file.new.panel\"/>"
"            <MenuItem Text=\"Form with declared widgets\" OnClicked=\"file.new.form\"/>"
"          </Menu>"
"          <MenuItem Text=\"Open...\" Shortcut=\"Ctrl+O\" OnClicked=\"file.open\"/>"
"          <Separator/>"
"          <MenuItem Text=\"Save\" Shortcut=\"Ctrl+S\" OnClicked=\"file.save\"/>"
"          <MenuItem Text=\"Save as...\" Shortcut=\"Ctrl+Shift+S\" OnClicked=\"file.save_as\"/>"
"          <Separator/>"
"          <MenuItem Text=\"Quit\" Shortcut=\"Ctrl+Q\" OnClicked=\"file.quit\"/>"
"        </Menu>",
"        <Menu Text=\"Edit\">"
"          <MenuItem Text=\"Undo\" Shortcut=\"Ctrl+Z\" OnClicked=\"edit.undo\"/>"
"          <MenuItem Text=\"Redo\" Shortcut=\"Ctrl+Y\" OnClicked=\"edit.redo\"/>"
"          <Separator/>"
"          <MenuItem Text=\"Duplicate\" Shortcut=\"Ctrl+D\" OnClicked=\"edit.duplicate\"/>"
"          <MenuItem Text=\"Delete\" Shortcut=\"Delete\" OnClicked=\"edit.delete\"/>"
"          <MenuItem Text=\"Move up\" Shortcut=\"Alt+Up\" OnClicked=\"edit.up\"/>"
"          <MenuItem Text=\"Move down\" Shortcut=\"Alt+Down\" OnClicked=\"edit.down\"/>"
"          <MenuItem Text=\"Move out\" Shortcut=\"Alt+Left\" OnClicked=\"edit.out\"/>"
"          <MenuItem Text=\"Move into the previous\" Shortcut=\"Alt+Right\" OnClicked=\"edit.in\"/>"
"          <Separator/>"
"          <Menu Text=\"Wrap in\">"
"            <MenuItem Text=\"VBox\" OnClicked=\"edit.wrap.VBox\"/>"
"            <MenuItem Text=\"HBox\" OnClicked=\"edit.wrap.HBox\"/>"
"            <MenuItem Text=\"Border\" OnClicked=\"edit.wrap.Border\"/>"
"            <MenuItem Text=\"ScrollBox\" OnClicked=\"edit.wrap.ScrollBox\"/>"
"            <MenuItem Text=\"Overlay\" OnClicked=\"edit.wrap.Overlay\"/>"
"          </Menu>"
"          <MenuItem Text=\"Unwrap\" OnClicked=\"edit.unwrap\"/>"
"          <Separator/>"
"          <MenuItem Text=\"Copy as XML\" Shortcut=\"Ctrl+Shift+C\" OnClicked=\"edit.copy\"/>"
"          <MenuItem Text=\"Paste XML inside\" Shortcut=\"Ctrl+Shift+V\" OnClicked=\"edit.paste\"/>"
"        </Menu>",
"        <Menu Text=\"View\">"
"          <MenuItem Name=\"theme.dark\" Text=\"Dark\" Checkable=\"true\" Checked=\"true\" OnClicked=\"view.theme.dark\"/>"
"          <MenuItem Name=\"theme.light\" Text=\"Light\" Checkable=\"true\" OnClicked=\"view.theme.light\"/>"
"          <MenuItem Name=\"theme.glass_dark\" Text=\"Dark glass\" Checkable=\"true\" OnClicked=\"view.theme.glass_dark\"/>"
"          <MenuItem Name=\"theme.glass_light\" Text=\"Light glass\" Checkable=\"true\" OnClicked=\"view.theme.glass_light\"/>"
"          <Separator/>"
"          <MenuItem Name=\"menu.interact\" Text=\"Use the preview\" Shortcut=\"F6\" Checkable=\"true\" OnClicked=\"view.interact\"/>"
"          <MenuItem Text=\"Run in a window\" Shortcut=\"F5\" OnClicked=\"view.run\"/>"
"          <Separator/>"
"          <MenuItem Text=\"Reset the panels\" OnClicked=\"view.reset\"/>"
"        </Menu>"
"      </MenuBar>"
"      <Button Text=\"Run\" Style=\"Primary\" OnClicked=\"view.run\" Tooltip=\"Opens the interface in a window of its own (F5)\""
"        Slot.HAlign=\"Right\" Slot.VAlign=\"Center\" Slot.Padding=\"8,0\" Metric.ControlHeight=\"26\"/>"
"    </HeaderBar>",
"    <DockArea Name=\"dock\" Slot.Fill=\"1\">"
"      <VBox Name=\"panel.preview\" Slot.Title=\"Preview\" Slot.Closable=\"false\">"
"        <HBox Spacing=\"6\" Slot.Padding=\"8,6\">"
"          <Checkbox Name=\"interact\" Text=\"Use the preview\" Style=\"Switch\" OnChanged=\"view.interact.switch\""
"            Tooltip=\"On: the preview reacts as the interface will. Off: a click selects.\" Slot.VAlign=\"Center\"/>"
"          <Spacer Slot.Fill=\"1\"/>"
"          <Label Name=\"stage.size\" Opacity=\"0.6\" Slot.VAlign=\"Center\"/>"
"        </HBox>"
"        <Separator/>"
"        <ScrollBox Name=\"stage.scroll\" ScrollBoth=\"true\" Slot.Fill=\"1\">"
"          <Overlay Name=\"stage\" Slot.Padding=\"28\" Slot.HAlign=\"Center\"/>"
"        </ScrollBox>"
"      </VBox>"
"      <VBox Name=\"panel.widgets\" Slot.Title=\"Widgets\" Slot.Dock=\"Left\" Slot.DockSize=\"0.2\" Slot.Closable=\"false\">"
"        <TextInput Name=\"palette.filter\" Placeholder=\"Search a widget\" Slot.Padding=\"6\" OnChanged=\"palette.filter\"/>"
"        <ListView Name=\"palette\" Slot.Fill=\"1\" OnActivated=\"palette.add\" Tooltip=\"Double click to add inside the selected element\"/>"
"        <Button Text=\"Add to the selection\" OnClicked=\"palette.add\" Slot.Padding=\"6\"/>"
"      </VBox>",
"      <VBox Name=\"panel.tree\" Slot.Title=\"Tree\" Slot.Dock=\"Bottom\" Slot.DockTo=\"panel.widgets\" Slot.DockSize=\"0.5\" Slot.Closable=\"false\">"
"        <WrapBox Spacing=\"0\" Slot.Padding=\"4,2\">"
"          <ToolButton Text=\"Up\" Tip=\"Before the previous element (Alt+Up)\" Do=\"edit.up\"/>"
"          <ToolButton Text=\"Down\" Tip=\"After the next element (Alt+Down)\" Do=\"edit.down\"/>"
"          <ToolButton Text=\"Out\" Tip=\"Out of its parent (Alt+Left)\" Do=\"edit.out\"/>"
"          <ToolButton Text=\"In\" Tip=\"Into the element before it (Alt+Right)\" Do=\"edit.in\"/>"
"          <ToolButton Text=\"Copy\" Tip=\"Duplicate (Ctrl+D)\" Do=\"edit.duplicate\"/>"
"          <ToolButton Text=\"Delete\" Tip=\"Delete (Delete)\" Do=\"edit.delete\"/>"
"        </WrapBox>"
"        <Separator/>"
"        <TreeView Name=\"tree\" Slot.Fill=\"1\"/>"
"      </VBox>",
"      <VBox Name=\"panel.properties\" Slot.Title=\"Properties\" Slot.Dock=\"Right\" Slot.DockSize=\"0.27\" Slot.Closable=\"false\">"
"        <VBox Slot.Padding=\"10,8\" Spacing=\"2\">"
"          <Label Name=\"inspector.title\" TextSize=\"17\"/>"
"          <Label Name=\"inspector.about\" Opacity=\"0.6\" Wrap=\"true\"/>"
"        </VBox>"
"        <Separator/>"
"        <ScrollBox Slot.Fill=\"1\">"
"          <VBox Name=\"inspector\" Spacing=\"4\" Slot.Padding=\"8,6\"/>"
"        </ScrollBox>"
"      </VBox>"
"      <TextEditor Name=\"source\" Language=\"XML\" LineNumbers=\"true\" Slot.Title=\"XML\" Slot.Dock=\"Bottom\" Slot.DockTo=\"panel.preview\""
"        Slot.DockSize=\"0.36\" Slot.Closable=\"false\"/>"
"      <ListView Name=\"problems\" Slot.Title=\"Problems\" Slot.Dock=\"Tab\" Slot.DockTo=\"source\" Slot.Closable=\"false\" OnActivated=\"problems.go\"/>"
"    </DockArea>"
"    <Separator/>"
"    <HBox Spacing=\"12\" Slot.Padding=\"10,4\">"
"      <Label Name=\"status\" Slot.Fill=\"1\" Opacity=\"0.8\"/>"
"      <Label Name=\"path\" Opacity=\"0.6\"/>"
"    </HBox>"
"  </VBox>"
"</Window>",
};

/* ---------------------------------------------------------------------------
 *  Documents to start from
 * ------------------------------------------------------------------------- */

static const char TEMPLATE_WINDOW[] =
"<Window Title=\"My window\" Width=\"520\" Height=\"360\">\n"
"\t<VBox Spacing=\"10\" Slot.Padding=\"16\">\n"
"\t\t<Label Text=\"Hello\" TextSize=\"22\"/>\n"
"\t\t<Label Text=\"Select an element, change it on the right, or type in the XML below.\" Wrap=\"true\" Opacity=\"0.7\"/>\n"
"\t\t<HBox Spacing=\"8\">\n"
"\t\t\t<TextInput Name=\"name\" Placeholder=\"Your name\" Slot.Fill=\"1\"/>\n"
"\t\t\t<Button Name=\"ok\" Text=\"Greet\" Style=\"Primary\" OnClicked=\"app.greet\"/>\n"
"\t\t</HBox>\n"
"\t\t<Checkbox Text=\"Remember me\" Style=\"Switch\"/>\n"
"\t\t<Slider Value=\"0.4\"/>\n"
"\t\t<Spacer Slot.Fill=\"1\"/>\n"
"\t\t<ProgressBar Value=\"0.6\"/>\n"
"\t</VBox>\n"
"</Window>\n";

static const char TEMPLATE_PANEL[] =
"<Border Padding=\"14\" Width=\"320\">\n"
"\t<VBox Spacing=\"8\">\n"
"\t\t<Label Text=\"A panel\" TextSize=\"18\"/>\n"
"\t\t<Separator/>\n"
"\t\t<Checkbox Text=\"One\" Checked=\"true\"/>\n"
"\t\t<Checkbox Text=\"Two\"/>\n"
"\t\t<Button Text=\"Apply\" Slot.HAlign=\"Right\"/>\n"
"\t</VBox>\n"
"</Border>\n";

static const char TEMPLATE_FORM[] =
"<Window Title=\"Sign up\" Width=\"460\" Height=\"380\">\n"
"\t<!-- A tag of our own: a label and an input on one line. -->\n"
"\t<Widget Name=\"Field\" Label=\"Name\" Hint=\"\">\n"
"\t\t<HBox Spacing=\"10\">\n"
"\t\t\t<Label Text=\"{Label}\" Width=\"90\" Slot.VAlign=\"Center\"/>\n"
"\t\t\t<TextInput Placeholder=\"{Hint}\" Slot.Fill=\"1\"/>\n"
"\t\t\t<Children/>\n"
"\t\t</HBox>\n"
"\t</Widget>\n"
"\t<VBox Spacing=\"10\" Slot.Padding=\"18\">\n"
"\t\t<Label Text=\"Create an account\" TextSize=\"20\"/>\n"
"\t\t<Field Label=\"Name\" Hint=\"Ada Lovelace\"/>\n"
"\t\t<Field Label=\"Email\" Hint=\"ada@example.com\"/>\n"
"\t\t<Field Label=\"Password\">\n"
"\t\t\t<Button Text=\"Show\" Style=\"Flat\"/>\n"
"\t\t</Field>\n"
"\t\t<Repeat Count=\"2\" Var=\"i\">\n"
"\t\t\t<Checkbox Text=\"Option {i + 1}\"/>\n"
"\t\t</Repeat>\n"
"\t\t<Spacer Slot.Fill=\"1\"/>\n"
"\t\t<Button Text=\"Sign up\" Style=\"Primary\" Slot.HAlign=\"Right\"/>\n"
"\t</VBox>\n"
"</Window>\n";

/** What the palette adds for a tag, when an empty element would not show. */
static const struct { const char* tag; const char* xml; } TEMPLATES[] = {
	{ "VBox", "<VBox Spacing=\"8\"/>" },
	{ "HBox", "<HBox Spacing=\"8\"/>" },
	{ "Grid", "<Grid Columns=\"1,1\" Spacing=\"8\"><Label Text=\"A\"/><Label Text=\"B\" Slot.Column=\"1\"/></Grid>" },
	{ "UniformGrid", "<UniformGrid Columns=\"2\" Spacing=\"8\"><Button Text=\"1\"/><Button Text=\"2\"/><Button Text=\"3\"/><Button Text=\"4\"/></UniformGrid>" },
	{ "Border", "<Border Padding=\"12\"><Label Text=\"Border\"/></Border>" },
	{ "SizeBox", "<SizeBox Width=\"120\" Height=\"80\"><Border/></SizeBox>" },
	{ "Spacer", "<Spacer Width=\"16\" Height=\"16\"/>" },
	{ "ScrollBox", "<ScrollBox Height=\"120\"><VBox Spacing=\"6\"><Label Text=\"One\"/><Label Text=\"Two\"/><Label Text=\"Three\"/></VBox></ScrollBox>" },
	{ "Splitter", "<Splitter Height=\"120\"><Label Text=\"Left\"/><Label Text=\"Right\"/></Splitter>" },
	{ "WrapBox", "<WrapBox Spacing=\"6\"><Button Text=\"One\"/><Button Text=\"Two\"/><Button Text=\"Three\"/></WrapBox>" },
	{ "Canvas", "<Canvas Height=\"120\"><Button Text=\"Here\" Slot.Position=\"20,20\" Slot.Size=\"80,28\"/></Canvas>" },
	{ "Switcher", "<Switcher><Label Text=\"First\"/><Label Text=\"Second\"/></Switcher>" },
	{ "TabView", "<TabView Height=\"140\"><Label Text=\"First tab\" Slot.Title=\"One\"/><Label Text=\"Second tab\" Slot.Title=\"Two\"/></TabView>" },
	{ "Expander", "<Expander Title=\"More\" Expanded=\"true\"><Label Text=\"Details\"/></Expander>" },
	{ "Label", "<Label Text=\"Label\"/>" },
	{ "Button", "<Button Text=\"Button\"/>" },
	{ "Checkbox", "<Checkbox Text=\"Checkbox\"/>" },
	{ "Slider", "<Slider Value=\"0.5\"/>" },
	{ "ProgressBar", "<ProgressBar Value=\"0.4\"/>" },
	{ "Image", "<Image Width=\"96\" Height=\"96\"/>" },
	{ "TextInput", "<TextInput Placeholder=\"Type here\"/>" },
	{ "SpinBox", "<SpinBox Min=\"0\" Max=\"100\" Value=\"10\"/>" },
	{ "ComboBox", "<ComboBox Items=\"One|Two|Three\" Selected=\"0\"/>" },
	{ "ListView", "<ListView Items=\"One|Two|Three\" Height=\"100\"/>" },
	{ "TreeView", "<TreeView Height=\"110\"><Item Text=\"Root\"><Item Text=\"Child\"/><Item Text=\"Other child\"/></Item></TreeView>" },
	{ "MenuBar", "<MenuBar><Menu Text=\"File\"><MenuItem Text=\"Open\"/><Separator/><MenuItem Text=\"Quit\"/></Menu></MenuBar>" },
	{ "Menu", "<Menu Text=\"Menu\"><MenuItem Text=\"Item\"/></Menu>" },
	{ "MenuItem", "<MenuItem Text=\"Item\"/>" },
	{ "Viewport", "<Viewport Width=\"160\" Height=\"100\"/>" },
	{ "Custom", "<Custom Width=\"80\" Height=\"40\"/>" },
	{ "TextEditor", "<TextEditor Height=\"110\" Placeholder=\"Notes\"/>" },
	{ "HeaderBar", "<HeaderBar Title=\"Title\"/>" },
	{ "FileBrowser", "<FileBrowser Height=\"300\"/>" },
	{ "DockArea", "<DockArea Height=\"220\"><Label Text=\"Center\" Slot.Title=\"Main\"/><Label Text=\"Side\" Slot.Title=\"Side\" Slot.Dock=\"Left\" Slot.DockSize=\"0.3\"/></DockArea>" },
	{ "Island", "<Island ExpandOnHover=\"true\"><Label Text=\"Island\"/><VBox Spacing=\"6\" Slot.Padding=\"8\"><Label Text=\"Unfolded\"/><Button Text=\"Action\"/></VBox></Island>" },
	{ "BackgroundBlur", "<BackgroundBlur Style=\"Frosted\" Padding=\"12\"><Label Text=\"Blur\"/></BackgroundBlur>" },
	{ "ColorPicker", "<ColorPicker Color=\"#3B82F6\"/>" },
	{ "Timeline", "<Timeline Duration=\"10\" Height=\"150\"><Track Name=\"Track\"><Clip Start=\"1\" Length=\"3\" Label=\"Clip\"/><Key Time=\"6\"/></Track><Marker Time=\"5\" Label=\"Mark\"/></Timeline>" },
	{ "NodeGraph", "<NodeGraph Height=\"220\"><Node Name=\"a\" Title=\"Source\" X=\"30\" Y=\"40\"><Output Name=\"out\" Label=\"Value\"/></Node>"
		"<Node Name=\"b\" Title=\"Result\" X=\"260\" Y=\"70\"><Input Name=\"in\" Label=\"Value\"/></Node><Link From=\"a.out\" To=\"b.in\"/></NodeGraph>" },
	{ "Item", "<Item Text=\"Item\"/>" },
	{ "Shortcut", "<Shortcut Keys=\"Ctrl+S\" Action=\"file.save\"/>" }
};

/** Tags that are not widgets, and what can be written on them. */
static const struct { const char* tag; const char* attributes; const char* about; } SPECIAL_TAGS[] = {
	{ "Item", "Text", "A row of the list, tree or combo box it is in." },
	{ "Shortcut", "Keys|Action", "Keys that fire an action anywhere in the window." },
	{ "Track", "Name|Color|Height|Locked|Muted", "A track of a Timeline: it holds Clip and Key elements." },
	{ "Clip", "Start|Length|End|Label|Color|Selected", "Something that lasts, on a track of a Timeline." },
	{ "Key", "Time|Label|Color", "A moment, on a track of a Timeline." },
	{ "Marker", "Time|Label|Color", "A flag on the ruler of a Timeline." },
	{ "Node", "Name|Title|X|Y|Color|Selected", "A node of a NodeGraph: Input and Output elements, and a widget for its inside." },
	{ "Input", "Name|Label|Type", "A pin on the left of a Node." },
	{ "Output", "Name|Label|Type", "A pin on the right of a Node." },
	{ "Link", "From|To", "A wire of a NodeGraph, from \"node.output\" to \"node.input\"." },
	{ "Type", "Name|Color", "The color of the pins of a type, in a NodeGraph." },
	{ "Widget", "Name", "Declares a tag: its other attributes are parameters, with their default values." },
	{ "Widgets", "", "A document of <Widget> declarations." },
	{ "Children", "Name", "Where the elements given to a declared tag go." },
	{ "Repeat", "Count|Var", "Repeats what it contains." },
	{ "ForEach", "Items|Var|Index|Separator", "Repeats what it contains for each text of a list." },
	{ "If", "Test", "Keeps what it contains when the test is true." },
	{ "Include", "Source", "Brings in another file." }
};

/* ---------------------------------------------------------------------------
 *  State
 * ------------------------------------------------------------------------- */

#define MAX_HISTORY 200
#define MAX_PROBLEMS 64

/** A row of the inspector: which attribute its editor writes. */
typedef struct row_t {
	char name[72];
	RXmlValueType type;
	/** Choices of its combo box: index 0 is "not set". */
	const char* choices;
	Rid swatch;
} row_t;

typedef struct problem_t {
	char text[260];
	int line;
	int error;
} problem_t;

static struct {
	Rid window, dock, source, tree, palette, palette_filter, inspector, problems_list, status, path_label;
	Rid stage, frame, outline, preview, interact_switch;
	Rid context_menu;
	Rid run_window;

	/** The document, and the element everything is about. */
	Rid document;
	Rid selected;
	Rid hovered;

	char* path;
	int modified;
	/** The text of the editor could not be read: the document is the last one that could. */
	int broken;

	/** Texts of the document, oldest first; `history_at` is the current one. */
	char* history[MAX_HISTORY];
	int history_count;
	int history_at;
	double history_typed_at;

	/** Set while the designer itself writes the text of the editor, or fills a list. */
	int syncing;
	Rid parse_timer;
	int interact;
	int quitting;

	row_t* rows;
	int row_count;
	int row_cap;

	problem_t problems[MAX_PROBLEMS];
	int problem_count;
	int collecting;

	/** Names of the palette as shown: what a row adds. */
	char** palette_tags;
	int palette_count;

	/** Where the outline of the selection is drawn: it glides to its place. */
	RSpring box[4];
	RSpring box_shown;
} g;

static void rebuild_all(int keep_inspector);
static void select_node(Rid node, int reveal);
static int load_text(const char* text, int record);

/* ---------------------------------------------------------------------------
 *  Small things
 * ------------------------------------------------------------------------- */

static char* copy_text(const char* text) {
	size_t length = strlen(text) + 1;
	char* copy = (char*)malloc(length);
	memcpy(copy, text, length);
	return copy;
}

static void set_status(const char* format, ...) {
	char text[320];
	va_list args;
	va_start(args, format);
	vsnprintf(text, sizeof(text), format, args);
	va_end(args);
	Rlabel_set_text(g.status, text);
}

static int same_text(const char* a, const char* b) {
	for (; *a && *b; a++, b++) {
		char x = *a, y = *b;
		if (x >= 'A' && x <= 'Z') x = (char)(x + 32);
		if (y >= 'A' && y <= 'Z') y = (char)(y + 32);
		if (x != y) return 0;
	}
	return *a == *b;
}

static int contains_text(const char* text, const char* part) {
	size_t length = strlen(part), i, j;
	if (length == 0) return 1;
	for (i = 0; text[i]; i++) {
		for (j = 0; j < length; j++) {
			char x = text[i + j], y = part[j];
			if (x >= 'A' && x <= 'Z') x = (char)(x + 32);
			if (y >= 'A' && y <= 'Z') y = (char)(y + 32);
			if (x != y) break;
		}
		if (j == length) return 1;
	}
	return 0;
}

static int special_tag(const char* tag) {
	int i;
	for (i = 0; i < (int)(sizeof(SPECIAL_TAGS) / sizeof(SPECIAL_TAGS[0])); i++) {
		if (same_text(SPECIAL_TAGS[i].tag, tag)) return i;
	}
	return -1;
}

static void update_title(void) {
	char title[600];
	const char* name = g.path ? g.path : "Untitled";
	const char* slash = strrchr(name, '/');
	const char* backslash = strrchr(name, '\\');
	if (backslash && (!slash || backslash > slash)) slash = backslash;
	snprintf(title, sizeof(title), "%s%s - Raven Designer", slash ? slash + 1 : name, g.modified ? " *" : "");
	Rwindow_set_title(g.window, title);
	Rheader_bar_set_title(Rwidget_find(g.window, "header"), title);
}

/* ---------------------------------------------------------------------------
 *  Problems: what the library reports while the preview is built
 * ------------------------------------------------------------------------- */

static void add_problem(const char* text, int line, int error) {
	problem_t* problem;
	if (g.problem_count >= MAX_PROBLEMS) return;
	problem = &g.problems[g.problem_count++];
	snprintf(problem->text, sizeof(problem->text), "%s", text);
	problem->line = line;
	problem->error = error;
}

static void on_error(RError code, RSeverity severity, const char* detail) {
	if (g.collecting && code == R_ERROR_XML) {
		/* "XML line 12, column 3: ..." */
		const char* at = strstr(detail, "line ");
		add_problem(detail, at ? atoi(at + 5) : 0, severity != R_SEVERITY_WARNING);
		return;
	}
	fprintf(stderr, "[Raven] %s (%s): %s\n", Rerror_to_string(code), Rseverity_to_string(severity), detail);
}

static void show_problems(void) {
	int i;
	Rlist_view_clear_items(g.problems_list);
	Rtext_editor_clear_markers(g.source);
	for (i = 0; i < g.problem_count; i++) {
		const problem_t* problem = &g.problems[i];
		Rlist_view_add_item(g.problems_list, problem->text);
		if (problem->line > 0) {
			Rtext_editor_add_marker(g.source, problem->line - 1, 0, -1, problem->error ? R_MARKER_ERROR : R_MARKER_WARNING, problem->text);
		}
	}
	if (g.problem_count == 0) {
		Rlist_view_add_item(g.problems_list, "No problems.");
		Rdock_area_set_panel_title(g.dock, g.problems_list, "Problems");
	} else {
		char title[32];
		snprintf(title, sizeof(title), "Problems (%d)", g.problem_count);
		Rdock_area_set_panel_title(g.dock, g.problems_list, title);
	}
}

/* ---------------------------------------------------------------------------
 *  Nodes
 * ------------------------------------------------------------------------- */

/* The place of a node as the indices from the root: what survives the text being read again. */
static int node_path(Rid node, int* path, int max) {
	int depth = 0, i;
	Rid walk;
	for (walk = node; Rxml_node_get_parent(walk) != R_INVALID_ID; walk = Rxml_node_get_parent(walk)) depth++;
	if (depth > max) return -1;
	for (walk = node, i = depth - 1; i >= 0; i--, walk = Rxml_node_get_parent(walk)) path[i] = Rxml_node_get_index(walk);
	return depth;
}

static Rid node_at_path(const int* path, int depth) {
	Rid node = Rxml_document_get_root(g.document);
	int i;
	for (i = 0; i < depth; i++) {
		Rid child = Rxml_node_get_child(node, path[i]);
		if (child == R_INVALID_ID) break;
		node = child;
	}
	return node;
}

/* "Button  ok  \"Greet\"": what the tree and the status bar call an element. */
static void node_caption(Rid node, char* out, size_t size) {
	const char* name;
	const char* text;
	if (Rxml_node_is_comment(node)) {
		snprintf(out, size, "<!--%.40s-->", Rxml_node_get_text(node));
		return;
	}
	name = Rxml_node_get_attribute(node, "Name");
	text = Rxml_node_get_attribute(node, "Text");
	if (!text) text = Rxml_node_get_attribute(node, "Title");
	if (!text && Rxml_node_get_text(node)[0]) text = Rxml_node_get_text(node);
	if (name && text) snprintf(out, size, "%s  %s  \"%.28s\"", Rxml_node_get_tag(node), name, text);
	else if (name) snprintf(out, size, "%s  %s", Rxml_node_get_tag(node), name);
	else if (text) snprintf(out, size, "%s  \"%.28s\"", Rxml_node_get_tag(node), text);
	else snprintf(out, size, "%s", Rxml_node_get_tag(node));
}

/* The widget that shows a node in the preview: its own, or the one of the nearest element around it that has one. */
static Rid node_widget(Rid node) {
	if (node == R_INVALID_ID || !Rxml_node_is_valid(node)) return R_INVALID_ID;
	for (; node != R_INVALID_ID; node = Rxml_node_get_parent(node)) {
		Rid widget = Rxml_node_get_widget(node);
		if (widget != R_INVALID_ID && Rwidget_is_valid(widget)) return widget;
	}
	/* the root of a window, or something in it that is not a widget (a shortcut): the frame that stands for the window */
	return g.frame;
}

/* ---------------------------------------------------------------------------
 *  History
 * ------------------------------------------------------------------------- */

static void history_push(const char* text, int typed) {
	int i;
	if (g.history_count > 0 && strcmp(g.history[g.history_at], text) == 0) return;
	/* what was undone is forgotten once something else is done */
	for (i = g.history_at + 1; i < g.history_count; i++) free(g.history[i]);
	if (g.history_count > 0) g.history_count = g.history_at + 1;
	/* characters typed in a row are one step */
	if (typed && g.history_typed_at > 0.0 && Rget_time() - g.history_typed_at < 1.5 && g.history_count > 1) {
		free(g.history[g.history_at]);
		g.history[g.history_at] = copy_text(text);
		g.history_typed_at = Rget_time();
		return;
	}
	if (g.history_count == MAX_HISTORY) {
		free(g.history[0]);
		memmove(g.history, g.history + 1, sizeof(char*) * (MAX_HISTORY - 1));
		g.history_count--;
	}
	g.history[g.history_count++] = copy_text(text);
	g.history_at = g.history_count - 1;
	g.history_typed_at = typed ? Rget_time() : 0.0;
}

static void history_clear(void) {
	int i;
	for (i = 0; i < g.history_count; i++) free(g.history[i]);
	g.history_count = 0;
	g.history_at = 0;
	g.history_typed_at = 0.0;
}

/* ---------------------------------------------------------------------------
 *  Preview
 * ------------------------------------------------------------------------- */

static void outline_paint(Rid widget, RPainter* painter, float width, float height, void* user_data) {
	float ox, oy, ow, oh, x, y, w, h, shown, r, gr, b, a;
	Rid target = node_widget(g.selected);
	Rid hover = g.interact ? R_INVALID_ID : node_widget(g.hovered);
	(void)user_data;
	(void)width;
	(void)height;
	Rwidget_get_rect(widget, &ox, &oy, &ow, &oh);
	Rtheme_get_color(R_COLOR_ACCENT, &r, &gr, &b, &a);

	if (hover != R_INVALID_ID && hover != target) {
		Rwidget_get_rect(hover, &x, &y, &w, &h);
		Rdraw_rect_outline(painter, x - ox, y - oy, w, h, 3.0f, 1.0f, r, gr, b, 0.45f);
	}
	/* the frame of the selection flies from one element to the next */
	shown = Rdraw_spring(painter, &g.box_shown, target != R_INVALID_ID ? 1.0f : 0.0f, R_MOTION_SMOOTH);
	if (target != R_INVALID_ID) {
		char caption[96];
		float text_width = 0.0f, text_height = 0.0f, label_y;
		Rwidget_get_rect(target, &x, &y, &w, &h);
		x = Rdraw_spring(painter, &g.box[0], x - ox, R_MOTION_BOUNCY);
		y = Rdraw_spring(painter, &g.box[1], y - oy, R_MOTION_BOUNCY);
		w = Rdraw_spring(painter, &g.box[2], w, R_MOTION_BOUNCY);
		h = Rdraw_spring(painter, &g.box[3], h, R_MOTION_BOUNCY);
		Rdraw_rect(painter, x, y, w, h, 4.0f, r, gr, b, 0.10f * shown);
		Rdraw_rect_outline(painter, x - 1.0f, y - 1.0f, w + 2.0f, h + 2.0f, 5.0f, 2.0f, r, gr, b, shown);
		/* its tag, on a tab above it (inside when there is no room) */
		snprintf(caption, sizeof(caption), "%s", Rxml_node_get_tag(g.selected));
		Rfont_measure_text(R_INVALID_ID, 11.0f, caption, &text_width, &text_height);
		label_y = y - 19.0f < -24.0f ? y + 2.0f : y - 19.0f;
		Rdraw_rect(painter, x - 1.0f, label_y, text_width + 12.0f, 17.0f, 4.0f, r, gr, b, shown);
		Rdraw_text(painter, R_INVALID_ID, 11.0f, x + 5.0f, label_y + 2.0f, caption, 1.0f, 1.0f, 1.0f, shown);
	}
}

static Rid node_under(float x, float y) {
	Rid widget = g.preview != R_INVALID_ID && Rwidget_is_valid(g.preview) ? Rwidget_pick(g.preview, x, y) : R_INVALID_ID;
	while (widget != R_INVALID_ID) {
		Rid node = Rwidget_get_xml_node(widget);
		if (node != R_INVALID_ID) return node;
		if (widget == g.preview) break;
		widget = Rwidget_get_parent(widget);
	}
	return R_INVALID_ID;
}

static int outline_event(Rid widget, const REvent* event, void* user_data) {
	Rid node;
	(void)user_data;
	switch (event->type) {
		case R_EVENT_MOUSE_MOVE:
			node = node_under(event->x, event->y);
			if (node != g.hovered) {
				g.hovered = node;
				Rwidget_invalidate(widget);
			}
			return R_TRUE;
		case R_EVENT_MOUSE_LEAVE:
			g.hovered = R_INVALID_ID;
			Rwidget_invalidate(widget);
			return R_FALSE;
		case R_EVENT_MOUSE_DOWN:
			Rwidget_focus(widget);
			node = node_under(event->x, event->y);
			/* a click outside of everything selects the root: the window itself */
			if (node == R_INVALID_ID) node = Rxml_document_get_root(g.document);
			select_node(node, R_TRUE);
			if (event->button == R_MOUSE_RIGHT) Rmenu_open(g.context_menu, g.window, event->x, event->y);
			return R_TRUE;
		case R_EVENT_MOUSE_UP:
			return R_TRUE;
		default:
			return R_FALSE;
	}
}

static void rebuild_preview(void) {
	Rid root = Rxml_document_get_root(g.document);
	const char* tag = Rxml_node_get_tag(root);
	char size[64];

	if (g.preview != R_INVALID_ID && Rwidget_is_valid(g.preview)) Rwidget_destroy(g.preview);
	g.preview = R_INVALID_ID;
	g.hovered = R_INVALID_ID;

	g.problem_count = 0;
	g.collecting = R_TRUE;
	g.preview = Rxml_document_create_widget(g.document);
	g.collecting = R_FALSE;

	if (same_text(tag, "Window")) {
		const char* width = Rxml_node_get_attribute(root, "Width");
		const char* height = Rxml_node_get_attribute(root, "Height");
		int w = width ? atoi(width) : 800, h = height ? atoi(height) : 600;
		if (w <= 0) w = 800;
		if (h <= 0) h = 600;
		Rwidget_set_size(g.frame, (float)w, (float)h);
		snprintf(size, sizeof(size), "%d x %d", w, h);
	} else {
		/* a piece of interface: the size it asks for */
		Rwidget_set_size(g.frame, -1.0f, -1.0f);
		snprintf(size, sizeof(size), "%s", tag);
	}
	Rlabel_set_text(Rwidget_find(g.window, "stage.size"), size);
	if (g.preview != R_INVALID_ID) Rwidget_add_child(g.frame, g.preview);
	show_problems();
	Rwidget_invalidate(g.outline);
}

/* ---------------------------------------------------------------------------
 *  Tree
 * ------------------------------------------------------------------------- */

static void tree_add(Rid parent_item, Rid node) {
	char caption[160];
	Rid item;
	int i, count = Rxml_node_get_child_count(node);
	node_caption(node, caption, sizeof(caption));
	item = Rtree_view_add_item(g.tree, parent_item, caption);
	Rtree_item_set_user_handle(item, (void*)(size_t)node);
	Rtree_item_set_expanded(item, R_TRUE);
	if (node == g.selected) Rtree_view_set_selected_item(g.tree, item);
	for (i = 0; i < count; i++) tree_add(item, Rxml_node_get_child(node, i));
}

static void rebuild_tree(void) {
	g.syncing++;
	Rtree_view_clear_items(g.tree);
	tree_add(R_INVALID_ID, Rxml_document_get_root(g.document));
	g.syncing--;
}

static Rid tree_item_of(Rid item, Rid node) {
	int i, count;
	if (item != R_INVALID_ID && (Rid)(size_t)Rtree_item_get_user_handle(item) == node) return item;
	count = item == R_INVALID_ID ? Rtree_view_get_item_count(g.tree) : Rtree_item_get_child_count(item);
	for (i = 0; i < count; i++) {
		Rid found = tree_item_of(item == R_INVALID_ID ? Rtree_view_get_item(g.tree, i) : Rtree_item_get_child(item, i), node);
		if (found != R_INVALID_ID) return found;
	}
	return R_INVALID_ID;
}

static void on_tree_selected(Rid tree, Rid item, void* user_data) {
	(void)tree;
	(void)user_data;
	if (g.syncing || item == R_INVALID_ID) return;
	select_node((Rid)(size_t)Rtree_item_get_user_handle(item), R_TRUE);
}

/* ---------------------------------------------------------------------------
 *  The document changed
 * ------------------------------------------------------------------------- */

/* After the nodes were changed: the text follows, then everything that shows the document. */
static void document_changed(int keep_inspector) {
	const char* text = Rxml_document_to_string(g.document);
	int line = g.selected != R_INVALID_ID && Rxml_node_is_valid(g.selected) ? Rxml_node_get_line(g.selected) : 0;
	g.syncing++;
	Rtext_editor_set_text(g.source, text);
	/* the text stays on what is being changed */
	if (line > 0) {
		Rtext_editor_set_cursor(g.source, line - 1, 0);
		Rtext_editor_scroll_to_line(g.source, line - 1);
	}
	g.syncing--;
	g.broken = R_FALSE;
	g.modified = R_TRUE;
	history_push(text, R_FALSE);
	rebuild_all(keep_inspector);
	update_title();
}

/* ---------------------------------------------------------------------------
 *  Inspector
 * ------------------------------------------------------------------------- */

static row_t* add_row_record(const char* name, RXmlValueType type, const char* choices) {
	row_t* row;
	if (g.row_count == g.row_cap) {
		/* (the editors keep pointers to their rows: the array never moves while they live) */
		return NULL;
	}
	row = &g.rows[g.row_count++];
	snprintf(row->name, sizeof(row->name), "%s", name);
	row->type = type;
	row->choices = choices;
	row->swatch = R_INVALID_ID;
	return row;
}

static void apply_value(row_t* row, const char* value) {
	if (g.selected == R_INVALID_ID || !Rxml_node_is_valid(g.selected)) return;
	if (strcmp(row->name, "#text") == 0) Rxml_node_set_text(g.selected, value ? value : "");
	else Rxml_node_set_attribute(g.selected, row->name, value && value[0] ? value : NULL);
	if (row->swatch != R_INVALID_ID) Rwidget_set_attribute(row->swatch, "Background", value && value[0] ? value : "transparent");
	document_changed(R_TRUE);
}

static void on_row_text(Rid input, const char* text, void* user_data) {
	(void)input;
	if (!g.syncing) apply_value((row_t*)user_data, text);
}

static void on_row_choice(Rid combo, int index, void* user_data) {
	if (!g.syncing) apply_value((row_t*)user_data, index <= 0 ? NULL : Rcombo_box_get_item_text(combo, index));
}

/* One line of the inspector: the name of an attribute and what edits its value. */
static void add_row(Rid box, const char* label, const char* name, RXmlValueType type, const char* choices, const char* value, const char* tip) {
	row_t* row = add_row_record(name, type, choices);
	Rid line, caption, editor;
	if (!row) return;
	line = Rwidget_create(R_WIDGET_HBOX);
	Rpanel_set_spacing(line, 6.0f);
	caption = Rlabel_create(label);
	Rwidget_set_size(caption, 104.0f, -1.0f);
	Rslot_set_valign(caption, R_VALIGN_CENTER);
	if (tip && tip[0]) Rwidget_set_tooltip(caption, tip);
	/* what is set stands out from what is left to its default */
	if (!value) Rwidget_set_opacity(caption, 0.6f);
	Rwidget_add_child(line, caption);

	if (type == R_XML_BOOL || type == R_XML_CHOICE) {
		const char* list = type == R_XML_BOOL ? "true|false" : choices;
		int selected = 0, index = 1, known = !value;
		editor = Rwidget_create(R_WIDGET_COMBO_BOX);
		Rcombo_box_add_item(editor, "(not set)");
		while (*list) {
			char item[96];
			size_t length = strcspn(list, "|");
			snprintf(item, sizeof(item), "%.*s", (int)length, list);
			Rcombo_box_add_item(editor, item);
			if (value && same_text(item, value)) {
				selected = index;
				known = R_TRUE;
			}
			index++;
			list += length;
			if (*list == '|') list++;
		}
		/* a value that is none of the choices (an expression of a declared widget) is kept as it is */
		if (!known) {
			Rcombo_box_add_item(editor, value);
			selected = index;
		}
		Rcombo_box_set_selected(editor, selected);
		Rcombo_box_set_changed_callback(editor, on_row_choice, row);
	} else {
		editor = Rwidget_create(R_WIDGET_TEXT_INPUT);
		Rtext_input_set_text(editor, value ? value : "");
		Rtext_input_set_placeholder(editor, type == R_XML_NUMBER ? "number" : type == R_XML_COLOR ? "#RRGGBB" : type == R_XML_NUMBERS ? "1,2"
			: type == R_XML_ACTION ? "action.name" : type == R_XML_FILE ? "path" : type == R_XML_LIST ? "One|Two" : "");
		Rtext_input_set_changed_callback(editor, on_row_text, row);
	}
	if (tip && tip[0]) Rwidget_set_tooltip(editor, tip);
	Rslot_set_fill(editor, 1.0f);
	Rwidget_add_child(line, editor);
	if (type == R_XML_COLOR) {
		row->swatch = Rwidget_create(R_WIDGET_BORDER);
		Rwidget_set_size(row->swatch, 22.0f, 22.0f);
		Rslot_set_valign(row->swatch, R_VALIGN_CENTER);
		Rwidget_set_attribute(row->swatch, "CornerRadius", "6");
		Rwidget_set_attribute(row->swatch, "BorderWidth", "1");
		Rwidget_set_attribute(row->swatch, "Background", value && value[0] ? value : "transparent");
		Rwidget_add_child(line, row->swatch);
	}
	Rwidget_add_child(box, line);
}

static Rid add_group(const char* title, int expanded) {
	Rid expander = Rwidget_create(R_WIDGET_EXPANDER);
	Rid box = Rwidget_create(R_WIDGET_VBOX);
	Rexpander_set_title(expander, title);
	Rexpander_set_expanded(expander, expanded);
	Rpanel_set_spacing(box, 4.0f);
	Rslot_set_padding(box, 4.0f, 4.0f, 0.0f, 6.0f);
	Rwidget_add_child(expander, box);
	Rwidget_add_child(g.inspector, expander);
	return box;
}

static int listed(const char* list, const char* name) {
	while (*list) {
		size_t length = strcspn(list, "|");
		if (strlen(name) == length) {
			char item[96];
			snprintf(item, sizeof(item), "%.*s", (int)length, list);
			if (same_text(item, name)) return 1;
		}
		list += length;
		if (*list == '|') list++;
	}
	return 0;
}

static void rebuild_inspector(void) {
	Rid node = g.selected;
	Rid title = Rwidget_find(g.window, "inspector.title");
	Rid about = Rwidget_find(g.window, "inspector.about");
	Rid box = R_INVALID_ID;
	const char* tag;
	const char* group = "";
	char known[4096];
	int special, count, i, known_length = 0;

	g.syncing++;
	Rwidget_clear_children(g.inspector);
	g.row_count = 0;
	known[0] = '\0';
	if (node == R_INVALID_ID || !Rxml_node_is_valid(node)) {
		Rlabel_set_text(title, "Nothing selected");
		Rlabel_set_text(about, "");
		g.syncing--;
		return;
	}
	if (Rxml_node_is_comment(node)) {
		Rlabel_set_text(title, "Comment");
		Rlabel_set_text(about, "Kept in the XML, ignored when the interface is built.");
		add_row(add_group("Comment", R_TRUE), "Text", "#text", R_XML_TEXT, NULL, Rxml_node_get_text(node), NULL);
		g.syncing--;
		return;
	}
	tag = Rxml_node_get_tag(node);
	special = special_tag(tag);
	Rlabel_set_text(title, tag);
	Rlabel_set_text(about, special >= 0 ? SPECIAL_TAGS[special].about
		: Rxml_is_widget_registered(tag) ? "A tag declared with <Widget>: its own attributes are its parameters." : Rxml_get_tag_description(tag));

	if (special >= 0) {
		const char* list = SPECIAL_TAGS[special].attributes;
		box = add_group(tag, R_TRUE);
		while (*list) {
			char name[72];
			size_t length = strcspn(list, "|");
			snprintf(name, sizeof(name), "%.*s", (int)length, list);
			add_row(box, name, name, R_XML_TEXT, NULL, Rxml_node_get_attribute(node, name), NULL);
			known_length += snprintf(known + known_length, sizeof(known) - (size_t)known_length, "%s|", name);
			list += length;
			if (*list == '|') list++;
		}
	} else {
		count = Rxml_get_attribute_count(tag);
		for (i = 0; i < count; i++) {
			RXmlAttributeInfo info;
			const char* label;
			if (!Rxml_get_attribute_info(tag, i, &info)) continue;
			if (strcmp(info.group, group) != 0) {
				int own = same_text(info.group, tag) || (strcmp(info.group, "Widget") != 0 && strcmp(info.group, "Slot") != 0
					&& strcmp(info.group, "Colors") != 0 && strcmp(info.group, "Metrics") != 0);
				group = info.group;
				box = add_group(strcmp(group, "Slot") == 0 ? "Place in the parent" : strcmp(group, "Widget") == 0 ? "Widget" : group,
					own || strcmp(group, "Slot") == 0 || strcmp(group, "Widget") == 0);
			}
			/* "Slot.Fill" and "Color.Text" read better without what their group already says */
			label = strchr(info.name, '.') ? strchr(info.name, '.') + 1 : info.name;
			add_row(box, label, info.name, info.type, info.choices, Rxml_node_get_attribute(node, info.name), info.description);
			if (known_length < (int)sizeof(known) - 80) known_length += snprintf(known + known_length, sizeof(known) - (size_t)known_length, "%s|", info.name);
		}
	}
	/* the text between the tags, where it means something */
	if (Rxml_node_get_text(node)[0] || Rxml_node_get_child_count(node) == 0) {
		if (Rxml_node_get_text(node)[0]) add_row(add_group("Text inside", R_TRUE), "Text", "#text", R_XML_TEXT, NULL, Rxml_node_get_text(node), NULL);
	}
	/* what is written on the element and is in no list: other names of an attribute, parameters, mistakes */
	box = R_INVALID_ID;
	count = Rxml_node_get_attribute_count(node);
	for (i = 0; i < count; i++) {
		const char* name = Rxml_node_get_attribute_name(node, i);
		if (listed(known, name)) continue;
		if (box == R_INVALID_ID) box = add_group(special >= 0 && same_text(tag, "Widget") ? "Parameters" : "Other attributes", R_TRUE);
		add_row(box, name, name, R_XML_TEXT, NULL, Rxml_node_get_attribute_value(node, i), NULL);
	}
	g.syncing--;
}

/* ---------------------------------------------------------------------------
 *  Selection
 * ------------------------------------------------------------------------- */

static void update_path_label(void) {
	char text[400], caption[96];
	int length = 0, depth = 0, i;
	Rid chain[32], walk;
	float x, y, w, h;
	if (g.selected == R_INVALID_ID || !Rxml_node_is_valid(g.selected)) {
		Rlabel_set_text(g.path_label, "");
		return;
	}
	for (walk = g.selected; walk != R_INVALID_ID && depth < 32; walk = Rxml_node_get_parent(walk)) chain[depth++] = walk;
	text[0] = '\0';
	for (i = depth - 1; i >= 0 && length < (int)sizeof(text) - 80; i--) {
		length += snprintf(text + length, sizeof(text) - (size_t)length, "%s%s", Rxml_node_is_comment(chain[i]) ? "comment" : Rxml_node_get_tag(chain[i]),
			i > 0 ? "  >  " : "");
	}
	walk = Rxml_node_get_widget(g.selected);
	if (walk != R_INVALID_ID && Rwidget_is_valid(walk)) {
		Rwidget_get_rect(walk, &x, &y, &w, &h);
		snprintf(caption, sizeof(caption), "     %.0f x %.0f", (double)w, (double)h);
		if (length < (int)sizeof(text) - 40) snprintf(text + length, sizeof(text) - (size_t)length, "%s", caption);
	}
	Rlabel_set_text(g.path_label, text);
}

static void select_node(Rid node, int reveal) {
	int line;
	if (node != R_INVALID_ID && !Rxml_node_is_valid(node)) node = R_INVALID_ID;
	g.selected = node;
	g.syncing++;
	Rtree_view_set_selected_item(g.tree, node != R_INVALID_ID ? tree_item_of(R_INVALID_ID, node) : R_INVALID_ID);
	g.syncing--;
	rebuild_inspector();
	update_path_label();
	Rwidget_invalidate(g.outline);
	/* the text shows where the element is written */
	line = node != R_INVALID_ID ? Rxml_node_get_line(node) : 0;
	if (reveal && line > 0 && !g.broken && !Rwidget_is_focused(g.source)) {
		Rtext_editor_set_cursor(g.source, line - 1, 0);
		Rtext_editor_scroll_to_line(g.source, line - 1);
	}
}

/* Everything that shows the document, from the document. */
static void rebuild_all(int keep_inspector) {
	rebuild_preview();
	rebuild_tree();
	if (!keep_inspector) rebuild_inspector();
	update_path_label();
}

/* ---------------------------------------------------------------------------
 *  Text -> document
 * ------------------------------------------------------------------------- */

/*
 * Reads a text as the document. On a mistake the document stays what it was
 * and the mistake is shown; R_FALSE is returned.
 */
static int load_text(const char* text, int record) {
	RXmlError error;
	Rid document = Rxml_document_parse(text, strlen(text), &error);
	int path[32], depth = -1;

	if (document == R_INVALID_ID) {
		char message[240];
		g.broken = R_TRUE;
		g.problem_count = 0;
		snprintf(message, sizeof(message), "XML line %d, column %d: %s", error.line, error.column, error.message);
		add_problem(message, error.line, R_TRUE);
		show_problems();
		set_status("%s", message);
		return R_FALSE;
	}
	if (g.document != R_INVALID_ID) {
		if (g.selected != R_INVALID_ID && Rxml_node_is_valid(g.selected)) depth = node_path(g.selected, path, 32);
		Rxml_document_destroy(g.document);
	}
	g.document = document;
	g.broken = R_FALSE;
	g.selected = depth >= 0 ? node_at_path(path, depth) : Rxml_document_get_root(document);
	g.hovered = R_INVALID_ID;
	if (record) history_push(text, record == 2);
	rebuild_all(R_FALSE);
	if (g.problem_count == 0) set_status("Ready.");
	else set_status("%d problem%s in the document: see the Problems tab.", g.problem_count, g.problem_count > 1 ? "s" : "");
	return R_TRUE;
}

/* Replaces everything: a new file, a file opened, a step of history. */
static int set_document_text(const char* text, int record) {
	g.syncing++;
	Rtext_editor_set_text(g.source, text);
	g.syncing--;
	return load_text(text, record);
}

static void on_parse_timer(Rid timer, void* user_data) {
	(void)timer;
	(void)user_data;
	g.parse_timer = R_INVALID_ID;
	if (load_text(Rtext_editor_get_text(g.source), 2)) {
		g.modified = R_TRUE;
		update_title();
	}
}

static void on_source_changed(Rid editor, void* user_data) {
	(void)editor;
	(void)user_data;
	if (g.syncing) return;
	/* read once the typing pauses: half-written tags are not worth reporting */
	if (g.parse_timer != R_INVALID_ID && Rtimer_is_valid(g.parse_timer)) Rtimer_destroy(g.parse_timer);
	g.parse_timer = Rtimer_create(0.30, R_FALSE, on_parse_timer, NULL);
	g.modified = R_TRUE;
	update_title();
}

/* The text of the editor was not read yet (the user is typing): read it now, before acting on the document. */
static int flush_source(void) {
	if (g.parse_timer != R_INVALID_ID && Rtimer_is_valid(g.parse_timer)) {
		Rtimer_destroy(g.parse_timer);
		g.parse_timer = R_INVALID_ID;
		load_text(Rtext_editor_get_text(g.source), 2);
	}
	if (g.broken) {
		set_status("The XML has a mistake: fix it first (see the Problems tab).");
		return R_FALSE;
	}
	return R_TRUE;
}

/* ---------------------------------------------------------------------------
 *  Palette
 * ------------------------------------------------------------------------- */

static void palette_fill(void) {
	const char* filter = Rtext_input_get_text(g.palette_filter);
	int i, total = Rxml_get_tag_count() + Rxml_get_registered_widget_count() + 2;
	static const char* const extra[] = { "Item", "Shortcut" };

	for (i = 0; i < g.palette_count; i++) free(g.palette_tags[i]);
	free(g.palette_tags);
	g.palette_tags = (char**)calloc((size_t)total, sizeof(char*));
	g.palette_count = 0;
	g.syncing++;
	Rlist_view_clear_items(g.palette);
	/* the tags the document declares come first: they are what this interface is made of */
	for (i = 0; i < Rxml_get_registered_widget_count(); i++) {
		const char* name = Rxml_get_registered_widget_name(i);
		if (!name || !contains_text(name, filter) || same_text(name, "ToolButton")) continue;
		g.palette_tags[g.palette_count++] = copy_text(name);
		Rlist_view_add_item(g.palette, name);
	}
	for (i = 1; i < Rxml_get_tag_count(); i++) {
		const char* name = Rxml_get_tag_name(i);
		if (!contains_text(name, filter)) continue;
		g.palette_tags[g.palette_count++] = copy_text(name);
		Rlist_view_add_item(g.palette, name);
	}
	for (i = 0; i < 2; i++) {
		if (!contains_text(extra[i], filter)) continue;
		g.palette_tags[g.palette_count++] = copy_text(extra[i]);
		Rlist_view_add_item(g.palette, extra[i]);
	}
	g.syncing--;
}

/* Adds a piece of XML inside the selection, or after it when it cannot contain anything. */
static void insert_xml(const char* xml) {
	Rid target = g.selected, added;
	int index = -1;
	if (!flush_source()) return;
	if (target == R_INVALID_ID || !Rxml_node_is_valid(target)) target = Rxml_document_get_root(g.document);
	if (Rxml_node_is_comment(target) || (special_tag(Rxml_node_get_tag(target)) < 0 && Rxml_get_tag_children(Rxml_node_get_tag(target)) == 0)) {
		Rid parent = Rxml_node_get_parent(target);
		if (parent == R_INVALID_ID) {
			set_status("<%s> cannot contain anything.", Rxml_node_get_tag(target));
			return;
		}
		index = Rxml_node_get_index(target) + 1;
		target = parent;
	}
	added = Rxml_node_add_xml(target, xml, strlen(xml), index);
	if (added == R_INVALID_ID) {
		set_status("This is not XML that can be added.");
		return;
	}
	g.selected = added;
	document_changed(R_FALSE);
	select_node(added, R_TRUE);
	set_status("Added <%s> to <%s>.", Rxml_node_get_tag(added), Rxml_node_get_tag(target));
}

static void action_palette_add(Rid sender, const char* action, void* user_data) {
	int index = Rlist_view_get_selected(g.palette), i;
	const char* tag;
	char xml[160];
	(void)sender;
	(void)action;
	(void)user_data;
	if (index < 0 || index >= g.palette_count) {
		set_status("Choose a widget in the list first.");
		return;
	}
	tag = g.palette_tags[index];
	for (i = 0; i < (int)(sizeof(TEMPLATES) / sizeof(TEMPLATES[0])); i++) {
		if (same_text(TEMPLATES[i].tag, tag)) {
			insert_xml(TEMPLATES[i].xml);
			return;
		}
	}
	snprintf(xml, sizeof(xml), "<%s/>", tag);
	insert_xml(xml);
}

static void action_palette_filter(Rid sender, const char* action, void* user_data) {
	(void)sender;
	(void)action;
	(void)user_data;
	if (!g.syncing) palette_fill();
}

/* ---------------------------------------------------------------------------
 *  Editing the tree
 * ------------------------------------------------------------------------- */

/* The element the commands of the Edit menu act on, or R_INVALID_ID with a word in the status bar. */
static Rid edit_target(int needs_parent) {
	Rid focus = Rwindow_get_focused_widget(g.window);
	/* (a key that also edits text, pressed in an input, was for the input) */
	if (focus != R_INVALID_ID && (Rwidget_get_type(focus) == R_WIDGET_TEXT_INPUT || Rwidget_get_type(focus) == R_WIDGET_TEXT_EDITOR)
		&& focus != g.palette_filter) {
		return R_INVALID_ID;
	}
	if (!flush_source()) return R_INVALID_ID;
	if (g.selected == R_INVALID_ID || !Rxml_node_is_valid(g.selected)) {
		set_status("Nothing is selected.");
		return R_INVALID_ID;
	}
	if (needs_parent && Rxml_node_get_parent(g.selected) == R_INVALID_ID) {
		set_status("This cannot be done to the root of the document.");
		return R_INVALID_ID;
	}
	return g.selected;
}

static void action_edit(Rid sender, const char* action, void* user_data) {
	const char* what = action + 5;
	Rid node, parent;
	int index;
	(void)sender;
	(void)user_data;

	if (strcmp(what, "copy") == 0) {
		if ((node = edit_target(R_FALSE)) == R_INVALID_ID) return;
		Rclipboard_set_text(Rxml_node_to_string(node));
		set_status("Copied <%s> as XML.", Rxml_node_get_tag(node));
		return;
	}
	if (strcmp(what, "paste") == 0) {
		const char* text = Rclipboard_get_text();
		if (text && text[0]) insert_xml(text);
		else set_status("The clipboard is empty.");
		return;
	}
	if ((node = edit_target(R_TRUE)) == R_INVALID_ID) return;
	parent = Rxml_node_get_parent(node);
	index = Rxml_node_get_index(node);

	if (strcmp(what, "delete") == 0) {
		Rid next = Rxml_node_get_child(parent, index + 1);
		if (next == R_INVALID_ID) next = index > 0 ? Rxml_node_get_child(parent, index - 1) : parent;
		Rxml_node_remove(node);
		g.selected = next;
		set_status("Deleted.");
	} else if (strcmp(what, "duplicate") == 0) {
		g.selected = Rxml_node_duplicate(node);
		set_status("Duplicated.");
	} else if (strcmp(what, "up") == 0) {
		if (index == 0) return;
		Rxml_node_move(node, parent, index - 1);
	} else if (strcmp(what, "down") == 0) {
		if (index + 1 >= Rxml_node_get_child_count(parent)) return;
		Rxml_node_move(node, parent, index + 1);
	} else if (strcmp(what, "out") == 0) {
		Rid grand = Rxml_node_get_parent(parent);
		if (grand == R_INVALID_ID) {
			set_status("It is already at the top.");
			return;
		}
		Rxml_node_move(node, grand, Rxml_node_get_index(parent) + 1);
	} else if (strcmp(what, "in") == 0) {
		Rid previous = index > 0 ? Rxml_node_get_child(parent, index - 1) : R_INVALID_ID;
		if (previous == R_INVALID_ID || Rxml_node_is_comment(previous)) {
			set_status("There is no element before it to move into.");
			return;
		}
		Rxml_node_move(node, previous, -1);
	} else if (strncmp(what, "wrap.", 5) == 0) {
		Rid wrapper = Rxml_node_add_child(parent, what + 5, index);
		if (wrapper == R_INVALID_ID) return;
		Rxml_node_move(node, wrapper, -1);
		/* its place in the parent goes to what now holds it */
		{
			static const char* const slot[] = { "Slot.Fill", "Slot.HAlign", "Slot.VAlign", "Slot.Padding", "Slot.Title", "Slot.Column", "Slot.Row", "Slot.Dock" };
			int i;
			for (i = 0; i < (int)(sizeof(slot) / sizeof(slot[0])); i++) {
				const char* value = Rxml_node_get_attribute(node, slot[i]);
				if (!value) continue;
				Rxml_node_set_attribute(wrapper, slot[i], value);
				Rxml_node_set_attribute(node, slot[i], NULL);
			}
		}
		g.selected = wrapper;
		set_status("Wrapped in <%s>.", what + 5);
	} else if (strcmp(what, "unwrap") == 0) {
		int count = Rxml_node_get_child_count(node), i;
		if (count == 0) {
			set_status("<%s> contains nothing to keep.", Rxml_node_get_tag(node));
			return;
		}
		g.selected = Rxml_node_get_child(node, 0);
		for (i = 0; i < count; i++) Rxml_node_move(Rxml_node_get_child(node, 0), parent, index + i);
		Rxml_node_remove(node);
		set_status("Unwrapped.");
	} else {
		return;
	}
	document_changed(R_FALSE);
	select_node(g.selected, R_TRUE);
}

static void action_history(Rid sender, const char* action, void* user_data) {
	int step = strcmp(action, "edit.undo") == 0 ? -1 : 1;
	Rid focus = Rwindow_get_focused_widget(g.window);
	(void)sender;
	(void)user_data;
	/* in a text, Ctrl+Z is for the text */
	if (focus != R_INVALID_ID && (Rwidget_get_type(focus) == R_WIDGET_TEXT_INPUT || Rwidget_get_type(focus) == R_WIDGET_TEXT_EDITOR)) return;
	if (!flush_source() && step > 0) return;
	if (g.history_at + step < 0 || g.history_at + step >= g.history_count) {
		set_status(step < 0 ? "Nothing to undo." : "Nothing to redo.");
		return;
	}
	g.history_at += step;
	g.history_typed_at = 0.0;
	set_document_text(g.history[g.history_at], R_FALSE);
	g.modified = R_TRUE;
	update_title();
	set_status(step < 0 ? "Undone." : "Redone.");
}

/* ---------------------------------------------------------------------------
 *  Files
 * ------------------------------------------------------------------------- */

static int confirm_discard(void) {
	if (!g.modified || g.quitting) return R_TRUE;
	return Rmessage_box(g.window, "Unsaved changes", "The document has changes that were not saved. Discard them?", R_MESSAGE_OK_CANCEL)
		== R_MESSAGE_RESULT_OK;
}

static void new_document(const char* text, const char* path) {
	free(g.path);
	g.path = path ? copy_text(path) : NULL;
	history_clear();
	if (g.document != R_INVALID_ID) Rxml_document_destroy(g.document);
	g.document = R_INVALID_ID;
	g.selected = R_INVALID_ID;
	if (!set_document_text(text, R_TRUE)) {
		/* a file that is not XML: an empty document to stand on, the text left to be fixed */
		g.document = Rxml_document_create("VBox");
		g.selected = Rxml_document_get_root(g.document);
		rebuild_all(R_FALSE);
		g.broken = R_TRUE;
	}
	g.modified = R_FALSE;
	update_title();
	palette_fill();
}

static void open_file(const char* path) {
	size_t size = 0;
	char* text = Rfile_read(path, &size);
	if (!text) {
		set_status("Cannot read %s", path);
		return;
	}
	/* the paths written in the file (images, fonts, includes) start from its folder */
	{
		char* folder = copy_text(path);
		char* slash = strrchr(folder, '/');
		char* backslash = strrchr(folder, '\\');
		if (backslash && (!slash || backslash > slash)) slash = backslash;
		if (slash) {
			*slash = '\0';
			Rxml_set_base_path(folder);
		}
		free(folder);
	}
	new_document(text, path);
	Rmemory_free(text);
	if (!g.broken) set_status("Opened %s", path);
}

static int save_file(int ask) {
	static const RFileFilter filters[] = { { "XML", "*.xml" }, { "All files", "*" } };
	const char* text;
	if (ask || !g.path) {
		RFileDialogDesc desc;
		const char* chosen;
		memset(&desc, 0, sizeof(desc));
		desc.parent = g.window;
		desc.title = "Save the interface";
		desc.file_name = "interface.xml";
		desc.filters = filters;
		desc.filter_count = 2;
		desc.default_extension = "xml";
		chosen = Rfile_dialog_save(&desc);
		if (!chosen) return R_FALSE;
		chosen = copy_text(chosen);
		free(g.path);
		g.path = (char*)chosen;
	}
	/* what is saved is the text as it is written, mistakes included: nothing typed is lost */
	text = Rtext_editor_get_text(g.source);
	if (!Rfile_write(g.path, text, strlen(text))) {
		set_status("Cannot write %s", g.path);
		return R_FALSE;
	}
	g.modified = R_FALSE;
	update_title();
	set_status("Saved %s", g.path);
	return R_TRUE;
}

static void action_file(Rid sender, const char* action, void* user_data) {
	static const RFileFilter filters[] = { { "XML", "*.xml" }, { "All files", "*" } };
	(void)sender;
	(void)user_data;
	if (strncmp(action, "file.new.", 9) == 0) {
		const char* kind = action + 9;
		if (!confirm_discard()) return;
		new_document(strcmp(kind, "panel") == 0 ? TEMPLATE_PANEL : strcmp(kind, "form") == 0 ? TEMPLATE_FORM : TEMPLATE_WINDOW, NULL);
		set_status("New document.");
	} else if (strcmp(action, "file.open") == 0) {
		RFileDialogDesc desc;
		const char* chosen;
		if (!confirm_discard()) return;
		memset(&desc, 0, sizeof(desc));
		desc.parent = g.window;
		desc.title = "Open an interface";
		desc.filters = filters;
		desc.filter_count = 2;
		chosen = Rfile_dialog_open(&desc);
		if (chosen) open_file(chosen);
	} else if (strcmp(action, "file.save") == 0) {
		save_file(R_FALSE);
	} else if (strcmp(action, "file.save_as") == 0) {
		save_file(R_TRUE);
	} else if (strcmp(action, "file.quit") == 0) {
		if (confirm_discard()) Rquit(0);
	}
}

static int on_close(Rid window, void* user_data) {
	(void)window;
	(void)user_data;
	if (!confirm_discard()) return R_FALSE;
	Rquit(0);
	return R_TRUE;
}

static int on_drop(Rid target, const RDropEvent* event, void* user_data) {
	(void)target;
	(void)user_data;
	/* files dropped on the window are opened; a text is added as XML */
	if (event->phase != R_DROP_DROPPED) return R_TRUE;
	if (event->path_count > 0) {
		if (confirm_discard()) open_file(event->paths[0]);
	} else if (event->text && event->text[0]) {
		insert_xml(event->text);
	}
	return R_TRUE;
}

/* ---------------------------------------------------------------------------
 *  View
 * ------------------------------------------------------------------------- */

static void set_interact(int interact) {
	g.interact = interact;
	/* the frame of the selection stays, but lets the pointer through to the interface */
	Rwidget_set_hit_testable(g.outline, !interact);
	g.syncing++;
	Rcheckbox_set_checked(g.interact_switch, interact);
	Rmenu_item_set_checked(Rwidget_find(g.window, "menu.interact"), interact);
	g.syncing--;
	g.hovered = R_INVALID_ID;
	Rwidget_invalidate(g.outline);
	set_status(interact ? "The preview reacts as the interface will." : "A click in the preview selects an element.");
}

static void action_view(Rid sender, const char* action, void* user_data) {
	static const struct { const char* name; RTheme theme; } themes[] = {
		{ "dark", R_THEME_DARK }, { "light", R_THEME_LIGHT }, { "glass_dark", R_THEME_GLASS_DARK }, { "glass_light", R_THEME_GLASS_LIGHT }
	};
	(void)user_data;
	if (strncmp(action, "view.theme.", 11) == 0) {
		int i;
		for (i = 0; i < 4; i++) {
			char name[40];
			snprintf(name, sizeof(name), "theme.%s", themes[i].name);
			Rmenu_item_set_checked(Rwidget_find(g.window, name), strcmp(themes[i].name, action + 11) == 0);
			if (strcmp(themes[i].name, action + 11) == 0) Rtheme_apply(themes[i].theme);
		}
	} else if (strcmp(action, "view.interact") == 0) {
		set_interact(!g.interact);
	} else if (strcmp(action, "view.interact.switch") == 0) {
		if (!g.syncing) set_interact(Rcheckbox_is_checked(sender));
	} else if (strcmp(action, "view.run") == 0) {
		if (!flush_source()) return;
		if (g.run_window != R_INVALID_ID && Rwindow_is_valid(g.run_window)) Rwindow_destroy(g.run_window);
		g.collecting = R_TRUE;
		if (same_text(Rxml_node_get_tag(Rxml_document_get_root(g.document)), "Window")) {
			g.run_window = Rxml_document_create_window(g.document);
		} else {
			/* a piece of interface: a window around it */
			Rid content = Rxml_document_create_widget(g.document);
			g.run_window = content != R_INVALID_ID ? Rwindow_create("Preview", 640, 480, R_WINDOW_DEFAULT) : R_INVALID_ID;
			if (g.run_window != R_INVALID_ID) Rwindow_set_content(g.run_window, content);
		}
		g.collecting = R_FALSE;
		/* (the widgets of the preview are what the nodes lead to, not those of this window) */
		rebuild_preview();
		set_status(g.run_window != R_INVALID_ID ? "Running in its own window." : "The interface could not be built: see the Problems tab.");
	} else if (strcmp(action, "view.reset") == 0) {
		Rdock_area_load_layout(g.dock, (const char*)Rwidget_get_user_handle(g.dock));
	}
}

static void action_problems(Rid sender, const char* action, void* user_data) {
	int index = Rlist_view_get_selected(g.problems_list);
	(void)sender;
	(void)action;
	(void)user_data;
	if (index < 0 || index >= g.problem_count || g.problems[index].line <= 0) return;
	Rdock_area_show_panel(g.dock, g.source);
	Rtext_editor_set_cursor(g.source, g.problems[index].line - 1, 0);
	Rtext_editor_scroll_to_line(g.source, g.problems[index].line - 1);
	Rwidget_focus(g.source);
}

/* Actions of the interface being designed have nothing bound to them here: say what would happen. */
static void on_unbound_action(Rid sender, const char* action, void* user_data) {
	(void)sender;
	(void)user_data;
	set_status("The interface fired the action \"%s\".", action);
}

/* ---------------------------------------------------------------------------
 *  Scripted runs
 * ------------------------------------------------------------------------- */

static void run_frames(int count) {
	int i;
	for (i = 0; i < count && Ris_running(); i++) {
		Rwindow_request_redraw(g.window);
		Rdraw_frame();
	}
}

/*
 * Commands, separated by spaces:
 *   f N            N frames            do NAME        fires an action
 *   mv X Y         moves the pointer   click          left click
 *   key CODE MODS  a key               text WORD      types a word
 *   pick INDEX     selects a row of the palette
 *   path I/J/K     selects an element by its indices from the root ("" = the root)
 *   set NAME VALUE sets an attribute of the selection ('-' removes it)
 *   expect TEXT    stops with an error unless the XML contains TEXT ('~' stands for a space)
 */
static int designer_script(char* script) {
	char* token = strtok(script, " ");
	while (token) {
		if (strcmp(token, "f") == 0) {
			run_frames(atoi(strtok(NULL, " ")));
		} else if (strcmp(token, "do") == 0) {
			Raction_fire(strtok(NULL, " "), R_INVALID_ID);
		} else if (strcmp(token, "mv") == 0) {
			float x = (float)atof(strtok(NULL, " ")), y = (float)atof(strtok(NULL, " "));
			Rwindow_inject_mouse_move(g.window, x, y);
		} else if (strcmp(token, "click") == 0) {
			Rwindow_inject_mouse_button(g.window, R_MOUSE_LEFT, R_TRUE);
			Rwindow_inject_mouse_button(g.window, R_MOUSE_LEFT, R_FALSE);
		} else if (strcmp(token, "key") == 0) {
			int key = atoi(strtok(NULL, " ")), modifiers = atoi(strtok(NULL, " "));
			Rwindow_inject_key(g.window, (RKey)key, R_TRUE, (Ruint)modifiers);
			Rwindow_inject_key(g.window, (RKey)key, R_FALSE, (Ruint)modifiers);
		} else if (strcmp(token, "text") == 0) {
			Rwindow_inject_text(g.window, strtok(NULL, " "));
		} else if (strcmp(token, "pick") == 0) {
			Rlist_view_set_selected(g.palette, atoi(strtok(NULL, " ")));
		} else if (strcmp(token, "path") == 0) {
			char* text = strtok(NULL, " ");
			int path[32], depth = 0;
			while (text && *text && *text != '.' && depth < 32) {
				path[depth++] = atoi(text);
				text = strchr(text, '/');
				if (text) text++;
			}
			select_node(node_at_path(path, depth), R_TRUE);
		} else if (strcmp(token, "set") == 0) {
			const char* name = strtok(NULL, " ");
			const char* value = strtok(NULL, " ");
			Rxml_node_set_attribute(g.selected, name, strcmp(value, "-") == 0 ? NULL : value);
			document_changed(R_FALSE);
		} else if (strcmp(token, "expect") == 0) {
			char* text = strtok(NULL, " ");
			char* cursor;
			for (cursor = text; *cursor; cursor++) if (*cursor == '~') *cursor = ' ';
			if (!strstr(Rtext_editor_get_text(g.source), text)) {
				fprintf(stderr, "raven_designer: expected the XML to contain: %s\n---\n%s---\n", text, Rtext_editor_get_text(g.source));
				return R_FALSE;
			}
		} else {
			fprintf(stderr, "raven_designer: unknown script command '%s'\n", token);
			return R_FALSE;
		}
		token = strtok(NULL, " ");
	}
	return R_TRUE;
}

static void capture(const char* path) {
	int width = 0, height = 0, i;
	uint8_t* pixels = Rwindow_capture(g.window, &width, &height);
	FILE* file = pixels ? fopen(path, "wb") : NULL;
	if (file) {
		fprintf(file, "P6\n%d %d\n255\n", width, height);
		for (i = 0; i < width * height; i++) fwrite(pixels + i * 4, 1, 3, file);
		fclose(file);
	}
	Rpixels_free(pixels);
}

/* ---------------------------------------------------------------------------
 *  Start
 * ------------------------------------------------------------------------- */

static int build_interface(void) {
	static const char* const edits[] = {
		"edit.delete", "edit.duplicate", "edit.up", "edit.down", "edit.out", "edit.in", "edit.unwrap", "edit.copy", "edit.paste",
		"edit.wrap.VBox", "edit.wrap.HBox", "edit.wrap.Border", "edit.wrap.ScrollBox", "edit.wrap.Overlay"
	};
	static const char* const files[] = { "file.new.window", "file.new.panel", "file.new.form", "file.open", "file.save", "file.save_as", "file.quit" };
	static const char* const views[] = {
		"view.theme.dark", "view.theme.light", "view.theme.glass_dark", "view.theme.glass_light", "view.interact", "view.interact.switch",
		"view.run", "view.reset"
	};
	static const struct { const char* text; const char* action; } context[] = {
		{ "Duplicate", "edit.duplicate" }, { "Delete", "edit.delete" }, { NULL, NULL }, { "Move up", "edit.up" }, { "Move down", "edit.down" },
		{ "Move out", "edit.out" }, { NULL, NULL }, { "Wrap in a VBox", "edit.wrap.VBox" }, { "Wrap in an HBox", "edit.wrap.HBox" },
		{ "Wrap in a Border", "edit.wrap.Border" }, { "Unwrap", "edit.unwrap" }, { NULL, NULL }, { "Copy as XML", "edit.copy" },
		{ "Paste XML inside", "edit.paste" }
	};
	size_t i;

	for (i = 0; i < sizeof(edits) / sizeof(edits[0]); i++) Raction_bind(edits[i], action_edit, NULL);
	for (i = 0; i < sizeof(files) / sizeof(files[0]); i++) Raction_bind(files[i], action_file, NULL);
	for (i = 0; i < sizeof(views) / sizeof(views[0]); i++) Raction_bind(views[i], action_view, NULL);
	Raction_bind("edit.undo", action_history, NULL);
	Raction_bind("edit.redo", action_history, NULL);
	Raction_bind("palette.add", action_palette_add, NULL);
	Raction_bind("palette.filter", action_palette_filter, NULL);
	Raction_bind("problems.go", action_problems, NULL);
	Raction_set_default_handler(on_unbound_action, NULL);

	{
		size_t length = 0, piece;
		char* xml;
		for (piece = 0; piece < sizeof(DESIGNER_XML) / sizeof(DESIGNER_XML[0]); piece++) length += strlen(DESIGNER_XML[piece]);
		xml = (char*)malloc(length + 1);
		xml[0] = '\0';
		for (piece = 0; piece < sizeof(DESIGNER_XML) / sizeof(DESIGNER_XML[0]); piece++) strcat(xml, DESIGNER_XML[piece]);
		g.window = Rwindow_create_from_xml(xml, length);
		free(xml);
	}
	if (g.window == R_INVALID_ID) return R_FALSE;
	g.dock = Rwidget_find(g.window, "dock");
	g.source = Rwidget_find(g.window, "source");
	g.tree = Rwidget_find(g.window, "tree");
	g.palette = Rwidget_find(g.window, "palette");
	g.palette_filter = Rwidget_find(g.window, "palette.filter");
	g.inspector = Rwidget_find(g.window, "inspector");
	g.problems_list = Rwidget_find(g.window, "problems");
	g.status = Rwidget_find(g.window, "status");
	g.path_label = Rwidget_find(g.window, "path");
	g.stage = Rwidget_find(g.window, "stage");
	g.interact_switch = Rwidget_find(g.window, "interact");
	Rdock_area_show_panel(g.dock, g.source);
	/* the arrangement the panels start with, for "Reset the panels" */
	Rwidget_set_user_handle(g.dock, copy_text(Rdock_area_save_layout(g.dock)));

	/* the stage: a frame that stands for the window, and above it what draws the selection */
	g.frame = Rwidget_create(R_WIDGET_BORDER);
	Rwidget_set_attribute(g.frame, "CornerRadius", "10");
	Rwidget_set_attribute(g.frame, "BorderWidth", "1");
	Rwidget_set_attribute(g.frame, "Shadow", "26,0,10");
	Rwidget_set_clipping(g.frame, R_TRUE);
	Rwidget_add_child(g.stage, g.frame);
	g.outline = Rwidget_create(R_WIDGET_CUSTOM);
	Rcustom_set_paint_callback(g.outline, outline_paint, NULL);
	Rwidget_set_event_callback(g.outline, outline_event, NULL);
	Rwidget_set_focusable(g.outline, R_TRUE);
	Rwidget_add_child(g.stage, g.outline);

	g.context_menu = Rwidget_create(R_WIDGET_MENU);
	for (i = 0; i < sizeof(context) / sizeof(context[0]); i++) {
		if (!context[i].text) Rmenu_add_separator(g.context_menu);
		else Rwidget_set_action(Rmenu_add_item(g.context_menu, context[i].text), context[i].action);
	}
	Rwidget_set_context_menu(g.tree, g.context_menu);

	Rtree_view_set_selection_changed_callback(g.tree, on_tree_selected, NULL);
	Rtext_editor_set_changed_callback(g.source, on_source_changed, NULL);
	Rtext_editor_set_tab_size(g.source, 4);
	Rwindow_set_close_callback(g.window, on_close, NULL);
	Rwindow_set_drop_callback(g.window, on_drop, NULL);

	g.row_cap = 512;
	g.rows = (row_t*)calloc((size_t)g.row_cap, sizeof(row_t));
	g.document = R_INVALID_ID;
	g.selected = R_INVALID_ID;
	g.hovered = R_INVALID_ID;
	g.preview = R_INVALID_ID;
	g.run_window = R_INVALID_ID;
	g.parse_timer = R_INVALID_ID;
	return R_TRUE;
}

int main(int argc, char** argv) {
	RInitDesc desc;
	const char* file = NULL;
	const char* capture_path = NULL;
	char* script = NULL;
	int frames = -1, i, ok = R_TRUE;

	memset(&desc, 0, sizeof(desc));
	desc.backend = R_BACKEND_VULKAN;
	desc.app_name = "Raven Designer";
	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--null") == 0) {
			desc.backend = R_BACKEND_NULL;
			desc.headless = R_TRUE;
		} else if (strcmp(argv[i], "--headless") == 0) {
			desc.headless = R_TRUE;
		} else if (strcmp(argv[i], "--frames") == 0 && i + 1 < argc) {
			frames = atoi(argv[++i]);
		} else if (strcmp(argv[i], "--script") == 0 && i + 1 < argc) {
			script = copy_text(argv[++i]);
		} else if (strcmp(argv[i], "--capture") == 0 && i + 1 < argc) {
			capture_path = argv[++i];
		} else if (argv[i][0] == '-') {
			fprintf(stderr, "usage: raven_designer [file.xml] [--null | --headless] [--frames N] [--script \"...\"] [--capture file.ppm]\n");
			return 2;
		} else {
			file = argv[i];
		}
	}
	Rerror_set_callback(on_error);
	if (!Rinit_ex(&desc)) {
		fprintf(stderr, "raven_designer: cannot start Raven (is a Vulkan driver installed?)\n");
		return 1;
	}
	/* scripted runs do not depend on how long a frame takes */
	if (frames >= 0 || script) {
		Rset_animation_time_step(1.0 / 60.0);
		g.quitting = R_TRUE;
	}
	if (!build_interface()) {
		Rshutdown();
		return 1;
	}
	new_document(TEMPLATE_WINDOW, NULL);
	if (file) open_file(file);

	if (script) ok = designer_script(script);
	if (frames >= 0) {
		run_frames(frames);
		if (capture_path) capture(capture_path);
	} else if (!script) {
		while (Rwait_frame(-1.0)) {}
	}

	history_clear();
	for (i = 0; i < g.palette_count; i++) free(g.palette_tags[i]);
	free(g.palette_tags);
	free(g.rows);
	free(g.path);
	free(script);
	if (Rwidget_is_valid(g.dock)) free(Rwidget_get_user_handle(g.dock));
	i = Rget_exit_code();
	Rshutdown();
	return ok ? i : 1;
}
