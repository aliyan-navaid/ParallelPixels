#include "dict.h"
#include "dlist.h"
#include "darray.h"

static void destroy_kv_pair(void* data) {
    kv_pair_t *pair = (kv_pair_t *)data;
    destroy(pair->key);
    destroy(pair->value);
}

static void destroy_dlist(void* data) {
    dlist_t *list = (dlist_t *)data;
    dlist_destroy_list(list);
}

DEFINE_TYPE(dlist, dlist_t, destroy_dlist, NULL)
DEFINE_TYPE(key_value_pair, kv_pair_t, destroy_kv_pair, NULL)

static void dict_init_internal(dict_t* dict, size_t initial_capacity, hash_func hash_fn, key_eq key_eq_fn) {
    dict->buckets = darray_init(initial_capacity);
    darray_resize(&dict->buckets, initial_capacity);
    for (int i = 0; i < initial_capacity; i++) {
        dlist_t list = dlist_create();
        darray_set(&dict->buckets, i, let_dlist_t(&list));
    }

    dict->capacity = initial_capacity;
    dict->size = 0;
    dict->hash_fn = hash_fn;
    dict->key_eq_fn = key_eq_fn;
    dict->load_factor = 0.75;

    dict->lock = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
    pthread_mutex_init(dict->lock, &attr);
    pthread_mutexattr_destroy(&attr);
}

dict_t dict_init(size_t initial_capacity, hash_func hash_fn, key_eq key_eq_fn) {
    dict_t dict;
    dict_init_internal(&dict, initial_capacity, hash_fn, key_eq_fn);
    return dict;
}

void dict_destroy(dict_t* dict) {
    if (!dict) {
        return;
    }

    darray_destroy(&dict->buckets);
    dict->size = 0;
    dict->capacity = 0;

    pthread_mutex_destroy(dict->lock);
}

static int find_key_in_chain(dlist_t *list, Object key, key_eq key_eq_fn) {
    int index = 0;
    dlist_node_t *node = list->head;

    while (node) {
        kv_pair_t *pair = get_kv_pair_t(node->data);
        if (key_eq_fn(pair->key, key)) {
            return index;
        }
        node = node->next;
        index++;
    }

    return -1;
}

static int should_resize(dict_t *dict) {
    if (!dict) {
        return 0;
    }
    return dict->size >= dict->capacity * dict->load_factor;
}

void dict_insert(dict_t *dict, Object key, Object value) {
    if (!dict || is_none(key) || is_none(value)) {
        return;
    }

    pthread_mutex_lock(dict->lock);

    size_t index = dict->hash_fn(key) % dict->capacity;
    dlist_t *list = get_dlist_t(darray_get(&dict->buckets, index));
    int i = find_key_in_chain(list, key, dict->key_eq_fn);

    if (i != -1) {
        kv_pair_t *pair = get_kv_pair_t(dlist_get_at(list, i));
        destroy(pair->value);
        pair->value = ref(value);
        return;
    }

    if (should_resize(dict)) {
        dict_resize(dict, dict->capacity * 2);
        index = dict->hash_fn(key) % dict->capacity;
        list = get_dlist_t(darray_get(&dict->buckets, index));
    }

    kv_pair_t pair = {ref(key), ref(value)};
    Object obj = let_kv_pair_t(&pair);

    dlist_insert_last(list, obj);
    dict->size++;
    pthread_mutex_unlock(dict->lock);
}

Object dict_get(dict_t *dict, Object key, Object default_value) {
    if (!dict || is_none(key)) {
        return default_value;
    }

    pthread_mutex_lock(dict->lock);

    size_t index = dict->hash_fn(key) % dict->capacity;
    dlist_t *list = get_dlist_t(darray_get(&dict->buckets, index));
    int i = find_key_in_chain(list, key, dict->key_eq_fn);

    if (i != -1) {
        kv_pair_t *pair = get_kv_pair_t(dlist_get_at(list, i));
        pthread_mutex_unlock(dict->lock);
        return ref(pair->value);
    }

    pthread_mutex_unlock(dict->lock);
    return ref(default_value);
}

Object dict_delete(dict_t *dict, Object key) {
    if (!dict || is_none(key)) {
        return None;
    }

    pthread_mutex_lock(dict->lock);

    size_t index = dict->hash_fn(key) % dict->capacity;
    dlist_t *list = get_dlist_t(darray_get(&dict->buckets, index));
    int i = find_key_in_chain(list, key, dict->key_eq_fn);

    if (index != -1) {
        kv_pair_t *pair = get_kv_pair_t(dlist_get_at(list, i));
        Object value = ref(pair->value);
        dlist_delete_at(list, i);
        dict->size--;
        pthread_mutex_unlock(dict->lock);
        return value;
    }

    pthread_mutex_unlock(dict->lock);
    return None;
}

void dict_resize(dict_t *dict, size_t new_capacity) {
    if (!dict || new_capacity <= dict->capacity) {
        return;
    }

    darray_t new_buckets = darray_init(new_capacity);
    darray_resize(&new_buckets, new_capacity);
    for (int i = 0; i < new_capacity; i++) {
        dlist_t list = dlist_create();
        darray_set(&new_buckets, i, let_dlist_t(&list));
    }

    for (size_t i = 0; i < dict->capacity; i++) {
        dlist_t *old_list = get_dlist_t(darray_get(&dict->buckets, i));
        dlist_node_t *node = old_list->head;

        while (node) {
            kv_pair_t *pair = get_kv_pair_t(node->data);
            size_t index = dict->hash_fn(pair->key) % new_capacity;
            dlist_t *new_list = get_dlist_t(darray_get(&new_buckets, index));
            dlist_insert_last(new_list, ref(node->data));
            node = node->next;
        }
    }

    darray_destroy(&dict->buckets);
    dict->buckets = new_buckets;
    dict->capacity = new_capacity;
}