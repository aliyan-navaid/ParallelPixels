#include "reconstruction.h"

void init_reconstruction(chunk_queue_t *processed_queue) {
    pthread_t thread;
    pthread_create(&thread, NULL, reconstruction_thread, processed_queue);
    pthread_detach(thread);
}
/*
You should read `dict.h` before going through this file. To brief, the `dict_t` is a hash table that maps keys to values, where both are the instnaces of `Object`. 

Here, the keys are the image names (of type `const char*`), and the values their corresponding list of chunks (implemented by `dlist_t`), which ultimately stores the objects of `image_chunk_t`.

The `dlist_t`, `const char*`, and the `image_chunk_t` all support the `Object` interface (defined in `Object.h`, `image.h`, and in `dlist.h`), so we can use them directly in `dict_t`.  

The only thing remaining is to provide `hash_func` and `compare_func` for the `dict_t` to work with `const char*`. The `hash_func` is a simple hash function that uses the CityHash library to hash the string, and the `compare_func` is a simple string comparison function.
*/

static inline size_t hash_string(Object key) { 
    const char* str = get_string_v(key);
    assert(str != NULL);
    
    // recall that the function "borrows" the Object, so no need to `destroy` it
    return XXH64(str, strlen(str), 0); // 0 is the seed, you can change it if needed
}

static inline int compare_strings(Object a, Object b) {
    const char* str1 = get_string_v(a);
    const char* str2 = get_string_v(b);
    assert(str1 != NULL && str2 != NULL);

    // recall that the function "borrows" the Object, so no need to `destroy` it
    return strcmp(str1, str2) == 0;
}

// ################################################
// # The following are the "Reconstruction" helper 
// # functions. These will be called implicitly by 
// # calling the `init_reconstruction` function.
// ################################################

/*
* @brief This function will be passed to the thread pool, along with the dlist instance, wrapped inside the Object instance (which is also the parameter of this function). 
* @param obj The Object instance that wraps the dlist instance.
*/
void task_function(Object obj) {
    assert(!is_none(obj));
    dlist_t *chunks_list = get_dlist(obj);

    image_t image = image_from_chunks(chunks_list);

    const char* org_name = get_image_chunk(chunks_list->head->data)->original_image_name;
    const char* suffix = generate_suffix(NULL, 0);
    const char* output_path = result_path(NULL, org_name, suffix);

    write_image(image, output_path);
    cleanup_image(&image);

    free((char*)output_path);
    free((char*)suffix);
}

static thread_pool_t* init_threadpool() {
    assert(RECONSTRUCTION_THREADS > 1);
    return thread_pool_create(RECONSTRUCTION_THREADS - 1);
}

static void shutdown_and_free_threadpool(thread_pool_t* pool) {
    thread_pool_destroy(pool);
    free(pool);
}

static bool handle_corrupted_chunk(dict_t *dict, image_chunk_t *chunk) {
    if (!chunk) { return true; }

    if (chunk->processing_status != CHUNK_STATUS_ERROR) { return false; }

    Object img_name = let_string_v(chunk->original_image_name);
    remove_image(dict, img_name);

    free_image_chunk(chunk);
    destroy(img_name);
    return true;
}

static bool enough_chunks(dict_t* dict, Object img_name) {
    Object value = dict_get(&dict, img_name, None); // we borrow the value

    if (is_none(value)) { 
        return 0; 
    }

    dlist_t *dlist = get_dlist(value);
    assert(dlist != NULL);

    if (dlist->size == 0) {
        return 0; 
    }

    Object first_chunk = dlist_get_at(dlist, 0); // we borrow the object
    size_t chunk_count = get_image_chunk(first_chunk)->original_image_num_chunks; 

    return chunk_count == dlist->size;
}

static void try_schedule_reconstruction(dict_t* dict, thread_pool_t* pool, Object img_name) {
    if (!enough_chunks(dict, img_name)) { return; }

    Object dlist_obj = dict_delete(&dict, img_name); // we get the ownership 
    thread_pool_add_task(pool, task_function, dlist_obj); // pool takes the ownership of dlist_obj
    destroy(dlist_obj); // release the count
}

static void insert_chunk(dict_t* dict, Object img_name, Object chunk_obj) {
    if (!dict_contains(dict, img_name)) {
        dlist_t dlist = dlist_init();
        Object value = let_dlist(&dlist);
        
        dict_insert(&dict, img_name, value); // dict takes the ownership of the object. 
        destroy(value); // release the reference count
    }
    
    Object value = dict_get(&dict, img_name, None); // we borrow the object here. no need to destroy
    assert(!is_none(value));

    dlist_t *dlist = get_dlist(value);
    assert(dlist != NULL);

    dlist_insert(dlist, chunk_obj); // dlist takes the ownership of the object
}

static void remove_image(dict_t* dict, Object img_name) {
    if (dict_contains(dict, img_name)) {
        Object value = dict_delete(&dict, img_name); // it transfers the ownership
        destroy(value); // free the dlist 
    }
}


void *reconstruction_thread(void *arg) {
    chunk_queue_t *processed_queue = (chunk_queue_t *)arg;
    dict_t dict = dict_init(hash_string, compare_strings);

    // create threadpool with n - 1 threads (1 is the reconstruction thread itself)
    thread_pool_t *pool = init_threadpool(); 

    image_chunk_t *chunk; // container to store the pointers to the chunks

    while (!stop_flag) {
        chunk = chunk_dequeue(processed_queue);

        if (handle_corrupted_chunk(&dict, chunk)) { continue; }

        Object img_name = let_string_v(chunk->original_image_name);
        Object chunk_obj = let_image_chunk(chunk);

        insert_chunk(&dict, img_name, chunk_obj); 

        // check if we have enough chunks to reconstruct the image
        try_schedule_reconstruction(&dict, pool, img_name);
        destroy(img_name);
        free(chunk);
    }

    shutdown_and_free_threadpool(pool);
    return NULL;
}