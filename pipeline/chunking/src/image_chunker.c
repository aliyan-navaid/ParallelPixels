#define STB_IMAGE_IMPLEMENTATION 

#include<stdio.h>      
#include<stdlib.h>    
#include<string.h>    
#include<math.h>      
#include<signal.h>     
#include<pthread.h>    
#include<stb_image.h>  
#include<image_chunker.h>     
#include<image_queue.h>      

extern volatile sig_atomic_t stop_flag;
extern image_name_queue_t name_queue;
extern chunk_queue_t chunker_filtering_queue;

unsigned char *load_image(const char *filename, int *width, int *height, int *channels) {
    unsigned char *data = stbi_load(filename, width, height, channels, 0);
    if (data == NULL) {
        fprintf(stderr, "Error loading image '%s': %s\n", filename, stbi_failure_reason());
        return NULL;
    }

    printf("Loaded image '%s' (%d x %d, %d channels)\n", filename, *width, *height, *channels);
    return data;
}

static int create_chunks_internal(const char *original_filename,
                                              unsigned char *image_data,
                                              int width, int height, int channels,
                                              int chunk_width, int chunk_height)
{
    if (!image_data || width <= 0 || height <= 0 || channels <= 0 || chunk_width <= 0 || chunk_height <= 0) {
        fprintf(stderr, "Thread %lu: create_chunks_internal: Invalid input parameters for %s.\n", pthread_self(), original_filename);
        return -1;
    }

    int num_chunks_x = (int)ceil((double)width / chunk_width);
    int num_chunks_y = (int)ceil((double)height / chunk_height);
    int num_chunks_total = num_chunks_x * num_chunks_y;

    if (num_chunks_total <= 0) {
         fprintf(stderr, "Thread %lu: create_chunks_internal: Calculated zero chunks for %s.\n", pthread_self(), original_filename);
         return -1;
    }

/*     image_chunk_t **chunks = (image_chunk_t **)malloc(*num_chunks_total * sizeof(image_chunk_t *));
    if (chunks == NULL) {
        perror("create_chunks_internal: Failed to allocate memory for chunk array");
        return NULL; // Should return -1 to match function signature
    } */

    int current_chunk_index = 0;
    int exit_status = 0; // Track if any chunk fails

    // channels -> RGB, Grayscale, etc.
    // RGB contains 3 bytes per pixel (0-255, 0-255, 0-255)

    size_t bytes_per_pixel = channels;

    printf("Thread %lu: Creating %d chunks for %s...\n", pthread_self(), num_chunks_total, original_filename);

    for (int cy = 0; cy < num_chunks_y && !stop_flag; cy++) { // Check stop_flag
        for (int cx = 0; cx < num_chunks_x && !stop_flag; cx++) { // Check stop_flag
            // Allocate chunk
            image_chunk_t *chunk = (image_chunk_t*)malloc(sizeof(image_chunk_t));
            if (chunk == NULL) {
                perror("create_chunks_internal: Failed to allocate memory for chunk struct");
                // free_image_chunk(chunk); // chunk is NULL here, no need to call free
                exit_status = -1; // Mark failure
                goto cleanup_loop; // Exit loops if allocation fails
            }
            // Initialize pointers to NULL for safe freeing in case of partial allocation failure
            chunk->original_image_name = NULL;
            chunk->pixel_data = NULL;


            chunk->offset_x = cx * chunk_width;
            chunk->offset_y = cy * chunk_height;

            /*
                Boundary values needs to be checked, if the current chunk exceeds the bounds of the image, width or height is
                effectively subtracted, the chunk obtained will be smalled than `chunk_width`
            */

            chunk->width = (chunk->offset_x + chunk_width > width) ? (width - chunk->offset_x) : chunk_width;
            chunk->height = (chunk->offset_y + chunk_height > height) ? (height - chunk->offset_y) : chunk_height;
            chunk->channels = channels;
            chunk->chunk_id = current_chunk_index;
            chunk->original_image_num_chunks = num_chunks_total;
            // Store original image dimensions for reconstruction
            chunk->original_image_width = width;
            chunk->original_image_height = height;

            // Use strdup for safer string copy & allocation
            chunk->original_image_name = strdup(original_filename);
            if (chunk->original_image_name == NULL) {
                perror("create_chunks_internal: Failed to allocate memory for chunk filename (strdup)");
                free_image_chunk(chunk); // Safe to call even if pixel_data is NULL
                // chunk = NULL; // Not necessary, chunk is freed
                exit_status = -1;
                goto cleanup_loop;
            }
            // strcpy(chunk->original_image_name, original_filename); // Replaced by strdup

            chunk->data_size_bytes = chunk->width * chunk->height * bytes_per_pixel;
            chunk->pixel_data = (unsigned char*)malloc(chunk->data_size_bytes);
            if (chunk->pixel_data == NULL) {
                perror("create_chunks_internal: Failed to allocate memory for chunk pixel data");
                free_image_chunk(chunk);
                // chunk = NULL; // Not necessary, chunk is freed
                exit_status = -1;
                goto cleanup_loop;
            }

            /*
                The pixel data in the orignal image & chunk is saved as a linear sequeunce of bytes, within each byte is contained
                a single value of R, G or B for a pixel:

                For Example: Consider an image containing 2 pixels only (each pixel takes 3 bytes due to RGB channel)
                unsigned char* image_data = [0, 255, 255, 255, 255, 255]

                The sequence of bytes is stored as follows: [P(0, 0)-R, P(0, 0)-G, P(0, 0)-B, P(0, 1)-R, P(0, 1)-G, P(0, 1)-B]
            */

            size_t src_row_stride = width * bytes_per_pixel; // pixels per row in the image
            // size_t chunk_row_stride = chunk_width * bytes_per_pixel; // Incorrect for memcpy size if chunk_width != chunk->width
            size_t chunk_row_bytes = chunk->width * bytes_per_pixel; // Bytes to copy per row for this chunk

            for(size_t row = 0; row < chunk->height; row++) {

                /*
                    Writing the bytes row by row, because data for a single chunk is not contigously stored.
                    `(chunk->offset_y + row) * src_row_stride` is the number of bytes to skip from the start of the image.
                    `chunk->offset_x * bytes_per_pixel` is the number of bytes to skip from the start of the row
                */

                unsigned char *src_ptr = image_data + (chunk->offset_y + row) * src_row_stride + chunk->offset_x * bytes_per_pixel;

                /*
                    The destination pointer only needs to calculate the pointer offset for the current row; Since,
                    only the chunk width number of bytes needs to be written

                */

                unsigned char *dst_ptr = chunk->pixel_data + row * chunk_row_bytes; // Use chunk_row_bytes for destination offset


                memcpy(dst_ptr, src_ptr, chunk_row_bytes); // Copy only the chunk's width worth of bytes
            }

            // Enqueueing the chunk as it is created

            chunk->processing_status = CHUNK_STATUS_CREATED;
            if (chunk_enqueue(&chunker_filtering_queue, chunk) != 0) {
                fprintf(stderr, "Thread %lu: create_chunks_internal: Failed to enqueue chunk %d for %s\n",
                        pthread_self(), current_chunk_index, original_filename);
                // CRITICAL: How to handle this? The chunk is created but not enqueued.
                // Maybe free this specific chunk?
                
                free_image_chunk(chunk); // Free the chunk that failed to enqueue
                // What about previously enqueued chunks? Need a strategy.
                // Maybe return an error code indicating partial failure.
                // For simplicity here, we might just free and continue, but that leaves orphans.
                // A better approach might be to signal downstream to discard based on original_image_name.
                // Or, revert to the original method if this complexity is too high.
                exit_status = -1;
                goto cleanup_loop; // Stop processing this image on enqueue failure
            } else {
                // Successfully enqueued, ownership transferred to queue.
                // Do NOT free 'chunk' here.
            }

            current_chunk_index++;
        }
    }

    cleanup_loop: 
        if (stop_flag) {
            printf("Thread %lu: Stop flag detected during chunk creation for %s.\n", pthread_self(), original_filename);
            exit_status = -1; 
        }

        if (exit_status == 0) 
            printf("Thread %lu: Finished creating %d chunks for %s.\n", pthread_self(), current_chunk_index, original_filename);
        else 
            fprintf(stderr, "Thread %lu: Failed or stopped during chunk creation for %s (processed %d chunks).\n", pthread_self(), original_filename, current_chunk_index);


    return exit_status; 
}

