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

int
app_video_drm_wakeup(struct app_video *video, const struct app_display_ops *ops)
{
	struct app_video_drm *drmvideo = video->data;
	int ret;

	fprintf(stderr, "app video drm wakeup\n");

	ret = drmSetMaster(drmvideo->fd);
	if (ret) {
		return -EACCES;
	}

	fprintf(stderr, "Set Drm Master!\n");

	ret = app_video_drm_hotplug(video, ops);

	if (ret) {
		drmDropMaster(drmvideo->fd);
		return ret;
	}

	return 0;
}

static void
app_video_drm_bind_display(struct app_video *video, drmModeResPtr res,
			   drmModeConnectorPtr conn,
			   const struct app_display_ops *ops)
{
	struct app_video_drm *vdrm;
	struct app_display *disp;
	struct app_display_drm *drmdisp;
	int ret, i;

	disp = app_display_create(ops);

	if (!disp) {
		return;
	}
	drmdisp = disp->data;

	for (i = 0; i < conn->count_modes; ++i) {
		drmdisp->mode_info = conn->modes[i];

		fprintf(stderr, "MODE %d %d\n",
			(int)drmdisp->mode_info.vdisplay,
			(int)drmdisp->mode_info.hdisplay);

		break;
	}

	/*if (!drmdisp->mode_info) {
		ret = -EFAULT;
		app_display_unref(disp);
		return;
	}*/

	drmdisp->conn_id = conn->connector_id;

}

int
app_video_drm_hotplug(struct app_video *video, const struct app_display_ops *ops)
{
	struct app_video_drm *vdrm = video->data;
	drmModeResPtr res;
	drmModeConnectorPtr conn;
	int i;

	res = drmModeGetResources(vdrm->fd);
	if (!res) {
		return -EACCES;
	}

	for (i = 0; i < res->count_connectors; ++i) {
		conn = drmModeGetConnector(vdrm->fd, res->connectors[i]);
		if (!conn) continue;
		if (conn->connection != DRM_MODE_CONNECTED) {
			drmModeFreeConnector(conn);
			continue;
		}

		app_video_drm_bind_display(video, res, conn, ops);

		drmModeFreeConnector(conn);
	}

	drmModeFreeResources(res);
	return 0;
}

static int
drm_io_event(int fd, uint32_t mask, void *data)
{
	fprintf(stderr, "drm event\n");
}

int
app_display_drm_init(struct app_display *disp, void *data)
{
	struct app_display_drm *drmdisp;

	drmdisp = malloc(sizeof *drmdisp);
	if (!drmdisp) {
		return -EFAULT;
	}

	disp->data = drmdisp;
	drmdisp->data = data;
	memset(&drmdisp->mode_info, 0, sizeof drmdisp->mode_info);
	drmdisp->saved_crtc = NULL;

	return 0;
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
