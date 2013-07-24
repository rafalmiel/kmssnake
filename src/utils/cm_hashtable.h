#ifndef CM_HASHTABLE_H
#define CM_HASHTABLE_H

#include <stdint.h>
#include "cm_utils.h"

typedef uint32_t (*cm_hash_func)(const void *);
typedef int (*cm_hash_equal_func)(const void *, const void *);
typedef void (*cm_hash_destroy_func)(void *);

struct cm_hashtable;

uint32_t cm_direct_hash(const void *);
uint32_t cm_int_hash(const void *);
uint32_t cm_int64_hash(const void *);
uint32_t cm_double_hash(const void *);
uint32_t cm_str_hash(const void *);

int cm_direct_equal(const void *, const void *);
int cm_int_equal(const void *, const void *);
int cm_int64_equal(const void *, const void *);
int cm_double_equal(const void *, const void *);
int cm_str_equal(const void *, const void *);

struct cm_hashtable *cm_hashtable_create(cm_hash_func hash_func,
					 cm_hash_equal_func equal_func);

struct cm_hashtable *cm_hashtable_create_full(cm_hash_func hash_func,
					      cm_hash_equal_func equal_func,
					      cm_hash_destroy_func key_destructor,
					      cm_hash_destroy_func value_destructor);

void cm_hashtable_insert(struct cm_hashtable *hashtable,
			 void *key,
			 void *value);

void *cm_hashtable_lookup(struct cm_hashtable *hashtable,
			  const void *key);

int cm_hashtable_remove(struct cm_hashtable *hashtable,
			const void *key);

uint32_t cm_hashtable_size(struct cm_hashtable *hashtable);

void cm_hashtable_ref(struct cm_hashtable *hashtable);

void cm_hashtable_unref(struct cm_hashtable *hashtable);



#endif // CM_HASHTABLE_H
