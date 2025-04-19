#pragma once // Prevent multiple inclusions

#include<stddef.h> // For size_t
#include<pthread.h> // For pthread types
#include<uthash.h>
#include<stdbool.h>

/**
 * @brief Represents the processing state of an image chunk.
 */
typedef enum {
    CHUNK_STATUS_CREATED,   /**< Chunk has been created by the chunker. */
    CHUNK_STATUS_FILTERED,  /**< Chunk has been processed by the filter stage. */
    CHUNK_STATUS_ERROR,      /**< An error occurred during processing (e.g., filtering). */
    // Add more states if needed, e.g., CHUNK_STATUS_RECONSTRUCTED
} chunk_processing_status_t;

/**
 * @brief Represents a rectangular chunk of a larger image.
 *
 * Contains metadata about the chunk's position and dimensions relative
 * to the original image, as well as a pointer to its own pixel data buffer.
 */
typedef struct {
    // Identification
    int chunk_id;               /**< Unique identifier for this chunk within its original image. */
    char* original_image_name;  /**< Dynamically allocated name of the source image file. Caller must manage memory. */

    // Geometry
    size_t offset_x;            /**< X-coordinate of the top-left corner of this chunk in the original image's coordinate system. */
    size_t offset_y;            /**< Y-coordinate of the top-left corner of this chunk in the original image's coordinate system. */
    size_t width;               /**< Width of this chunk in pixels. */
    size_t height;              /**< Height of this chunk in pixels. */

    // Pixel Data
    unsigned char* pixel_data;  /**< Pointer to the buffer containing the chunk's pixel data (allocated separately). */
    size_t data_size_bytes;     /**< Size of the pixel_data buffer in bytes (width * height * channels). */
    int channels;               /**< Number of color channels per pixel (e.g., 1 for Grayscale, 3 for RGB, 4 for RGBA). */

    // Optional Metadata (Currently commented out)
    int original_image_num_chunks;
    int original_image_width;
    int original_image_height;
    int processing_status;

} image_chunk_t;

/**
 * @brief Node structure for a linked list queue holding image chunks.
 * @note This structure embeds the image_chunk_t directly. For efficiency and
 *       correctness (avoiding unintended copies of potentially large data),
 *       consider storing a pointer (image_chunk_t*) instead.
 * IMPORTANT: The image_chunk_t* is created only once and changes ownership, make sure to free it after reconstruction.
 */
typedef struct chunk_queue_node { // Renamed for clarity
    struct chunk_queue_node* next; /**< Pointer to the next node in the queue, or NULL if this is the last node. */
    image_chunk_t* chunk;           /**< The image chunk data stored in this node. Consider using a pointer instead. */
} chunk_queue_node_t; // Renamed type

/**
 * @brief Manages a thread-safe queue of image chunks.
 *
 * Contains pointers to the head and tail nodes, along with synchronization
 * primitives (mutex and condition variable) for thread-safe operations.
 * IMPORTANT: The image_chunk_t* is created only once and changes ownership, make sure to free it after reconstruction.
 */
typedef struct {
    chunk_queue_node_t *head;       /**< Pointer to the first node in the queue (or NULL if empty). */
    chunk_queue_node_t *tail;       /**< Pointer to the last node in the queue (or NULL if empty). */
    pthread_mutex_t lock;           /**< Mutex to protect access to the queue structure. */
    pthread_cond_t cond_not_empty;  /**< Condition variable to signal when the queue is not empty. */
    // Add cond_not_full if implementing a bounded queue
} chunk_queue_t;

/**
 * @brief Initializes a chunk queue.
 *
 * Sets head and tail to NULL and initializes the mutex and condition variable.
 * Must be called before using the queue.
 *
 * @param q Pointer to the chunk_queue_t structure to initialize.
 * @return 0 on success, non-zero on error initializing mutex or condition variable.
 */
int chunk_queue_init(chunk_queue_t* q);

/**
 * @brief Enqueues an image chunk into the thread-safe queue.
 *
 * Creates a new node, takes the ownership of chunk data pointer,
 * adds it to the tail of the queue, and signals any waiting consumers.
 * IMPORTANT: The image_chunk_t* is created only once and changes ownership, make sure to free it after reconstruction.
 *
 * @param q Pointer to the chunk_queue_t structure.
 * @param c Pointer to the image chunk data to enqueue. The queue takes ownership.
 * @return 0 on success, non-zero on failure (e.g., memory allocation).
 */
int chunk_enqueue(chunk_queue_t* q, image_chunk_t* c);

/**
 * @brief Dequeues an image chunk from the thread-safe queue.
 *
 * Blocks if the queue is empty until an item is available. Removes the head node
 * and returns the contained chunk data.
 *
 * @param q Pointer to the chunk_queue_t structure.
 * @return Pointer to the dequeued image chunk data. The caller is responsible for
 *         managing the memory of the returned chunk and its contents (pixel_data, original_image_name).\
 *         Returns NULL if an error occurs (though typically it blocks).
 */
