#ifndef EV_LOOP_H
#define EV_LOOP_H

#include <stdint.h>
#include "cm_utils.h"

enum {
	EV_EVENT_READABLE = 0x01,
	EV_EVENT_WRITABLE = 0x02,
	EV_EVENT_HANGUP   = 0x04,
	EV_EVENT_ERROR    = 0x08
};

struct ev_event_loop;
struct ev_event_source;

typedef int (*ev_event_loop_fd_func_t)(int fd, uint32_t mask, void *data);
typedef int (*ev_event_loop_timer_func_t)(void *data);
typedef int (*ev_event_loop_signal_func_t)(int signum, void *data);

struct ev_event_loop *ev_event_loop_create(void);

void ev_event_loop_ref(struct ev_event_loop *loop);
void ev_event_loop_unref(struct ev_event_loop *loop);

struct ev_event_source *ev_event_loop_add_fd(struct ev_event_loop *loop,
					     int mask,
					     int fd,
					     ev_event_loop_fd_func_t func,
					     void *data);

struct ev_event_source *ev_event_loop_add_timer(struct ev_event_loop *loop,
						ev_event_loop_timer_func_t func,
						void *data);

struct ev_event_source *ev_event_loop_add_signal(struct ev_event_loop *loop,
						 int signum,
						 ev_event_loop_signal_func_t func,
						 void *data);

int ev_event_source_timer_update(struct ev_event_source *timer,
				 int ms, int interval);

int ev_event_source_remove(struct ev_event_source *src);

int ev_event_loop_dispatch(struct ev_event_loop *loop, int timeout);

int ev_event_loop_run(struct ev_event_loop *loop);

int ev_event_loop_stop(struct ev_event_loop *loop);

void ev_event_loop_add_destroy_listener(struct ev_event_loop *loop,
					struct cm_listener *listener);

#endif // EV_LOOP_H
