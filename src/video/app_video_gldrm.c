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
#include <GL/gl.h>
#include <GL/glext.h>
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

	memset(disp, 0, sizeof *disp);

	ret = app_display_drm_init(display, disp);

	if (ret) {
		goto err_init;
	}

	return 0;

err_init:
	free(disp);
	return ret;
}

static void
bo_destroy_event(struct gbm_bo *bo, void *data)
{
	struct app_display_gldrm_rb *rb = data;
	struct app_video_drm *vdrm;

	if (!rb)
		return;

	vdrm = rb->disp->video->data;
	drmModeRmFB(vdrm->fd, rb->fb);
	free(rb);
}

static struct app_display_gldrm_rb *
bo_to_rb(struct app_display *display, struct gbm_bo *bo)
{
	struct app_display_gldrm_rb *rb = gbm_bo_get_user_data(bo);
	struct app_video *video = display->video;
	struct app_video_drm *vdrm = video->data;
	int ret;
	unsigned int stride, handle, width, height;

	if (rb)
		return rb;

	rb = malloc(sizeof *rb);
	if (!rb) {
		return NULL;
	}

	memset(rb, 0, sizeof *rb);

	rb->disp = display;
	rb->bo = bo;

	stride = gbm_bo_get_stride(rb->bo);
	handle = gbm_bo_get_handle(rb->bo).u32;
	width = gbm_bo_get_width(rb->bo);
	height = gbm_bo_get_height(rb->bo);

	ret = drmModeAddFB(vdrm->fd, width, height, 24, 32,
			   stride, handle, &rb->fb);

	if (ret) {
		free(rb);
		return NULL;
	}

	gbm_bo_set_user_data(bo, rb, bo_destroy_event);
	return rb;
}

static int
gldrm_display_activate(struct app_display *display)
{
	struct app_video *video = display->video;
	struct app_video_drm *vdrm = video->data;
	struct app_video_gldrm *vgldrm = vdrm->data;
	struct app_display_drm *ddrm = display->data;
	struct app_display_gldrm *dgldrm = ddrm->data;
	struct gbm_bo *bo;
	drmModeModeInfoPtr minfo;
	int ret;

	minfo = &ddrm->mode_info;

	dgldrm->current = NULL;
	dgldrm->next = NULL;

	ret = app_display_drm_activate(display);
	if (ret)
		return ret;

	dgldrm->gbm = gbm_surface_create(vgldrm->gbm_device,
					 minfo->hdisplay, minfo->vdisplay,
					 GBM_FORMAT_XRGB8888,
					 GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);

	if (!dgldrm->gbm) {
		fprintf(stderr, "gldrm: gbm_surface_create failed\n");
		goto err_gbm_create;
	}

	dgldrm->surface = eglCreateWindowSurface(vgldrm->egl_display,
						 vgldrm->egl_config,
						 (EGLNativeWindowType)dgldrm->gbm,
						 NULL);

	if (dgldrm->surface == EGL_NO_SURFACE) {
		fprintf(stderr, "gldrm: eglCreateWindowSurface failed\n");
		ret = -EFAULT;
		goto err_egl_create_surface;
	}

	if (!eglMakeCurrent(vgldrm->egl_display, dgldrm->surface,
			    dgldrm->surface, vgldrm->egl_context)) {
		fprintf(stderr, "gldrm: eglMakeCurrent failed\n");
		ret = -EFAULT;
		goto err_egl_make_current;
	}

	glViewport(0, 0, ddrm->mode_info.hdisplay, ddrm->mode_info.vdisplay);

	glMatrixMode (GL_PROJECTION);
	glLoadIdentity ();
	glOrtho (0, ddrm->mode_info.hdisplay, ddrm->mode_info.vdisplay, 0, 0, 1);
	glMatrixMode (GL_MODELVIEW);

	glDisable(GL_DEPTH_TEST);

	glClearColor(0.8, 0.8, 0.8, 1);
	glClear(GL_COLOR_BUFFER_BIT);

	if (!eglSwapBuffers(vgldrm->egl_display, dgldrm->surface)) {
		fprintf(stderr, "gldrm: eglSwapBuffers failed\n");
		ret = -EFAULT;
		goto err_egl_swap_buffers;
	}

	bo = gbm_surface_lock_front_buffer(dgldrm->gbm);
	if (!bo) {
		fprintf(stderr, "gldrm: gbm_surface_lock_front_buffer failed\n");
		ret = -EFAULT;
		goto err_egl_swap_buffers;
	}

	dgldrm->current = bo_to_rb(display, bo);

	if (!dgldrm->current) {
		fprintf(stderr, "gldrm: bo_to_rb failed\n");
		ret = -EFAULT;
		goto err_bo;
	}

	ret = drmModeSetCrtc(vdrm->fd, ddrm->crtc_id, dgldrm->current->fb,
			     0, 0, &ddrm->conn_id, 1, minfo);

	if (ret) {
		fprintf(stderr, "gldrm: drmModeSetCrtc failed\n");
		ret = -EFAULT;
		goto err_bo;
	}

	return 0;
err_bo:
	gbm_surface_release_buffer(dgldrm->gbm, bo);
err_egl_swap_buffers:
	eglMakeCurrent(vgldrm->egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE,
		       vgldrm->egl_context);
err_egl_make_current:
	eglDestroySurface(vgldrm->egl_display, dgldrm->surface);
err_egl_create_surface:
	gbm_surface_destroy(dgldrm->surface);
err_gbm_create:
	app_display_drm_deactivate(display);

}

