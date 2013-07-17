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
#include "app_video_drm_internal.h"
#include "app_video_internal.h"

static void
page_flip_handler(struct app_video * app_video)
{

}

static int
gldrm_video_init(struct app_video *app_video, const char *node)
{
	struct app_video_gldrm *gldrm;

	gldrm = malloc(sizeof *gldrm);
	if (!gldrm)
		return -ENOMEM;

	if (app_video_drm_init(app_video, page_flip_handler, node, gldrm)) {
		free(gldrm);
		return -ENOMEM;
	}

	return 0;
}

static void
gldrm_video_destroy(struct app_video *app_video)
{
	struct app_video_drm *vdrm;
	struct app_video_gldrm *gldrm;

	vdrm = app_video->data;
	gldrm = vdrm->data;
	app_video_drm_destroy(app_video);
	free(gldrm);
}

const static struct app_video_ops gldrm_video_ops = {
	gldrm_video_init,
	gldrm_video_destroy
};

CM_EXPORT int
app_video_gldrm_init(struct app_video *app_video)
{
	app_video->ops = &gldrm_video_ops;
	return 0;
}

