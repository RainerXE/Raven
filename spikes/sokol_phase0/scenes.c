/**
 * Phase 0 spike driver: renders the same XML scenes with the Vulkan backend or with the
 * sokol backend and writes each frame as a PPM (compare.py diffs the two sets).
 *
 *   scenes vulkan <out dir>
 *   scenes sokol  <out dir>
 */
#include <Raven.h>
#include <raven/raven_backend.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const RBackendVTable* spk_backend(void);

typedef struct scene_t {
	const char* name;
	const char* theme; /* NULL: default */
	const char* xml;
} scene_t;

static const scene_t g_scenes[] = {
	{ "shapes", NULL,
		"<Window Width=\"400\" Height=\"300\" ClearColor=\"#336699\"><Canvas>"
		"<Border Slot.Position=\"10,10\" Slot.Size=\"100,60\" Background=\"#FF0000\" CornerRadius=\"0\" BorderWidth=\"0\"/>"
		"<Border Slot.Position=\"150,10\" Slot.Size=\"100,100\" Background=\"#00FF00\" CornerRadius=\"50\" BorderWidth=\"0\"/>"
		"<Border Slot.Position=\"10,150\" Slot.Size=\"100,100\" Background=\"#000000\" CornerRadius=\"0\" BorderWidth=\"10\" BorderColor=\"#FFFFFF\"/>"
		"<Border Slot.Position=\"150,150\" Slot.Size=\"100,100\" Background=\"#000000\" CornerRadius=\"0\" BorderWidth=\"0\">"
		"<Border Background=\"#FFFFFF\" Opacity=\"0.5\" CornerRadius=\"0\" BorderWidth=\"0\"/></Border>"
		"<Border Slot.Position=\"280,20\" Slot.Size=\"100,100\" Background=\"#FFAA00\" CornerRadius=\"24\" BorderWidth=\"3\" BorderColor=\"#222222\"/>"
		"</Canvas></Window>" },
	{ "clipping", NULL,
		"<Window Width=\"300\" Height=\"200\" ClearColor=\"#000000\"><Canvas>"
		"<Canvas Slot.Position=\"50,50\" Slot.Size=\"100,100\" Clip=\"true\">"
		"<Border Slot.Position=\"-40,-40\" Slot.Size=\"300,300\" Background=\"#FF0000\" CornerRadius=\"30\" BorderWidth=\"0\"/>"
		"</Canvas></Canvas></Window>" },
	{ "text", NULL,
		"<Window Width=\"420\" Height=\"200\" ClearColor=\"#101014\"><Canvas>"
		"<Label Slot.Position=\"20,10\" TextSize=\"40\" Color=\"#FFFFFF\">Raven</Label>"
		"<Label Slot.Position=\"20,70\" TextSize=\"16\" Color=\"#CCCCCC\">The quick brown fox jumps over the lazy dog</Label>"
		"<Label Slot.Position=\"20,100\" TextSize=\"12\" Color=\"#88AAFF\">Small text 0123456789 {}[]()</Label>"
		"<Label Slot.Position=\"20,130\" TextSize=\"24\" Color=\"#FFCC00\">Sizes and AA</Label>"
		"</Canvas></Window>" },
	{ "controls_dark", NULL,
		"<Window Width=\"460\" Height=\"360\"><VBox Spacing=\"10\" Slot.Padding=\"16\">"
		"<HBox Spacing=\"8\"><Button Text=\"Button\"/><Button Text=\"Primary\" Style=\"Primary\"/><Checkbox Text=\"Switch\" Style=\"Switch\" Checked=\"true\"/></HBox>"
		"<Slider Value=\"0.4\"/><ProgressBar Value=\"0.6\"/>"
		"<TextInput Placeholder=\"Type here\"/>"
		"<ComboBox Selected=\"0\"><Item Text=\"One\"/><Item Text=\"Two\"/></ComboBox>"
		"<TabView Slot.Fill=\"1\"><Label Slot.Title=\"First\">Page one</Label><Label Slot.Title=\"Second\">Other</Label></TabView>"
		"</VBox></Window>" },
	{ "controls_light", "light",
		"<Window Width=\"460\" Height=\"240\"><VBox Spacing=\"10\" Slot.Padding=\"16\">"
		"<HBox Spacing=\"8\"><Button Text=\"Button\"/><Button Text=\"Primary\" Style=\"Primary\"/><Checkbox Text=\"Check\" Checked=\"true\"/></HBox>"
		"<Slider Value=\"0.7\"/><TextInput Placeholder=\"Light theme\"/>"
		"</VBox></Window>" },
	{ "glass", NULL,
		"<Window Width=\"400\" Height=\"200\" ClearColor=\"#000000\"><Overlay><Canvas>"
		"<Border Slot.Position=\"0,0\" Slot.Size=\"200,200\" Background=\"#FF0000\" CornerRadius=\"0\" BorderWidth=\"0\"/>"
		"<Border Slot.Position=\"200,0\" Slot.Size=\"200,200\" Background=\"#0000FF\" CornerRadius=\"0\" BorderWidth=\"0\"/>"
		"<Border Slot.Position=\"40,60\" Slot.Size=\"120,70\" Background=\"#FFFF00\" CornerRadius=\"35\" BorderWidth=\"0\"/>"
		"</Canvas><Border Material=\"Glass\" Width=\"220\" Height=\"110\" CornerRadius=\"28\"/></Overlay></Window>" },
	{ "glass_theme", "glass_dark",
		"<Window Width=\"420\" Height=\"320\"><Overlay>"
		"<VBox Spacing=\"10\" Slot.Padding=\"20\">"
		"<HBox Spacing=\"8\"><Button Text=\"Glass\"/><Button Text=\"Tinted\" Style=\"Primary\"/><Checkbox Text=\"Switch\" Style=\"Switch\" Checked=\"true\"/></HBox>"
		"<Slider Value=\"0.5\"/><TextInput Placeholder=\"Text\"/>"
		"<ComboBox Selected=\"0\"><Item Text=\"One\"/><Item Text=\"Two\"/></ComboBox>"
		"<TabView Slot.Fill=\"1\"><Label Slot.Title=\"First\">Page</Label><Label Slot.Title=\"Second\">Other</Label></TabView>"
		"</VBox>"
		"<Border Material=\"Glass\" Width=\"160\" Height=\"90\" Slot.HAlign=\"Right\" Slot.VAlign=\"Bottom\" Slot.Padding=\"12\"/>"
		"</Overlay></Window>" },
	{ "blur", NULL,
		"<Window Width=\"320\" Height=\"120\" ClearColor=\"#000000\"><Overlay>"
		"<HBox Name=\"stripes\"><Border Width=\"40\" Background=\"#FFFFFF\"/><Border Width=\"40\" Background=\"#FF3030\"/><Border Width=\"40\" Background=\"#30FF30\"/><Border Width=\"40\" Background=\"#3030FF\"/><Border Width=\"40\" Background=\"#FFFFFF\"/><Border Width=\"40\" Background=\"#FF3030\"/><Border Width=\"40\" Background=\"#30FF30\"/><Border Width=\"40\" Background=\"#3030FF\"/></HBox>"
		"<HBox Slot.Padding=\"10,30\" Spacing=\"20\">"
		"<BackgroundBlur Style=\"Plain\" Radius=\"16\" Width=\"130\" CornerRadius=\"0\"/>"
		"<BackgroundBlur Style=\"Frosted\" Radius=\"16\" Width=\"130\" CornerRadius=\"0\" Grain=\"0\"/>"
		"</HBox></Overlay></Window>" }
};

