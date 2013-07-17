#ifndef APP_VIDEO_DRM_INTERNAL_H
#define APP_VIDEO_DRM_INTERNAL_H

#include <xf86drm.h>
#include <xf86drmMode.h>

struct app_video;
struct ev_event_source;

typedef void (*app_drm_page_flip_t)(struct app_video *);

struct app_video_drm {
	int fd;
	struct ev_event_source *ev_source;
	app_drm_page_flip_t page_flip_fun;
	void *data;
};

int app_video_drm_init(struct app_video *app, app_drm_page_flip_t pageflip_func,
			const char *node, void *data);

void app_video_drm_destroy(struct app_video *app);

#endif // APP_VIDEO_DRM_INTERNAL_H
