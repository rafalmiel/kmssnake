#include <stdio.h>
#include <stdlib.h>

#include "utils/ev_loop.h"

#define LOG_SUBSYSTEM "main"

#include "utils/cm_log.h"

#include "sn_app.h"

int main(int argc, char *argv[])
{
	log_configure(argc, argv);

	log_debug("test %d haha", 33);

	struct sn_app *app = sn_app_create();

	if (!app) {
		log_fatal("failed to create the app\n");
		return 1;
	}

	sn_app_run(app);

	printf("That's all folks!\n");

	sn_app_unref(app);
	return 0;
}
