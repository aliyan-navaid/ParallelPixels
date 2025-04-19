#pragma once

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include "dlist.h"
#include "darray.h"
#include "Object.h"

#define WAIT_FOR_TASK 1

typedef struct
{
    void (*function)(Object);
    Object arg;
} task_t;

extern DType task_type;
DEFINE_TYPE_PROTO(task, task_type, task_t)

extern DType thread_object_type;
DEFINE_TYPE_PROTO(thread, thread_object_type, pthread_t)

typedef struct
{
    dlist_t task_queue;
    darray_t threads;
    pthread_mutex_t *lock;
    pthread_cond_t *cond;

    int shutdown_f;
} thread_pool_t;

thread_pool_t *thread_pool_create(int num_threads);
void thread_pool_destroy(thread_pool_t *pool);
void thread_pool_add_task(thread_pool_t *pool, void (*function)(Object), Object arg);