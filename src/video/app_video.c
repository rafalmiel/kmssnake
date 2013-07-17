#include <stdlib.h>
#include <utils/cm_utils.h>
#include "app_video.h"
#include "app_video_internal.h"
#include "app_video_gldrm.h"

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
	cm_list_init(&app_video->displays);

	ev_event_loop_ref(evloop);

	//TODO: choose backend based on config
	ret = app_video_gldrm_init(app_video);

	if (ret) {
		fprintf(stderr, "app_vid: failed to init gldrm\n");
		ev_event_loop_unref(evloop);
		free(app_video);
		return NULL;
	}

	app_video->ops->init(app_video, node);

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

	app->ops->destroy(app);
	ev_event_loop_unref(app->evloop);
	free(app);
}
