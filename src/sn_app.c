#include <stdlib.h>
#include <signal.h>

#include "sn_app.h"

#include "utils/cm_utils.h"
#include "utils/ev_loop.h"

#include "video/app_video.h"

struct sn_app {
	int ref;
	struct ev_event_loop *event_loop;

	struct ev_event_source *signal_source;
	struct app_video *video;
};

static int
sn_app_signal_handler(int signal, void *data)
{
	struct sn_app *app = data;

	if (signal == SIGINT) {
		ev_event_loop_stop(app->event_loop);
		ev_event_source_remove(app->signal_source);
	}
}

CM_EXPORT void
sn_app_ref(struct sn_app* app)
{
	if (!app || !app->ref)
		return;

	++app->ref;
}

CM_EXPORT void
sn_app_unref(struct sn_app* app)
{
	if (!app || !app->ref || --app->ref)
		return;

	app_video_unref(app->video);
	ev_event_loop_unref(app->event_loop);
	free(app);
}

CM_EXPORT struct sn_app *
sn_app_create(void)
{
	struct sn_app *app;

	app = malloc(sizeof *app);
	if (!app)
		return NULL;

	app->ref = 1;
	app->event_loop = ev_event_loop_create();
	app->video = app_video_create(app->event_loop, "/dev/dri/card0");

	if (!app->video) {
		fprintf(stderr, "app: failed to init video\n");
		ev_event_loop_unref(app->event_loop);
		free(app);
		return NULL;
	}

	struct ev_event_source *sigsrc =
			ev_event_loop_add_signal(app->event_loop,
						 SIGINT,
						 sn_app_signal_handler,
						 app);
	if (!sigsrc) {
		fprintf(stderr, "app: failed to add signal src\n");
		app_video_unref(app->video);
		ev_event_loop_unref(app->event_loop);
		return NULL;
	}

	app->signal_source = sigsrc;

	return app;
}


CM_EXPORT void
sn_app_run(struct sn_app *app)
{
	ev_event_loop_run(app->event_loop);
}
