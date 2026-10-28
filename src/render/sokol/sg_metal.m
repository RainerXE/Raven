/**
 * Raven - render/sokol/sg_metal.m
 * The Metal part of the sokol_gfx backend (the counterpart of sg_egl.c): the device, the read-back, and
 * the autorelease pool of the render thread. Compiled with ARC (CMakeLists.txt).
 *
 * Like the GLES3 build, this renders into offscreen images that Rwindow_capture reads back; presenting to
 * the CAMetalLayer of a window is phase 3a of docs/SOKOL_BACKEND_PLAN.md.
 *
 * Status: written without a Mac at hand; the first runs are those of the macOS job of the CI.
 */

#import <Foundation/Foundation.h>
#import <Metal/Metal.h>

#include "sg_backend.h"

/* libobjc: a pool that can be pushed and popped from C, with or without ARC */
extern void* objc_autoreleasePoolPush(void);
extern void objc_autoreleasePoolPop(void* pool);

static id<MTLDevice> g_device;

int rvn_sg_context_init(const RBackendInitInfo* info, sg_environment* environment) {
	RVN_UNUSED(info);
	g_device = MTLCreateSystemDefaultDevice();
	if (!g_device) {
		rvn_error(R_ERROR_BACKEND, R_SEVERITY_ERROR, "Sokol: no Metal device on this machine");
		return R_FALSE;
	}
	environment->metal.device = (__bridge const void*)g_device;
	return R_TRUE;
}

void rvn_sg_context_shutdown(void) {
	g_device = nil;
}

void rvn_sg_context_finish(void) {
	/* the queue is serial: when an empty buffer committed now is done, so is everything before it */
	id<MTLCommandQueue> queue = (__bridge id<MTLCommandQueue>)sg_mtl_command_queue();
	id<MTLCommandBuffer> commands = [queue commandBuffer];
	[commands commit];
	[commands waitUntilCompleted];
}

uint8_t* rvn_sg_read_target(const rvn_sg_target_t* target) {
	sg_mtl_image_info info = sg_mtl_query_image_info(target->image);
	id<MTLTexture> texture = (__bridge id<MTLTexture>)info.tex[info.active_slot];
	id<MTLCommandQueue> queue = (__bridge id<MTLCommandQueue>)sg_mtl_command_queue();
	NSUInteger width = (NSUInteger)target->width, height = (NSUInteger)target->height;
	NSUInteger row = width * 4, size = row * height;
	uint8_t* pixels = Rpixels_alloc(size);

	/* the images of sokol_gfx live in private memory: copy to a buffer the CPU can read */
	id<MTLBuffer> buffer = [g_device newBufferWithLength:size options:MTLResourceStorageModeShared];
	id<MTLCommandBuffer> commands = [queue commandBuffer];
	id<MTLBlitCommandEncoder> blit = [commands blitCommandEncoder];
	[blit copyFromTexture:texture
		sourceSlice:0
		sourceLevel:0
		sourceOrigin:MTLOriginMake(0, 0, 0)
		sourceSize:MTLSizeMake(width, height, 1)
		toBuffer:buffer
		destinationOffset:0
		destinationBytesPerRow:row
		destinationBytesPerImage:size];
	[blit endEncoding];
	[commands commit];
	/* the frame was committed before this buffer on the same queue: waiting for this one waits for both */
	[commands waitUntilCompleted];
	/* the image is stored with the top row first: no flip */
	memcpy(pixels, [buffer contents], size);
	return pixels;
}

void* rvn_sg_pool_push(void) {
	return objc_autoreleasePoolPush();
}

void rvn_sg_pool_pop(void* pool) {
	objc_autoreleasePoolPop(pool);
}
