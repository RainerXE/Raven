/**
 * Raven Designer - project.c
 * A project is a folder built with the tools of its language: CMake (C, C++),
 * Cargo (Rust), go (Go), dotnet (C#), zig build (Zig), or nothing to build
 * (Python, Lua). The Explorer shows its files; each opens where it belongs.
 * New project writes one that builds as it is: its interface with its code
 * (main.xmlc, main.xmlrs...), what its tools need, and strings.csv.
 * The settings of the IDE for a project are in its file .raven-ide.
 */

#include "ide.h"

#ifndef RAVEN_IDE_INCLUDE_DIR
#define RAVEN_IDE_INCLUDE_DIR ""
#endif

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char* g_project_dir = NULL;
ide_settings_t g_settings;

static struct {
	Rid explorer;
	char** paths;    /* what the items of the Explorer point at */
	int path_count;
	int entry_count;
} g_p;

/* ---------------------------------------------------------------------------
 *  Settings
 * ------------------------------------------------------------------------- */

/* The package Raven itself comes in, when the IDE runs from one (bin/raven_designer). */
static void guess_raven_prefix(char* out, size_t size) {
	char* prefix = ide_join(ide_exe_dir(), "..");
	char* config = ide_join(prefix, "lib/cmake/raven/ravenConfig.cmake");
	out[0] = 0;
	if (ide_file_exists(config)) snprintf(out, size, "%s", prefix);
	free(config);
	free(prefix);
}

static void load_settings(void) {
	char* path;
	char* text;
	size_t size = 0;
	{
		/* (the package already chosen stays, unless the project says another one) */
		char kept[sizeof(g_settings.raven_prefix)];
		snprintf(kept, sizeof(kept), "%s", g_settings.raven_prefix);
		memset(&g_settings, 0, sizeof(g_settings));
		snprintf(g_settings.raven_prefix, sizeof(g_settings.raven_prefix), "%s", kept);
	}
	snprintf(g_settings.build_type, sizeof(g_settings.build_type), "Debug");
	if (!g_settings.raven_prefix[0]) guess_raven_prefix(g_settings.raven_prefix, sizeof(g_settings.raven_prefix));
	if (!g_project_dir) return;
	path = ide_join(g_project_dir, ".raven-ide");
	text = Rfile_read(path, &size);
	free(path);
	if (!text) return;
	{
		char* line = text;
		while (line && *line) {
			char* end = strchr(line, '\n');
			char* equal;
			if (end) *end = 0;
			if (end && end > line && end[-1] == '\r') end[-1] = 0;
			equal = strchr(line, '=');
			if (equal) {
				*equal = 0;
#define READ(field, key) if (!strcmp(line, key)) snprintf(g_settings.field, sizeof(g_settings.field), "%s", equal + 1)
				READ(build_type, "build_type");
				READ(raven_prefix, "raven_prefix");
				READ(generator, "generator");
				READ(run_arguments, "run_arguments");
#undef READ
			}
			line = end ? end + 1 : NULL;
		}
	}
	Rmemory_free(text);
}

void project_save_settings(void) {
	ide_buffer_t out = { 0 };
	char* path;
	if (!g_project_dir) return;
	ide_buffer_printf(&out, "build_type=%s\nraven_prefix=%s\ngenerator=%s\nrun_arguments=%s\n", g_settings.build_type,
		g_settings.raven_prefix, g_settings.generator, g_settings.run_arguments);
	path = ide_join(g_project_dir, ".raven-ide");
	Rfile_write(path, out.data, out.size);
	free(path);
	ide_buffer_free(&out);
}

/* ---------------------------------------------------------------------------
 *  Explorer
 * ------------------------------------------------------------------------- */

static int skipped(const char* name, int is_dir) {
	if (name[0] == '.') return R_TRUE;
	if (is_dir && (ide_iequal(name, "build") || !strncmp(name, "build-", 6) || ide_iequal(name, "out") || ide_iequal(name, "node_modules")
		|| ide_iequal(name, "CMakeFiles"))) return R_TRUE;
	return R_FALSE;
}

static void explorer_add(Rid parent, const char* folder, int depth) {
	ide_entry_t* entries;
	int count, i;
	if (depth > 8 || g_p.entry_count > 4000) return;
	count = ide_list_dir(folder, &entries);
	for (i = 0; i < count; i++) {
		char* full;
		Rid item;
		if (skipped(entries[i].name, entries[i].is_dir)) continue;
		full = ide_join(folder, entries[i].name);
		item = Rtree_view_add_item(g_p.explorer, parent, entries[i].name);
		g_p.paths = (char**)realloc(g_p.paths, sizeof(char*) * (size_t)(g_p.path_count + 1));
		g_p.paths[g_p.path_count++] = full;
		Rtree_item_set_user_handle(item, full);
		g_p.entry_count++;
		if (entries[i].is_dir) {
			Rtree_item_set_expanded(item, depth < 1);
			explorer_add(item, full, depth + 1);
		} else {
			const char* ext = ide_extension(entries[i].name);
			float r = 0.85f, gr = 0.85f, b = 0.85f;
			int colored = 1;
			if (ide_interface_language(entries[i].name)) { r = 0.55f; gr = 0.75f; b = 1.0f; }
			else if (ide_iequal(ext, "xml")) { r = 0.55f; gr = 0.85f; b = 0.75f; }
			else if (ide_iequal(ext, "csv")) { r = 1.0f; gr = 0.8f; b = 0.45f; }
			else if (ide_iequal(entries[i].name, "CMakeLists.txt")) { r = 0.9f; gr = 0.6f; b = 0.9f; }
			else colored = 0;
			if (colored) Rtree_item_set_text_color(item, r, gr, b, 1.0f);
		}
	}
	free(entries);
}

