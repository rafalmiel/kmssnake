#ifndef APP_VIDEO_GLDRM_H
#define APP_VIDEO_GLDRM_H

struct app_video;
struct app_display;

int app_display_gldrm_init(struct app_display *app_display);

int app_video_gldrm_init(struct app_video *app_video);

#endif // APP_VIDEO_GLDRM_H
