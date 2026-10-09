/**
 * HUI example - editor
 * A small scene editor. It shows the features an editor needs:
 *
 *   - the whole window is described in editor.xml,
 *   - widgets are found by name and driven through named actions,
 *   - a viewport shows a scene rendered by the application into a texture
 *     (here a software renderer running on the HUI job system),
 *   - a custom widget draws a frame-time graph with the HUI_Draw functions,
 *   - a virtual list shows a log of any length without storing rows in HUI.
 */

#include "../common/example.h"

#include <math.h>
#include <stdarg.h>

#ifndef HUI_EXAMPLE_DIR
#define HUI_EXAMPLE_DIR "."
#endif

/* ---------------------------------------------------------------------------
 *  Scene
 * ------------------------------------------------------------------------- */

#define MAX_OBJECTS 64

typedef struct Object {
	char name[48];
	float x, y;
	float radius;
	float speed;
	float phase;
	int color;
	int visible;
	/* handle of the row in the outliner */
	HUI_id item;
} Object;

static const float g_palette[4][3] = {
	{ 1.00f, 0.55f, 0.15f }, { 0.25f, 0.55f, 1.00f }, { 0.30f, 0.85f, 0.45f }, { 1.00f, 0.40f, 0.70f }
};

static Object g_objects[MAX_OBJECTS];
static int g_object_count;
static int g_selected = -1;
static int g_playing = 1;
static double g_time;
static int g_created;

/* ---------------------------------------------------------------------------
 *  User interface
 * ------------------------------------------------------------------------- */

static struct {
	HUI_id window, outliner, scene, graph, log, stats, play, render_info;
	HUI_id name, radius, speed, color, visible;
	HUI_id texture;
	/* set while the property widgets are filled from code, to ignore their callbacks */
	int updating;
} ui;

/* ---- log: only the strings live here, the list asks for the rows it draws ---- */

#define LOG_CAPACITY 2000
static char g_log[LOG_CAPACITY][96];
static int g_log_count;

static const char* log_row(HUI_id list, int index, void* user_data) {
	(void)list;
	(void)user_data;
	return g_log[index];
}

static void log_add(const char* format, ...) {
	va_list arguments;
	if (g_log_count == LOG_CAPACITY) return;
	va_start(arguments, format);
	vsnprintf(g_log[g_log_count], sizeof(g_log[0]), format, arguments);
	va_end(arguments);
	g_log_count++;
	HUI_SetListVirtualItems(ui.log, g_log_count, log_row, NULL);
	HUI_ScrollToListItem(ui.log, g_log_count - 1);
}

/* ---- properties ---- */

static void properties_refresh(void) {
	const Object* object = g_selected >= 0 ? &g_objects[g_selected] : NULL;
	int enabled = object != NULL;

	ui.updating = 1;
	HUI_SetTextInputText(ui.name, object ? object->name : "");
	HUI_SetSliderValue(ui.radius, object ? object->radius : 0.0f);
	HUI_SetSpinBoxValue(ui.speed, object ? object->speed : 0.0);
	HUI_SetComboBoxSelected(ui.color, object ? object->color : 0);
	HUI_SetCheckboxChecked(ui.visible, object ? object->visible : 0);
	ui.updating = 0;

	HUI_SetWidgetEnabled(ui.name, enabled);
	HUI_SetWidgetEnabled(ui.radius, enabled);
	HUI_SetWidgetEnabled(ui.speed, enabled);
	HUI_SetWidgetEnabled(ui.color, enabled);
	HUI_SetWidgetEnabled(ui.visible, enabled);
}

static int object_of_item(HUI_id item) {
	int i;
	for (i = 0; i < g_object_count; i++) {
		if (g_objects[i].item == item) return i;
	}
	return -1;
}

/* ---- scene edition ---- */

