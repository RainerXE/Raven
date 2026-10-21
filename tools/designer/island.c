/*
 * Raven IDE - island.c
 * The island at the top of the window (raven_shell.h): what is going on,
 * at a glance, and what to do about it.
 *
 * Folded, a capsule hanging from the top edge: a dot (gray: nothing going
 * on, blue: building or running, green: the last build went well, orange:
 * with warnings, red: it failed), the project and the file, and the state.
 * Unfolded (the pointer over it, or a click), the project and its folder,
 * the file and whether it has unsaved changes, the build (under way for how
 * long, or how the last one ended), Build / Run / Stop, the problems, the
 * content drawer, Save all. When a build ends, the island unfolds by itself
 * for a few seconds to say how.
 */

#include "ide.h"

#include <stdio.h>
#include <string.h>

static const char ISLAND_XML[] =
"<Island Name=\"island\" Attached=\"true\">"
"  <HBox Spacing=\"8\" Slot.Padding=\"14,5\">"
"    <Border Name=\"island.dot\" Width=\"8\" Height=\"8\" CornerRadius=\"4\" Background=\"#8E8E93\" Slot.VAlign=\"Center\" AccessibleHidden=\"true\"/>"
"    <Label Name=\"island.title\" Text=\"Raven IDE\" Slot.VAlign=\"Center\"/>"
"    <Label Name=\"island.state\" Opacity=\"0.6\" Slot.VAlign=\"Center\" Visibility=\"Collapsed\"/>"
"  </HBox>"
"  <VBox Spacing=\"10\" Slot.Padding=\"18,14\" Width=\"420\">"
"    <HBox Spacing=\"10\">"
"      <Border Name=\"island.dot.big\" Width=\"10\" Height=\"10\" CornerRadius=\"5\" Background=\"#8E8E93\" Slot.VAlign=\"Center\" AccessibleHidden=\"true\"/>"
"      <VBox Slot.Fill=\"1\" Spacing=\"2\">"
"        <Label Name=\"island.project\" TextSize=\"16\"/>"
"        <Label Name=\"island.where\" Opacity=\"0.55\" TextSize=\"11\"/>"
"      </VBox>"
"    </HBox>"
"    <Label Name=\"island.file\" Opacity=\"0.85\" Wrap=\"true\"/>"
"    <Separator/>"
"    <Label Name=\"island.build\" Wrap=\"true\"/>"
"    <HBox Spacing=\"6\">"
"      <Button Text=\"Build\" OnClicked=\"build.build\" Tooltip=\"Ctrl+B\"/>"
"      <Button Text=\"Run\" Style=\"Primary\" OnClicked=\"build.run\" Tooltip=\"F5\"/>"
"      <Button Name=\"island.stop\" Text=\"Stop\" OnClicked=\"build.stop\" Tooltip=\"Ctrl+F5\"/>"
"      <Spacer Slot.Fill=\"1\"/>"
"      <Button Name=\"island.problems\" Text=\"No problem\" Style=\"Flat\" OnClicked=\"island.problems\" Tooltip=\"The problems of the interface, then those of the last build\"/>"
"    </HBox>"
"    <HBox Spacing=\"6\">"
"      <Button Text=\"Content drawer  (Ctrl+Space)\" Style=\"Flat\" OnClicked=\"drawer.toggle\" Slot.Fill=\"1\"/>"
"      <Button Text=\"Save all\" Style=\"Flat\" OnClicked=\"file.save_all\" Tooltip=\"Ctrl+Alt+S\"/>"
"    </HBox>"
"  </VBox>"
"</Island>";

static struct {
	Rid island, timer;
	int was_running;
	char problems_text[48];
} g_i = { R_INVALID_ID, R_INVALID_ID, 0, "" };

static Rid find(const char* name) {
	return Rwidget_find(g_i.island, name);
}

/* (a text is set only when it changes: no layout for nothing, four times a second) */
static void set_text(const char* name, const char* text) {
	Rid label = find(name);
	const char* now = Rlabel_get_text(label);
	if (!now || strcmp(now, text) != 0) Rlabel_set_text(label, text);
}

static void set_dot(const char* color) {
	Rwidget_set_attribute(find("island.dot"), "Background", color);
	Rwidget_set_attribute(find("island.dot.big"), "Background", color);
}

