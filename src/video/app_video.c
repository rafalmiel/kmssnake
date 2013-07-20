#include <stdlib.h>
#include <utils/cm_utils.h>
#include "app_video.h"
#include "app_video_internal.h"
#include "app_video_gldrm.h"

struct app_display *
app_display_create(const struct app_display_ops *ops)
{
	struct app_display *display;
	int ret;

	display = malloc(sizeof *display);
	if (!display) {
		return NULL;
	}

	display->ref = 1;
	display->data = NULL;
	display->ops = ops;

	ret = display->ops->init(display);

	if (ret) {
		fprintf(stderr, "Display init failed\n");
		free(display);
		return NULL;
	}

	return display;
}

CM_EXPORT void
app_display_ref(struct app_display *app_display)
{
	if (!app_display || !app_display->ref)
		return;

	++app_display->ref;
}

CM_EXPORT void
app_display_unref(struct app_display *app_display)
{
	if (!app_display || !app_display->ref || --app_display->ref)
		return;

	app_display->ops->deactivate(app_display);
	free(app_display);
}

CM_EXPORT struct app_video *
app_video_create(struct ev_event_loop *evloop, const char *node)
{
	struct app_video *app_video;
	int ret;

	app_video = malloc(sizeof *app_video);
	if (!app_video) {
		fprintf(stderr, "app_vid: failed to allocate video\n");
		return NULL;
	}

	app_video->ref = 1;
	app_video->evloop = evloop;
	app_video->display = NULL;

	ev_event_loop_ref(evloop);

	//TODO: choose backend based on config
	ret = app_video_gldrm_init(app_video);

	if (ret) {
		fprintf(stderr, "app_vid: failed to init gldrm\n");
		ev_event_loop_unref(evloop);
		free(app_video);
		return NULL;
	}

	ret = app_video->ops->init(app_video, node);

	if (ret) {
		free(app_video);
		return NULL;
	}

	fprintf(stderr, "waking up wideo\n");
	app_video->ops->wake_up(app_video);
	fprintf(stderr, "activating display\n");
	fprintf(stderr, "Display ptr %p\n", app_video->display);
	ret = app_video->display->ops->activate(app_video->display);

	if (ret) {
		fprintf(stderr, "app: display activate failed\n");
		return NULL;
	}

	return app_video;
}

CM_EXPORT void
app_video_ref(struct app_video* app)
{
	if (!app || !app->ref)
		return;

	++app->ref;
}

CM_EXPORT void
app_video_unref(struct app_video* app)
{
	if (!app || !app->ref || --app->ref)
		return;

	if (app->display)
		app_display_unref(app->display);
	app->ops->destroy(app);
	ev_event_loop_unref(app->evloop);
	free(app);
}