static void free_paths(void) {
	int i;
	for (i = 0; i < g_p.path_count; i++) free(g_p.paths[i]);
	free(g_p.paths);
	g_p.paths = NULL;
	g_p.path_count = 0;
}

void project_refresh_explorer(void) {
	if (g_p.explorer == R_INVALID_ID) return;
	Rtree_view_clear_items(g_p.explorer);
	free_paths();
	g_p.entry_count = 0;
	if (!g_project_dir) {
		Rtree_view_add_item(g_p.explorer, R_INVALID_ID, "No project: Project > Open folder, or New project");
		return;
	}
	explorer_add(R_INVALID_ID, g_project_dir, 0);
}

static void on_explorer_activated(Rid tree, Rid item, void* user_data) {
	const char* path;
	(void)tree;
	(void)user_data;
	if (item == R_INVALID_ID) return;
	path = (const char*)Rtree_item_get_user_handle(item);
	if (!path || ide_is_dir(path)) return;
	ide_open_file(path, 0);
}

/* ---------------------------------------------------------------------------
 *  Opening files
 * ------------------------------------------------------------------------- */

/* The visual editor holds changes: ask before it shows another interface. */
static int interface_can_change(void) {
	RMessageResult answer;
	if (!designer_is_modified()) return R_TRUE;
	answer = Rmessage_box(g_ide_window, "Unsaved changes", "The interface has changes that were not saved. Save them?", R_MESSAGE_YES_NO_CANCEL);
	if (answer == R_MESSAGE_RESULT_CANCEL) return R_FALSE;
	if (answer == R_MESSAGE_RESULT_YES) return designer_save();
	return R_TRUE;
}

void designer_open_file(const char* path);

void ide_open_file(const char* path, int line) {
	const char* ext = ide_extension(path);
	if (ide_interface_language(path)) {
		if (!(xmlc_is_open() && ide_same_path(xmlc_path(), path))) {
			if (!interface_can_change()) return;
			if (!xmlc_open(path)) return;
		}
		Rdock_area_open_panel(g_ide_dock, g_ide_center);
		if (line > 0) {
			Rid script;
			int editor_line;
			if (xmlc_locate(line, &script, &editor_line)) {
				if (script != R_INVALID_ID) {
					Rdock_area_open_panel(g_ide_dock, script);
					Rtext_editor_set_cursor(script, editor_line - 1, 0);
					Rtext_editor_scroll_to_line(script, editor_line - 1);
					Rwidget_focus(script);
				} else {
					designer_go_to_line(editor_line);
				}
			}
		}
	} else if (ide_iequal(ext, "xml")) {
		if (!(designer_get_path() && ide_same_path(designer_get_path(), path))) {
			if (!interface_can_change()) return;
			if (xmlc_is_open()) xmlc_close();
			designer_open_file(path);
		}
		if (line > 0) designer_go_to_line(line);
	} else if (ide_iequal(ext, "csv") && line <= 0) {
		strings_open(path);
	} else {
		editors_open(path, line);
	}
}

/* ---------------------------------------------------------------------------
 *  Templates of a new project, one per language
 *
 *  {{NAME}} is the name of the project, {{BINDINGS}} the folder of the
 *  bindings of Raven (share/raven/bindings in a package).
 * ------------------------------------------------------------------------- */

static const char TEMPLATE_STRINGS[] =
"key,en,fr\n"
"hello.title,Hello!,Bonjour !\n"
"hello.none,You have not clicked yet.,Vous n'avez pas encore cliqué.\n"
"hello.button,Click me,Cliquez-moi\n"
"clicks.one,{n} click,{n} clic\n"
"clicks.other,{n} clicks,{n} clics\n";

/* the interface, the same in every language: {{CALLBACK}} is the name of the function */
static const char TEMPLATE_WINDOW_XML[] =
"<Window Title=\"{{NAME}}\" Width=\"480\" Height=\"320\">\n"
"\t<VBox Spacing=\"12\" Slot.Padding=\"24\">\n"
"\t\t<Label Text=\"@hello.title\" TextSize=\"24\"/>\n"
"\t\t<Label Name=\"status\" Text=\"@hello.none\" Opacity=\"0.7\"/>\n"
"\t\t<Button Text=\"@hello.button\" Style=\"Primary\" OnClicked=\"{{{CALLBACK}}}\" Slot.HAlign=\"Left\"/>\n"
"\t</VBox>\n"
"</Window>\n";

static const char TEMPLATE_CMAKE[] =
"cmake_minimum_required(VERSION 3.16)\n"
"project({{NAME}} {{CMAKE_LANGUAGE}})\n"
"\n"
"# Raven: the package (bin/, include/, lib/). The IDE gives its folder in CMAKE_PREFIX_PATH.\n"
"find_package(raven REQUIRED)\n"
"\n"
"# {{MAIN}} holds the interface and its code: ravenc turns it into {{LANGUAGE}} (raven_target_xmlc)\n"
"add_executable({{NAME}} {{MAIN}} strings.csv)\n"
"target_link_libraries({{NAME}} PRIVATE raven::raven)\n"
"raven_target_xmlc({{NAME}})\n"
"if(MSVC)\n"
"\ttarget_compile_definitions({{NAME}} PRIVATE _CRT_SECURE_NO_WARNINGS)\n"
"endif()\n";

static const char SCRIPT_C[] =
"<!-- {{NAME}}: the interface and its C code, in one file. F5 builds and runs it. -->\n"
"<script>\n"
"#include <stdio.h>\n"
"\n"
"static int g_clicks = 0;\n"
"\n"
"/* OnClicked=\"{on_hello}\" below calls this function: the compiler checks it */\n"
"static void on_hello(Rid sender, const char* action, void* user_data) {\n"
"\t(void)sender;\n"
"\t(void)action;\n"
"\t(void)user_data;\n"
"\tg_clicks++;\n"
"\t/* ui.status: the widget with Name=\"status\" */\n"
"\tRlabel_set_text(ui.status, Rtr_plural(\"clicks\", g_clicks));\n"
"}\n"
"\n"
"int main(void) {\n"
"\tif (!Rinit(R_BACKEND_VULKAN)) return 1;\n"
"\tRlocale_load_csv(\"strings.csv\");\n"
"\tRlocale_set_language(Rlocale_get_system_language());\n"
"\tmain_create();\n"
"\twhile (Rdraw_frame()) {}\n"
"\tRshutdown();\n"
"\treturn 0;\n"
"}\n"
"</script>\n"
"\n";

