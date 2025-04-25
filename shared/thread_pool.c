#include "thread_pool.h"

void task_destroy(void* ptr) {
    task_t* task = (task_t*)ptr;
    if (task) { destroy(task->arg); }
}

DType task_type = {"task_t", sizeof(task_t), task_destroy, NULL};
DEFINE_TYPE(task, task_type, task_t)

void* worker_thread(void* arg) {
    thread_pool_t *pool = (thread_pool_t *)arg;

    while (1) {
        pthread_mutex_lock(pool->lock);

        while (pool->task_queue.size == 0 && !pool->shutdown_f) {
            pthread_cond_wait(pool->cond, pool->lock);
        }

        if (pool->shutdown_f) {
            // exit if no tasks are left or wait_for_task is not set
            if (pool->task_queue.size == 0 || !WAIT_FOR_TASK) {
                pthread_mutex_unlock(pool->lock);
                break;
            }
        }

        // delete operation lends the ownership to the caller, 
        // so no need to use ref() to take ownership here
        Object task_obj = dlist_delete_first(&pool->task_queue); 
        pthread_mutex_unlock(pool->lock);
        
        task_t *task = get_task(task_obj);
        task->function(task->arg);
        destroy(task_obj); // this will (should!) call task_destroy() and free the task
    }

    return NULL;
}

void destroy_thread(void* ptr) {
    pthread_t *thread = (pthread_t *)ptr;
    pthread_join(*thread, NULL);
}

DType thread_object_type = {"pthread_t", sizeof(pthread_t), destroy_thread, NULL};
DEFINE_TYPE(thread, thread_object_type, pthread_t)

thread_pool_t* thread_pool_create(int num_threads) {
    thread_pool_t* pool = (thread_pool_t *)malloc(sizeof(thread_pool_t));
    pool->task_queue = dlist_init();
    pool->threads = darray_init(num_threads);


    pool->lock = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
    pool->cond = (pthread_cond_t *)malloc(sizeof(pthread_cond_t));
    pthread_mutex_init(pool->lock, NULL);
    pthread_cond_init(pool->cond, NULL);
    pool->shutdown_f = 0;

    for (int i = 0; i < num_threads; ++i) {
        Object thread_obj = let_thread(NULL);
        darray_insert_at(&pool->threads, i, thread_obj);
        pthread_t *thread = get_thread(thread_obj);
        pthread_create(thread, NULL, worker_thread, pool);

        destroy(thread_obj); // reduce the reference count
    }

    return pool;
}

void thread_pool_add_task(thread_pool_t* pool, void (*function)(Object), Object arg) {
    Object task_obj = let_task(NULL);
    task_t* task = get_task(task_obj);
    task->function = function;
    task->arg = ref(arg); // take ownership

    pthread_mutex_lock(pool->lock);
    dlist_insert_last(&pool->task_queue, task_obj); // insert takes ownership
    pthread_cond_signal(pool->cond);
    pthread_mutex_unlock(pool->lock);

    destroy(task_obj); // reduce the reference count
}

void thread_pool_destroy(thread_pool_t* pool) {
    pthread_mutex_lock(pool->lock);
    pool->shutdown_f = 1;
    pthread_cond_broadcast(pool->cond);
    pthread_mutex_unlock(pool->lock);

    // this will join all threads because of custom destroy function
    darray_clear(&pool->threads); 
    // this will free all tasks in the queue (all will be destroyed)
    dlist_destroy(&pool->task_queue);

    pthread_mutex_destroy(pool->lock);
    pthread_cond_destroy(pool->cond);

    free(pool->lock);
    free(pool->cond);
    // shouldn't use free(pool); since one could define the instance on stack too.
}