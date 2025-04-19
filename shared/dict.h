#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <assert.h>

#include "Object.h"
#include "dlist.h"
#include "darray.h"

typedef size_t (*hash_func)(Object key);
typedef int (*key_eq)(Object key1, Object key2);

typedef struct
{
    Object key;
    Object value;
} kv_pair_t;

typedef struct
{
    darray_t buckets;
    size_t capacity, size;
    hash_func hash_fn;
    key_eq key_eq_fn;
    double load_factor;

    pthread_mutex_t *lock;
} dict_t;

dict_t dict_init(hash_func hash_fn, key_eq key_eq_fn);
void dict_destroy(dict_t *dict);

void dict_insert(dict_t *dict, Object key, Object value);
Object dict_get(dict_t *dict, Object key, Object default_value);
Object dict_delete(dict_t *dict, Object key);

void dict_resize(dict_t *dict, size_t new_capacity);