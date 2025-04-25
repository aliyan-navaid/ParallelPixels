#include<image_chunker.h>
#include<stdlib.h>
#include<signal.h>
#include<image.h>
#include<stdio.h>
#include<errno.h> 

extern volatile sig_atomic_t stop_flag;

// #######################################
// # Add Object Interface for image_chunk_t
// #######################################

void destroy_image_chunk(void* chunk) {
    clear_image_chunk((image_chunk_t*)chunk);
}

DType chunk_dtype = {"image-chunk", sizeof(image_chunk_t), destroy_image_chunk, NULL };

DEFINE_TYPE(image_chunk, chunk_dtype, image_chunk_t)

// #######################################
// # To free Chunks
// #######################################

void clear_image_chunk(image_chunk_t *chunk) {
    if (chunk == NULL)
        return;

    free(chunk->original_image_name);
    free(chunk->pixel_data);
}

void free_image_chunk(image_chunk_t *chunk) {
    if (chunk == NULL)
        return;

    clear_image_chunk(chunk);
    free(chunk);
}

// #######################################
// # Chunk Queue Implementation
// #######################################
int chunk_queue_init(chunk_queue_t* q) {
    if (q == NULL) 
        return EINVAL; 

    q->head = NULL;
    q->tail = NULL;

    if (pthread_mutex_init(&q->lock, NULL) != 0) {
        perror("chunk_queue_init: Failed to initialize mutex");
        return -1;
    }

    if (pthread_cond_init(&q->cond_not_empty, NULL) != 0) {
        perror("chunk_queue_init: Failed to initialize condition variable");
        pthread_mutex_destroy(&q->lock); 
        return -1;
    }

    return 0;
}

int chunk_enqueue(chunk_queue_t* q, image_chunk_t* c) {
    if (q == NULL || c == NULL) 
        return EINVAL;

    chunk_queue_node_t *new_node = (chunk_queue_node_t*)malloc(sizeof(chunk_queue_node_t));
    if (new_node == NULL) {
        perror("chunk_enqueue: Failed to allocate memory for new queue node");
        return -1;
    }
    
    new_node->next = NULL;
    new_node->chunk = c; 

    pthread_mutex_lock(&q->lock);

    if (q->tail == NULL) { 
        // very rare
        if (q->head != NULL) {
            fprintf(stderr, "chunk_enqueue: Queue inconsistency detected (tail is NULL, head is not)\n");
            pthread_mutex_unlock(&q->lock);
            free(new_node);
            return -1; 
        }
        
        q->head = new_node;
        q->tail = new_node;
    } 
    
    else { 
        q->tail->next = new_node;
        q->tail = new_node;
    }

    pthread_cond_signal(&q->cond_not_empty);
    pthread_mutex_unlock(&q->lock);

    //printf("Chunk enqueued successfully (ID: %d)\n", c->chunk_id); 

    return 0;
}

image_chunk_t* chunk_dequeue(chunk_queue_t* q) {
    if (q == NULL) 
        return NULL;

    pthread_mutex_lock(&q->lock);
    while (q->head == NULL && !stop_flag) 
        pthread_cond_wait(&q->cond_not_empty, &q->lock);

    if (stop_flag && q->head == NULL) {
        pthread_mutex_unlock(&q->lock);
        printf("chunk_dequeue: Stop flag detected, returning NULL.\n"); 
        return NULL;
    }

    chunk_queue_node_t* dequeue_node = q->head;
    image_chunk_t* chunk = dequeue_node->chunk; 

    q->head = dequeue_node->next;

    if (q->head == NULL) 
        q->tail = NULL;

    pthread_mutex_unlock(&q->lock);
    free(dequeue_node); 

    //printf("Chunk dequeued successfully (ID: %d)\n", chunk->chunk_id); 

    return chunk;
}

void broadcast_chunk_queue(chunk_queue_t* q) {
    pthread_mutex_lock(&q->lock);
    pthread_cond_broadcast(&q->cond_not_empty);
    pthread_mutex_unlock(&q->lock);
}

void chunk_queue_destroy(chunk_queue_t* q) {
    if (q == NULL) 
        return;

    chunk_queue_node_t* curr = q->head;
    chunk_queue_node_t* temp;

    while(curr != NULL) {
        free_image_chunk(curr->chunk);
        temp = curr;
        curr = curr->next;
        free(temp);     
    }

    q->head = NULL;
    q->tail = NULL;

    pthread_mutex_destroy(&q->lock);
    pthread_cond_destroy(&q->cond_not_empty);

    printf("Chunk queue destroyed successfully\n");
}

discarded_image_entry_t* discarded_images_head = NULL;
pthread_mutex_t discarded_images_lock;

int discarded_images_init(void) {
    if (pthread_mutex_init(&discarded_images_lock, NULL) != 0) {
        perror("discard_images_init - Cannot initialize mutex");
        return -1;
    }
}

int discarded_images_table_add(const char *filename) {
    discarded_image_entry_t* entry;

    pthread_mutex_lock(&discarded_images_lock);
    HASH_FIND_STR(discarded_images_head, filename, entry);

    if (entry == NULL) {
        entry = (discarded_image_entry_t*)malloc(sizeof(discarded_image_entry_t));
        if (!entry) {
            perror("discarded_images_table_add - Failed to allocate memory for hash entry");
            return -1;
        }     

        strncpy(entry->name, filename, sizeof(entry->name) - 1);
        entry->name[sizeof(entry->name) - 1] = '\0';

        HASH_ADD_STR(discarded_images_head, name, entry);
    }

    pthread_mutex_unlock(&discarded_images_lock);

    return 0;
}

bool discarded_images_table_contains(const char *filename) {
    discarded_image_entry_t* entry;

    pthread_mutex_lock(&discarded_images_lock);
    HASH_FIND_STR(discarded_images_head, filename, entry);
    pthread_mutex_unlock(&discarded_images_lock);

    return entry != NULL;
}

void free_discarded_images_table(void) {
    discarded_image_entry_t* current_entry, *tmp;

    pthread_mutex_lock(&discarded_images_lock);
    HASH_ITER(hh, discarded_images_head, current_entry, tmp) {
        HASH_DEL(discarded_images_head, current_entry); 
        free(current_entry); 
    }

    pthread_mutex_unlock(&discarded_images_lock);
    pthread_mutex_destroy(&discarded_images_lock);
}