static void scene_add(const char* name) {
	Object* object;
	if (g_object_count == MAX_OBJECTS) {
		log_add("The scene is full (%d objects)", MAX_OBJECTS);
		return;
	}
	object = &g_objects[g_object_count];
	memset(object, 0, sizeof(*object));
	g_created++;
	if (name) snprintf(object->name, sizeof(object->name), "%s", name);
	else snprintf(object->name, sizeof(object->name), "Object %d", g_created);
	object->x = 0.2f + 0.6f * (float)((g_created * 37) % 100) / 100.0f;
	object->y = 0.25f + 0.5f * (float)((g_created * 61) % 100) / 100.0f;
	object->radius = 0.08f + 0.02f * (float)(g_created % 4);
	object->speed = 1.0f + 0.25f * (float)(g_created % 5);
	object->phase = (float)g_created * 1.7f;
	object->color = (g_created - 1) % 4;
	object->visible = 1;
	object->item = HUI_AddTreeItem(ui.outliner, HUI_INVALID_ID, object->name);
	g_selected = g_object_count++;
	HUI_SetTreeSelectedItem(ui.outliner, object->item);
	properties_refresh();
	log_add("Added \"%s\"", object->name);
}

static void scene_delete_selected(void) {
	int i;
	if (g_selected < 0) return;
	log_add("Deleted \"%s\"", g_objects[g_selected].name);
	HUI_DeleteTreeItem(g_objects[g_selected].item);
	for (i = g_selected; i + 1 < g_object_count; i++) g_objects[i] = g_objects[i + 1];
	g_object_count--;
	g_selected = -1;
	properties_refresh();
}

/* ---------------------------------------------------------------------------
 *  Software renderer: fills an RGBA image, one band of rows per job
 * ------------------------------------------------------------------------- */

static struct {
	uint8_t* pixels;
	int width, height;
	/* viewport size in pixels, and the divisor chosen in the combo box */
	int viewport_width, viewport_height;
	double render_ms;
} g_render;

static void render_rows(HUI_uint begin, HUI_uint end, void* user_data) {
	const int width = g_render.width, height = g_render.height;
	const float aspect = (float)width / (float)height;
	HUI_uint row;
	(void)user_data;

	for (row = begin; row < end; row++) {
		uint8_t* out = g_render.pixels + (size_t)row * (size_t)width * 4;
		const float v = ((float)row + 0.5f) / (float)height;
		int column, i;
		for (column = 0; column < width; column++) {
			const float u = ((float)column + 0.5f) / (float)width;
			/* background: a slow gradient with a grid */
			float r = 0.07f + 0.05f * v, g = 0.08f + 0.06f * v, b = 0.11f + 0.10f * v;
			const float grid_u = u * aspect * 10.0f, grid_v = v * 10.0f;
			if (grid_u - floorf(grid_u) < 0.03f || grid_v - floorf(grid_v) < 0.03f) {
				r += 0.04f;
				g += 0.04f;
				b += 0.05f;
			}
			for (i = 0; i < g_object_count; i++) {
				const Object* object = &g_objects[i];
				float dx, dy, distance, shade, edge;
				if (!object->visible) continue;
				dx = (u - object->x) * aspect;
				dy = v - (object->y + 0.06f * sinf((float)g_time * object->speed + object->phase));
				distance = sqrtf(dx * dx + dy * dy);
				if (distance > object->radius + 0.01f) continue;
				/* a shaded disc with a soft edge; the selected one gets a ring */
				shade = 1.0f - 0.55f * (distance / object->radius) * (distance / object->radius);
				edge = (object->radius - distance) * (float)height * 0.5f;
				edge = edge < 0.0f ? 0.0f : (edge > 1.0f ? 1.0f : edge);
				if (i == g_selected && distance > object->radius - 0.012f) shade = 1.6f;
				r += (g_palette[object->color][0] * shade - r) * edge;
				g += (g_palette[object->color][1] * shade - g) * edge;
				b += (g_palette[object->color][2] * shade - b) * edge;
			}
			out[column * 4 + 0] = (uint8_t)(255.0f * (r > 1.0f ? 1.0f : r));
			out[column * 4 + 1] = (uint8_t)(255.0f * (g > 1.0f ? 1.0f : g));
			out[column * 4 + 2] = (uint8_t)(255.0f * (b > 1.0f ? 1.0f : b));
			out[column * 4 + 3] = 255;
		}
	}
}

