/**
 * Raven phase 0 spike: a rendering backend on sokol_gfx.
 *
 * Standalone (public headers only): registered with Rbackend_register and
 * selected with R_BACKEND_CUSTOM. GLES 3.0 through EGL "surfaceless", so it
 * runs headless on Mesa llvmpipe; every window renders into an offscreen
 * image that Rwindow_capture reads back.
 *
 * What it proves (see README.md):
 *   - the instance layout and the UI shaders work unchanged through sokol_gfx
 *   - textures, atlas updates, the erase pipeline and a glass frame
 *   - the pass structure of the glass backdrop on sokol's pass model
 */

#define SOKOL_IMPL
#define SOKOL_GLES3
#include "../../third_party/sokol/sokol_gfx.h"

#include <Raven.h>
#include <raven/raven_backend.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "shaders/shader_sources.h"
#include "shaders/effect_sources.h"

#define SPK_MAX_TEXTURES 4096
#define SPK_MAX_SURFACES 64
#define SPK_BLUR_LEVELS 3
#define SPK_MAX_EFFECTS 256

/* ---------------------------------------------------------------------------
 *  State
 * ------------------------------------------------------------------------- */

typedef struct spk_texture_t {
	int used;
	int width, height;
	RTextureFormat format;
	RFilterType filter;
	sg_image image;
	sg_view view;
} spk_texture_t;

typedef struct spk_target_t {
	sg_image image;
	sg_view color;   /* render into it */
	sg_view sample;  /* sample it */
	int width, height;
} spk_target_t;

typedef struct spk_surface_t {
	int used;
	Rid window;
	int width, height;
	int transparent;
	spk_target_t frame;
	/* glass (created the first time a frame contains a backdrop command) */
	int glass_ready;
	int glass_filled;
	spk_target_t scene;
	spk_target_t down[SPK_BLUR_LEVELS];
	spk_target_t up[SPK_BLUR_LEVELS - 1];
} spk_surface_t;

typedef struct spk_upload_t {
	sg_image image;
	int x, y, width, height;
	uint8_t* pixels; /* RGBA8, owned */
} spk_upload_t;

typedef struct spk_effect_t {
	int used;
	sg_shader shader;
	sg_pipeline pipeline;
} spk_effect_t;

static struct {
	int initialized;
	EGLDisplay display;
	EGLContext context;

	sg_shader shader;
	sg_pipeline pipeline;       /* UI: (ONE, 1-SRC_A) */
	sg_pipeline erase_pipeline; /* (ZERO, 1-SRC_A) */
	sg_shader blur_shader;
	sg_pipeline blur_pipeline;  /* full-screen triangle, no blending: reduce / enlarge / copy */
	sg_sampler linear, nearest;
	spk_texture_t white;

	spk_texture_t textures[SPK_MAX_TEXTURES];
	spk_surface_t surfaces[SPK_MAX_SURFACES];
	spk_effect_t effects[SPK_MAX_EFFECTS];

	sg_buffer instances;
	size_t instances_size;

	sg_buffer staging;
	size_t staging_size;
	spk_upload_t* uploads;
	size_t upload_count, upload_cap;
} g;

static void spk_log(const char* tag, uint32_t level, uint32_t id, const char* message, uint32_t line, const char* file, void* user) {
	(void)tag; (void)id; (void)user;
	if (level <= 2) fprintf(stderr, "sokol[%u] %s (%s:%u)\n", level, message ? message : "?", file, line);
}

/* ---------------------------------------------------------------------------
 *  EGL context (headless)
 * ------------------------------------------------------------------------- */

static int spk_context_create(void) {
	static const EGLint config_attribs[] = {
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT, EGL_SURFACE_TYPE, 0,
		EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8, EGL_ALPHA_SIZE, 8, EGL_NONE
	};
	static const EGLint context_attribs[] = { EGL_CONTEXT_MAJOR_VERSION, 3, EGL_CONTEXT_MINOR_VERSION, 0, EGL_NONE };
	PFNEGLGETPLATFORMDISPLAYEXTPROC get_display = (PFNEGLGETPLATFORMDISPLAYEXTPROC)eglGetProcAddress("eglGetPlatformDisplayEXT");
	EGLConfig config;
	EGLint count = 0, major, minor;

	g.display = get_display ? get_display(EGL_PLATFORM_SURFACELESS_MESA, EGL_DEFAULT_DISPLAY, NULL) : EGL_NO_DISPLAY;
	if (g.display == EGL_NO_DISPLAY) g.display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	if (g.display == EGL_NO_DISPLAY || !eglInitialize(g.display, &major, &minor)) return 0;
	eglBindAPI(EGL_OPENGL_ES_API);
	if (!eglChooseConfig(g.display, config_attribs, &config, 1, &count) || count == 0) return 0;
	g.context = eglCreateContext(g.display, config, EGL_NO_CONTEXT, context_attribs);
	if (g.context == EGL_NO_CONTEXT) return 0;
	return eglMakeCurrent(g.display, EGL_NO_SURFACE, EGL_NO_SURFACE, g.context) == EGL_TRUE;
}

/* ---------------------------------------------------------------------------
 *  Uploads: sokol_gfx writes into a transient staging buffer, then copies
 *  (outside a pass). All writes of a frame must come before the first copy,
 *  so the uploads are queued and flushed together at the start of a frame.
 * ------------------------------------------------------------------------- */

