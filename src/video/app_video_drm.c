#ifndef APP_VIDEO_DRM_C
#define APP_VIDEO_DRM_C

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <utils/cm_log.h>
#include "app_video_internal.h"
#include "app_video_drm_internal.h"

#define LOG_SUBSYSTEM "drm"

int
app_video_drm_wake_up(struct app_video *video, const struct app_display_ops *ops)
{
	struct app_video_drm *drmvideo = video->data;
	int ret;

	log_debug("waking up video")

	ret = drmSetMaster(drmvideo->fd);
	if (ret) {
		log_fatal("failed to set drm master");
		return -EACCES;
	}

	ret = app_video_drm_hotplug(video, ops);

	if (ret) {
		log_fatal("failed to hotplug drm video");
		drmDropMaster(drmvideo->fd);
		return ret;
	}

	return 0;
}

static int
app_video_drm_bind_display(struct app_video *video, drmModeResPtr res,
			   drmModeConnectorPtr conn,
			   const struct app_display_ops *ops)
{
	struct app_video_drm *vdrm;
	struct app_display *disp;
	struct app_display_drm *drmdisp;
	int i;

	log_debug("binding display to video")

	disp = app_display_create(ops);
	disp->video = video;

	if (!disp) {
		log_fatal("failed to bind display to video")
		return -EFAULT;
	}
	drmdisp = disp->data;

	app_display_bind(video, disp);

	disp->flags |= DISPLAY_AVAILABLE;

	for (i = 0; i < conn->count_modes; ++i) {
		drmdisp->mode_info = conn->modes[i];

		break;
	}

	drmdisp->conn_id = conn->connector_id;

	return 0;
}

int
app_video_drm_hotplug(struct app_video *video, const struct app_display_ops *ops)
{
	struct app_video_drm *vdrm = video->data;
	struct app_display *display;
	struct app_display_drm *ddrm;
	struct cm_list *iter, *tmp;
	drmModeResPtr res;
	drmModeConnectorPtr conn;
	int i, ret;

	log_debug("hotplugging video")

	res = drmModeGetResources(vdrm->fd);
	if (!res) {
		return -EACCES;
	}

	cm_list_foreach(iter, &video->displays) {
		display = cm_list_entry(iter, struct app_display, link);
		display->flags &= ~(DISPLAY_AVAILABLE);
	}

	ret = -ENODEV;
	for (i = res->count_connectors - 1; i >= 0; --i) {
		conn = drmModeGetConnector(vdrm->fd, res->connectors[i]);
		if (!conn) continue;
		if (conn->connection != DRM_MODE_CONNECTED) {
			drmModeFreeConnector(conn);
			continue;
		}

		cm_list_foreach(iter, &video->displays) {
			display = cm_list_entry(iter, struct app_display, link);
			ddrm = display->data;

			if (ddrm->conn_id != res->connectors[i])
				continue;

			display->flags |= DISPLAY_AVAILABLE;

			break;
		}

		log_trace("using connector %d", conn->connector_id)

		if (iter == &video->displays) {
			log_trace("binding new display")

			ret = app_video_drm_bind_display(video, res, conn, ops);

			if (ret) {
				log_fatal("failed to bind drm display");
				return ret;
			}
		}

		drmModeFreeConnector(conn);

		ret = 0;
	}

	drmModeFreeResources(res);

	cm_list_foreach_safe(iter, tmp, &video->displays) {
		display = cm_list_entry(iter, struct app_display, link);
		if (!(display->flags & DISPLAY_AVAILABLE)) {
			app_display_unbind(display);
		}
	}

	return ret;
}

static void
display_event(int fd, unsigned int frame, unsigned int sec,
	      unsigned int usec, void *data)
{
	struct app_display_drm *disp = data;

	disp->pflip = 1;
}

static int
app_video_drm_read_events(struct app_video *video)
{
	struct app_video_drm *vdrm = video->data;
	drmEventContext ev;
	int ret;

	memset(&ev, 0, sizeof ev);
	ev.version = DRM_EVENT_CONTEXT_VERSION;
	ev.page_flip_handler = display_event;
	errno = 0;
	ret = drmHandleEvent(vdrm->fd, &ev);

	if (ret < 0 && errno != EAGAIN)
		return -EFAULT;

	return 0;
}