static void scene_render(void) {
	static const int divisors[3] = { 1, 2, 4 };
	int divisor = divisors[HUI_GetComboBoxSelected(HUI_FindWidget(ui.window, "render.scale")) % 3];
	int width = g_render.viewport_width / divisor, height = g_render.viewport_height / divisor;
	int threaded = HUI_IsCheckboxChecked(HUI_FindWidget(ui.window, "render.threads"));
	double start;
	char info[128];

	if (width < 2 || height < 2) return;
	if (width != g_render.width || height != g_render.height) {
		free(g_render.pixels);
		g_render.pixels = (uint8_t*)malloc((size_t)width * (size_t)height * 4);
		g_render.width = width;
		g_render.height = height;
	}

	start = HUI_GetTime();
	/* bands of 8 rows are shared between the worker threads and this thread */
	if (threaded) HUI_ParallelFor((HUI_uint)height, 8, render_rows, NULL);
	else render_rows(0, (HUI_uint)height, NULL);
	g_render.render_ms = (HUI_GetTime() - start) * 1000.0;

	/* the pixels are copied: the renderer uploads them on its own thread */
	if (ui.texture == HUI_INVALID_ID) {
		ui.texture = HUI_CreateTextureFromPixels(g_render.pixels, width, height);
		HUI_SetViewportTexture(ui.scene, ui.texture);
	} else {
		HUI_UpdateTexture(ui.texture, g_render.pixels, width, height);
	}

	snprintf(info, sizeof(info), "%d x %d pixels in %.2f ms on %d thread%s", width, height, g_render.render_ms,
		threaded ? HUI_GetWorkerThreadCount() + 1 : 1, threaded && HUI_GetWorkerThreadCount() > 0 ? "s" : "");
	HUI_SetLabelText(ui.render_info, info);
}

static void on_scene_resized(HUI_id viewport, int width, int height, void* user_data) {
	(void)viewport;
	(void)user_data;
	g_render.viewport_width = width;
	g_render.viewport_height = height;
}

/* Mouse in the viewport: select the object under the cursor and drag it. */
static int on_scene_event(HUI_id viewport, const HUI_Event* event, void* user_data) {
	static int dragging;
	float x, y, width, height, u, v;
	int i;
	(void)user_data;

	HUI_GetWidgetRect(viewport, &x, &y, &width, &height);
	if (width <= 0.0f || height <= 0.0f) return HUI_FALSE;
	u = event->local_x / width;
	v = event->local_y / height;

	if (event->type == HUI_EVENT_MOUSE_DOWN && event->button == HUI_MOUSE_LEFT) {
		for (i = g_object_count - 1; i >= 0; i--) {
			float dx = (u - g_objects[i].x) * width / height, dy = v - g_objects[i].y;
			if (g_objects[i].visible && dx * dx + dy * dy < g_objects[i].radius * g_objects[i].radius * 1.5f) break;
		}
		g_selected = i;
		HUI_SetTreeSelectedItem(ui.outliner, i >= 0 ? g_objects[i].item : HUI_INVALID_ID);
		properties_refresh();
		dragging = i >= 0;
		return HUI_TRUE;
	}
	if (event->type == HUI_EVENT_MOUSE_MOVE && dragging && g_selected >= 0) {
		g_objects[g_selected].x = u < 0.0f ? 0.0f : (u > 1.0f ? 1.0f : u);
		g_objects[g_selected].y = v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
		return HUI_TRUE;
	}
	if (event->type == HUI_EVENT_MOUSE_UP) dragging = 0;
	return HUI_FALSE;
}

/* ---------------------------------------------------------------------------
 *  Custom widget: frame-time graph
 * ------------------------------------------------------------------------- */

#define GRAPH_SAMPLES 240
static float g_frame_ms[GRAPH_SAMPLES];
static int g_frame_cursor;