static int
gldrm_display_swap(struct app_display *display)
{
	struct app_display_gldrm *dgldrm = app_display_drm_get_data(display);
	struct app_video *video = display->video;
	struct app_video_gldrm *vgldrm = app_video_drm_get_data(video);
	struct app_display_gldrm_rb *rb;
	struct gbm_bo *bo;
	int ret;
	int i = 0;

	if (!gbm_surface_has_free_buffers(dgldrm->gbm)) {
		fprintf(stderr, "Cannot swap EGL buffers BUSY %m\n");
		return -EBUSY;
	}

	if (!eglSwapBuffers(vgldrm->egl_display, dgldrm->surface)) {
		fprintf(stderr, "Cannot swap EGL buffers %m\n");
		return -EFAULT;
	}

	bo = gbm_surface_lock_front_buffer(dgldrm->gbm);
	if (!bo) {
		fprintf(stderr, "Cannot lock front buffer\n");
		return -EFAULT;
	}

	rb = bo_to_rb(display, bo);

	if (!rb) {
		fprintf(stderr, "cannot lock front gbm buffer\n");
		return -EFAULT;
	}

	ret = app_display_drm_swap(display, rb->fb);

	if (ret) {
		gbm_surface_release_buffer(dgldrm->gbm, bo);
		return ret;
	}

	dgldrm->next = rb;

	return 0;
}

static void
gldrm_display_deactivate(struct app_display *display)
{
	struct app_display_drm *ddrm = display->data;
	struct app_display_gldrm *dgldrm = ddrm->data;
	struct app_video_drm *vdrm = display->video->data;
	struct app_video_gldrm *vgldrm = vdrm->data;

	if (dgldrm->current) {
		gbm_surface_release_buffer(dgldrm->gbm, dgldrm->current->bo);
		dgldrm->current = NULL;
	}
	if (dgldrm->next) {
		gbm_surface_release_buffer(dgldrm->gbm, dgldrm->next->bo);
		dgldrm->current = NULL;
	}
	eglMakeCurrent(vgldrm->egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE,
		       vgldrm->egl_context);
	eglDestroySurface(vgldrm->egl_display, dgldrm->surface);

	gbm_surface_destroy(dgldrm->gbm);
	free(dgldrm);
	app_display_drm_deactivate(display);
}

static void
page_flip_handler(struct app_display * app_display)
{
	struct app_display_gldrm *dgldrm = app_display_drm_get_data(app_display);

	if (dgldrm->next) {
		if (dgldrm->current)
			gbm_surface_release_buffer(dgldrm->gbm,
						   dgldrm->current->bo);
		dgldrm->current = dgldrm->next;
		dgldrm->next = NULL;
	}
}

static int
gldrm_video_init(struct app_video *app_video, const char *node)
{
	static const EGLint conf_att[] = {
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
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

	api = EGL_OPENGL_API;
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
	gldrm_display_deactivate
};

static int
gldrm_video_wake_up(struct app_video *app_video)
{
	int ret;
	if (!app_video->display) {
		ret = app_video_drm_wake_up(app_video, &gldrm_display_ops);

		if (ret) {
			fprintf(stderr, "Failed to wake up video\n");
			return ret;
		}
	}

	return 0;
}

const static struct app_video_ops gldrm_video_ops = {
	gldrm_video_init,
	gldrm_video_destroy,
	gldrm_video_wake_up
};

CM_EXPORT int
app_video_gldrm_init(struct app_video *app_video)
{
	app_video->ops = &gldrm_video_ops;
	return 0;
}

