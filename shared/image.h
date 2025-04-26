#pragma once

#include<stddef.h> // For size_t
#include<stdint.h>
#include<pthread.h> // For pthread types
#include<uthash.h>
#include<stdbool.h>

#include "Object.h" // For Object type

typedef enum {
    CHUNK_STATUS_CREATED,
    CHUNK_STATUS_FILTERED,
    CHUNK_STATUS_ERROR,
} chunk_processing_status_t;

typedef struct {
    int chunk_id;
    char* original_image_name;
    size_t offset_x;
    size_t offset_y;
    size_t width;
    size_t height;
    unsigned char* pixel_data;
    size_t data_size_bytes;
    int channels;
    int original_image_num_chunks;
    int original_image_width;
    int original_image_height;
    int processing_status;
} image_chunk_t;

typedef struct chunk_queue_node {
    struct chunk_queue_node* next;
    image_chunk_t* chunk;
} chunk_queue_node_t;

extern DType chunk_dtype; // Declare the DType for image_chunk_t
DEFINE_TYPE_PROTO(image_chunk, chunk_dtype, image_chunk_t)

void clear_image_chunk(image_chunk_t* chunk);
void free_image_chunk(image_chunk_t *chunk);

typedef struct {
    unsigned char *pixel_data;

    size_t width, height;
    uint32_t channels;
} image_t;

typedef struct {
    chunk_queue_node_t *head;
    chunk_queue_node_t *tail;
    pthread_mutex_t lock;
    pthread_cond_t cond_not_empty;
} chunk_queue_t;

int chunk_queue_init(chunk_queue_t* q);
int chunk_enqueue(chunk_queue_t* q, image_chunk_t* c);
image_chunk_t* chunk_dequeue(chunk_queue_t* q);
void broadcast_chunk_queue(chunk_queue_t* q);
void chunk_queue_destroy(chunk_queue_t* q);

extern chunk_queue_t chunker_filtering_queue;
extern chunk_queue_t filtering_reconstruction_queue;

typedef struct {
    char name[256];
    UT_hash_handle hh;
} discarded_image_entry_t;

extern pthread_mutex_t discarded_images_lock;
extern discarded_image_entry_t *discarded_images_head;

int discarded_images_init(void);
int discarded_images_table_add(const char *filename);
bool discarded_images_table_contains(const char *filename);
void free_discarded_images_table(void);