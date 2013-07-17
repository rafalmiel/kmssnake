
#include <stddef.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/epoll.h>
#include <sys/signalfd.h>
#include <sys/timerfd.h>
#include <unistd.h>
#include <assert.h>

#include "ev_loop.h"
#include "cm_utils.h"

struct ev_event_loop {
	int ref;
	int epoll_fd;
	int running;

	struct cm_list destroy_list;

	struct cm_signal destroy_signal;
};

struct ev_event_source_interface {
	int (*dispatch)(struct ev_event_source *source,
			struct epoll_event *ep);
};

struct ev_event_source {
	struct ev_event_source_interface *interface;
	struct ev_event_loop *loop;
	struct cm_list link;
	void *data;
	int fd;
};

struct ck_event_source_fd {
	struct ev_event_source base;
	ev_event_loop_fd_func_t func;
	int fd;
};
struct ck_event_source_timer {
	struct ev_event_source base;
	ev_event_loop_timer_func_t func;
};
struct ck_event_source_signal {
	struct ev_event_source base;
	int signum;
	ev_event_loop_signal_func_t func;
};

static int
ev_event_source_fd_dispatch(struct ev_event_source *src,
			    struct epoll_event *ep)
{
	struct ck_event_source_fd *source = (struct ck_event_source_fd *) src;

	uint32_t mask;

	mask = 0;
	if (ep->events & EPOLLIN)
		mask |= EV_EVENT_READABLE;
	if (ep->events & EPOLLOUT)
		mask |= EV_EVENT_WRITABLE;
	if (ep->events & EPOLLHUP)
		mask |= EV_EVENT_HANGUP;
	if (ep->events & EPOLLERR)
		mask |= EV_EVENT_ERROR;

	source->func(source->fd, mask, source->base.data);
}

static int
ev_event_source_timer_dispatch(struct ev_event_source *src,
			       struct epoll_event *ep)
{
	struct ck_event_source_timer *timer_source =
		(struct ck_event_source_timer *) src;
	uint64_t expires;
	int len;

	len = read(src->fd, &expires, sizeof expires);
	if (len != sizeof expires)
		/* Is there anything we can do here?  Will this ever happen? */
		fprintf(stderr, "timerfd read error: %m\n");

	timer_source->func(timer_source->base.data);
}

static int
ev_event_source_signal_dispatch(struct ev_event_source *src,
			      struct epoll_event *ep)
{
	struct ck_event_source_signal *signal_source =
			(struct ck_event_source_signal *) src;
	struct signalfd_siginfo siginfo;
	int len;

	len = read(src->fd, &siginfo, sizeof siginfo);
	if (len != sizeof siginfo) {
		/* Is there anything we can do here?  Will this ever happen? */
		fprintf(stderr, "signalfd read error: %m\n");
	}

	signal_source->func(signal_source->signum, src->data);
}

struct ev_event_source_interface timer_source_interface = {
	ev_event_source_timer_dispatch,
};

struct ev_event_source_interface fd_source_interface = {
	ev_event_source_fd_dispatch,
};

struct ev_event_source_interface signal_source_interface = {
	ev_event_source_signal_dispatch,
};

static struct ev_event_source*
add_source(struct ev_event_loop *loop,
	   struct ev_event_source *source,
	   uint32_t mask,
	   void *data)
{
	struct epoll_event ep;

	if (source->fd < 0) {
		free(source);
		return NULL;
	}

	memset(&ep, 0, sizeof ep);

	source->loop = loop;
	source->data = data;

	cm_list_init(&source->link);

	if (mask & EV_EVENT_READABLE)
		ep.events |= EPOLLIN;
	if (mask & EV_EVENT_WRITABLE)
		ep.events |= EPOLLOUT;

	ep.data.ptr = source;

	if (epoll_ctl(loop->epoll_fd, EPOLL_CTL_ADD, source->fd, &ep) < 0) {
		close(source->fd);
		free(source);
		return NULL;
	}

	return source;
}

static int
ev_event_loop_process_destroy_list(struct ev_event_loop *loop)
{
	struct ev_event_source *src;
	struct cm_list *iter, *tmp;

	cm_list_foreach_safe(iter, tmp, &loop->destroy_list) {
		src = cm_list_entry(iter, struct ev_event_source, link);
		free(src);
	}

	cm_list_init(&loop->destroy_list);
}

CM_EXPORT int
ev_event_source_timer_update(struct ev_event_source *timer, int ms, int interval)
{
	struct itimerspec spec;

	spec.it_interval.tv_sec = interval / 1000;
	spec.it_interval.tv_nsec = (interval % 1000) * 1000 * 1000;
	spec.it_value.tv_sec = ms / 1000;
	spec.it_value.tv_nsec = (ms % 1000) * 1000 * 1000;

	return timerfd_settime(timer->fd, 0, &spec, 0);
}