static const char SCRIPT_CPP[] =
"<!-- {{NAME}}: the interface and its C++ code, in one file. F5 builds and runs it. -->\n"
"<script>\n"
"#include <string>\n"
"\n"
"static int g_clicks = 0;\n"
"\n"
"// OnClicked=\"{on_hello}\" below calls this function: the compiler checks it\n"
"static void on_hello(Rid sender, const char* action) {\n"
"\t(void)sender;\n"
"\t(void)action;\n"
"\tg_clicks++;\n"
"\t// ui.status: the widget with Name=\"status\"\n"
"\tRlabel_set_text(ui.status, Rtr_plural(\"clicks\", g_clicks));\n"
"}\n"
"\n"
"int main() {\n"
"\tif (!Rinit(R_BACKEND_VULKAN)) return 1;\n"
"\tRlocale_load_csv(\"strings.csv\");\n"
"\tRlocale_set_language(Rlocale_get_system_language());\n"
"\tmain_create();\n"
"\twhile (Rdraw_frame()) {}\n"
"\tRshutdown();\n"
"\treturn 0;\n"
"}\n"
"</script>\n"
"\n";

static const char CARGO_TOML[] =
"[package]\n"
"name = \"{{NAME}}\"\n"
"version = \"0.1.0\"\n"
"edition = \"2021\"\n"
"\n"
"[dependencies]\n"
"raven = { path = \"{{BINDINGS}}/rust\" }\n"
"\n"
"[build-dependencies]\n"
"raven-build = { path = \"{{BINDINGS}}/rust-build\" }\n";

static const char CARGO_BUILD_RS[] =
"// ravenc turns src/main.xmlrs into Rust ($OUT_DIR/main.xmlrs.rs)\n"
"fn main() {\n"
"    raven_build::compile(\"src/main.xmlrs\");\n"
"}\n";

static const char CARGO_MAIN_RS[] =
"// The program is src/main.xmlrs: the interface and its Rust code.\n"
"include!(concat!(env!(\"OUT_DIR\"), \"/main.xmlrs.rs\"));\n";

static const char SCRIPT_RUST[] =
"<!-- {{NAME}}: the interface and its Rust code, in one file. F5 builds and runs it. -->\n"
"<script>\n"
"use raven::*;\n"
"use std::sync::atomic::{AtomicI32, Ordering};\n"
"\n"
"static CLICKS: AtomicI32 = AtomicI32::new(0);\n"
"\n"
"/// OnClicked=\"{on_hello}\" below calls this function: the compiler checks it\n"
"fn on_hello(_sender: Rid, _action: &str) {\n"
"    let clicks = CLICKS.fetch_add(1, Ordering::SeqCst) + 1;\n"
"    // ui().status: the widget with Name=\"status\"\n"
"    unsafe { Rlabel_set_text(ui().status, Rtr_plural(cstr(\"clicks\").as_ptr(), clicks)) };\n"
"}\n"
"\n"
"fn main() {\n"
"    unsafe {\n"
"        if Rinit(R_BACKEND_VULKAN) == 0 {\n"
"            return;\n"
"        }\n"
"        Rlocale_load_csv(cstr(\"strings.csv\").as_ptr());\n"
"        Rlocale_set_language(Rlocale_get_system_language());\n"
"    }\n"
"    main_create();\n"
"    unsafe {\n"
"        while Rdraw_frame() != 0 {}\n"
"        Rshutdown();\n"
"    }\n"
"}\n"
"</script>\n"
"\n";

static const char SCRIPT_PYTHON[] =
"<!-- {{NAME}}: the interface and its Python code, in one file. F5 runs it. -->\n"
"<script>\n"
"import sys\n"
"import raven as r\n"
"\n"
"clicks = 0\n"
"\n"
"\n"
"# OnClicked=\"{on_hello}\" below calls this function\n"
"def on_hello(sender, action):\n"
"    global clicks\n"
"    clicks += 1\n"
"    # ui.status: the widget with Name=\"status\"\n"
"    r.Rlabel_set_text(ui.status, r.Rtr_plural(\"clicks\", clicks))\n"
"\n"
"\n"
"if __name__ == \"__main__\":\n"
"    if not r.Rinit(r.R_BACKEND_VULKAN):\n"
"        sys.exit(1)\n"
"    r.Rlocale_load_csv(\"strings.csv\")\n"
"    r.Rlocale_set_language(r.Rlocale_get_system_language())\n"
"    main_create()\n"
"    while r.Rdraw_frame():\n"
"        pass\n"
"    r.Rshutdown()\n"
"</script>\n"
"\n";

static const char GO_MOD[] =
"module {{NAME}}\n"
"\n"
"go 1.18\n"
"\n"
"require raven v0.0.0\n"
"\n"
"replace raven => {{BINDINGS}}/go\n";

static const char GO_GENERATE[] =
"package main\n"
"\n"
"// ravenc turns main.xmlgo into Go (main.xmlgo.go): go generate, then go build\n"
"//go:generate ravenc main.xmlgo -o main.xmlgo.go\n";

