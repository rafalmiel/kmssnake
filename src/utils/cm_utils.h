#ifndef CM_LIST_H
#define CM_LIST_H

#ifndef CK_LIST_H
#define CK_LIST_H

#include <math.h>
#include <stddef.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>

#ifdef  __cplusplus
extern "C" {
#endif

#if defined(__GNUC__) && __GNUC__ >= 4
#define CM_EXPORT __attribute__ ((visibility("default")))
#else
#define CM_EXPORT
#endif

struct cm_list {
	struct cm_list *prev;
	struct cm_list *next;
};

void cm_list_init(struct cm_list *list);
void cm_list_insert(struct cm_list *list, struct cm_list *elm);
void cm_list_remove(struct cm_list *elm);
int cm_list_length(const struct cm_list *list);
int cm_list_empty(const struct cm_list *list);
void cm_list_insert_list(struct cm_list *list, struct cm_list *other);

#define cm_container_of(ptr, type, member) ({					\
	const __typeof__( ((type *)0)->member ) *__mptr = (ptr);		\
	(type *)( (char *)__mptr - offsetof(type,member) );})

#define cm_list_entry(ptr, type, member)					\
	cm_container_of((ptr), type, member)

#define cm_list_foreach(iter, head)						\
	for (iter = (head)->next;						\
	iter != (head);							\
	iter = iter->next)							\

#define cm_list_foreach_safe(pos, tmp, head)					\
	for (pos = (head)->next,						\
	tmp = pos->next;							\
	pos != (head);							\
	pos = tmp,								\
	tmp = pos->next)							\


#define cm_list_foreach_reverse(pos, head)					\
	for (pos = (head)->prev;						\
	pos != (head);							\
	pos = pos->prev)							\

#define cm_list_foreach_reverse_safe(pos, tmp, head, member)			\
	for (pos = (head)->prev,						\
	tmp = pos->prev;							\
	pos != (head);							\
	pos = tmp,								\
	tmp = pos->prev)							\

struct cm_listener;
typedef void (*cm_notify_func_t)(struct cm_listener* listener, void *data);

struct cm_listener {
	struct cm_list link;
	cm_notify_func_t func;
};

struct cm_signal {
	struct cm_list listeners;
};

static void inline
cm_signal_init(struct cm_signal *signal)
{
	cm_list_init(&signal->listeners);
}

static void inline
cm_signal_add_listener(struct cm_signal *signal, struct cm_listener *listener)
{
	cm_list_insert(signal->listeners.prev, &listener->link);
}

static void inline
cm_signal_emit(struct cm_signal *signal, void *data)
{
	struct cm_listener *listener;
	struct cm_list *iter, *tmp;
	cm_list_foreach_safe(iter, tmp, &signal->listeners) {
		listener = cm_list_entry(iter, struct cm_listener, link);
		listener->func(listener, data);
	}
}


#ifdef  __cplusplus
}
#endif

#endif // CK_LIST_H


#endif // CM_LIST_H
