#include "app_video_gldrm.h"

#include <stdio.h>
#include <stdlib.h>
#include <utils/cm_utils.h>
#include "app_video_gldrm_internal.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <errno.h>
#include <fcntl.h>
#include <gbm.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <inttypes.h>
#include <string.h>
#include <unistd.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include "app_video.h"
#include "app_video_drm_internal.h"
#include "app_video_internal.h"

static int
gldrm_display_init(struct app_display *display)
{
	struct app_display_gldrm *disp;
	int ret;

	disp = malloc(sizeof *disp);
	if (!disp) {
		return -EFAULT;
	}

	ret = app_display_drm_init(display, disp);

	if (ret) {
		goto err_init;
	}

err_init:
	free(disp);
	return ret;
}

static void
gldrm_display_activate(struct app_display *display)
{

}

static void
gldrm_display_swap(struct app_display *display)
{

}

static void
gldrm_display_destroy(struct app_display *display)
{

}

static void
page_flip_handler(struct app_display * app_video)
{

}

static int
gldrm_video_init(struct app_video *app_video, const char *node)
{
	static const EGLint conf_att[] = {
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
		EGL_RED_SIZE, 1,
		EGL_GREEN_SIZE, 1,
		EGL_BLUE_SIZE, 1,
		EGL_ALPHA_SIZE, 0,
		EGL_NONE
	};
	static const EGLint ctx_att[] = {
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE
	};
	const char *ext;
	int ret;
	EGLint major, minor, n;
	EGLenum api;
	EGLBoolean b;
	struct app_video_drm *vdrm;
	struct app_video_gldrm *gldrm;

	gldrm = malloc(sizeof *gldrm);
	if (!gldrm)
		return -ENOMEM;
	memset(gldrm, 0, sizeof *gldrm);

	ret = app_video_drm_init(app_video, page_flip_handler, node, gldrm);
	if (ret) {
		goto err_free;
	}

	vdrm = app_video->data;

	gldrm->gbm_device = gbm_create_device(vdrm->fd);
	if (!gldrm->gbm_device) {
		ret = -EFAULT;
		goto err_video;
	}

	gldrm->egl_display = eglGetDisplay((EGLNativeDisplayType) gldrm->gbm_device);
	if (gldrm->egl_display == EGL_NO_DISPLAY) {
		ret = -EFAULT;
		goto err_gbm;
	}

	b = eglInitialize(gldrm->egl_display, &major, &minor);
	if (!b) {
		ret = -EFAULT;
		goto err_gbm;
	}

	printf("EGL Init %d.%d\n", major, minor);
	printf("EGL Version %s\n", eglQueryString(gldrm->egl_display, EGL_VERSION));
	printf("EGL Vendor %s\n", eglQueryString(gldrm->egl_display, EGL_VENDOR));
	ext = eglQueryString(gldrm->egl_display, EGL_EXTENSIONS);
	printf("EGL Extenstions %s\n", ext);

	if (!ext || !strstr(ext, "EGL_KHR_surfaceless_context")) {
		ret = -EFAULT;
		goto err_disp;
	}

	api = EGL_OPENGL_ES_API;
	if (!eglBindAPI(api)) {
		ret = -EFAULT;
		goto err_disp;
	}

	b = eglChooseConfig(gldrm->egl_display, conf_att, &gldrm->egl_config, 1, &n);
	if (!b || n != 1) {
		ret = -EFAULT;
		goto err_disp;
	}

	gldrm->egl_context = eglCreateContext(gldrm->egl_display, gldrm->egl_config, EGL_NO_CONTEXT,
					      ctx_att);
	if (gldrm->egl_context == EGL_NO_CONTEXT) {
		ret = -EFAULT;
		goto err_disp;
	}

	if (!eglMakeCurrent(gldrm->egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE,
			    gldrm->egl_context)) {
		ret = -EFAULT;
		goto err_ctx;
	}

	return 0;
err_ctx:
	eglDestroyContext(gldrm->egl_display, gldrm->egl_context);
err_disp:
	eglTerminate(gldrm->egl_display);
err_gbm:
	gbm_device_destroy(gldrm->gbm_device);
err_video:
	app_video_drm_destroy(app_video);
err_free:
	free(gldrm);
	return ret;
}

static void
gldrm_video_destroy(struct app_video *app_video)
{
	struct app_video_drm *vdrm;
	struct app_video_gldrm *gldrm;

	vdrm = app_video->data;
	gldrm = vdrm->data;

	eglMakeCurrent(gldrm->egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE,
		       EGL_NO_CONTEXT);
	eglDestroyContext(gldrm->egl_display, gldrm->egl_context);
	eglTerminate(gldrm->egl_display);
	gbm_device_destroy(gldrm->gbm_device);
	free(gldrm);
	app_video_drm_destroy(app_video);
}

const static struct app_display_ops gldrm_display_ops = {
	gldrm_display_init,
	gldrm_display_activate,
	gldrm_display_swap,
	gldrm_display_destroy
};

static void
gldrm_video_wakeup(struct app_video *app_video)
{
	fprintf(stderr, "gldrm wakeup\n");
	if (!app_video->display) {
		fprintf(stderr, "gldrm wakeup\n");
		app_video_drm_wakeup(app_video, &gldrm_display_ops);
	}
}

const static struct app_video_ops gldrm_video_ops = {
	gldrm_video_init,
	gldrm_video_destroy,
	gldrm_video_wakeup
};

CM_EXPORT int
app_video_gldrm_init(struct app_video *app_video)
{
	app_video->ops = &gldrm_video_ops;
	return 0;
}