static const char SCRIPT_GO[] =
"<!-- {{NAME}}: the interface and its Go code, in one file. F5 builds and runs it. -->\n"
"<script>\n"
"package main\n"
"\n"
"import \"raven/raven\"\n"
"\n"
"var clicks = 0\n"
"\n"
"// OnClicked=\"{on_hello}\" below calls this function: the compiler checks it\n"
"func on_hello(sender raven.ID, action string) {\n"
"\tclicks++\n"
"\t// ui.status: the widget with Name=\"status\"\n"
"\traven.LabelSetText(ui.status, raven.TrPlural(\"clicks\", clicks))\n"
"}\n"
"\n"
"func main() {\n"
"\tif raven.Init(raven.R_BACKEND_VULKAN) == 0 {\n"
"\t\treturn\n"
"\t}\n"
"\traven.LocaleLoadCsv(\"strings.csv\")\n"
"\traven.LocaleSetLanguage(raven.LocaleGetSystemLanguage())\n"
"\tmain_create()\n"
"\tfor raven.DrawFrame() != 0 {\n"
"\t}\n"
"\traven.Shutdown()\n"
"}\n"
"</script>\n"
"\n";

static const char CSPROJ[] =
"<Project Sdk=\"Microsoft.NET.Sdk\">\n"
"  <PropertyGroup>\n"
"    <OutputType>Exe</OutputType>\n"
"    <TargetFramework>net8.0</TargetFramework>\n"
"    <Nullable>disable</Nullable>\n"
"    <ImplicitUsings>disable</ImplicitUsings>\n"
"  </PropertyGroup>\n"
"  <ItemGroup>\n"
"    <Compile Include=\"{{BINDINGS}}/csharp/Raven.cs\" Link=\"Raven.cs\" />\n"
"    <None Update=\"strings.csv\" CopyToOutputDirectory=\"PreserveNewest\" />\n"
"  </ItemGroup>\n"
"  <!-- ravenc turns every .xmlcs of the project into C# before the compiler runs -->\n"
"  <Import Project=\"{{BINDINGS}}/csharp/Raven.XmlCs.targets\" />\n"
"</Project>\n";

static const char SCRIPT_CSHARP[] =
"<!-- {{NAME}}: the interface and its C# code, in one file. F5 builds and runs it. -->\n"
"<script>\n"
"using Raven;\n"
"\n"
"public static partial class MainUi\n"
"{\n"
"    static int clicks;\n"
"\n"
"    // OnClicked=\"{OnHello}\" below calls this method: the compiler checks it\n"
"    static void OnHello(uint sender, string action)\n"
"    {\n"
"        clicks++;\n"
"        // status: the widget with Name=\"status\"\n"
"        R.Rlabel_set_text(status, R.Rtr_plural(\"clicks\", clicks));\n"
"    }\n"
"}\n"
"\n"
"public static class Program\n"
"{\n"
"    public static int Main()\n"
"    {\n"
"        if (R.Rinit(RBackend.R_BACKEND_VULKAN) == 0) return 1;\n"
"        R.Rlocale_load_csv(\"strings.csv\");\n"
"        R.Rlocale_set_language(R.Rlocale_get_system_language());\n"
"        MainUi.Create();\n"
"        while (R.Rdraw_frame() != 0) { }\n"
"        R.Rshutdown();\n"
"        return 0;\n"
"    }\n"
"}\n"
"</script>\n"
"\n";

static const char BUILD_ZIG[] =
"const std = @import(\"std\");\n"
"// (copies of bindings/zig of Raven: a build.zig only imports files of its project)\n"
"const xmlzig = @import(\"raven/xmlzig.zig\");\n"
"\n"
"pub fn build(b: *std.Build) void {\n"
"    const target = b.standardTargetOptions(.{});\n"
"    const optimize = b.standardOptimizeOption(.{});\n"
"    const raven = b.createModule(.{ .root_source_file = b.path(\"raven/raven.zig\"), .target = target, .optimize = optimize });\n"
"    // ravenc turns main.xmlzig into Zig: it is the program\n"
"    const exe = b.addExecutable(.{\n"
"        .name = \"{{NAME}}\",\n"
"        .root_module = b.createModule(.{ .root_source_file = xmlzig.compile(b, \"main.xmlzig\"), .target = target, .optimize = optimize }),\n"
"    });\n"
"    exe.root_module.addImport(\"raven\", raven);\n"
"    exe.root_module.linkSystemLibrary(\"raven\", .{});\n"
"    exe.root_module.link_libc = true;\n"
"    if (b.graph.env_map.get(\"RAVEN_LIB_DIR\")) |dir| exe.root_module.addLibraryPath(.{ .cwd_relative = dir });\n"
"    b.installArtifact(exe);\n"
"    const run = b.addRunArtifact(exe);\n"
"    run.setCwd(b.path(\".\"));\n"
"    b.step(\"run\", \"Run the program\").dependOn(&run.step);\n"
"}\n";

static const char SCRIPT_ZIG[] =
"<!-- {{NAME}}: the interface and its Zig code, in one file. F5 builds and runs it. -->\n"
"<script>\n"
"const r = @import(\"raven\");\n"
"\n"
"var clicks: c_int = 0;\n"
"\n"
"// OnClicked=\"{on_hello}\" below calls this function: the compiler checks it\n"
"fn on_hello(sender: r.Rid, action: ?[*:0]const u8, user_data: ?*anyopaque) callconv(.C) void {\n"
"    _ = sender;\n"
"    _ = action;\n"
"    _ = user_data;\n"
"    clicks += 1;\n"
"    // ui.status: the widget with Name=\"status\"\n"
"    r.Rlabel_set_text(ui.status, r.Rtr_plural(\"clicks\", clicks));\n"
"}\n"
"\n"
"pub fn main() void {\n"
"    if (r.Rinit(r.R_BACKEND_VULKAN) == 0) return;\n"
"    _ = r.Rlocale_load_csv(\"strings.csv\");\n"
"    _ = r.Rlocale_set_language(r.Rlocale_get_system_language());\n"
"    _ = main_create();\n"
"    while (r.Rdraw_frame() != 0) {}\n"
"    r.Rshutdown();\n"
"}\n"
"</script>\n"
"\n";

