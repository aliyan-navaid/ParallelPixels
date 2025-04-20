#pragma once

#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "Object.h"

typedef struct
{
    Object *arr;
    size_t size, capacity;
    pthread_mutex_t* lock;
} darray_t;

extern DType darray_type;
DEFINE_TYPE_PROTO(darray, darray_type, darray_t)

darray_t darray_init(size_t initial_capacity);
void darray_destroy(darray_t *arr);

void darray_insert_last(darray_t *arr, Object obj);
void darray_insert_at(darray_t *arr, size_t index, Object obj);

Object darray_delete_last(darray_t *arr);
Object darray_delete_at(darray_t *arr, size_t index);

Object darray_get(darray_t *arr, size_t index);
void darray_set(darray_t *arr, size_t index, Object obj);

void darray_resize(darray_t *arr, size_t new_size);
void darray_reserve(darray_t *arr, size_t new_capacity);
void darray_clear(darray_t *arr);