static void write_ppm(const char* dir, const char* name, const uint8_t* rgba, int w, int h) {
	char path[512];
	FILE* f;
	int i;
	snprintf(path, sizeof(path), "%s/%s.ppm", dir, name);
	f = fopen(path, "wb");
	if (!f) { perror(path); return; }
	fprintf(f, "P6\n%d %d\n255\n", w, h);
	for (i = 0; i < w * h; i++) fwrite(rgba + (size_t)i * 4, 1, 3, f);
	fclose(f);
}

int main(int argc, char** argv) {
	RInitDesc desc;
	size_t i;
	if (argc < 3) { fprintf(stderr, "usage: scenes vulkan|sokol <out dir>\n"); return 2; }
	memset(&desc, 0, sizeof(desc));
	desc.headless = R_TRUE;
	desc.app_name = "sokol_phase0";
	desc.ui_scale = 1.0f;
	if (strcmp(argv[1], "sokol") == 0) {
		Rbackend_register(spk_backend());
		desc.backend = R_BACKEND_CUSTOM;
	} else {
		desc.backend = R_BACKEND_VULKAN;
	}
	if (!Rinit_ex(&desc)) { fprintf(stderr, "Rinit failed\n"); return 1; }
	Rset_animations_enabled(R_FALSE);
	for (i = 0; i < sizeof(g_scenes) / sizeof(g_scenes[0]); i++) {
		const scene_t* s = &g_scenes[i];
		Rid window;
		uint8_t* pixels;
		int w = 0, h = 0;
		if (s->theme && strcmp(s->theme, "light") == 0) Rtheme_apply(R_THEME_LIGHT);
		else if (s->theme && strcmp(s->theme, "glass_dark") == 0) Rtheme_apply(R_THEME_GLASS_DARK);
		else Rtheme_apply(R_THEME_DARK);
		window = Rwindow_create_from_xml(s->xml, strlen(s->xml));
		Rdraw_frame();
		pixels = Rwindow_capture(window, &w, &h);
		if (!pixels) { fprintf(stderr, "%s: no capture\n", s->name); }
		else {
			write_ppm(argv[2], s->name, pixels, w, h);
			printf("%-14s %dx%d\n", s->name, w, h);
			Rpixels_free(pixels);
		}
		Rwindow_destroy(window);
	}
	Rshutdown();
	return 0;
}
