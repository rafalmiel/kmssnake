#ifndef APP_VIDEO_INTERNAL_H
#define APP_VIDEO_INTERNAL_H

#include "app_video.h"
#include <utils/cm_utils.h>

struct ev_event_loop;

struct app_display {
	struct cm_list link;

	void *data;
};

struct app_video {
	int ref;
	struct ev_event_loop *evloop;
	const struct app_video_ops *ops;
	struct cm_list displays;

	void *data;
};

typedef void (*app_drm_page_flip_t) (struct app_video *);

struct app_video_ops {
	int (*init)(struct app_video *, const char *node);
	void (*destroy)(struct app_video *);
};

struct app_display_ops {
	int (*init)(struct app_display *);
	void (*activate)(struct app_display *);
	void (*swap)(struct app_display *);
};

#endif // APP_VIDEO_INTERNAL_H