static void spk_queue_upload(sg_image image, int x, int y, int width, int height, const uint8_t* rgba) {
	spk_upload_t* upload;
	size_t size = (size_t)width * height * 4;
	if (g.upload_count == g.upload_cap) {
		g.upload_cap = g.upload_cap ? g.upload_cap * 2 : 16;
		g.uploads = (spk_upload_t*)realloc(g.uploads, g.upload_cap * sizeof(spk_upload_t));
	}
	upload = &g.uploads[g.upload_count++];
	upload->image = image;
	upload->x = x; upload->y = y; upload->width = width; upload->height = height;
	upload->pixels = (uint8_t*)malloc(size);
	memcpy(upload->pixels, rgba, size);
}

static void spk_flush_uploads(void) {
	size_t i, total = 0;
	if (g.upload_count == 0) return;
	for (i = 0; i < g.upload_count; i++) total += (((size_t)g.uploads[i].width * g.uploads[i].height * 4) + 255) & ~(size_t)255;
	if (total > g.staging_size) {
		size_t size = g.staging_size ? g.staging_size : (size_t)1 << 20;
		while (size < total) size *= 2;
		if (g.staging.id) sg_destroy_buffer(g.staging);
		sg_buffer_desc desc;
		memset(&desc, 0, sizeof(desc));
		desc.size = size;
		desc.usage.staging_buffer = true;
		desc.usage.write_transient = true;
		desc.usage.copy_src = true;
		desc.label = "raven-staging";
		g.staging = sg_make_buffer(&desc);
		g.staging_size = size;
	}
	{
		size_t offset = 0;
		for (i = 0; i < g.upload_count; i++) {
			spk_upload_t* u = &g.uploads[i];
			size_t size = (size_t)u->width * u->height * 4;
			sg_write_buffer_desc write;
			memset(&write, 0, sizeof(write));
			write.src.data.ptr = u->pixels;
			write.src.data.size = size;
			write.dst.buffer = g.staging;
			write.dst.offset = offset;
			sg_write_buffer_transient(&write);
			offset += (size + 255) & ~(size_t)255;
		}
	}
	{
		size_t offset = 0;
		for (i = 0; i < g.upload_count; i++) {
			spk_upload_t* u = &g.uploads[i];
			size_t size = (size_t)u->width * u->height * 4;
			sg_copy_buffer_to_image_desc copy;
			memset(&copy, 0, sizeof(copy));
			copy.src.buffer = g.staging;
			copy.src.offset = offset;
			copy.src.bytes_per_row = u->width * 4;
			copy.src.bytes_per_slice = u->width * 4 * u->height;
			copy.dst.image = u->image;
			copy.dst.x = u->x;
			copy.dst.y = u->y;
			copy.size.width = u->width;
			copy.size.height = u->height;
			sg_copy_buffer_to_image(&copy);
			offset += (size + 255) & ~(size_t)255;
			free(u->pixels);
		}
	}
	g.upload_count = 0;
}

/* ---------------------------------------------------------------------------
 *  Textures
 * ------------------------------------------------------------------------- */

static int spk_image_create(spk_texture_t* t, int width, int height, const uint8_t* rgba) {
	sg_image_desc image;
	sg_view_desc view;
	memset(&image, 0, sizeof(image));
	image.type = SG_IMAGETYPE_2D;
	image.width = width;
	image.height = height;
	image.pixel_format = SG_PIXELFORMAT_RGBA8;
	image.usage.copy_dst = true;
	t->image = sg_make_image(&image);
	if (sg_query_image_state(t->image) != SG_RESOURCESTATE_VALID) return 0;
	memset(&view, 0, sizeof(view));
	view.texture.image = t->image;
	t->view = sg_make_view(&view);
	t->width = width;
	t->height = height;
	if (rgba) spk_queue_upload(t->image, 0, 0, width, height, rgba);
	return 1;
}

/* ALPHA8 (the glyph atlas) is sampled as (1, 1, 1, a) by Raven: expand it, no shader change needed. */
static uint8_t* spk_expand_alpha(const uint8_t* alpha, size_t count) {
	uint8_t* out = (uint8_t*)malloc(count * 4);
	size_t i;
	for (i = 0; i < count; i++) {
		out[i * 4 + 0] = out[i * 4 + 1] = out[i * 4 + 2] = 255;
		out[i * 4 + 3] = alpha[i];
	}
	return out;
}

static int spk_create_texture(Rid texture, const RTextureDesc* desc, const void* pixels) {
	spk_texture_t* t;
	uint32_t index = R_ID_INDEX(texture);
	uint8_t* expanded = NULL;
	int ok;
	if (index >= SPK_MAX_TEXTURES) return 0;
	t = &g.textures[index];
	memset(t, 0, sizeof(*t));
	t->used = 1;
	t->format = desc->format;
	t->filter = desc->filter;
	if (pixels && desc->format == R_TEXTURE_ALPHA8) pixels = expanded = spk_expand_alpha((const uint8_t*)pixels, (size_t)desc->width * desc->height);
	ok = spk_image_create(t, (int)desc->width, (int)desc->height, (const uint8_t*)pixels);
	free(expanded);
	if (!ok) t->used = 0;
	return ok;
}

