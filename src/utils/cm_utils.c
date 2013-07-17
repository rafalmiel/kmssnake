#include "cm_utils.h"

CM_EXPORT void
cm_list_init(struct cm_list *list)
{
	list->next = list;
	list->prev = list;
}

CM_EXPORT void
cm_list_insert(struct cm_list *list, struct cm_list *elm)
{
	elm->prev = list;
	elm->next = list->next;
	list->next = elm;
	elm->next->prev = elm;
}

CM_EXPORT void
cm_list_remove(struct cm_list *elm)
{
	elm->next->prev = elm->prev;
	elm->prev->next = elm->next;
	elm->next = NULL;
	elm->prev = NULL;
}

CM_EXPORT int
cm_list_length(const struct cm_list *list)
{
	struct cm_list *elm = list->next;

	int c = 0;
	while (elm != list) {
		c++;
		elm = elm->next;
	}

	return c;
}

CM_EXPORT int
cm_list_empty(const struct cm_list *list)
{
	return list->next == list;
}

CM_EXPORT void
cm_list_insert_list(struct cm_list *list, struct cm_list *other)
{
	if (cm_list_empty(other))
		return;

	other->next->prev = list;
	other->prev->next = list->next;
	list->next->prev = other->prev;
	list->next = other->next;
}

