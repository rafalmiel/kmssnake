#ifndef SN_APP_VIDEO_H
#define SN_APP_VIDEO_H

#include <utils/ev_loop.h>

struct app_video;
struct app_display;
struct app_display_ops;

struct app_display *app_display_create(const struct app_display_ops *ops);

void app_display_ref(struct app_display *app_display);
void app_display_unref(struct app_display *app_display);

struct app_video *app_video_create(struct ev_event_loop *evloop, const char *node);

void app_video_ref(struct app_video *app_video);
void app_video_unref(struct app_video *app_video);

#endif // SN_APP_VIDEO_H