static void spk_update_texture(Rid texture, Ruint x, Ruint y, Ruint width, Ruint height, const void* pixels) {
	spk_texture_t* t = &g.textures[R_ID_INDEX(texture)];
	uint8_t* expanded = NULL;
	if (R_ID_INDEX(texture) >= SPK_MAX_TEXTURES || !t->used) return;
	if (t->format == R_TEXTURE_ALPHA8) pixels = expanded = spk_expand_alpha((const uint8_t*)pixels, (size_t)width * height);
	spk_queue_upload(t->image, (int)x, (int)y, (int)width, (int)height, (const uint8_t*)pixels);
	free(expanded);
}

static void spk_set_texture_filter(Rid texture, RFilterType filter) {
	if (R_ID_INDEX(texture) < SPK_MAX_TEXTURES) g.textures[R_ID_INDEX(texture)].filter = filter;
}

static void spk_destroy_texture(Rid texture) {
	spk_texture_t* t;
	if (R_ID_INDEX(texture) >= SPK_MAX_TEXTURES) return;
	t = &g.textures[R_ID_INDEX(texture)];
	if (!t->used) return;
	/* uploads still queued for it must not run */
	{
		size_t i;
		for (i = 0; i < g.upload_count; i++) if (g.uploads[i].image.id == t->image.id) g.uploads[i].width = g.uploads[i].height = 0;
	}
	sg_destroy_view(t->view);
	sg_destroy_image(t->image);
	t->used = 0;
}

/* ---------------------------------------------------------------------------
 *  Render targets, shader, pipelines
 * ------------------------------------------------------------------------- */

static void spk_target_destroy(spk_target_t* t) {
	if (!t->image.id) return;
	sg_destroy_view(t->color);
	sg_destroy_view(t->sample);
	sg_destroy_image(t->image);
	memset(t, 0, sizeof(*t));
}

static int spk_target_create(spk_target_t* t, int width, int height, const char* label) {
	sg_image_desc image;
	sg_view_desc view;
	memset(t, 0, sizeof(*t));
	memset(&image, 0, sizeof(image));
	image.type = SG_IMAGETYPE_2D;
	image.width = width;
	image.height = height;
	image.pixel_format = SG_PIXELFORMAT_RGBA8;
	image.usage.color_attachment = true;
	image.label = label;
	t->image = sg_make_image(&image);
	if (sg_query_image_state(t->image) != SG_RESOURCESTATE_VALID) return 0;
	memset(&view, 0, sizeof(view));
	view.color_attachment.image = t->image;
	t->color = sg_make_view(&view);
	memset(&view, 0, sizeof(view));
	view.texture.image = t->image;
	t->sample = sg_make_view(&view);
	t->width = width;
	t->height = height;
	return 1;
}

/* The UI vertex shader with a fragment shader: the UI itself, or an effect (same inputs and bindings). */
static sg_shader spk_make_ui_shader(const char* fragment, const char* const sampler_names[3], const char* label) {
	sg_shader_desc shader;
	static const char* names[8] = { "in_rect", "in_uv", "in_clip", "in_radii", "in_color", "in_border_color", "in_border_width", "in_flags" };
	int i;
	memset(&shader, 0, sizeof(shader));
	shader.vertex_func.source = spk_ui_vert_es300;
	shader.fragment_func.source = fragment;
	for (i = 0; i < 8; i++) {
		shader.attrs[i].glsl_name = names[i];
		shader.attrs[i].base_type = i == 7 ? SG_SHADERATTRBASETYPE_UINT : SG_SHADERATTRBASETYPE_FLOAT;
	}
	shader.uniform_blocks[0].stage = SG_SHADERSTAGE_VERTEX;
	shader.uniform_blocks[0].size = 32;
	shader.uniform_blocks[0].layout = SG_UNIFORMLAYOUT_STD140;
	shader.uniform_blocks[0].glsl_uniforms[0].type = SG_UNIFORMTYPE_FLOAT4;
	shader.uniform_blocks[0].glsl_uniforms[0].array_count = 2;
	shader.uniform_blocks[0].glsl_uniforms[0].glsl_name = "PushConstants";
	for (i = 0; i < 3; i++) {
		shader.views[i].texture.stage = SG_SHADERSTAGE_FRAGMENT;
		shader.views[i].texture.image_type = SG_IMAGETYPE_2D;
		shader.views[i].texture.sample_type = SG_IMAGESAMPLETYPE_FLOAT;
		shader.texture_sampler_pairs[i].stage = SG_SHADERSTAGE_FRAGMENT;
		shader.texture_sampler_pairs[i].view_slot = (uint8_t)i;
		shader.texture_sampler_pairs[i].sampler_slot = i == 0 ? 0 : 1;
		shader.texture_sampler_pairs[i].glsl_name = sampler_names[i];
	}
	for (i = 0; i < 2; i++) {
		shader.samplers[i].stage = SG_SHADERSTAGE_FRAGMENT;
		shader.samplers[i].sampler_type = SG_SAMPLERTYPE_FILTERING;
	}
	shader.label = label;
	return sg_make_shader(&shader);
}

