#pragma once

#include<pthread.h> 

// Forward declaration
struct image_name_queue_node;

/**
 * @brief Node structure for a linked list queue holding image filenames.
 */
typedef struct image_name_queue_node {
    struct image_name_queue_node* next; /**< Pointer to the next node, or NULL if last. */
    char* name;                         /**< Dynamically allocated copy of the filename. */
} image_name_queue_node_t;

/**
 * @brief Manages a thread-safe queue of image filenames.
 *
 * Contains pointers to the head and tail nodes, along with synchronization
 * primitives (mutex and condition variable) for thread-safe operations.
 */
typedef struct {
    image_name_queue_node_t* head;          /**< Pointer to the first node in the queue (or NULL if empty). */
    image_name_queue_node_t* tail;          /**< Pointer to the last node in the queue (or NULL if empty). */
    pthread_mutex_t lock;               /**< Mutex to protect access to the queue structure. */
    pthread_cond_t cond_not_empty;      /**< Condition variable to signal when the queue is not empty. */
    // Add cond_not_full if implementing a bounded queue
} image_name_queue_t;

/**
 * @brief Initializes an image name queue.
 *
 * Sets head and tail to NULL and initializes the mutex and condition variable.
 * Must be called before using the queue.
 *
 * @param q Pointer to the image_name_queue_t structure to initialize.
 * @return 0 on success, non-zero on error initializing mutex or condition variable.
 */
int image_name_queue_init(image_name_queue_t* q);

/**
 * @brief Enqueues an image filename into the thread-safe queue.
 *
 * Makes a dynamic copy of the provided filename string, creates a new node,
 * adds it to the tail of the queue, and signals any waiting consumers.
 *
 * @param q Pointer to the image_name_queue_t structure.
 * @param name The filename string to enqueue. A copy will be made.
 * @return 0 on success, -1 on memory allocation failure or mutex/cond error.
 */
int enqueue_image_name(image_name_queue_t *q, const char *name);

/**
 * @brief Dequeues an image filename from the thread-safe queue.
 *
 * Blocks if the queue is empty until an item is available. Removes the head node
 * and returns the contained filename string. The caller is responsible for freeing
 * the returned string using free().
 *
 * @param q Pointer to the image_name_queue_t structure.
 * @return A pointer to a dynamically allocated filename string, or NULL if
 *         an error occurs or shutdown is signaled (implementation dependent).
 */
char* dequeue_image_name(image_name_queue_t *q);

void broadcast_image_name_queue(image_name_queue_t* q);

/**
 * @brief Destroys an image name queue, freeing all nodes and associated resources.
 *
 * Frees all remaining nodes and their associated filename strings. Also destroys
 * the mutex and condition variable. This function is NOT thread-safe and should
 * only be called when no other threads are accessing the queue.
 *
 * @param q Pointer to the image_name_queue_t structure to destroy.
 */
void image_name_queue_destroy(image_name_queue_t* q);

// Removed global extern declarations as they are now part of the struct:
// extern image_name_queue_t queue;
// extern pthread_mutex_t image_name_queue_lock;
// extern pthread_cond_t image_name_queue_cond;