static const char SCRIPT_LUA[] =
"<!-- {{NAME}}: the interface and its Lua code (LuaJIT), in one file. F5 runs it. -->\n"
"<script>\n"
"local r = require(\"raven\")\n"
"\n"
"clicks = 0\n"
"\n"
"-- OnClicked=\"{on_hello}\" below calls this function (a global function)\n"
"function on_hello(sender, action)\n"
"\tclicks = clicks + 1\n"
"\t-- ui.status: the widget with Name=\"status\"\n"
"\tr.Rlabel_set_text(ui.status, r.Rtr_plural(\"clicks\", clicks))\n"
"end\n"
"\n"
"if r.Rinit(r.R_BACKEND_VULKAN) == 0 then os.exit(1) end\n"
"r.Rlocale_load_csv(\"strings.csv\")\n"
"r.Rlocale_set_language(r.Rlocale_get_system_language())\n"
"main_create()\n"
"while r.Rdraw_frame() ~= 0 do end\n"
"r.Rshutdown()\n"
"</script>\n"
"\n";

/* "{{NAME}}" and the others, replaced */
static char* expand(const char* text, const char* name, const char* bindings, const char* extra_key, const char* extra_value) {
	ide_buffer_t out = { 0 };
	const char* p = text;
	while (*p) {
		const char* open = strstr(p, "{{");
		const char* value = NULL;
		size_t skip = 0;
		if (!open) {
			ide_buffer_add(&out, p, strlen(p));
			break;
		}
		ide_buffer_add(&out, p, (size_t)(open - p));
		if (!strncmp(open, "{{NAME}}", 8)) { value = name; skip = 8; }
		else if (!strncmp(open, "{{BINDINGS}}", 12)) { value = bindings; skip = 12; }
		else if (extra_key && !strncmp(open, extra_key, strlen(extra_key))) { value = extra_value; skip = strlen(extra_key); }
		if (value) {
			ide_buffer_add(&out, value, strlen(value));
			p = open + skip;
		} else {
			ide_buffer_add(&out, "{", 1);
			p = open + 1;
		}
	}
	return out.data ? out.data : ide_strdup("");
}

/* Where the bindings of Raven are: the package, or the sources the IDE was built from. */
static char* bindings_dir(void) {
	char* candidates[3];
	char* found = NULL;
	int i;
	candidates[0] = g_settings.raven_prefix[0] ? ide_join(g_settings.raven_prefix, "share/raven/bindings") : NULL;
	candidates[1] = ide_join(ide_exe_dir(), "../share/raven/bindings");
	candidates[2] = RAVEN_IDE_INCLUDE_DIR[0] ? ide_join(RAVEN_IDE_INCLUDE_DIR, "../bindings") : NULL;
	for (i = 0; i < 3; i++) {
		if (!found && candidates[i] && ide_is_dir(candidates[i])) found = ide_normalize(candidates[i]);
		free(candidates[i]);
	}
	return found ? found : ide_strdup("RAVEN/share/raven/bindings");
}

static int copy_file(const char* from, const char* to) {
	size_t size = 0;
	char* data = Rfile_read(from, &size);
	int ok;
	if (!data) return R_FALSE;
	ok = Rfile_write(to, data, size);
	Rmemory_free(data);
	return ok;
}

/* ---------------------------------------------------------------------------
 *  Projects
 * ------------------------------------------------------------------------- */

int project_target_name(char* out, size_t size) {
	char* path;
	char* text;
	const char* p;
	size_t length = 0;
	out[0] = 0;
	if (!g_project_dir) return R_FALSE;
	path = ide_join(g_project_dir, "CMakeLists.txt");
	text = Rfile_read(path, &length);
	free(path);
	if (!text) return R_FALSE;
	for (p = text; (p = strstr(p, "add_executable")) != NULL; p++) {
		const char* q = p + 14;
		size_t n = 0;
		while (*q == ' ' || *q == '\t') q++;
		if (*q != '(') continue;
		q++;
		while (isspace((unsigned char)*q)) q++;
		while (q[n] && (isalnum((unsigned char)q[n]) || q[n] == '_' || q[n] == '-' || q[n] == '.' || q[n] == '$' || q[n] == '{' || q[n] == '}')) n++;
		if (n == 0) continue;
		/* add_executable(${PROJECT_NAME} ...): the name of the project */
		if (!strncmp(q, "${PROJECT_NAME}", 15)) {
			const char* pr = strstr(text, "project(");
			if (pr) {
				pr += 8;
				while (isspace((unsigned char)*pr)) pr++;
				n = 0;
				while (pr[n] && (isalnum((unsigned char)pr[n]) || pr[n] == '_' || pr[n] == '-')) n++;
				snprintf(out, size, "%.*s", (int)n, pr);
			}
		} else {
			snprintf(out, size, "%.*s", (int)n, q);
		}
		break;
	}
	Rmemory_free(text);
	return out[0] != 0;
}

int project_open(const char* folder) {
	char* normalized;
	if (!ide_is_dir(folder)) {
		ide_status("%s is not a folder.", folder);
		return R_FALSE;
	}
	normalized = ide_normalize(folder);
	{
		size_t n = strlen(normalized);
		while (n > 1 && normalized[n - 1] == '/') normalized[--n] = 0;
	}
	free(g_project_dir);
	g_project_dir = normalized;
	load_settings();
	project_refresh_explorer();
	strings_reload();
	{
		char title[400];
		snprintf(title, sizeof(title), "Explorer - %s", ide_basename(g_project_dir));
		Rdock_area_set_panel_title(g_ide_dock, Rwidget_find(g_ide_window, "panel.explorer"), title);
	}
	/* the interface of the project, when there is one: main.xml?? at the top or in src/ */
	{
		static const char* const folders[] = { "", "src" };
		int f, i, opened = 0;
		for (f = 0; f < 2 && !opened; f++) {
			char* dir = f ? ide_join(g_project_dir, folders[f]) : ide_strdup(g_project_dir);
			for (i = 0; i < ide_language_count() && !opened; i++) {
				char name[64];
				char* path;
				snprintf(name, sizeof(name), "main.%s", ide_language_at(i)->extension);
				path = ide_join(dir, name);
				if (ide_file_exists(path)) {
					ide_open_file(path, 0);
					opened = 1;
				}
				free(path);
			}
			free(dir);
		}
	}
	ide_status("Project %s opened. F5 builds and runs it.", ide_basename(g_project_dir));
	return R_TRUE;
}

