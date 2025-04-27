#include <pthread.h>
#include <image.h>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <image_chunker.h>
#include <chunk_threader.h>
#include <filter.h>

#include "macros.h"

extern volatile sig_atomic_t stop_flag;
extern const char* out_directory;
extern const char* effects;

void *process_chunk(void *arg) {
    while (!stop_flag) {
        image_chunk_t *chunk = chunk_dequeue(&chunker_filtering_queue);
        
        
        if (chunk == NULL || stop_flag || discarded_images_table_contains(chunk->original_image_name)) {
            // If the queue is empty, image discarded, wait or check for stop_flag
            continue;
        }
        
        if (!effects) {
            stop_flag =1;
            continue;;
        }
        
        int filter_result = EXIT_FAILURE;
        if (strcmp(effects, "greyscale") == 0) {
            filter_result = greyscale(chunk);
        } else if (strcmp(effects, "posterize") == 0) {
            filter_result = posterize(chunk, 4);
        } else if (strcmp(effects, "directional_blur") == 0) {
            filter_result = directional_blur(chunk, 50);
        } else {
            FPRINTF(stderr, "Unknown effect: %s\n", effects);
            stop_flag = 1;
            continue;;
        }

        if (filter_result != EXIT_SUCCESS) {
            stop_flag = 1;
            continue;;
        }
        
        // Enqueue the filtered chunk into the next queue
        if (chunk_enqueue(&filtering_reconstruction_queue, chunk) != 0) {
            FPRINTF(stderr, "Error: Failed to enqueue filtered chunk (ID: %d).\n", chunk->chunk_id);
            free_image_chunk(chunk); // Free the chunk if enqueueing fails
            chunk = NULL;
        }
    }

    return NULL;
}
/*
void assign_threads_to_chunk(void) {
    while (!stop_flag) {

        // Create a new thread for the chunk
        pthread_t thread;
        int result = pthread_create(&thread, NULL, process_chunk, (void*) NULL);
        
        if (result) {
            perror("Error creating thread for chunk");
            continue;
        }
        
        // Detach the thread to avoid needing to join it later
        pthread_detach(thread);
    }

    pthread_mutex_lock(&chunker_filtering_queue.lock);
        pthread_cond_broadcast(&chunker_filtering_queue.cond_not_empty);
    pthread_mutex_unlock(&chunker_filtering_queue.lock);
}
*/