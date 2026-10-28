/**
 * Raven - render/sokol/sg_texture.c
 * Textures. Every texture is an RGBA8 image that can be a copy destination; the pixels come in through
 * a transient staging buffer (sokol_gfx), so uploads are queued and written together at the start of
 * the next frame (rvn_sg_uploads_flush).
 *
 * R_TEXTURE_ALPHA8 (the glyph atlas) is sampled by Raven as (1, 1, 1, a): it is stored as RGBA8 with
 * those values, which keeps the shaders identical to the Vulkan ones.
 */

#include "sg_backend.h"

typedef struct rvn_sg_upload_t {
	sg_image image;
	int x, y, width, height;
	/** RGBA8, owned. */
	uint8_t* pixels;
} rvn_sg_upload_t;

static struct {
	sg_buffer staging;
	size_t staging_size;
	rvn_sg_upload_t* uploads;
	uint32_t count, capacity;
} g_up;

static void rvn_sg_upload_queue(sg_image image, int x, int y, int width, int height, const uint8_t* rgba) {
	rvn_sg_upload_t* upload;
	size_t size = (size_t)width * (size_t)height * 4;
	RVN_ARRAY_RESERVE(g_up.uploads, g_up.capacity, g_up.count + 1);
	upload = &g_up.uploads[g_up.count++];
	upload->image = image;
	upload->x = x;
	upload->y = y;
	upload->width = width;
	upload->height = height;
	upload->pixels = (uint8_t*)rvn_alloc(size);
	memcpy(upload->pixels, rgba, size);
}

static size_t rvn_sg_upload_size(const rvn_sg_upload_t* upload) {
	/* every upload starts at a multiple of 256 bytes of the staging buffer */
	return (((size_t)upload->width * (size_t)upload->height * 4) + 255) & ~(size_t)255;
}

void rvn_sg_uploads_flush(void) {
	uint32_t i;
	size_t total = 0, offset;
	if (g_up.count == 0) return;
	for (i = 0; i < g_up.count; i++) total += rvn_sg_upload_size(&g_up.uploads[i]);
	if (total > g_up.staging_size) {
		sg_buffer_desc desc;
		size_t size = g_up.staging_size ? g_up.staging_size : (size_t)1 << 20;
		while (size < total) size *= 2;
		if (g_up.staging.id) sg_destroy_buffer(g_up.staging);
		memset(&desc, 0, sizeof(desc));
		desc.size = size;
		desc.usage.staging_buffer = true;
		desc.usage.write_transient = true;
		desc.usage.copy_src = true;
		desc.label = "raven-staging";
		g_up.staging = sg_make_buffer(&desc);
		g_up.staging_size = size;
	}
	offset = 0;
	for (i = 0; i < g_up.count; i++) {
		const rvn_sg_upload_t* upload = &g_up.uploads[i];
		sg_write_buffer_desc write;
		memset(&write, 0, sizeof(write));
		write.src.data.ptr = upload->pixels;
		write.src.data.size = (size_t)upload->width * (size_t)upload->height * 4;
		write.dst.buffer = g_up.staging;
		write.dst.offset = offset;
		if (upload->width > 0 && upload->height > 0) sg_write_buffer_transient(&write);
		offset += rvn_sg_upload_size(upload);
	}
	offset = 0;
	for (i = 0; i < g_up.count; i++) {
		rvn_sg_upload_t* upload = &g_up.uploads[i];
		sg_copy_buffer_to_image_desc copy;
		memset(&copy, 0, sizeof(copy));
		copy.src.buffer = g_up.staging;
		copy.src.offset = offset;
		copy.src.bytes_per_row = upload->width * 4;
		copy.src.bytes_per_slice = upload->width * 4 * upload->height;
		copy.dst.image = upload->image;
		copy.dst.x = upload->x;
		copy.dst.y = upload->y;
		copy.size.width = upload->width;
		copy.size.height = upload->height;
		/* (an upload of a texture destroyed since it was queued has no size) */
		if (upload->width > 0 && upload->height > 0) sg_copy_buffer_to_image(&copy);
		offset += rvn_sg_upload_size(upload);
		rvn_free(upload->pixels);
		upload->pixels = NULL;
	}
	g_up.count = 0;
}

/* ---------------------------------------------------------------------------
 *  Textures
 * ------------------------------------------------------------------------- */

