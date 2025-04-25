#pragma once

#include <stdlib.h>
#include <pthread.h>

#include "Object.h"

typedef struct node
{
    Object data;
    struct node *prev, *next;
} dlist_node_t;

dlist_node_t *dlist_node_create(Object data);
void dlist_node_destroy(dlist_node_t *node);
dlist_node_t *dlist_later_node(dlist_node_t *node, size_t offset);

typedef struct
{
    dlist_node_t *head, *tail;
    size_t size;
    pthread_mutex_t* lock;
} dlist_t;

extern DType dlist_type;
DEFINE_TYPE_PROTO(dlist, dlist_type, dlist_t)

dlist_t dlist_init();
Object dlist_get_at(dlist_t *list, size_t index);
void dlist_set_at(dlist_t *list, size_t index, Object data);
void dlist_insert_first(dlist_t *list, Object data);
void dlist_insert_last(dlist_t *list, Object data);
void dlist_insert_at(dlist_t *list, size_t index, Object data);
Object dlist_delete_first(dlist_t *list);
Object dlist_delete_last(dlist_t *list);
Object dlist_delete_at(dlist_t *list, size_t index);
void dlist_destroy(dlist_t *list);