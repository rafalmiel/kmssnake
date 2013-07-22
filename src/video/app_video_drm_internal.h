#ifndef APP_VIDEO_DRM_INTERNAL_H
#define APP_VIDEO_DRM_INTERNAL_H

#include <xf86drm.h>
#include <xf86drmMode.h>

#include "app_video_internal.h"

struct app_video;
struct app_display;
struct ev_event_source;

typedef void (*app_drm_page_flip_t) (struct app_display *);

struct app_display_drm {
	uint32_t conn_id;
	int crtc_id;
	drmModeCrtc *saved_crtc;
	drmModeModeInfo mode_info;
	void *data;
};

struct app_video_drm {
	int fd;
	struct ev_event_source *ev_source;
	app_drm_page_flip_t page_flip_fun;
	void *data;
};

int app_display_drm_init(struct app_display *disp, void *data);

int app_display_drm_activate(struct app_display *disp);

void app_display_drm_deactivate(struct app_display *disp);

int app_display_drm_swap(struct app_display *disp, uint32_t fb);

static inline void *app_display_drm_get_data(struct app_display *disp)
{
	struct app_display_drm *d = disp->data;

	return d->data;
}

int app_video_drm_wake_up(struct app_video *video, const struct app_display_ops *ops);

int app_video_drm_hotplug(struct app_video *video, const struct app_display_ops *ops);

int app_video_drm_init(struct app_video *app, app_drm_page_flip_t pageflip_func,
			const char *node, void *data);

void app_video_drm_destroy(struct app_video *app);

static inline void *app_video_drm_get_data(struct app_video *video)
{
	struct app_video_drm *v = video->data;

	return v->data;
}


#endif // APP_VIDEO_DRM_INTERNAL_H