void *chunk_image_thread(void *arg) {
    while(!stop_flag) {

        char* filename = dequeue_image_name(&name_queue);    
        if (filename == NULL) {
            fprintf(stderr, "Chunk Image Thread: Cannot proceed - filename = NULL");
            free(filename);
            continue;
        }

        int width, height, channels;
        
        unsigned char* image_data = load_image(filename, &width, &height, &channels);    
        if (image_data == NULL) {
            fprintf(stderr, "Chunk Image Thread: Cannot proceed - Image Data = NULL");
            continue;
        }

        const int fixed_chunk_width = 128; 
        const int fixed_chunk_height = 128; 

        int calc_chunk_width = (width < fixed_chunk_width)? width: fixed_chunk_width;
        int calc_chunk_height = (height < fixed_chunk_height)? height: fixed_chunk_height;

        if (width > 0 && calc_chunk_width <= 0) calc_chunk_width = 1;
        if (height > 0 && calc_chunk_height <= 0) calc_chunk_height = 1;

        printf("Chunker thread %lu: Processing %s with target chunk size: %dx%d\n",
            pthread_self(), filename, calc_chunk_width, calc_chunk_height);

        int output = create_chunks_internal(
            filename,
            image_data,
            width, height, channels,
            calc_chunk_width, calc_chunk_height 
        );

        stbi_image_free(image_data);
        image_data = NULL;
        free(filename); 
        filename = NULL;

        if (output != 0) {
            fprintf(stderr, "Chunker thread failed for %s.\n", filename);
            // need handling;
        }
    }

    printf("Chunker thread finished successfully.\n");
    
    
    return NULL;
}

void free_image_chunk(image_chunk_t *chunk) {
    if (chunk == NULL)
        return;

    free(chunk->original_image_name);
    free(chunk->pixel_data);
    free(chunk);
}