static sg_pipeline spk_make_ui_pipeline(sg_shader shader, int erase, const char* label) {
	sg_pipeline_desc pipeline;
	memset(&pipeline, 0, sizeof(pipeline));
	pipeline.shader = shader;
	pipeline.primitive_type = SG_PRIMITIVETYPE_TRIANGLE_STRIP;
	pipeline.layout.buffers[0].stride = (int)sizeof(RDrawInstance);
	pipeline.layout.buffers[0].step_func = SG_VERTEXSTEP_PER_INSTANCE;
	pipeline.layout.attrs[0] = (sg_vertex_attr_state){ 0, (int)offsetof(RDrawInstance, rect), SG_VERTEXFORMAT_FLOAT4 };
	pipeline.layout.attrs[1] = (sg_vertex_attr_state){ 0, (int)offsetof(RDrawInstance, uv), SG_VERTEXFORMAT_FLOAT4 };
	pipeline.layout.attrs[2] = (sg_vertex_attr_state){ 0, (int)offsetof(RDrawInstance, clip), SG_VERTEXFORMAT_FLOAT4 };
	pipeline.layout.attrs[3] = (sg_vertex_attr_state){ 0, (int)offsetof(RDrawInstance, radii), SG_VERTEXFORMAT_FLOAT4 };
	pipeline.layout.attrs[4] = (sg_vertex_attr_state){ 0, (int)offsetof(RDrawInstance, color), SG_VERTEXFORMAT_UBYTE4N };
	pipeline.layout.attrs[5] = (sg_vertex_attr_state){ 0, (int)offsetof(RDrawInstance, border_color), SG_VERTEXFORMAT_UBYTE4N };
	pipeline.layout.attrs[6] = (sg_vertex_attr_state){ 0, (int)offsetof(RDrawInstance, border_width), SG_VERTEXFORMAT_FLOAT };
	pipeline.layout.attrs[7] = (sg_vertex_attr_state){ 0, (int)offsetof(RDrawInstance, flags), SG_VERTEXFORMAT_UINT };
	pipeline.color_count = 1;
	pipeline.colors[0].pixel_format = SG_PIXELFORMAT_RGBA8;
	pipeline.colors[0].blend.enabled = true;
	pipeline.colors[0].blend.src_factor_rgb = erase ? SG_BLENDFACTOR_ZERO : SG_BLENDFACTOR_ONE;
	pipeline.colors[0].blend.dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
	pipeline.colors[0].blend.src_factor_alpha = erase ? SG_BLENDFACTOR_ZERO : SG_BLENDFACTOR_ONE;
	pipeline.colors[0].blend.dst_factor_alpha = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
	pipeline.depth.pixel_format = SG_PIXELFORMAT_NONE;
	pipeline.label = label;
	return sg_make_pipeline(&pipeline);
}

static int spk_pipelines_create(void) {
	sg_shader_desc shader;
	sg_pipeline_desc pipeline;
	static const char* const ui_samplers[3] = { "u_texture", "u_backdrop", "u_backdrop_blur" };
	g.shader = spk_make_ui_shader(spk_ui_frag_es300, ui_samplers, "raven-ui");
	if (sg_query_shader_state(g.shader) != SG_RESOURCESTATE_VALID) return 0;
	g.pipeline = spk_make_ui_pipeline(g.shader, 0, "raven-ui");
	g.erase_pipeline = spk_make_ui_pipeline(g.shader, 1, "raven-erase");
	if (sg_query_pipeline_state(g.pipeline) != SG_RESOURCESTATE_VALID || sg_query_pipeline_state(g.erase_pipeline) != SG_RESOURCESTATE_VALID) return 0;

	/* the backdrop filter (blur.frag): one fragment uniform vec4 (xy = step, z = mode), one texture */
	memset(&shader, 0, sizeof(shader));
	shader.vertex_func.source = spk_blur_vert_es300;
	shader.fragment_func.source = spk_blur_frag_es300;
	shader.uniform_blocks[0].stage = SG_SHADERSTAGE_FRAGMENT;
	shader.uniform_blocks[0].size = 16;
	shader.uniform_blocks[0].layout = SG_UNIFORMLAYOUT_STD140;
	shader.uniform_blocks[0].glsl_uniforms[0].type = SG_UNIFORMTYPE_FLOAT4;
	shader.uniform_blocks[0].glsl_uniforms[0].array_count = 1;
	shader.uniform_blocks[0].glsl_uniforms[0].glsl_name = "PushConstants";
	shader.views[0].texture.stage = SG_SHADERSTAGE_FRAGMENT;
	shader.views[0].texture.image_type = SG_IMAGETYPE_2D;
	shader.views[0].texture.sample_type = SG_IMAGESAMPLETYPE_FLOAT;
	shader.samplers[0].stage = SG_SHADERSTAGE_FRAGMENT;
	shader.samplers[0].sampler_type = SG_SAMPLERTYPE_FILTERING;
	shader.texture_sampler_pairs[0].stage = SG_SHADERSTAGE_FRAGMENT;
	shader.texture_sampler_pairs[0].view_slot = 0;
	shader.texture_sampler_pairs[0].sampler_slot = 0;
	shader.texture_sampler_pairs[0].glsl_name = "u_source";
	shader.label = "raven-blur";
	g.blur_shader = sg_make_shader(&shader);
	if (sg_query_shader_state(g.blur_shader) != SG_RESOURCESTATE_VALID) return 0;
	memset(&pipeline, 0, sizeof(pipeline));
	pipeline.shader = g.blur_shader;
	pipeline.primitive_type = SG_PRIMITIVETYPE_TRIANGLES;
	pipeline.color_count = 1;
	pipeline.colors[0].pixel_format = SG_PIXELFORMAT_RGBA8;
	pipeline.depth.pixel_format = SG_PIXELFORMAT_NONE;
	pipeline.label = "raven-blur";
	g.blur_pipeline = sg_make_pipeline(&pipeline);
	return sg_query_pipeline_state(g.blur_pipeline) == SG_RESOURCESTATE_VALID;
}