static int valid_name(const char* name) {
	const char* p;
	if (!name || !name[0] || !(isalpha((unsigned char)name[0]) || name[0] == '_')) return R_FALSE;
	for (p = name; *p; p++) if (!(isalnum((unsigned char)*p) || *p == '_' || *p == '-')) return R_FALSE;
	return R_TRUE;
}

static int write_new(const char* folder, const char* file, const char* text) {
	char* path = ide_join(folder, file);
	int ok = R_TRUE;
	if (!ide_file_exists(path)) ok = Rfile_write(path, text, strlen(text));
	free(path);
	return ok;
}

/* The interface, after the scripts, with the callback of the language. */
static char* main_file(const char* script, const char* name, const char* bindings, const char* callback) {
	char* head = expand(script, name, bindings, NULL, NULL);
	char* window = expand(TEMPLATE_WINDOW_XML, name, bindings, "{{CALLBACK}}", callback);
	char* both = (char*)malloc(strlen(head) + strlen(window) + 1);
	strcpy(both, head);
	strcat(both, window);
	free(head);
	free(window);
	return both;
}

static int write_expanded(const char* folder, const char* file, const char* text, const char* name, const char* bindings) {
	char* content = expand(text, name, bindings, NULL, NULL);
	int ok = write_new(folder, file, content);
	free(content);
	return ok;
}

int project_create(const char* parent_folder, const char* name, const char* language) {
	char* folder;
	char* bindings;
	char* text;
	const char* main_name = NULL;
	int ok = R_TRUE;
	if (!valid_name(name)) {
		ide_status("A project name is made of letters, digits, '_' and '-', and starts with a letter.");
		return R_FALSE;
	}
	folder = ide_join(parent_folder, name);
	if (!ide_make_dirs(folder)) {
		ide_status("Cannot create the folder %s", folder);
		free(folder);
		return R_FALSE;
	}
	bindings = bindings_dir();
	if (!language || !language[0]) language = "C";

	if (ide_iequal(language, "C") || ide_iequal(language, "C++")) {
		int cpp = ide_iequal(language, "C++");
		char* cmake = expand(TEMPLATE_CMAKE, name, bindings, "{{CMAKE_LANGUAGE}}", cpp ? "CXX" : "C");
		char* cmake2 = expand(cmake, name, bindings, "{{MAIN}}", cpp ? "main.xmlc++" : "main.xmlc");
		char* cmake3 = expand(cmake2, name, bindings, "{{LANGUAGE}}", cpp ? "C++" : "C");
		ok = write_new(folder, "CMakeLists.txt", cmake3);
		free(cmake);
		free(cmake2);
		free(cmake3);
		main_name = cpp ? "main.xmlc++" : "main.xmlc";
		text = main_file(cpp ? SCRIPT_CPP : SCRIPT_C, name, bindings, "on_hello");
		ok = ok && write_new(folder, main_name, text);
		free(text);
		ok = ok && write_new(folder, ".gitignore", "build/\n.raven-ide\n");
	} else if (ide_iequal(language, "Rust")) {
		char* src = ide_join(folder, "src");
		ok = write_expanded(folder, "Cargo.toml", CARGO_TOML, name, bindings);
		ok = ok && write_new(folder, "build.rs", CARGO_BUILD_RS);
		ok = ok && ide_make_dirs(src);
		ok = ok && write_new(src, "main.rs", CARGO_MAIN_RS);
		text = main_file(SCRIPT_RUST, name, bindings, "on_hello");
		ok = ok && write_new(src, "main.xmlrs", text);
		free(text);
		free(src);
		ok = ok && write_new(folder, ".gitignore", "target/\n.raven-ide\n");
		main_name = "src/main.xmlrs";
	} else if (ide_iequal(language, "Python")) {
		main_name = "main.xmlpy";
		text = main_file(SCRIPT_PYTHON, name, bindings, "on_hello");
		ok = write_new(folder, main_name, text);
		free(text);
		ok = ok && write_new(folder, ".gitignore", "__pycache__/\n.raven-ide\n");
	} else if (ide_iequal(language, "Go")) {
		main_name = "main.xmlgo";
		ok = write_expanded(folder, "go.mod", GO_MOD, name, bindings);
		ok = ok && write_new(folder, "generate.go", GO_GENERATE);
		text = main_file(SCRIPT_GO, name, bindings, "on_hello");
		ok = ok && write_new(folder, main_name, text);
		free(text);
		ok = ok && write_new(folder, ".gitignore", "main.xmlgo.go\n.raven-ide\n");
	} else if (ide_iequal(language, "C#")) {
		char csproj[300];
		main_name = "main.xmlcs";
		snprintf(csproj, sizeof(csproj), "%s.csproj", name);
		ok = write_expanded(folder, csproj, CSPROJ, name, bindings);
		text = main_file(SCRIPT_CSHARP, name, bindings, "OnHello");
		ok = ok && write_new(folder, main_name, text);
		free(text);
		ok = ok && write_new(folder, ".gitignore", "bin/\nobj/\n.raven-ide\n");
	} else if (ide_iequal(language, "Zig")) {
		char* vendored = ide_join(folder, "raven");
		char* from;
		char* to;
		main_name = "main.xmlzig";
		ok = write_expanded(folder, "build.zig", BUILD_ZIG, name, bindings);
		ok = ok && ide_make_dirs(vendored);
		/* (a build.zig only imports the files of its project: the bindings are copied in it) */
		from = ide_join(bindings, "zig/raven.zig");
		to = ide_join(vendored, "raven.zig");
		ok = ok && copy_file(from, to);
		free(from);
		free(to);
		from = ide_join(bindings, "zig/xmlzig.zig");
		to = ide_join(vendored, "xmlzig.zig");
		ok = ok && copy_file(from, to);
		free(from);
		free(to);
		free(vendored);
		text = main_file(SCRIPT_ZIG, name, bindings, "on_hello");
		ok = ok && write_new(folder, main_name, text);
		free(text);
		ok = ok && write_new(folder, ".gitignore", "zig-out/\n.zig-cache/\n.raven-ide\n");
	} else if (ide_iequal(language, "Lua")) {
		main_name = "main.xmllua";
		text = main_file(SCRIPT_LUA, name, bindings, "on_hello");
		ok = write_new(folder, main_name, text);
		free(text);
		ok = ok && write_new(folder, ".gitignore", "*.xmllua.lua\n.raven-ide\n");
	} else {
		ide_status("Unknown language %s", language);
		ok = R_FALSE;
	}
	ok = ok && write_new(folder, "strings.csv", TEMPLATE_STRINGS);
	free(bindings);
	if (!ok) {
		ide_status("Cannot write the files of the project in %s (are the bindings of Raven there?)", folder);
		free(folder);
		return R_FALSE;
	}
	(void)main_name;
	ok = project_open(folder);
	free(folder);
	return ok;
}

