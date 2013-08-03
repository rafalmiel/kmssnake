#ifndef APP_VIDEO_INTERNAL_H
#define APP_VIDEO_INTERNAL_H

#include "app_video.h"
#include <utils/cm_utils.h>

struct ev_event_loop;
struct app_video;

typedef void (*frame_func_t)(struct app_display *);

#define DISPLAY_AVAILABLE 0x1

struct app_display {
	struct cm_list link;
	unsigned int ref;
	struct app_video *video;
	const struct app_display_ops *ops;
	frame_func_t frame_func;
	int frame_cnt;
	int disp_id;
	unsigned int flags;

	void *data;
};

struct app_video {
	unsigned int ref;
	struct ev_event_loop *evloop;
	const struct app_video_ops *ops;
	struct cm_list displays;

	void *data;
};

struct app_video_ops {
	int (*init)(struct app_video *, const char *node);
	void (*destroy)(struct app_video *);
	int (*wake_up)(struct app_video *);
	int (*poll)(struct app_video *);
};

struct app_display_ops {
	int (*init)(struct app_display *);
	int (*activate)(struct app_display *);
	int (*swap)(struct app_display *);
	void (*deactivate)(struct app_display *);
};

int app_display_bind(struct app_video *video, struct app_display *display);
void app_display_unbind(struct app_display *display);

#endif // APP_VIDEO_INTERNAL_H