/* ---------------------------------------------------------------------------
 *  Backend: init / surfaces
 * ------------------------------------------------------------------------- */

static int spk_init(const RBackendInitInfo* info) {
	sg_desc desc;
	sg_sampler_desc sampler;
	static const uint8_t white[4] = { 255, 255, 255, 255 };
	(void)info;
	if (!spk_context_create()) {
		fprintf(stderr, "spike: cannot create a headless GLES3 context through EGL\n");
		return 0;
	}
	memset(&desc, 0, sizeof(desc));
	desc.logger.func = spk_log;
	desc.environment.defaults.color_format = SG_PIXELFORMAT_RGBA8;
	desc.environment.defaults.depth_format = SG_PIXELFORMAT_NONE;
	desc.environment.defaults.sample_count = 1;
	sg_setup(&desc);
	if (!sg_isvalid()) return 0;

	memset(&sampler, 0, sizeof(sampler));
	sampler.min_filter = sampler.mag_filter = SG_FILTER_LINEAR;
	sampler.wrap_u = sampler.wrap_v = sampler.wrap_w = SG_WRAP_CLAMP_TO_EDGE;
	g.linear = sg_make_sampler(&sampler);
	sampler.min_filter = sampler.mag_filter = SG_FILTER_NEAREST;
	g.nearest = sg_make_sampler(&sampler);
	if (!spk_pipelines_create()) return 0;
	g.white.used = 1;
	if (!spk_image_create(&g.white, 1, 1, white)) return 0;
	g.initialized = 1;
	return 1;
}

static void spk_surface_release_glass(spk_surface_t* s) {
	int i;
	spk_target_destroy(&s->scene);
	for (i = 0; i < SPK_BLUR_LEVELS; i++) spk_target_destroy(&s->down[i]);
	for (i = 0; i < SPK_BLUR_LEVELS - 1; i++) spk_target_destroy(&s->up[i]);
	s->glass_ready = s->glass_filled = 0;
}

static void spk_surface_release(spk_surface_t* s) {
	spk_surface_release_glass(s);
	spk_target_destroy(&s->frame);
}

static int spk_create_surface(Rid window, const RNativeWindow* native, Ruint width, Ruint height) {
	spk_surface_t* s;
	if (R_ID_INDEX(window) >= SPK_MAX_SURFACES) return 0;
	s = &g.surfaces[R_ID_INDEX(window)];
	memset(s, 0, sizeof(*s));
	s->used = 1;
	s->window = window;
	s->transparent = native && native->transparent;
	s->width = (int)(width ? width : 1);
	s->height = (int)(height ? height : 1);
	return spk_target_create(&s->frame, s->width, s->height, "raven-frame");
}

static void spk_destroy_surface(Rid window) {
	spk_surface_t* s = &g.surfaces[R_ID_INDEX(window)];
	if (R_ID_INDEX(window) >= SPK_MAX_SURFACES || !s->used) return;
	spk_surface_release(s);
	s->used = 0;
}

