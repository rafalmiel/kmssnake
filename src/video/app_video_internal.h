#ifndef APP_VIDEO_INTERNAL_H
#define APP_VIDEO_INTERNAL_H

#include "app_video.h"
#include <utils/cm_utils.h>

struct ev_event_loop;
struct app_video;

struct app_display {
	unsigned int ref;
	struct app_video *video;
	const struct app_display_ops *ops;

	void *data;
};

struct app_video {
	unsigned int ref;
	struct ev_event_loop *evloop;
	const struct app_video_ops *ops;
	//lets just support one display for now
	struct app_display *display;

	void *data;
};

struct app_video_ops {
	int (*init)(struct app_video *, const char *node);
	void (*destroy)(struct app_video *);
	void (*wake_up)(struct app_video *);
};

struct app_display_ops {
	int (*init)(struct app_display *);
	void (*activate)(struct app_display *);
	void (*swap)(struct app_display *);
	void (*deactivate)(struct app_display *);
};

#endif // APP_VIDEO_INTERNAL_H
