#ifndef APP_VIDEO_DRM_C
#define APP_VIDEO_DRM_C

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "app_video_internal.h"
#include "app_video_drm_internal.h"

static int drm_io_event(int fd, uint32_t mask, void *data)
{
	fprintf(stderr, "drm event\n");
}

int
app_video_drm_init(struct app_video *app, app_drm_page_flip_t pageflip_func,
		   const char *node, void *data)
{
	struct app_video_drm *vdrm;
	int ret;

	vdrm = malloc(sizeof *vdrm);
	if (!vdrm)
		return -ENOMEM;

	memset(vdrm, 0, sizeof *vdrm);

	vdrm->page_flip_fun = pageflip_func;
	vdrm->data = data;
	app->data = vdrm;

	vdrm->fd = open(node, O_RDWR | O_CLOEXEC | O_NONBLOCK);
	if (vdrm->fd < 0) {
		fprintf(stderr, "Cannot open drm device %s : %m", node);
		ret = -EFAULT;
		goto err_drm_open;
	}

	drmDropMaster(vdrm->fd);

	struct ev_event_source *source= ev_event_loop_add_fd(
				app->evloop, EV_EVENT_READABLE,
				vdrm->fd,
				drm_io_event,
				app);

	if (!source) {
		ret = -EFAULT;
		goto err_ev_source;
	}

	vdrm->ev_source = source;

	return 0;

err_ev_source:
	close(vdrm->fd);
err_drm_open:
	free(vdrm);
	return ret;
}

void
app_video_drm_destroy(struct app_video *app)
{
	struct app_video_drm *vdrm = app->data;

	ev_event_source_remove(vdrm->ev_source);
	close(vdrm->fd);
	free(vdrm);
}

#endif // APP_VIDEO_DRM_C