static void spk_shutdown(void) {
	size_t i;
	for (i = 0; i < SPK_MAX_SURFACES; i++) if (g.surfaces[i].used) spk_surface_release(&g.surfaces[i]);
	free(g.uploads);
	if (sg_isvalid()) sg_shutdown();
	if (g.display != EGL_NO_DISPLAY) {
		eglMakeCurrent(g.display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
		eglDestroyContext(g.display, g.context);
		eglTerminate(g.display);
	}
	memset(&g, 0, sizeof(g));
}

static void spk_wait_idle(void) {
	glFinish();
}

/* ---------------------------------------------------------------------------
 *  Frame
 * ------------------------------------------------------------------------- */

static uint8_t* spk_read_target(const spk_target_t* t, int transparent) {
	sg_gl_image_info info = sg_gl_query_image_info(t->image);
	GLuint fbo = 0;
	size_t count = (size_t)t->width * t->height, i;
	uint8_t* pixels = Rpixels_alloc(count * 4);
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, info.tex[info.active_slot], 0);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glReadPixels(0, 0, t->width, t->height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDeleteFramebuffers(1, &fbo);
	/* sokol caches GL state: tell it the framebuffer binding changed behind its back */
	sg_reset_state_cache();
	/*
	 * Raven draws in "Vulkan convention" (pixel row 0 = top, NDC y down). On GL that makes
	 * framebuffer row 0 the top row of the UI, so glReadPixels already returns it top-down.
	 */
	if (!transparent) for (i = 0; i < count; i++) pixels[i * 4 + 3] = 255;
	return pixels;
}

static void spk_apply_ui_bindings(const spk_surface_t* s, const RDrawData* data, const RDrawCommand* command, const spk_texture_t* texture,
	sg_view backdrop_sharp, sg_view backdrop_blur) {
	sg_bindings bindings;
	memset(&bindings, 0, sizeof(bindings));
	bindings.vertex_buffers[0] = g.instances;
	bindings.vertex_buffer_offsets[0] = (int)(command->first_instance * sizeof(RDrawInstance));
	bindings.views[0] = texture->view;
	bindings.views[1] = backdrop_sharp;
	bindings.views[2] = backdrop_blur;
	bindings.samplers[0] = texture->filter == R_FILTER_NEAREST ? g.nearest : g.linear;
	bindings.samplers[1] = g.linear;
	sg_apply_bindings(&bindings);
	(void)s; (void)data;
}

/*
 * Raven draws in "Vulkan convention": pixel row 0 is the top of the UI and NDC y points down. On GL
 * that stores the UI top in framebuffer row 0, so images and read-backs need no flip, but the
 * viewport/scissor `origin_top_left` argument of sokol must then be what the backend reports natively
 * (false on GL, true on Metal / D3D11 / WebGPU): rectangles are given in memory-row space.
 */
static bool spk_origin(void) {
	return sg_query_features().origin_top_left;
}

static void spk_apply_uniforms_ui(const spk_surface_t* s, const RDrawData* data) {
	float uniforms[8];
	sg_range range;
	uniforms[0] = 2.0f / (float)s->width;
	uniforms[1] = 2.0f / (float)s->height;
	uniforms[2] = -1.0f;
	uniforms[3] = -1.0f;
	uniforms[4] = data->pointer[0];
	uniforms[5] = data->pointer[1];
	uniforms[6] = uniforms[7] = 0.0f;
	range.ptr = uniforms;
	range.size = sizeof(uniforms);
	sg_apply_uniforms(0, &range);
}

/* ---- glass: scene, backdrop capture (dual filter), present (port of vk_glass.c) ---- */

static int spk_glass_prepare(spk_surface_t* s) {
	int i;
	if (s->glass_ready) return 1;
	if (!spk_target_create(&s->scene, s->width, s->height, "raven-scene")) goto failed;
	for (i = 0; i < SPK_BLUR_LEVELS; i++) {
		int w = s->width >> (i + 1), h = s->height >> (i + 1);
		if (w < 1) w = 1;
		if (h < 1) h = 1;
		if (!spk_target_create(&s->down[i], w, h, "raven-down")) goto failed;
		if (i + 1 < SPK_BLUR_LEVELS && !spk_target_create(&s->up[i], w, h, "raven-up")) goto failed;
	}
	s->glass_ready = 1;
	s->glass_filled = 0;
	return 1;
failed:
	spk_surface_release_glass(s);
	return 0;
}

/* Filters `source` into `target`: all of it, or only `area` (pixels of the target), the rest keeping its content. */
static void spk_glass_filter(const spk_target_t* source, const spk_target_t* target, int mode, float spread, const int* area) {
	sg_pass pass;
	sg_bindings bindings;
	float params[4];
	sg_range range;
	memset(&pass, 0, sizeof(pass));
	pass.action.colors[0].load_action = area ? SG_LOADACTION_LOAD : SG_LOADACTION_DONTCARE;
	pass.action.colors[0].store_action = SG_STOREACTION_STORE;
	pass.attachments.colors[0] = target->color;
	sg_begin_pass(&pass);
	sg_apply_viewport(0, 0, target->width, target->height, spk_origin());
	if (area) sg_apply_scissor_rect(area[0], area[1], area[2], area[3], spk_origin());
	sg_apply_pipeline(g.blur_pipeline);
	memset(&bindings, 0, sizeof(bindings));
	bindings.views[0] = source->sample;
	bindings.samplers[0] = g.linear;
	sg_apply_bindings(&bindings);
	/* offsets are in texels of the source: a whole one when reducing, halves when enlarging */
	params[0] = spread * (mode == 0 ? 1.0f : 0.5f) / (float)source->width;
	params[1] = spread * (mode == 0 ? 1.0f : 0.5f) / (float)source->height;
	params[2] = (float)mode;
	params[3] = 0.0f;
	range.ptr = params;
	range.size = sizeof(params);
	sg_apply_uniforms(0, &range);
	sg_draw(0, 3, 1);
	sg_end_pass();
}

static int spk_glass_area(const spk_surface_t* s, const spk_target_t* target, float x0, float y0, float x1, float y1, int area[4]) {
	float fx = (float)target->width / (float)s->width;
	float fy = (float)target->height / (float)s->height;
	int ax0 = (int)floorf(x0 * fx), ay0 = (int)floorf(y0 * fy);
	int ax1 = (int)ceilf(x1 * fx), ay1 = (int)ceilf(y1 * fy);
	if (ax0 < 0) ax0 = 0;
	if (ay0 < 0) ay0 = 0;
	if (ax1 > target->width) ax1 = target->width;
	if (ay1 > target->height) ay1 = target->height;
	if (ax1 <= ax0 || ay1 <= ay0) return 0;
	area[0] = ax0; area[1] = ay0; area[2] = ax1 - ax0; area[3] = ay1 - ay0;
	return 1;
}

/* Between two scene passes: copies and blurs the frame drawn so far into the backdrop images. */
static void spk_glass_capture(spk_surface_t* s, float scale, const int rect[4]) {
	float spread = scale * 0.5f + 0.5f;
	float margin, x0, y0, x1, y1, width = (float)s->width, height = (float)s->height;
	int partial, i, area[4];
	if (spread < 1.0f) spread = 1.0f;
	if (spread > 2.5f) spread = 2.5f;
	margin = 18.0f * spread + 20.0f;
	x0 = (float)rect[0] - margin; y0 = (float)rect[1] - margin;
	x1 = (float)(rect[0] + rect[2]) + margin; y1 = (float)(rect[1] + rect[3]) + margin;
	if (rect[2] <= 0 || rect[3] <= 0) { x0 = y0 = 0.0f; x1 = width; y1 = height; }
	if (x0 < 0.0f) x0 = 0.0f;
	if (y0 < 0.0f) y0 = 0.0f;
	if (x1 > width) x1 = width;
	if (y1 > height) y1 = height;
	if (x1 <= x0 || y1 <= y0) return;
	partial = s->glass_filled && (x1 - x0) * (y1 - y0) < width * height * 0.8f;

	/* scene -> 1/2 -> 1/4 -> 1/8: down[0] doubles as the sharp backdrop */
	for (i = 0; i < SPK_BLUR_LEVELS; i++) {
		const spk_target_t* source = i == 0 ? &s->scene : &s->down[i - 1];
		if (partial && !spk_glass_area(s, &s->down[i], x0, y0, x1, y1, area)) continue;
		spk_glass_filter(source, &s->down[i], 0, i == 0 ? 1.0f : spread, partial ? area : NULL);
	}
	/* ... and back up to half size: up[0] is the blurred backdrop */
	for (i = SPK_BLUR_LEVELS - 2; i >= 0; i--) {
		const spk_target_t* source = i == SPK_BLUR_LEVELS - 2 ? &s->down[SPK_BLUR_LEVELS - 1] : &s->up[i + 1];
		if (partial && !spk_glass_area(s, &s->up[i], x0, y0, x1, y1, area)) continue;
		spk_glass_filter(source, &s->up[i], 1, spread, partial ? area : NULL);
	}
	s->glass_filled = 1;
}

static void spk_begin_ui_pass(const spk_target_t* target, const sg_color* clear) {
	sg_pass pass;
	memset(&pass, 0, sizeof(pass));
	pass.action.colors[0].load_action = clear ? SG_LOADACTION_CLEAR : SG_LOADACTION_LOAD;
	pass.action.colors[0].store_action = SG_STOREACTION_STORE;
	if (clear) pass.action.colors[0].clear_value = *clear;
	pass.attachments.colors[0] = target->color;
	sg_begin_pass(&pass);
}

static void spk_draw_commands(spk_surface_t* s, const RDrawData* data, int glass) {
	uint32_t i;
	sg_view sharp = glass ? s->down[0].sample : g.white.view;
	sg_view blurred = glass ? s->up[0].sample : g.white.view;
	const spk_target_t* target = glass ? &s->scene : &s->frame;

	sg_apply_viewport(0, 0, target->width, target->height, spk_origin());
	sg_apply_pipeline(g.pipeline);
	spk_apply_uniforms_ui(s, data);
	for (i = 0; i < data->command_count; i++) {
		const RDrawCommand* command = &data->commands[i];
		const spk_texture_t* texture;
		if (command->type == R_DRAW_BACKDROP) {
			sg_color clear_unused = { 0, 0, 0, 0 };
			(void)clear_unused;
			if (!glass) continue;
			sg_end_pass();
			spk_glass_capture(s, data->scale, command->rect);
			spk_begin_ui_pass(&s->scene, NULL);
			sg_apply_viewport(0, 0, target->width, target->height, spk_origin());
			sg_apply_pipeline(g.pipeline);
			spk_apply_uniforms_ui(s, data);
			continue;
		}
		if (command->type != R_DRAW_INSTANCES && command->type != R_DRAW_ERASE && command->type != R_DRAW_SHADER) continue;
		if (command->type == R_DRAW_SHADER && (R_ID_INDEX(command->shader) >= SPK_MAX_EFFECTS || !g.effects[R_ID_INDEX(command->shader)].used)) continue;
		if (command->instance_count == 0 || command->first_instance + command->instance_count > data->instance_count) continue;
		texture = &g.white;
		if (command->texture != R_INVALID_ID && R_ID_INDEX(command->texture) < SPK_MAX_TEXTURES && g.textures[R_ID_INDEX(command->texture)].used) {
			texture = &g.textures[R_ID_INDEX(command->texture)];
		}
		if (command->type == R_DRAW_ERASE || command->type == R_DRAW_SHADER) {
			sg_apply_pipeline(command->type == R_DRAW_ERASE ? g.erase_pipeline : g.effects[R_ID_INDEX(command->shader)].pipeline);
			spk_apply_uniforms_ui(s, data);
		}
		spk_apply_ui_bindings(s, data, command, texture, sharp, blurred);
		sg_draw(0, 4, (int)command->instance_count);
		if (command->type == R_DRAW_ERASE || command->type == R_DRAW_SHADER) {
			sg_apply_pipeline(g.pipeline);
			spk_apply_uniforms_ui(s, data);
		}
	}
}

/* Inside the frame pass: draws the finished scene (blur.frag mode 2 = copy). */
static void spk_glass_present(spk_surface_t* s) {
	sg_bindings bindings;
	float params[4] = { 0.0f, 0.0f, 2.0f, 0.0f };
	sg_range range = { params, sizeof(params) };
	spk_begin_ui_pass(&s->frame, NULL);
	sg_apply_viewport(0, 0, s->width, s->height, spk_origin());
	sg_apply_pipeline(g.blur_pipeline);
	memset(&bindings, 0, sizeof(bindings));
	bindings.views[0] = s->scene.sample;
	bindings.samplers[0] = g.linear;
	sg_apply_bindings(&bindings);
	sg_apply_uniforms(0, &range);
	sg_draw(0, 3, 1);
	sg_end_pass();
}

static void spk_render_frame(const RDrawData* data, RCapture* capture) {
	spk_surface_t* s;
	size_t needed;
	uint32_t i;
	int glass = 0;
	sg_color clear;

	if (capture) memset(capture, 0, sizeof(*capture));
	if (R_ID_INDEX(data->window) >= SPK_MAX_SURFACES) return;
	s = &g.surfaces[R_ID_INDEX(data->window)];
	if (!s->used || data->framebuffer_width == 0 || data->framebuffer_height == 0) return;

	if ((int)data->framebuffer_width != s->width || (int)data->framebuffer_height != s->height) {
		spk_surface_release(s);
		s->width = (int)data->framebuffer_width;
		s->height = (int)data->framebuffer_height;
		if (!spk_target_create(&s->frame, s->width, s->height, "raven-frame")) return;
	}
	for (i = 0; i < data->command_count && !glass; i++) glass = data->commands[i].type == R_DRAW_BACKDROP;
	if (glass && !spk_glass_prepare(s)) glass = 0;

	spk_flush_uploads();

	needed = (size_t)data->instance_count * sizeof(RDrawInstance);
	if (needed > g.instances_size) {
		sg_buffer_desc desc;
		size_t size = g.instances_size ? g.instances_size : (size_t)1024 * sizeof(RDrawInstance);
		while (size < needed) size *= 2;
		if (g.instances.id) sg_destroy_buffer(g.instances);
		memset(&desc, 0, sizeof(desc));
		desc.size = size;
		desc.usage.vertex_buffer = true;
		desc.usage.write_transient = true;
		desc.label = "raven-instances";
		g.instances = sg_make_buffer(&desc);
		g.instances_size = size;
	}
	if (needed) {
		sg_write_buffer_desc write;
		memset(&write, 0, sizeof(write));
		write.src.data.ptr = data->instances;
		write.src.data.size = needed;
		write.dst.buffer = g.instances;
		sg_write_buffer_transient(&write);
	}

	clear = (sg_color){ data->clear_color[0], data->clear_color[1], data->clear_color[2], data->clear_color[3] };
	spk_begin_ui_pass(glass ? &s->scene : &s->frame, &clear);
	if (needed) spk_draw_commands(s, data, glass);
	sg_end_pass();
	if (glass) spk_glass_present(s);
	sg_commit();

	if (capture) {
		capture->pixels = spk_read_target(&s->frame, s->transparent);
		capture->width = s->width;
		capture->height = s->height;
	}
}

/* ---------------------------------------------------------------------------
 *  Effects (raven_shader.h): the SPIR-V cannot be translated here, so the precompiled GLES3 variant
 *  is looked up by the hash of the SPIR-V (the stand-in for an .rfx bundle).
 * ------------------------------------------------------------------------- */

static uint64_t spk_fnv(const void* data, size_t size) {
	const uint8_t* p = (const uint8_t*)data;
	uint64_t h = 0xcbf29ce484222325ull;
	size_t i;
	for (i = 0; i < size; i++) h = (h ^ p[i]) * 0x100000001b3ull;
	return h;
}

static int spk_create_shader(Rid shader, const uint32_t* spirv, size_t size) {
	static const char* const fx_samplers[3] = { "rvn_texture", "rvn_backdrop", "rvn_backdrop_blur" };
	uint64_t hash = spk_fnv(spirv, size);
	size_t i;
	spk_effect_t* e;
	if (R_ID_INDEX(shader) >= SPK_MAX_EFFECTS) return 0;
	for (i = 0; i < sizeof(spk_fx_table) / sizeof(spk_fx_table[0]); i++) {
		if (spk_fx_table[i].hash != hash) continue;
		e = &g.effects[R_ID_INDEX(shader)];
		memset(e, 0, sizeof(*e));
		e->shader = spk_make_ui_shader(spk_fx_table[i].glsl, fx_samplers, spk_fx_table[i].name);
		if (sg_query_shader_state(e->shader) != SG_RESOURCESTATE_VALID) return 0;
		e->pipeline = spk_make_ui_pipeline(e->shader, 0, spk_fx_table[i].name);
		e->used = sg_query_pipeline_state(e->pipeline) == SG_RESOURCESTATE_VALID;
		return e->used;
	}
	fprintf(stderr, "spike: no precompiled GLES3 variant for this effect (hash %016llx): drawn as rectangles\n", (unsigned long long)hash);
	return 0;
}

static void spk_destroy_shader(Rid shader) {
	spk_effect_t* e;
	if (R_ID_INDEX(shader) >= SPK_MAX_EFFECTS) return;
	e = &g.effects[R_ID_INDEX(shader)];
	if (!e->used) return;
	sg_destroy_pipeline(e->pipeline);
	sg_destroy_shader(e->shader);
	e->used = 0;
}

const RBackendVTable* spk_backend(void) {
	static const RBackendVTable vtable = {
		"sokol-gles3-spike",
		spk_init,
		spk_shutdown,
		spk_create_surface,
		spk_destroy_surface,
		spk_create_texture,
		spk_update_texture,
		spk_set_texture_filter,
		spk_destroy_texture,
		spk_render_frame,
		spk_wait_idle,
		spk_create_shader,
		spk_destroy_shader
	};
	return &vtable;
}
