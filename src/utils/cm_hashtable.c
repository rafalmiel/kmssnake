#include <stdlib.h>
#include <glib.h>
#include "cm_hashtable.h"

struct cm_hashtable {
	int ref;
	GHashTable *hashtable;
};

CM_EXPORT uint32_t
cm_direct_hash(const void *d)
{
	return g_direct_hash(d);
}

CM_EXPORT uint32_t
cm_int_hash(const void *d)
{
	return g_int64_hash(d);
}

CM_EXPORT uint32_t
cm_int64_hash(const void *d)
{
	return g_int64_hash(d);
}

CM_EXPORT uint32_t
cm_double_hash(const void *d)
{
	return g_double_hash(d);
}

CM_EXPORT uint32_t
cm_str_hash(const void *d)
{
	return g_str_hash(d);
}

CM_EXPORT int
cm_direct_equal(const void *d1, const void *d2)
{
	return g_direct_equal(d1, d2);
}

CM_EXPORT int
cm_int_equal(const void *d1, const void *d2)
{
	return g_int_equal(d1, d2);
}

CM_EXPORT int
cm_int64_equal(const void *d1, const void *d2)
{
	return g_int64_equal(d1, d2);
}

CM_EXPORT int
cm_double_equal(const void *d1, const void *d2)
{
	return g_double_equal(d1, d2);
}

CM_EXPORT int
cm_str_equal(const void *d1, const void *d2)
{
	return g_string_equal(d1, d2);
}

CM_EXPORT void
cm_hashtable_ref(struct cm_hashtable *hashtable)
{
	if (!hashtable || !hashtable->ref)
		return;

	++hashtable->ref;
}

CM_EXPORT void
cm_hashtable_unref(struct cm_hashtable *hashtable)
{
	if (!hashtable || !hashtable->ref || --hashtable->ref)
		return;

	g_hash_table_unref(hashtable->hashtable);
	free(hashtable);
}

CM_EXPORT struct cm_hashtable *
cm_hashtable_create(cm_hash_func hash_func,
		    cm_hash_equal_func equal_func)
{
	struct cm_hashtable *hashtable;

	hashtable = malloc(sizeof *hashtable);

	if (!hashtable)
		return NULL;

	hashtable->ref = 1;
	hashtable->hashtable = g_hash_table_new(hash_func,
						equal_func);

	return hashtable;
}

CM_EXPORT struct cm_hashtable *
cm_hashtable_create_full(cm_hash_func hash_func,
			 cm_hash_equal_func equal_func,
			 cm_hash_destroy_func key_destructor,
			 cm_hash_destroy_func value_destructor)
{
	struct cm_hashtable *hashtable;

	hashtable = malloc(sizeof *hashtable);

	if (!hashtable)
		return NULL;

	hashtable->ref = 1;
	hashtable->hashtable = g_hash_table_new_full(hash_func,
						     equal_func,
						     key_destructor,
						     value_destructor);

	return hashtable;
}

CM_EXPORT void
cm_hashtable_insert(struct cm_hashtable *hashtable,
		    void *key,
		    void *value)
{
	g_hash_table_insert(hashtable->hashtable,
			    key,
			    value);
}

CM_EXPORT void *
cm_hashtable_lookup(struct cm_hashtable *hashtable,
			 const void *key)
{
	return g_hash_table_lookup(hashtable->hashtable,
				   key);
}

CM_EXPORT int
cm_hashtable_remove(struct cm_hashtable *hashtable,
		    const void *key)
{
	return g_hash_table_remove(hashtable->hashtable,
				   key);
}

CM_EXPORT uint32_t
cm_hashtable_size(struct cm_hashtable *hashtable)
{
	return g_hash_table_size(hashtable->hashtable);
}
