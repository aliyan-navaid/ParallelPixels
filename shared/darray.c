#include <stdio.h>
#include <assert.h>
#include "darray.h"

void destroy_darray(void* data) {
    darray_t* arr = (darray_t*)data;
    darray_destroy(arr);
}

DType darray_type = {"darray", sizeof(darray_t), destroy_darray, NULL};
DEFINE_TYPE(darray, darray_type, darray_t)

static void darray_init_internal(darray_t* arr, size_t initial_capacity) {
    arr->arr = (Object*)malloc(sizeof(Object) * initial_capacity);

    arr->size = 0;
    arr->capacity = initial_capacity;
   
    arr->lock = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
    pthread_mutex_init(arr->lock, &attr);
    pthread_mutexattr_destroy(&attr);
}

darray_t darray_init(size_t initial_capacity) {
    assert(initial_capacity > 0);
    darray_t arr;
    darray_init_internal(&arr, initial_capacity);
    return arr;
}

void darray_destroy(darray_t *arr) {
    pthread_mutex_lock(arr->lock);

    if (arr == NULL || arr->arr == NULL) {
        return;
    }

    for (size_t i = 0; i < arr->size; i++) {
        destroy(arr->arr[i]); // release each object
    }

    free(arr->arr);

    
    // Reset fields
    arr->arr = NULL;
    arr->size = 0;
    arr->capacity = 0;

    pthread_mutex_unlock(arr->lock);
    pthread_mutex_destroy(arr->lock);
    free(arr->lock);
}

void darray_resize(darray_t *arr, size_t new_size) {
    pthread_mutex_lock(arr->lock);

    if (new_size > arr->capacity) {
        darray_reserve(arr, new_size);
    }
    for (int i = arr->size; i < new_size; i++) {
        arr->arr[i] = None;
    }

    arr->size = new_size;

    pthread_mutex_unlock(arr->lock);
}

void darray_reserve(darray_t *arr, size_t new_capacity) {
    pthread_mutex_lock(arr->lock);

    if (new_capacity <= arr->capacity) { return; }

    Object *new_arr = (Object *)realloc(arr->arr, new_capacity * sizeof(Object));
    assert(new_arr != NULL);

    arr->arr = new_arr;
    arr->capacity = new_capacity;

    pthread_mutex_unlock(arr->lock);
}

void darray_insert_last(darray_t *arr, Object obj) {
    pthread_mutex_lock(arr->lock);

    if (arr->size == arr->capacity) {
        darray_reserve(arr, arr->capacity == 0 ? 1 : arr->capacity * 2);
    }

    arr->arr[arr->size++] = ref(obj);   // take ownership

    pthread_mutex_unlock(arr->lock);
}

void darray_insert_at(darray_t *arr, size_t index, Object obj) {
    pthread_mutex_lock(arr->lock);

    assert(index <= arr->size);

    if (arr->size == arr->capacity) {
        darray_reserve(arr, arr->capacity == 0 ? 1 : arr->capacity * 2);
    }

    // Shift elements to the right
    memmove(&arr->arr[index + 1], &arr->arr[index], (arr->size - index) * sizeof(Object));

    // Insert the object
    arr->arr[index] = ref(obj);     // take ownership
    arr->size++;

    pthread_mutex_unlock(arr->lock);
}

Object darray_delete_last(darray_t *arr) {
    pthread_mutex_lock(arr->lock);

    assert(arr->size > 0);

    Object obj = arr->arr[arr->size - 1]; // just gonna lend the object to caller
    arr->arr[--arr->size] = None; // Clear the reference

    pthread_mutex_unlock(arr->lock);
    return obj;
}

Object darray_delete_at(darray_t *arr, size_t index) {
    pthread_mutex_lock(arr->lock);

    assert(index < arr->size);
    
    Object obj = arr->arr[index]; // just gonna lend the object to caller
    arr->arr[index] = None; // Clear the reference

    // Shift elements to left
    memmove(&arr->arr[index], &arr->arr[index + 1], (arr->size - index - 1) * sizeof(Object));

    arr->size--;

    pthread_mutex_unlock(arr->lock);
    return obj;
}

Object darray_get_at(darray_t *arr, size_t index) {
    pthread_mutex_lock(arr->lock);

    assert(index < arr->size);
    Object obj = arr->arr[index]; // just gonna lend the object to caller

    pthread_mutex_unlock(arr->lock);
    return obj;
}

void darray_set_at(darray_t *arr, size_t index, Object obj) {
    pthread_mutex_lock(arr->lock);

    assert(index < arr->size);

    destroy(arr->arr[index]); // release the old object
    arr->arr[index] = ref(obj); // take ownership

    pthread_mutex_unlock(arr->lock);
}

void darray_clear(darray_t *arr) {
    pthread_mutex_lock(arr->lock);

    for (size_t i = 0; i < arr->size; i++) {
        destroy(arr->arr[i]); // release each object
    }

    arr->size = 0;

    pthread_mutex_unlock(arr->lock);
}