static void refresh(void) {
	build_summary_t build;
	const char* path = designer_get_path();
	const char* file = path ? ide_basename(path) : NULL;
	int unsaved = designer_is_modified() || editors_any_modified();
	int problems = designer_problem_count();
	double now = Rget_time();
	char title[300], state[96], text[400];

	build_get_summary(&build);

	/* folded: the project and the file */
	if (g_project_dir && file) snprintf(title, sizeof(title), "%s  \xC2\xB7  %s%s", ide_basename(g_project_dir), file, unsaved ? " \xE2\x80\xA2" : "");
	else if (g_project_dir) snprintf(title, sizeof(title), "%s", ide_basename(g_project_dir));
	else if (file) snprintf(title, sizeof(title), "%s%s", file, unsaved ? " \xE2\x80\xA2" : "");
	else snprintf(title, sizeof(title), "Raven IDE");
	set_text("island.title", title);

	/* the state, and its color */
	state[0] = '\0';
	if (build.running && build.program) {
		snprintf(state, sizeof(state), "running");
		set_dot("#0A84FF");
	} else if (build.running) {
		snprintf(state, sizeof(state), "building %.0f s", now - build.started);
		set_dot("#0A84FF");
	} else if (build.has_result && !build.ok) {
		snprintf(state, sizeof(state), "%d error%s", build.errors, build.errors == 1 ? "" : "s");
		set_dot("#FF453A");
	} else if (build.has_result && build.warnings) {
		snprintf(state, sizeof(state), "%d warning%s", build.warnings, build.warnings == 1 ? "" : "s");
		set_dot("#FF9F0A");
	} else if (build.has_result) {
		snprintf(state, sizeof(state), "built");
		set_dot("#30D158");
	} else if (problems) {
		snprintf(state, sizeof(state), "%d problem%s", problems, problems == 1 ? "" : "s");
		set_dot("#FF9F0A");
	} else {
		set_dot("#8E8E93");
	}
	set_text("island.state", state);
	Rwidget_set_visibility(find("island.state"), state[0] ? R_VISIBLE : R_COLLAPSED);

	/* a build just ended: the island says how, for a few seconds */
	if (g_i.was_running && !build.running && build.has_result && !Risland_is_expanded(g_i.island)) Risland_expand_for(g_i.island, 4.0);

	/* unfolded */
	set_text("island.project", g_project_dir ? ide_basename(g_project_dir) : "No project");
	if (g_project_dir) {
		char pretty[600];
		ide_pretty_path(g_project_dir, pretty, sizeof(pretty));
		set_text("island.where", pretty);
	} else {
		set_text("island.where", "Project > Open folder, or New project");
	}
	if (file) snprintf(text, sizeof(text), "%s%s", file, unsaved ? "  \xC2\xB7  unsaved changes" : "  \xC2\xB7  saved");
	else snprintf(text, sizeof(text), unsaved ? "Unsaved changes in the code editors" : "No interface open");
	set_text("island.file", text);

	/* (what follows is only seen unfolded: "12 s ago" does not redraw the window every second for nobody) */
	if (!Risland_is_expanded(g_i.island)) {
		g_i.was_running = build.running;
		return;
	}
	if (build.running && build.program) {
		snprintf(text, sizeof(text), "The program runs (for %.0f s). Stop ends it.", now - build.started);
	} else if (build.running) {
		snprintf(text, sizeof(text), "Building for %.0f s...", now - build.started);
	} else if (build.has_result) {
		double ago = now - build.finished;
		char when[48];
		if (ago < 60.0) snprintf(when, sizeof(when), "%.0f s ago", ago);
		else if (ago < 3600.0) snprintf(when, sizeof(when), "%.0f min ago", ago / 60.0);
		else snprintf(when, sizeof(when), "%.0f h ago", ago / 3600.0);
		snprintf(text, sizeof(text), "%s %s: %d error%s, %d warning%s.", build.ok ? "Built" : "Failed", when,
			build.errors, build.errors == 1 ? "" : "s", build.warnings, build.warnings == 1 ? "" : "s");
	} else {
		snprintf(text, sizeof(text), g_project_dir ? "Not built yet: Build, or Run." : "Open a project to build it.");
	}
	set_text("island.build", text);
	Rwidget_set_enabled(find("island.stop"), build.running);

	if (problems || build.errors || build.warnings) {
		snprintf(text, sizeof(text), "Problems (%d)", problems + build.errors + build.warnings);
	} else {
		snprintf(text, sizeof(text), "No problem");
	}
	if (strcmp(g_i.problems_text, text) != 0) {
		snprintf(g_i.problems_text, sizeof(g_i.problems_text), "%.47s", text);
		Rbutton_set_text(find("island.problems"), text);
	}

	g_i.was_running = build.running;
}

static void on_timer(Rid timer, void* user_data) {
	(void)timer;
	(void)user_data;
	if (g_i.island != R_INVALID_ID && Rwidget_is_valid(g_i.island)) refresh();
}

static void on_changed(Rid island, int expanded, void* user_data) {
	(void)island;
	(void)user_data;
	if (expanded) refresh();
}

static void action_island(Rid sender, const char* action, void* user_data) {
	Rid problems = Rwidget_find(g_ide_window, "problems");
	Rid build_problems = Rwidget_find(g_ide_window, "build.problems");
	build_summary_t build;
	(void)sender;
	(void)user_data;
	(void)action;
	build_get_summary(&build);
	/* the problems of the interface first; else those of the build */
	if (designer_problem_count() || !(build.errors || build.warnings)) Rdock_area_open_panel(g_ide_dock, problems);
	else Rdock_area_open_panel(g_ide_dock, build_problems);
	Risland_set_expanded(g_i.island, R_FALSE);
}

void island_init(void) {
	g_i.island = Rwidget_create_from_xml(ISLAND_XML, sizeof(ISLAND_XML) - 1);
	if (g_i.island == R_INVALID_ID) return;
	Rwindow_set_island(g_ide_window, g_i.island);
	Raction_bind("island.problems", action_island, NULL);
	Risland_set_changed_callback(g_i.island, on_changed, NULL);
	refresh();
	g_i.timer = Rtimer_create(0.25, R_TRUE, on_timer, NULL);
}

void island_refresh(void) {
	if (g_i.island != R_INVALID_ID && Rwidget_is_valid(g_i.island)) refresh();
}

void island_shutdown(void) {
	if (g_i.timer != R_INVALID_ID && Rtimer_is_valid(g_i.timer)) Rtimer_destroy(g_i.timer);
	g_i.timer = R_INVALID_ID;
}
