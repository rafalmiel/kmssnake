#ifndef SN_APP_H
#define SN_APP_H

struct sn_app;

struct sn_app *sn_app_create(void);

void sn_app_run(struct sn_app *app);
void sn_app_ref(struct sn_app* app);
void sn_app_unref(struct sn_app* app);

#endif // SN_APP_H
