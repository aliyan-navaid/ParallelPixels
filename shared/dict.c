#include "dict.h"

static void destroy_kv_pair(void* data) {
    kv_pair_t *pair = (kv_pair_t *)data;
    destroy(pair->key);
    destroy(pair->value);
}

DType kv_pair_type = {"kv_pair_t", sizeof(kv_pair_t), destroy_kv_pair, NULL};

DEFINE_TYPE(key_value_pair, kv_pair_type, kv_pair_t)

static dlist_t* find_chain(dict_t *dict, Object key) {
    if (!dict || is_none(key)) {
        return NULL;
    }
    
    // recall that caller borrows the object.
    size_t index = dict->hash_fn(key) % dict->capacity;
    return get_dlist(darray_get_at(&dict->buckets, index));
}

static kv_pair_t* find_pair(dict_t* dict, Object key, int* index) {
    dlist_t *list = find_chain(dict, key);
    if (!list) { return NULL; }
    
    dlist_node_t *node = list->head;
    int i = 0;

    while (node) {
        kv_pair_t *pair = get_key_value_pair(node->data);

        if (dict->key_eq_fn(pair->key, key)) {
            if (index) { *index = i; }
            return pair;
        }

        node = node->next;
        i++;
    }

    if (index) { *index = -1; }
    return NULL;
}

static void store_pair(dlist_t *list, Object key, Object value) {
    kv_pair_t pair = {ref(key), ref(value)};
    Object obj = let_key_value_pair(&pair); // creates new object
    dlist_insert_last(list, obj); // takes the ownership of the object
    destroy(obj); // release the reference count
}

static darray_t init_buckets(size_t initial_capacity) {
    darray_t buckets = darray_init(initial_capacity);
    for (int i = 0; i < initial_capacity; ++i) {
        Object list = let_dlist_v(dlist_init());
        darray_insert_last(&buckets, list); // takes the ownership of the list
        destroy(list);  // release the reference count
    }
    return buckets;
}

static void dict_init_internal(dict_t* dict, size_t initial_capacity, hash_func hash_fn, key_eq key_eq_fn) {
    assert(initial_capacity > 0);

    dict->buckets = init_buckets(initial_capacity);
    dict->capacity = initial_capacity;
    dict->size = 0;
    dict->hash_fn = hash_fn;
    dict->key_eq_fn = key_eq_fn;
    dict->load_factor = 0.75;
    
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
    
    dict->lock = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(dict->lock, &attr);
    
    pthread_mutexattr_destroy(&attr);
}

dict_t dict_init(hash_func hash_fn, key_eq key_eq_fn) {
    dict_t dict;
    dict_init_internal(&dict, 1, hash_fn, key_eq_fn);
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

    if (should_resize(dict)) {
        dict_resize(dict, dict->capacity * 2);
    }

    if (dict_contains(dict, key)) {
        pthread_mutex_unlock(dict->lock);
        return;
    }

    dlist_t *list = find_chain(dict, key);
    if (!list) {
        pthread_mutex_unlock(dict->lock);
        return;
    }

    store_pair(list, key, value);
    dict->size++;

    pthread_mutex_unlock(dict->lock);
}

int dict_contains(dict_t * dict, Object key) {
    if (!dict || is_none(key)) {
        return 0;
    }

    pthread_mutex_lock(dict->lock);
    
    dlist_t *list = find_chain(dict, key);
    if (!list) {
        pthread_mutex_unlock(dict->lock);
        return 0;
    }

    kv_pair_t *pair = find_pair(dict, key, NULL);
    int result = pair != NULL;

    pthread_mutex_unlock(dict->lock);
    return result;
}

Object dict_get(dict_t *dict, Object key, Object default_value) {
    pthread_mutex_lock(dict->lock);
    
    if (!dict || is_none(key) || !dict_contains(dict, key)) {
        return default_value;
    }

    kv_pair_t *pair = find_pair(dict, key, NULL);
    if (!pair) { 
        pthread_mutex_unlock(dict->lock);
        return default_value;
    }

    pthread_mutex_unlock(dict->lock);
    return pair->value;
}

Object dict_delete(dict_t *dict, Object key) {
    pthread_mutex_lock(dict->lock);
    
    if (!dict || is_none(key) || !dict_contains(dict, key)) {
        return None;
    }

    int i;
    dlist_t *list = find_chain(dict, key);
    if (!list) {
        pthread_mutex_unlock(dict->lock);
        return None;
    }

    kv_pair_t *pair = find_pair(dict, key, &i);
    if (!pair) {
        pthread_mutex_unlock(dict->lock);
        return None;
    }

    Object value = ref(pair->value);
    destroy(dlist_delete_at(list, i));
    dict->size--;

    pthread_mutex_unlock(dict->lock);
    return value;
}

void dict_resize(dict_t *dict, size_t new_capacity) {
    if (!dict || new_capacity <= dict->capacity) {
        return;
    }

    pthread_mutex_lock(dict->lock);
    dict_t new_dict;
    new_dict.buckets = init_buckets(new_capacity);
    new_dict.lock = dict->lock;
    new_dict.capacity = new_capacity;
    new_dict.hash_fn = dict->hash_fn;
    new_dict.key_eq_fn = dict->key_eq_fn;
    new_dict.load_factor = dict->load_factor;
    new_dict.size = 0;

    // traverse every pair and insert them into the new dict
    for (int i = 0; i < dict->capacity; ++i) {
        dlist_t *list = get_dlist(darray_get_at(&dict->buckets, i));
        dlist_node_t *node = list->head;
        
        while (node) {
            kv_pair_t *pair = get_key_value_pair(node->data);
            dict_insert(&new_dict, pair->key, pair->value);
            node = node->next;
        }
    }

    // destroy the old dict
    darray_destroy(&dict->buckets);
    dict->buckets = new_dict.buckets;
    dict->capacity = new_dict.capacity;
}