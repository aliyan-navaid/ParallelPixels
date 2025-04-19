#include <assert.h>
#include "dlist.h"

static void destroy_dlist(void* data) {
    dlist_t *list = (dlist_t *)data;
    dlist_destroy_list(list);
}

DType dlist_type = {"dlist_t",sizeof(dlist_t),destroy_dlist,NULL};

DEFINE_TYPE(dlist, dlist_type, dlist_t)

dlist_node_t *dlist_node_create(Object data) {
    dlist_node_t *node = (dlist_node_t *)malloc(sizeof(dlist_node_t));
    if (node == NULL) {
        return NULL;
    }

    node->data = ref(data);
    node->prev = NULL;
    node->next = NULL;

    return node;
}

void dlist_node_destroy(dlist_node_t *node) {
    if (node == NULL) {
        return;
    }

    destroy(node->data);
    free(node);
}

dlist_node_t *dlist_later_node(dlist_node_t *node, size_t offset) {
    assert(node != NULL);
    if (offset == 0) {
        return node;
    }

    assert(node->next != NULL);
    return dlist_later_node(node->next, offset - 1);
}

dlist_t dlist_create() {
    dlist_t list;

    list.head = NULL;
    list.tail = NULL;
    list.size = 0;
    list.lock = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);

    pthread_mutex_init(list.lock, &attr);
    pthread_mutexattr_destroy(&attr);
    return list;
}

Object dlist_get_at(dlist_t *list, size_t index) {
    if (!list || index >= list->size) {
        return None;
    }

    pthread_mutex_lock(list->lock);
    dlist_node_t *node = dlist_later_node(list->head, index);
    Object data = ref(node->data);
    pthread_mutex_unlock(list->lock);

    return data;
}

void dlist_set_at(dlist_t *list, size_t index, Object data) {
    if (!list || index >= list->size) {
        return;
    }

    pthread_mutex_lock(list->lock);
    dlist_node_t *node = dlist_later_node(list->head, index);
    destroy(node->data);
    node->data = ref(data);
    pthread_mutex_unlock(list->lock);
}

void dlist_insert_first(dlist_t *list, Object data) {
    if (!list) return;

    pthread_mutex_lock(list->lock);

    dlist_node_t *new_node = dlist_node_create(data);
    new_node->prev = NULL;
    new_node->next = list->head;

    if (list->head) {
        list->head->prev = new_node;
    } else {
        list->tail = new_node;
    }

    list->head = new_node;
    list->size += 1;

    pthread_mutex_unlock(list->lock);
}

void dlist_insert_last(dlist_t *list, Object data) {
    if (!list) return;

    pthread_mutex_lock(list->lock);

    dlist_node_t *new_node = dlist_node_create(data);
    new_node->next = NULL;
    new_node->prev = list->tail;

    if (list->tail) {
        list->tail->next = new_node;
    } else {
        list->head = new_node;
    }

    list->tail = new_node;
    list->size += 1;

    pthread_mutex_unlock(list->lock);
}

void dlist_insert_at(dlist_t *list, size_t index, Object data) {
    if (!list || index > list->size) return;

    pthread_mutex_lock(list->lock);

    if (index == 0) {
        dlist_insert_first(list, data);
        pthread_mutex_unlock(list->lock);
        return;
    }

    if (index == list->size) {
        dlist_insert_last(list, data);
        pthread_mutex_unlock(list->lock);
        return;
    }

    dlist_node_t *new_node = dlist_node_create(data);
    dlist_node_t *current = dlist_later_node(list->head, index);

    new_node->prev = current->prev;
    new_node->next = current;

    if (current->prev) {
        current->prev->next = new_node;
    }
    current->prev = new_node;

    list->size += 1;

    pthread_mutex_unlock(list->lock);
}

Object dlist_delete_first(dlist_t *list) {
    if (!list || !list->head) {
        return None;
    }

    pthread_mutex_lock(list->lock);

    dlist_node_t *node_to_delete = list->head;
    Object data = ref(node_to_delete->data);
    list->head = node_to_delete->next;

    if (list->head) {
        list->head->prev = NULL;
    } else {
        list->tail = NULL;
    }

    dlist_node_destroy(node_to_delete);
    list->size -= 1;

    pthread_mutex_unlock(list->lock);
    return data;
}

Object dlist_delete_last(dlist_t *list) {
    if (!list || !list->tail) {
        return None;
    }

    pthread_mutex_lock(list->lock);

    dlist_node_t *node_to_delete = list->tail;
    Object data = ref(node_to_delete->data);
    list->tail = node_to_delete->prev;

    if (list->tail) {
        list->tail->next = NULL;
    } else {
        list->head = NULL;
    }

    dlist_node_destroy(node_to_delete);
    list->size -= 1;

    pthread_mutex_unlock(list->lock);
    return data;
}

Object dlist_delete_at(dlist_t *list, size_t index) {
    if (!list || index >= list->size) {
        return None;
    }

    pthread_mutex_lock(list->lock);

    if (index == 0) {
        pthread_mutex_unlock(list->lock);
        return dlist_delete_first(list);
    }

    if (index == list->size - 1) {
        pthread_mutex_unlock(list->lock);
        return dlist_delete_last(list);
    }

    dlist_node_t *node_to_delete = dlist_later_node(list->head, index);
    Object data = ref(node_to_delete->data);

    if (node_to_delete->prev) {
        node_to_delete->prev->next = node_to_delete->next;
    }
    if (node_to_delete->next) {
        node_to_delete->next->prev = node_to_delete->prev;
    }

    dlist_node_destroy(node_to_delete);
    list->size -= 1;

    pthread_mutex_unlock(list->lock);
    return data;
}

void dlist_destroy_list(dlist_t *list) {
    if (!list) return;

    pthread_mutex_lock(list->lock);

    dlist_node_t *current = list->head;
    while (current) {
        dlist_node_t *next = current->next;
        dlist_node_destroy(current);
        current = next;
    }

    pthread_mutex_unlock(list->lock);
    pthread_mutex_destroy(list->lock);
    free(list->lock);
}
