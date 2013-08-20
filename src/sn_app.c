#include <stdlib.h>
#include <signal.h>
#include <libudev.h>
#include <string.h>
#include <fcntl.h>

#include "sn_app.h"

#include "utils/cm_utils.h"
#include "utils/cm_log.h"
#include "utils/ev_loop.h"
#include "video/app_video.h"

#define LOG_SUBSYSTEM "app"

struct sn_app {
	int ref;
	struct ev_event_loop *event_loop;

	struct ev_event_source *signal_source;
	struct ev_event_source *timer_source;
	struct app_video *video;

	struct udev *udev;
	struct udev_monitor *udev_mon;
};

static int
sn_app_signal_handler(int signal, void *data)
{
	struct sn_app *app = data;

	if (signal == SIGINT) {
		log_info("SIGINT signal caught")
		ev_event_loop_stop(app->event_loop);
		ev_event_source_remove(app->signal_source);
	}
}

static int
sn_app_timer_poll_handler(void *data)
{
	struct sn_app* app = data;

	log_trace("timer poll")

	app_video_poll(app->video);
}

static struct udev_device *
find_primary_gpu(struct sn_app *app)
{
	struct udev_enumerate *e;
	struct udev_device *device, *drm_device, *pci;
	struct udev_list_entry *entry;
	const char *path, *bvga;

	e = udev_enumerate_new(app->udev);
	udev_enumerate_add_match_subsystem(e, "drm");
	udev_enumerate_add_match_sysname(e, "card[0-9]*");

	udev_enumerate_scan_devices(e);

	drm_device = NULL;
	udev_list_entry_foreach(entry, udev_enumerate_get_list_entry(e)) {
		path = udev_list_entry_get_name(entry);
		device = udev_device_new_from_syspath(app->udev, path);

		pci = udev_device_get_parent_with_subsystem_devtype(device,
								    "pci", NULL);

		if (pci) {
			bvga = udev_device_get_sysattr_value(pci, "boot_vga");

			if (bvga != NULL && !strcmp(bvga, "1")) {
				if (drm_device) {
					udev_device_unref(drm_device);
				}
				drm_device = device;
				break;
			}
		}

		if (!drm_device) {
			drm_device = device;
		} else {
			udev_device_unref(device);
		}
	}

	udev_enumerate_unref(e);
	return drm_device;
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
	udev_unref(app->udev);
	ev_event_loop_unref(app->event_loop);
	free(app);
}

static int
udev_monitor_fun(int fd, uint32_t mask, void *data)
{
	struct sn_app *app = data;

	log_info("=========== drm change event")

	udev_monitor_receive_device(app->udev_mon);

	app_video_poll(app->video);
}

static void
init_udev_monitor(struct sn_app *app)
{
	int ufd, set;

	app->udev_mon = udev_monitor_new_from_netlink(app->udev, "udev");

	udev_monitor_filter_add_match_subsystem_devtype(app->udev_mon,
							"drm", "drm_minor");

	udev_monitor_enable_receiving(app->udev_mon);

	ufd = udev_monitor_get_fd(app->udev_mon);

	set = fcntl(ufd, F_GETFL);
	set |= O_NONBLOCK;
	fcntl(ufd, F_SETFL, set);

	ev_event_loop_add_fd(app->event_loop, EV_EVENT_READABLE, ufd,
			     udev_monitor_fun, app);
}

CM_EXPORT struct sn_app *
sn_app_create(void)
{
	struct sn_app *app;
	struct udev_device *drm_dev;
	const char *node;

	app = malloc(sizeof *app);
	if (!app)
		return NULL;

	app->ref = 1;
	app->event_loop = ev_event_loop_create();

	app->udev = udev_new();

	init_udev_monitor(app);

	if (!app->udev) {
		log_fatal("failed to create udev")
		goto err_udev;
	}

	drm_dev = find_primary_gpu(app);

	if (!drm_dev) {
		log_fatal("failed to find primary gpu")
		goto err_udev;
	}

	node = udev_device_get_devnode(drm_dev);

	log_info("found primary gpu: %s", node)

	app->video = app_video_create(app->event_loop, node);

	if (!app->video) {
		log_fatal("failed to init the video");
		goto err_video;
	}

	struct ev_event_source *sigsrc =
			ev_event_loop_add_signal(app->event_loop,
						 SIGINT,
						 sn_app_signal_handler,
						 app);
	if (!sigsrc) {
		log_fatal("failed to add source to event loop");
		goto err_evloop;
	}

	app->signal_source = sigsrc;

	/*struct ev_event_source *timsrc =
			ev_event_loop_add_timer(app->event_loop,
						 sn_app_timer_poll_handler,
						 app);
	ev_event_source_timer_update(timsrc, 1000, 3000);

	app->timer_source = timsrc;*/

	udev_device_unref(drm_dev);

	return app;


err_evloop:
	app_video_unref(app->video);
err_video:
	udev_unref(app->udev);
err_udev:
	ev_event_loop_unref(app->event_loop);
	free(app);
	return NULL;
}


CM_EXPORT void
sn_app_run(struct sn_app *app)
{
	ev_event_loop_run(app->event_loop);
}