CM_EXPORT struct ev_event_source *
ev_event_loop_add_timer(struct ev_event_loop *loop,
			ev_event_loop_timer_func_t func,
			void *data)
{
	struct ck_event_source_timer *source;

	source = malloc(sizeof *source);
	if (source == NULL)
		return NULL;

	source->base.interface = &timer_source_interface;
	source->base.fd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC);

	if (source->base.fd < 0) {
		free(source);
		return NULL;
	}

	source->func = func;

	return add_source(loop, &source->base,
			  EV_EVENT_READABLE,
			  data);
}

CM_EXPORT struct ev_event_source *
ev_event_loop_add_fd(struct ev_event_loop *loop,
		     int mask,
		     int fd,
		     ev_event_loop_fd_func_t func,
		     void *data)
{
	struct ck_event_source_fd *source = NULL;

	source = malloc(sizeof *source);
	if (source == NULL) {
		return NULL;
	}

	source->base.interface = &fd_source_interface;
	source->base.fd = fcntl(fd, F_DUPFD_CLOEXEC, 0);
	source->fd = fd;
	source->func = func;

	return add_source(loop, &source->base, mask, data);
}

CM_EXPORT struct ev_event_source *
ev_event_loop_add_signal(struct ev_event_loop *loop, int signum,
			 ev_event_loop_signal_func_t func,
			 void *data)
{
	struct ck_event_source_signal *signal_source;
	sigset_t sigset;

	signal_source = malloc(sizeof *signal_source);
	if (!signal_source)
		return NULL;

	signal_source->base.interface = &signal_source_interface;
	signal_source->signum = signum;
	signal_source->func = func;

	sigemptyset(&sigset);
	sigaddset(&sigset, signum);
	signal_source->base.fd = signalfd(-1, &sigset, SFD_CLOEXEC);
	sigprocmask(SIG_BLOCK, &sigset, NULL);

	return add_source(loop, &signal_source->base, EV_EVENT_READABLE, data);
}

CM_EXPORT int
ev_event_source_remove(struct ev_event_source *src)
{
	struct ev_event_loop *loop = src->loop;

	if (src->fd >= 0) {
		epoll_ctl(loop->epoll_fd, EPOLL_CTL_DEL, src->fd, NULL);
		close(src->fd);
		src->fd = -1;
	}

	cm_list_insert(&loop->destroy_list, &src->link);

	return 0;
}

CM_EXPORT int
ev_event_loop_dispatch(struct ev_event_loop* loop, int timeout)
{
	struct epoll_event ep[32];
	struct ev_event_source *source;
	int count, i;

	count = epoll_wait(loop->epoll_fd, ep, 32, -1);

	for (i = 0; i < count; ++i) {
		source = ep[i].data.ptr;
		if (source->fd != -1) {
			source->interface->dispatch(source, &ep[i]);
		}
	}

	ev_event_loop_process_destroy_list(loop);

	return 0;
}

CM_EXPORT int
ev_event_loop_stop(struct ev_event_loop *loop)
{
	loop->running = 0;
}

CM_EXPORT int
ev_event_loop_run(struct ev_event_loop *loop)
{
	loop->running = 1;
	while (loop->running) {
		if (ev_event_loop_dispatch(loop, -1) < 0)
			break;
	}

	return 0;
}

CM_EXPORT void
ev_event_loop_ref(struct ev_event_loop *loop)
{
	if (!loop || !loop->ref)
		return;

	++loop->ref;
}

CM_EXPORT void
ev_event_loop_unref(struct ev_event_loop *loop)
{
	if (!loop || !loop->ref || --loop->ref)
		return;

	cm_signal_emit(&loop->destroy_signal, loop);

	ev_event_loop_process_destroy_list(loop);
	close(loop->epoll_fd);
	free(loop);
}

CM_EXPORT struct ev_event_loop*
ev_event_loop_create(void)
{
	struct ev_event_loop *loop = NULL;

	loop = malloc(sizeof *loop);
	if (loop == NULL) {
		return NULL;
	}

	loop->epoll_fd = epoll_create1(EPOLL_CLOEXEC);
	if (loop->epoll_fd < 0) {
		free(loop);
		return NULL;
	}
	loop->running = 0;
	loop->ref = 1;
	cm_signal_init(&loop->destroy_signal);
	cm_list_init(&loop->destroy_list);

	return loop;
}

CM_EXPORT void
ev_event_loop_add_destroy_listener(struct ev_event_loop *loop,
					struct cm_listener *listener)
{
	cm_signal_add_listener(&loop->destroy_signal, listener);
}