static int rvn_sg_image_create(rvn_sg_texture_t* texture, int width, int height, const uint8_t* rgba) {
	sg_image_desc image;
	sg_view_desc view;
	memset(&image, 0, sizeof(image));
	image.type = SG_IMAGETYPE_2D;
	image.width = width;
	image.height = height;
	image.pixel_format = SG_PIXELFORMAT_RGBA8;
	image.usage.copy_dst = true;
	texture->image = sg_make_image(&image);
	if (sg_query_image_state(texture->image) != SG_RESOURCESTATE_VALID) return R_FALSE;
	memset(&view, 0, sizeof(view));
	view.texture.image = texture->image;
	texture->view = sg_make_view(&view);
	texture->width = width;
	texture->height = height;
	if (rgba) rvn_sg_upload_queue(texture->image, 0, 0, width, height, rgba);
	return R_TRUE;
}

static uint8_t* rvn_sg_expand_alpha(const uint8_t* alpha, size_t count) {
	uint8_t* rgba = (uint8_t*)rvn_alloc(count * 4);
	size_t i;
	for (i = 0; i < count; i++) {
		rgba[i * 4 + 0] = rgba[i * 4 + 1] = rgba[i * 4 + 2] = 255;
		rgba[i * 4 + 3] = alpha[i];
	}
	return rgba;
}

int rvn_sg_textures_init(void) {
	static const uint8_t white[4] = { 255, 255, 255, 255 };
	memset(&g_sg.white, 0, sizeof(g_sg.white));
	g_sg.white.used = R_TRUE;
	g_sg.white.filter = R_FILTER_NEAREST;
	return rvn_sg_image_create(&g_sg.white, 1, 1, white);
}

void rvn_sg_textures_shutdown(void) {
	uint32_t i;
	for (i = 0; i < g_up.count; i++) rvn_free(g_up.uploads[i].pixels);
	rvn_free(g_up.uploads);
	memset(&g_up, 0, sizeof(g_up));
}

int rvn_sg_texture_create(Rid texture, const RTextureDesc* desc, const void* pixels) {
	uint32_t index = R_ID_INDEX(texture);
	rvn_sg_texture_t* t;
	uint8_t* expanded = NULL;
	int ok;
	if (index >= RVN_SG_MAX_TEXTURES) return R_FALSE;
	t = &g_sg.textures[index];
	memset(t, 0, sizeof(*t));
	t->used = R_TRUE;
	t->format = desc->format;
	t->filter = desc->filter;
	if (pixels && desc->format == R_TEXTURE_ALPHA8) {
		expanded = rvn_sg_expand_alpha((const uint8_t*)pixels, (size_t)desc->width * desc->height);
		pixels = expanded;
	}
	ok = rvn_sg_image_create(t, (int)desc->width, (int)desc->height, (const uint8_t*)pixels);
	rvn_free(expanded);
	if (!ok) t->used = R_FALSE;
	return ok;
}

void rvn_sg_texture_update(Rid texture, Ruint x, Ruint y, Ruint width, Ruint height, const void* pixels) {
	uint32_t index = R_ID_INDEX(texture);
	rvn_sg_texture_t* t;
	uint8_t* expanded = NULL;
	if (index >= RVN_SG_MAX_TEXTURES || !g_sg.textures[index].used || !pixels) return;
	t = &g_sg.textures[index];
	if (t->format == R_TEXTURE_ALPHA8) {
		expanded = rvn_sg_expand_alpha((const uint8_t*)pixels, (size_t)width * height);
		pixels = expanded;
	}
	rvn_sg_upload_queue(t->image, (int)x, (int)y, (int)width, (int)height, (const uint8_t*)pixels);
	rvn_free(expanded);
}

void rvn_sg_texture_set_filter(Rid texture, RFilterType filter) {
	uint32_t index = R_ID_INDEX(texture);
	if (index < RVN_SG_MAX_TEXTURES && g_sg.textures[index].used) g_sg.textures[index].filter = filter;
}

void rvn_sg_texture_destroy(Rid texture) {
	uint32_t index = R_ID_INDEX(texture), i;
	rvn_sg_texture_t* t;
	if (index >= RVN_SG_MAX_TEXTURES) return;
	t = &g_sg.textures[index];
	if (!t->used) return;
	/* what is still queued for it must not run */
	for (i = 0; i < g_up.count; i++) {
		if (g_up.uploads[i].image.id == t->image.id) g_up.uploads[i].width = g_up.uploads[i].height = 0;
	}
	sg_destroy_view(t->view);
	sg_destroy_image(t->image);
	memset(t, 0, sizeof(*t));
}

const rvn_sg_texture_t* rvn_sg_texture_find(Rid texture) {
	uint32_t index = R_ID_INDEX(texture);
	if (texture != R_INVALID_ID && index < RVN_SG_MAX_TEXTURES && g_sg.textures[index].used) return &g_sg.textures[index];
	return &g_sg.white;
}