/* ---------------------------------------------------------------------------
 *  Dialogs
 * ------------------------------------------------------------------------- */

static void on_dialog_button(Rid button, void* user_data) {
	Rdialog_close(button, (int)(size_t)user_data);
}

static void on_browse_folder(Rid button, void* user_data) {
	RFileDialogDesc desc;
	const char* chosen;
	(void)button;
	memset(&desc, 0, sizeof(desc));
	desc.parent = g_ide_window;
	desc.title = "Choose a folder";
	chosen = Rfile_dialog_select_folder(&desc);
	if (chosen) Rtext_input_set_text((Rid)(size_t)user_data, chosen);
}

static const char NEW_PROJECT_XML[] =
"<Border Padding=\"20\" Width=\"520\">"
"  <VBox Spacing=\"12\">"
"    <Label Text=\"New project\" TextSize=\"20\"/>"
"    <Label Text=\"An interface with its code in one file (main.xmlc, main.xmlrs...), what the tools of its language need, and its translations (strings.csv). It builds as it is.\" Wrap=\"true\" Opacity=\"0.7\"/>"
"    <HBox Spacing=\"8\"><Label Text=\"Name\" Width=\"90\" Slot.VAlign=\"Center\"/><TextInput Name=\"name\" Text=\"my_app\" Slot.Fill=\"1\"/></HBox>"
"    <HBox Spacing=\"8\"><Label Text=\"Language\" Width=\"90\" Slot.VAlign=\"Center\"/>"
"      <ComboBox Name=\"language\" Items=\"C (CMake)|C++ (CMake)|Rust (Cargo)|Python|Go|C# (dotnet)|Zig (zig build)|Lua (LuaJIT)\" Selected=\"0\" Slot.Fill=\"1\"/></HBox>"
"    <HBox Spacing=\"8\"><Label Text=\"In the folder\" Width=\"90\" Slot.VAlign=\"Center\"/><TextInput Name=\"folder\" Slot.Fill=\"1\"/>"
"      <Button Name=\"browse\" Text=\"Browse...\"/></HBox>"
"    <HBox Spacing=\"8\" Slot.HAlign=\"Right\"><Button Name=\"cancel\" Text=\"Cancel\"/><Button Name=\"create\" Text=\"Create\" Style=\"Primary\"/></HBox>"
"  </VBox>"
"</Border>";

/* Rdialog_run deletes the content when it closes: the texts are kept as they change. */
static char g_new_name[256], g_new_folder[1024];
static int g_new_language = 0;
static void keep_language(Rid combo, int index, void* user_data) {
	(void)combo;
	(void)user_data;
	g_new_language = index;
}
static void keep_text(Rid input, const char* text, void* user_data) {
	(void)input;
	snprintf((char*)user_data, user_data == g_new_name ? sizeof(g_new_name) : sizeof(g_new_folder), "%s", text);
}

void project_new(void);
void project_new(void) {
	Rid content = Rwidget_create_from_xml(NEW_PROJECT_XML, sizeof(NEW_PROJECT_XML) - 1);
	Rid name, folder;
	if (content == R_INVALID_ID) return;
	name = Rwidget_find(content, "name");
	folder = Rwidget_find(content, "folder");
	snprintf(g_new_name, sizeof(g_new_name), "%s", Rtext_input_get_text(name));
	snprintf(g_new_folder, sizeof(g_new_folder), "%s", g_project_dir ? g_project_dir : "");
	Rtext_input_set_text(folder, g_new_folder);
	Rtext_input_set_changed_callback(name, keep_text, g_new_name);
	Rtext_input_set_changed_callback(folder, keep_text, g_new_folder);
	Rcombo_box_set_selected(Rwidget_find(content, "language"), g_new_language);
	Rcombo_box_set_changed_callback(Rwidget_find(content, "language"), keep_language, NULL);
	Rbutton_set_clicked_callback(Rwidget_find(content, "browse"), on_browse_folder, (void*)(size_t)folder);
	Rbutton_set_clicked_callback(Rwidget_find(content, "cancel"), on_dialog_button, (void*)0);
	Rbutton_set_clicked_callback(Rwidget_find(content, "create"), on_dialog_button, (void*)1);
	if (Rdialog_run(g_ide_window, content) != 1) return;
	if (!g_new_folder[0]) {
		ide_status("Choose the folder the project goes in.");
		return;
	}
	{
		static const char* const languages[] = { "C", "C++", "Rust", "Python", "Go", "C#", "Zig", "Lua" };
		project_create(g_new_folder, g_new_name, languages[g_new_language >= 0 && g_new_language < 8 ? g_new_language : 0]);
	}
}

