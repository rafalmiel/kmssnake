#ifndef APP_VIDEO_GLDRM_INTERNAL_H
#define APP_VIDEO_GLDRM_INTERNAL_H

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

struct app_video_gldrm {
	struct gbm_device *gbm_device;
	EGLContext egl_context;
	EGLDisplay egl_display;
	EGLConfig egl_config;
};

#endif // APP_VIDEO_GLDRM_INTERNAL_H