image_chunk_t* chunk_dequeue(chunk_queue_t* q);

void broadcast_chunk_queue(chunk_queue_t* q);

/**
 * @brief Destroys a chunk queue, freeing all nodes and associated resources.
 *
 * Frees all remaining nodes in the queue. Also destroys the mutex and
 * condition variable. This function is NOT thread-safe and should only be
 * called when no other threads are accessing the queue.
 *
 * @param q Pointer to the chunk_queue_t structure to destroy.
 */
void chunk_queue_destroy(chunk_queue_t* q);

// --- Global Shared Variables ---
// These represent shared queues between pipeline stages.
// Ensure they are properly initialized using chunk_queue_init()
// and destroyed using chunk_queue_destroy().

/** @brief Shared queue for passing chunks from the chunker stage to the filtering stage. */
extern chunk_queue_t chunker_filtering_queue;
/** @brief Shared queue for passing chunks from the filtering stage to the reconstruction stage. */
extern chunk_queue_t filtering_reconstruction_queue;

/**
 * @brief Represents an entry in a hash table storing the names of images
 *        for which processing has failed at some stage.
 *
 * This structure is used as the element type within a uthash hash table.
 * It simply stores the filename (which acts as the key) and the necessary
 * handle for uthash management.
 *
 * @note Access to the hash table containing these entries is synchronized
 *       using the global `discarded_images_lock`.
 */
typedef struct {
    char name[256];     /**< The filename of the image that failed processing (acts as the key). */
    UT_hash_handle hh;  /**< Handle used by uthash library to manage the hash table structure. */
    // Removed mutex and cond_var - they don't belong in the entry struct
} discarded_image_entry_t;

/**
 * @brief Global mutex to protect concurrent access to the discarded images hash table.
 *
 * This mutex is used internally by `discarded_images_table_add` and
 * `discarded_images_table_contains` to ensure thread safety. It is initialized
 * by `discarded_images_init()` and destroyed by `free_discarded_images_table()`.
 * Direct locking/unlocking by the caller is generally not required for these functions.
 */
extern pthread_mutex_t discarded_images_lock;

/**
 * @brief Global head pointer for the hash table storing discarded image entries.
 *
 * This pointer serves as the entry point for the uthash hash table containing
 * `discarded_image_entry_t` structures. It is managed internally by uthash macros
 * (e.g., HASH_ADD_STR, HASH_FIND_STR, HASH_DEL) and the associated functions.
 * It should be initialized to NULL (e.g., by `discarded_images_init()`).
 * @warning Direct manipulation of this pointer outside of uthash macros or the
 *          provided functions is strongly discouraged and requires manual locking
 *          of `discarded_images_lock`.
 */
extern discarded_image_entry_t *discarded_images_head; // Renamed from discarded_images_table for clarity

/**
 * @brief Initializes the discarded images tracking mechanism.
 *
 * Sets the global head pointer (`discarded_images_head`) to NULL and initializes
 * the global mutex (`discarded_images_lock`).
 * This function MUST be called once at application startup before any other
 * operations on the discarded images set are performed.
 *
 * @return 0 on successful initialization, non-zero if mutex initialization fails.
 */
int discarded_images_init(void);

/**
 * @brief Adds an image filename to the set of discarded images, if not already present.
 *
 * This function marks an image as having failed processing. Subsequent pipeline
 * stages can use `discarded_images_table_contains` to check this status and
 * skip processing chunks related to this image.
 *
 * @note This function is thread-safe due to internal locking using `discarded_images_lock`.
 *
 * @param filename The null-terminated string containing the filename to add.
 *                 The string is copied into the hash table entry.
 * @return 0 if the filename was successfully added or if it already existed.
 * @return -1 if memory allocation for the new entry fails.
 */
int discarded_images_table_add(const char *filename);

/**
 * @brief Checks if an image filename is present in the set of discarded images.
 *
 * Used by pipeline stages to determine if chunks originating from a specific
 * image should be skipped due to a prior processing failure.
 *
 * @note This function is thread-safe due to internal locking using `discarded_images_lock`.
 *
 * @param filename The null-terminated string containing the filename to check.
 * @return `true` if the filename is found in the discarded set, `false` otherwise.
 */
bool discarded_images_table_contains(const char *filename);

/**
 * @brief Frees all memory associated with the discarded images hash table and destroys the mutex.
 *
 * Iterates through the entire hash table, removing each entry using `HASH_DEL`
 * and freeing the memory allocated for the `discarded_image_entry_t` structure.
 * Finally, it destroys the global `discarded_images_lock` mutex.
 *
 * @warning This function is NOT thread-safe. It should only be called during
 *          application shutdown or cleanup when it is guaranteed that no other
 *          threads are attempting to access the discarded images table or its mutex.
 */
void free_discarded_images_table(void);