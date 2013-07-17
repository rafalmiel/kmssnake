#ifndef SN_APP_VIDEO_H
#define SN_APP_VIDEO_H

#include <utils/ev_loop.h>

struct app_video;

struct app_video *app_video_create(struct ev_event_loop *evloop, const char *node);

void app_video_ref(struct app_video *app_video);
void app_video_unref(struct app_video *app_video);

#endif // SN_APP_VIDEO_H