static int
drm_io_event(int fd, uint32_t mask, void *data)
{
	int ret;
	struct app_display *display;
	struct app_display_drm *ddrm;
	struct cm_list *iter;
	struct app_video *video = data;
	struct app_video_drm *vdrm = video->data;

	ret = app_video_drm_read_events(video);

	if (ret) {
		return ret;
	}

	cm_list_foreach(iter, &video->displays) {
		display = cm_list_entry(iter, struct app_display, link);
		ddrm = display->data;
		if (ddrm->pflip) {
			vdrm->page_flip_fun(display);

			display->frame_func(display);

			ddrm->pflip = 0;
		}

	}
}

int
app_display_drm_init(struct app_display *disp, void *data)
{
	struct app_display_drm *drmdisp;

	log_debug("initialising display")

	drmdisp = malloc(sizeof *drmdisp);
	if (!drmdisp) {
		log_fatal("failed to initialise display")
		return -EFAULT;
	}

	disp->data = drmdisp;
	drmdisp->data = data;
	memset(&drmdisp->mode_info, 0, sizeof drmdisp->mode_info);
	drmdisp->saved_crtc = NULL;
	drmdisp->conn_id = 0;
	drmdisp->crtc_id = 0;
	drmdisp->pflip = 0;

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

	log_debug("activating display")

	res = drmModeGetResources(vdrm->fd);
	if (!res) {
		log_fatal("drm: failed to get resources");
		return -EFAULT;
	}

	conn = drmModeGetConnector(vdrm->fd, ddrm->conn_id);
	if (!conn) {
		log_fatal("drm: failed to get connector");
		drmModeFreeResources(res);
		return -EFAULT;
	}

	crtc = -1;
	for (i = 0; i < conn->count_encoders; ++i) {
		enc = drmModeGetEncoder(vdrm->fd, conn->encoders[i]);
		if (!enc) {
			continue;
		}

		log_trace("using crtc_id: %d", enc->crtc_id)

		crtc = enc->crtc_id;

		drmModeFreeEncoder(enc);
		if (crtc >= 0)
			break;
	}

	drmModeFreeConnector(conn);
	drmModeFreeResources(res);

	if (crtc < 0) {
		log_fatal("drm: crtc not found");
		return -ENODEV;
	}

	ddrm->crtc_id = crtc;
	if (ddrm->saved_crtc) {
		drmModeFreeCrtc(ddrm->saved_crtc);
	}

	ddrm->saved_crtc = drmModeGetCrtc(vdrm->fd, ddrm->crtc_id);

	disp->flags |= DISPLAY_ACTIVATED;

	return 0;
}

int
app_display_drm_swap(struct app_display *disp, uint32_t fb)
{
	struct app_display_drm *ddrm = disp->data;
	struct app_video *video = disp->video;
	struct app_video_drm *vdrm = video->data;
	int ret;
	drmModeModeInfoPtr mode;

	ret = drmModePageFlip(vdrm->fd, ddrm->crtc_id, fb,
			      DRM_MODE_PAGE_FLIP_EVENT, ddrm);

	if (ret) {
		log_fatal("Cannot page flip on DRM CRTC %m");
		return -EFAULT;
	}

	return 0;
}

void
app_display_drm_deactivate(struct app_display *disp)
{
	struct app_video *video = disp->video;
	struct app_video_drm *vdrm = video->data;
	struct app_display_drm *ddrm = disp->data;

	log_debug("deactivating display")

	if (ddrm->saved_crtc) {
		log_trace("restoring saved crtc")
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

	disp->flags &= ~DISPLAY_ACTIVATED;
}

int
app_video_drm_init(struct app_video *app, app_drm_page_flip_t pageflip_func,
		   const char *node, void *data)
{
	struct app_video_drm *vdrm;
	int ret;

	log_debug("initialising video")

	vdrm = malloc(sizeof *vdrm);
	if (!vdrm) {
		log_fatal("failed to allocate video: no mem")
		return -ENOMEM;
	}

	memset(vdrm, 0, sizeof *vdrm);

	vdrm->page_flip_fun = pageflip_func;
	vdrm->data = data;
	app->data = vdrm;

	vdrm->fd = open(node, O_RDWR | O_CLOEXEC | O_NONBLOCK);
	if (vdrm->fd < 0) {
		log_fatal("Cannot open drm device %s : %m", node);
		ret = -EFAULT;
		goto err_drm_open;
	}

	drmDropMaster(vdrm->fd);

	struct ev_event_source *source =
			ev_event_loop_add_fd(
				app->evloop, EV_EVENT_READABLE,
				vdrm->fd,
				drm_io_event,
				app);

	if (!source) {
		log_fatal("failed to add drm fd to event loop");
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

	log_debug("destroying video")

	ev_event_source_remove(vdrm->ev_source);
	close(vdrm->fd);
	free(vdrm);
}

#endif // APP_VIDEO_DRM_C