static void graph_paint(HUI_id widget, HUI_Painter* painter, float width, float height, void* user_data) {
	float text_r, text_g, text_b, text_a, peak = 20.0f, scale;
	char label[32];
	int i;
	(void)widget;
	(void)user_data;

	/* theme colors keep the widget readable in both themes */
	HUI_GetThemeColor(HUI_COLOR_TEXT, &text_r, &text_g, &text_b, &text_a);
	HUI_DrawRect(painter, 0.0f, 0.0f, width, height, 6.0f, 0.5f, 0.5f, 0.5f, 0.10f);

	for (i = 0; i < GRAPH_SAMPLES; i++) peak = g_frame_ms[i] > peak ? g_frame_ms[i] : peak;
	scale = (height - 24.0f) / peak;

	/* the 16.7 ms line: 60 frames per second */
	HUI_DrawLine(painter, 0.0f, height - 16.7f * scale, width, height - 16.7f * scale, 1.0f, 0.3f, 0.8f, 0.4f, 0.8f);
	HUI_DrawText(painter, HUI_INVALID_ID, 11.0f, 6.0f, height - 16.7f * scale - 16.0f, "16.7 ms", text_r, text_g, text_b, 0.6f);

	for (i = 1; i < GRAPH_SAMPLES; i++) {
		/* oldest sample on the left */
		float previous = g_frame_ms[(g_frame_cursor + i - 1) % GRAPH_SAMPLES];
		float current = g_frame_ms[(g_frame_cursor + i) % GRAPH_SAMPLES];
		float x0 = width * (float)(i - 1) / (float)(GRAPH_SAMPLES - 1);
		float x1 = width * (float)i / (float)(GRAPH_SAMPLES - 1);
		HUI_DrawLine(painter, x0, height - previous * scale, x1, height - current * scale, 1.5f, 0.25f, 0.6f, 1.0f, 1.0f);
	}

	snprintf(label, sizeof(label), "peak %.1f ms", peak);
	HUI_DrawText(painter, HUI_INVALID_ID, 12.0f, 8.0f, 6.0f, label, text_r, text_g, text_b, text_a);
}

/* ---------------------------------------------------------------------------
 *  Actions (named in editor.xml)
 * ------------------------------------------------------------------------- */

static void on_action(HUI_id sender, const char* action, void* user_data) {
	Object* object = g_selected >= 0 ? &g_objects[g_selected] : NULL;
	(void)user_data;

	if (strcmp(action, "app.quit") == 0) {
		HUI_Quit(0);
	} else if (strcmp(action, "scene.new") == 0) {
		HUI_ClearTreeItems(ui.outliner);
		g_object_count = 0;
		g_selected = -1;
		properties_refresh();
		log_add("New scene");
	} else if (strcmp(action, "scene.add") == 0) {
		scene_add(NULL);
	} else if (strcmp(action, "scene.delete") == 0) {
		scene_delete_selected();
	} else if (strcmp(action, "scene.play") == 0) {
		g_playing = !g_playing;
		HUI_SetButtonText(ui.play, g_playing ? "Pause" : "Play");
		/* no animation: the window is only redrawn when something changes */
		HUI_SetViewportRealtime(ui.scene, g_playing);
		log_add(g_playing ? "Animation started" : "Animation paused");
	} else if (strcmp(action, "view.dark") == 0 || strcmp(action, "view.light") == 0) {
		int dark = strcmp(action, "view.dark") == 0;
		HUI_ApplyTheme(dark ? HUI_THEME_DARK : HUI_THEME_LIGHT);
		HUI_SetMenuItemChecked(HUI_FindWidget(ui.window, "menu.dark"), dark);
		HUI_SetMenuItemChecked(HUI_FindWidget(ui.window, "menu.light"), !dark);
	} else if (strcmp(action, "view.compact") == 0) {
		HUI_SetThemeMetric(HUI_METRIC_FONT_SIZE, 12.0f);
		HUI_SetThemeMetric(HUI_METRIC_CONTROL_HEIGHT, 22.0f);
		HUI_SetThemeMetric(HUI_METRIC_ROW_HEIGHT, 20.0f);
	} else if (strcmp(action, "view.comfortable") == 0) {
		HUI_SetThemeMetric(HUI_METRIC_FONT_SIZE, 15.0f);
		HUI_SetThemeMetric(HUI_METRIC_CONTROL_HEIGHT, 32.0f);
		HUI_SetThemeMetric(HUI_METRIC_ROW_HEIGHT, 28.0f);
	} else if (object && !ui.updating) {
		/* the property widgets edit the selected object */
		if (strcmp(action, "prop.radius") == 0) {
			object->radius = HUI_GetSliderValue(sender);
		} else if (strcmp(action, "prop.speed") == 0) {
			object->speed = (float)HUI_GetSpinBoxValue(sender);
		} else if (strcmp(action, "prop.color") == 0) {
			object->color = HUI_GetComboBoxSelected(sender) & 3;
		} else if (strcmp(action, "prop.visible") == 0) {
			object->visible = HUI_IsCheckboxChecked(sender);
		}
	}
	/* a paused scene is rendered again only when it was edited */
	if (!g_playing) HUI_InvalidateWidget(ui.scene);
}

