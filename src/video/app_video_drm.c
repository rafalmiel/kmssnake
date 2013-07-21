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
app_video_drm_wake_up(struct app_video *video, const struct app_display_ops *ops)
{
	struct app_video_drm *drmvideo = video->data;
	int ret;

	ret = drmSetMaster(drmvideo->fd);
	if (ret) {
		return -EACCES;
	}

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
	disp->video = video;

	if (!disp) {
		return;
	}
	drmdisp = disp->data;
	video->display = disp;

	for (i = 0; i < conn->count_modes; ++i) {
		drmdisp->mode_info = conn->modes[i];

		break;
	}

	drmdisp->conn_id = conn->connector_id;

}

int
app_video_drm_hotplug(struct app_video *video, const struct app_display_ops *ops)
{
	struct app_video_drm *vdrm = video->data;
	drmModeResPtr res;
	drmModeConnectorPtr conn;
	int i, ret;

	res = drmModeGetResources(vdrm->fd);
	if (!res) {
		return -EACCES;
	}

	ret = -ENODEV;
	for (i = 0; i < res->count_connectors; ++i) {
		conn = drmModeGetConnector(vdrm->fd, res->connectors[i]);
		if (!conn) continue;
		if (conn->connection != DRM_MODE_CONNECTED) {
			drmModeFreeConnector(conn);
			continue;
		}

		app_video_drm_bind_display(video, res, conn, ops);

		drmModeFreeConnector(conn);

		ret = 0;
		break;
	}

	drmModeFreeResources(res);
	return ret;
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
	drmdisp->conn_id = 0;
	drmdisp->crtc_id = 0;

	return 0;
}

int
app_display_drm_activate(struct app_display *disp)
{
	struct app_video *video = disp->video;
	struct app_video_drm *vdrm = video->data;
	struct app_display_drm *ddrm = disp->data;
	drmModeResPtr res;
	drmModeConnectorPtr conn;
	drmModeEncoderPtr enc;
	int i, j, crtc;

	res = drmModeGetResources(vdrm->fd);
	if (!res) {
		fprintf(stderr, "drm: failed to get resources\n");
		return -EFAULT;
	}

	conn = drmModeGetConnector(vdrm->fd, ddrm->conn_id);
	if (!conn) {
		fprintf(stderr, "drm: failed to get connector\n");
		drmModeFreeResources(res);
		return -EFAULT;
	}

	crtc = -1;
	for (i = 0; i < conn->count_encoders; ++i) {
		enc = drmModeGetEncoder(vdrm->fd, conn->encoders[i]);
		if (!enc) {
			continue;
		}

		crtc = enc->crtc_id;

		drmModeFreeEncoder(enc);
		if (crtc >= 0)
			break;
	}

	drmModeFreeConnector(conn);
	drmModeFreeResources(res);

	if (crtc < 0) {
		fprintf(stderr, "drm: crtc not found\n");
		return -ENODEV;
	}

	ddrm->crtc_id = crtc;
	if (ddrm->saved_crtc) {
		drmModeFreeCrtc(ddrm->saved_crtc);
	}

	ddrm->saved_crtc = drmModeGetCrtc(vdrm->fd, ddrm->crtc_id);

	return 0;
}

void
app_display_drm_deactivate(struct app_display *disp)
{
	struct app_video *video = disp->video;
	struct app_video_drm *vdrm = video->data;
	struct app_display_drm *ddrm = disp->data;

	if (ddrm->saved_crtc) {
		drmModeSetCrtc(vdrm->fd, ddrm->saved_crtc->crtc_id,
			       ddrm->saved_crtc->buffer_id,
			       ddrm->saved_crtc->x,
			       ddrm->saved_crtc->y,
			       &ddrm->conn_id, 1,
			       &ddrm->saved_crtc->mode);

		drmModeFreeCrtc(ddrm->saved_crtc);
		ddrm->saved_crtc = NULL;
	}

	ddrm->crtc_id = 0;
	free(ddrm);
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