static const char SETTINGS_XML[] =
"<Border Padding=\"20\" Width=\"600\">"
"  <VBox Spacing=\"12\">"
"    <Label Text=\"Build settings\" TextSize=\"20\"/>"
"    <HBox Spacing=\"8\"><Label Text=\"Build type\" Width=\"130\" Slot.VAlign=\"Center\"/>"
"      <ComboBox Name=\"type\" Items=\"Debug|Release|RelWithDebInfo|MinSizeRel\" Slot.Fill=\"1\"/></HBox>"
"    <HBox Spacing=\"8\"><Label Text=\"Raven package\" Width=\"130\" Slot.VAlign=\"Center\" Tooltip=\"The folder of the Raven package (bin/, include/, lib/): find_package(raven) looks there\"/>"
"      <TextInput Name=\"prefix\" Slot.Fill=\"1\" Placeholder=\"the folder with bin/, include/ and lib/\"/><Button Name=\"browse.prefix\" Text=\"Browse...\"/></HBox>"
"    <HBox Spacing=\"8\"><Label Text=\"Generator\" Width=\"130\" Slot.VAlign=\"Center\"/>"
"      <TextInput Name=\"generator\" Slot.Fill=\"1\" Placeholder=\"the default of CMake (Ninja, MinGW Makefiles...)\"/></HBox>"
"    <HBox Spacing=\"8\"><Label Text=\"Run arguments\" Width=\"130\" Slot.VAlign=\"Center\"/><TextInput Name=\"arguments\" Slot.Fill=\"1\"/></HBox>"
"    <HBox Spacing=\"8\" Slot.HAlign=\"Right\"><Button Name=\"cancel\" Text=\"Cancel\"/><Button Name=\"ok\" Text=\"Save\" Style=\"Primary\"/></HBox>"
"  </VBox>"
"</Border>";

static ide_settings_t g_edit;
static void keep_setting(Rid input, const char* text, void* user_data) {
	(void)input;
	snprintf((char*)user_data, 1024, "%s", text);
}
static void keep_type(Rid combo, int index, void* user_data) {
	(void)user_data;
	if (index >= 0) snprintf(g_edit.build_type, sizeof(g_edit.build_type), "%s", Rcombo_box_get_item_text(combo, index));
}

void project_settings_dialog(void);
void project_settings_dialog(void) {
	Rid content = Rwidget_create_from_xml(SETTINGS_XML, sizeof(SETTINGS_XML) - 1);
	Rid type;
	int i;
	if (content == R_INVALID_ID) return;
	g_edit = g_settings;
	type = Rwidget_find(content, "type");
	for (i = 0; i < Rcombo_box_get_item_count(type); i++) if (ide_iequal(Rcombo_box_get_item_text(type, i), g_settings.build_type)) Rcombo_box_set_selected(type, i);
	Rcombo_box_set_changed_callback(type, keep_type, NULL);
#define FIELD(name, field) Rtext_input_set_text(Rwidget_find(content, name), g_edit.field); \
	Rtext_input_set_changed_callback(Rwidget_find(content, name), keep_setting, g_edit.field)
	FIELD("prefix", raven_prefix);
	FIELD("arguments", run_arguments);
	Rtext_input_set_text(Rwidget_find(content, "generator"), g_edit.generator);
	Rtext_input_set_changed_callback(Rwidget_find(content, "generator"), keep_setting, g_edit.generator);
#undef FIELD
	Rbutton_set_clicked_callback(Rwidget_find(content, "browse.prefix"), on_browse_folder, (void*)(size_t)Rwidget_find(content, "prefix"));
	Rbutton_set_clicked_callback(Rwidget_find(content, "cancel"), on_dialog_button, (void*)0);
	Rbutton_set_clicked_callback(Rwidget_find(content, "ok"), on_dialog_button, (void*)1);
	if (Rdialog_run(g_ide_window, content) != 1) return;
	g_settings = g_edit;
	project_save_settings();
	ide_status("Settings saved. The next build configures the project again.");
}

static void action_project(Rid sender, const char* action, void* user_data) {
	(void)sender;
	(void)user_data;
	if (!strcmp(action, "project.new")) {
		project_new();
	} else if (!strcmp(action, "project.open")) {
		RFileDialogDesc desc;
		const char* chosen;
		memset(&desc, 0, sizeof(desc));
		desc.parent = g_ide_window;
		desc.title = "Open a project folder";
		chosen = Rfile_dialog_select_folder(&desc);
		if (chosen) {
			char* copy = ide_strdup(chosen);
			project_open(copy);
			free(copy);
		}
	} else if (!strcmp(action, "project.refresh")) {
		project_refresh_explorer();
	} else if (!strcmp(action, "project.settings")) {
		project_settings_dialog();
	}
}

void project_init(void) {
	g_p.explorer = Rwidget_find(g_ide_window, "explorer");
	Rtree_view_set_activated_callback(g_p.explorer, on_explorer_activated, NULL);
	Raction_bind("project.new", action_project, NULL);
	Raction_bind("project.open", action_project, NULL);
	Raction_bind("project.refresh", action_project, NULL);
	Raction_bind("project.settings", action_project, NULL);
	load_settings();
	project_refresh_explorer();
}

void project_shutdown(void) {
	free_paths();
	free(g_project_dir);
	g_project_dir = NULL;
}
