#include <stdio.h>
#include <stdlib.h>

#include "utils/ev_loop.h"
#include "sn_app.h"

int main()
{
	struct sn_app *app = sn_app_create();

	if (!app) {
		return 1;
	}

	sn_app_run(app);

	printf("Hello world\n");

	sn_app_unref(app);
	return 0;
}
