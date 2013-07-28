#include <stdio.h>
#include <stdlib.h>

#include "utils/ev_loop.h"
#include "utils/cm_log.h"
#include "sn_app.h"

#define LOG_SUBSYSTEM "main"

int main(int argc, char *argv[])
{
    cm_log_configure(argc, argv);

	struct sn_app *app = sn_app_create();

	if (!app) {
		log_fatal("failed to create the app\n");
		return 1;
	}

	log_info("running the app")
	sn_app_run(app);

	log_info("finishing the app");

	sn_app_unref(app);
	return 0;
}