/* Every keystroke in the name field renames the selected object. */
static void on_name_changed(HUI_id input, const char* text, void* user_data) {
	(void)input;
	(void)user_data;
	if (g_selected < 0 || ui.updating) return;
	snprintf(g_objects[g_selected].name, sizeof(g_objects[g_selected].name), "%s", text);
	HUI_SetTreeItemText(g_objects[g_selected].item, text);
}

/* A row of the outliner was selected, with the mouse or the keyboard. */
static void on_outliner_selection(HUI_id tree, HUI_id item, void* user_data) {
	(void)tree;
	(void)user_data;
	g_selected = object_of_item(item);
	properties_refresh();
	if (!g_playing) HUI_InvalidateWidget(ui.scene);
}

/* ---------------------------------------------------------------------------
 *  Frame
 * ------------------------------------------------------------------------- */

/* Called once per HUI_Update, before the layout. */
static void on_frame(double delta_seconds, void* user_data) {
	static double stats_timer;
	(void)user_data;
	if (!HUI_IsValidWindow(ui.window)) return;

	if (g_playing) g_time += delta_seconds;
	scene_render();

	g_frame_ms[g_frame_cursor] = (float)(delta_seconds * 1000.0);
	g_frame_cursor = (g_frame_cursor + 1) % GRAPH_SAMPLES;
	HUI_InvalidateWidget(ui.graph);

	stats_timer += delta_seconds;
	if (stats_timer > 0.25) {
		HUI_FrameStats stats;
		char text[160];
		stats_timer = 0.0;
		HUI_GetFrameStats(ui.window, &stats);
		snprintf(text, sizeof(text), "%u widgets   %u primitives in %u draw call%s   layout %.2f ms   paint %.2f ms   gpu %.2f ms",
			stats.widget_count, stats.instance_count, stats.draw_call_count, stats.draw_call_count > 1 ? "s" : "",
			stats.layout_ms, stats.paint_ms, stats.render_ms);
		HUI_SetLabelText(ui.stats, text);
	}
}

int main(void) {
	if (!example_init("editor")) return 1;

	ui.texture = HUI_INVALID_ID;
	ui.window = HUI_CreateWindowFromXmlFile(HUI_EXAMPLE_DIR "/editor.xml");
	if (ui.window == HUI_INVALID_ID) {
		HUI_Shutdown();
		return 1;
	}

	/* widgets are found by the Name given in the XML */
	ui.outliner = HUI_FindWidget(ui.window, "outliner");
	ui.scene = HUI_FindWidget(ui.window, "scene");
	ui.graph = HUI_FindWidget(ui.window, "graph");
	ui.log = HUI_FindWidget(ui.window, "log");
	ui.stats = HUI_FindWidget(ui.window, "stats");
	ui.play = HUI_FindWidget(ui.window, "play");
	ui.render_info = HUI_FindWidget(ui.window, "render.info");
	ui.name = HUI_FindWidget(ui.window, "prop.name");
	ui.radius = HUI_FindWidget(ui.window, "prop.radius");
	ui.speed = HUI_FindWidget(ui.window, "prop.speed");
	ui.color = HUI_FindWidget(ui.window, "prop.color");
	ui.visible = HUI_FindWidget(ui.window, "prop.visible");

	/* one handler receives every action that has no function of its own */
	HUI_SetDefaultActionHandler(on_action, NULL);

	/* events without an action name are plain callbacks */
	HUI_SetTreeSelectionChangedCallback(ui.outliner, on_outliner_selection, NULL);
	HUI_SetTextInputChangedCallback(ui.name, on_name_changed, NULL);
	HUI_SetCustomPaintCallback(ui.graph, graph_paint, NULL);
	HUI_SetViewportResizeCallback(ui.scene, on_scene_resized, NULL);
	HUI_SetWidgetEventCallback(ui.scene, on_scene_event, NULL);
	HUI_SetFrameCallback(on_frame, NULL);

	log_add("HUI %s, %d worker thread(s)", HUI_GetVersion(), HUI_GetWorkerThreadCount());
	scene_add("Sun");
	scene_add("Planet");
	scene_add("Moon");

	example_run(ui.window);

	free(g_render.pixels);
	HUI_Shutdown();
	return 0;
}
