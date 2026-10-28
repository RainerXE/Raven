/**
 * Raven - render/sokol/sg_egl.c
 * GLES 3.0 context through EGL, without a surface ("surfaceless"): every window renders into an
 * offscreen image. This is the context of headless runs and of tests; presenting to a window
 * (GLX/EGL on X11, flip on present) is phase 3 of docs/SOKOL_BACKEND_PLAN.md.
 *
 * The EGL library is linked; loading it at run time (as the Vulkan loader is) is a later step.
 */

#include "sg_backend.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>

static struct {
	EGLDisplay display;
	EGLContext context;
} g_egl;

int rvn_sg_context_init(const RBackendInitInfo* info, sg_environment* environment) {
	static const EGLint config_attributes[] = {
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT, EGL_SURFACE_TYPE, 0,
		EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8, EGL_ALPHA_SIZE, 8, EGL_NONE
	};
	static const EGLint context_attributes[] = { EGL_CONTEXT_MAJOR_VERSION, 3, EGL_CONTEXT_MINOR_VERSION, 0, EGL_NONE };
	PFNEGLGETPLATFORMDISPLAYEXTPROC get_display = (PFNEGLGETPLATFORMDISPLAYEXTPROC)eglGetProcAddress("eglGetPlatformDisplayEXT");
	EGLConfig config;
	EGLint count = 0, major, minor;
	RVN_UNUSED(info);
	RVN_UNUSED(environment);

	g_egl.display = get_display ? get_display(EGL_PLATFORM_SURFACELESS_MESA, EGL_DEFAULT_DISPLAY, NULL) : EGL_NO_DISPLAY;
	if (g_egl.display == EGL_NO_DISPLAY) g_egl.display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	if (g_egl.display == EGL_NO_DISPLAY || !eglInitialize(g_egl.display, &major, &minor)) {
		rvn_error(R_ERROR_BACKEND, R_SEVERITY_ERROR, "Sokol: no EGL display (install a GLES3 driver, for example Mesa)");
		return R_FALSE;
	}
	eglBindAPI(EGL_OPENGL_ES_API);
	if (!eglChooseConfig(g_egl.display, config_attributes, &config, 1, &count) || count == 0) {
		rvn_error(R_ERROR_BACKEND, R_SEVERITY_ERROR, "Sokol: the EGL display has no GLES 3.0 configuration");
		return R_FALSE;
	}
	g_egl.context = eglCreateContext(g_egl.display, config, EGL_NO_CONTEXT, context_attributes);
	if (g_egl.context == EGL_NO_CONTEXT || !eglMakeCurrent(g_egl.display, EGL_NO_SURFACE, EGL_NO_SURFACE, g_egl.context)) {
		rvn_error(R_ERROR_BACKEND, R_SEVERITY_ERROR, "Sokol: cannot create a GLES 3.0 context without a surface");
		return R_FALSE;
	}
	return R_TRUE;
}

void rvn_sg_context_shutdown(void) {
	if (g_egl.display != EGL_NO_DISPLAY && g_egl.display) {
		eglMakeCurrent(g_egl.display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
		if (g_egl.context) eglDestroyContext(g_egl.display, g_egl.context);
		eglTerminate(g_egl.display);
	}
	memset(&g_egl, 0, sizeof(g_egl));
}

void rvn_sg_context_finish(void) {
	glFinish();
}

uint8_t* rvn_sg_read_target(const rvn_sg_target_t* target) {
	sg_gl_image_info info = sg_gl_query_image_info(target->image);
	size_t count = (size_t)target->width * (size_t)target->height;
	uint8_t* pixels = Rpixels_alloc(count * 4);
	GLuint framebuffer = 0;
	glGenFramebuffers(1, &framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, info.tex[info.active_slot], 0);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	/* the UI top is framebuffer row 0 (see sg_backend.h): the rows come out top first */
	glReadPixels(0, 0, target->width, target->height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDeleteFramebuffers(1, &framebuffer);
	/* sokol caches the GL state: tell it the framebuffer binding changed behind its back */
	sg_reset_state_cache();
	return pixels;
}

void* rvn_sg_pool_push(void) {
	return NULL;
}

void rvn_sg_pool_pop(void* pool) {
	RVN_UNUSED(pool);
}
