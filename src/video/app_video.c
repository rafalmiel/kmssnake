#include <stdlib.h>
#include <utils/cm_utils.h>
#include <utils/cm_log.h>
#include <GL/gl.h>
#include <GL/glext.h>
#include "string.h"
#include "app_video.h"
#include "app_video_internal.h"
#include "app_video_gldrm.h"

#define LOG_SUBSYSTEM "video"

static int frame = 0;

#define PI 3.14159265359

static void
drawCircle(int px, int py, int width, int height) {
	glBegin(GL_TRIANGLE_FAN);
	int num_segments = 360;
	int ii = 0;
	for(ii = 0; ii < num_segments; ii++)
	{
		float theta = 2.0f * 3.1415926f * (float)ii / (float)num_segments;//get the current angle

		float x = width * cosf(theta);//calculate the x component
		float y = height * sinf(theta);//calculate the y component

		glVertex2f(x + px, y + py);//output vertex

	}
	glEnd();
}

static int t = 0;

static int sideA = 0;
static int wasChange = 0;

static void
draw(uint32_t i, int sp)
{
	glClearColor(0.8, 0.8, 0.8, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);     // Clear The Screen And The Depth Buffer
	glLoadIdentity();

	int moneta_width = 300;
	int moneta_height = 300;
	int posX = 1920/2;// - moneta_width/2;
	int posY = 1080/2;// - 50 - moneta_height;


	double speed = fabs(cosf(2.0*PI*t/(2.0*360.0)));
	t += sp;
	t = t % 360;

	if (!wasChange && t >= 180) {
		sideA = !sideA;
		wasChange = 1;
	} else if (wasChange && t < 180) {
		wasChange = 0;
	}

	moneta_width = moneta_height * speed;


	//brzeg start
	glColor3f(0,0,0);

	int sideWidth= 0;
	int sideRight = 0;
	int sideLeft= 0;
	int side = 20;

	if (t <= 180) {
		sideWidth = (int) floor((t / 180.0)*side );
		sideRight  = 1;
	}
	if (t >= 180){
		sideWidth = (int)floor((side *2.0 - (t / 180.0)*side ));
		sideLeft = 1;
	}

	if (sideLeft) {
		drawCircle(posX - sideWidth, posY, moneta_width, moneta_height);
	}
	if (sideRight ) {
		drawCircle(posX + sideWidth, posY, moneta_width, moneta_height);
	}

	glRecti(1920/2 - sideWidth, posY - moneta_height, 1920/2 + sideWidth, posY + moneta_height);

	if (sideA) glColor3f(1.0, 0, 0);
	else glColor3f(0, 0, 1.00);;

	if (sideLeft) {
		drawCircle(posX + sideWidth, posY, moneta_width, moneta_height);
	} else if (sideRight ) {
		drawCircle(posX - sideWidth, posY, moneta_width, moneta_height);
	}


	glFlush();
}

static void
display_frame(struct app_display *disp)
{
	draw(frame++, 3);
	disp->ops->swap(disp);
}

struct app_display *
app_display_create(const struct app_display_ops *ops)
{
	struct app_display *display;
	int ret;

	log_debug("creating app_display")

	display = malloc(sizeof *display);
	if (!display) {
		return NULL;
	}

	memset(display, 0, sizeof *display);

	display->ref = 1;
	display->data = NULL;
	display->ops = ops;
	display->frame_func = display_frame;

	ret = display->ops->init(display);

	if (ret) {
		log_fatal("failed to create display");
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

	log_debug("creating app_video")

	app_video = malloc(sizeof *app_video);
	if (!app_video) {
		log_fatal("failed to create video: no mem");
		return NULL;
	}

	app_video->ref = 1;
	app_video->evloop = evloop;
	app_video->display = NULL;

	ev_event_loop_ref(evloop);

	//TODO: choose backend based on config
	ret = app_video_gldrm_init(app_video);

	if (ret) {
		log_fatal("failed to init gldrm");
		ev_event_loop_unref(evloop);
		free(app_video);
		return NULL;
	}

	ret = app_video->ops->init(app_video, node);

	if (ret) {
		log_fatal("failed to init the video");
		free(app_video);
		return NULL;
	}

	ret = app_video->ops->wake_up(app_video);

	if (ret) {
		log_fatal("failed to wake up video");
		app_video->ops->destroy(app_video);
		return NULL;
	}

	ret = app_video->display->ops->activate(app_video->display);

	if (ret) {
		log_fatal("failed to activate display");
		return NULL;
	}

	glClearColor(0.8, 0.8, 0.8, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	app_video->display->ops->swap(app_video->display);

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

