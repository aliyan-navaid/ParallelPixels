#include "filter.h"

#include <stdio.h>

#define MIN(a,b) a>b ? b : a 

extern chunk_queue_t filtering_reconstruction_queue;

int greyscale(image_chunk_t* chunk) {
    
    if (!chunk) {
        fprintf(stderr, "Error: chunk is NULL\n");
        return EXIT_FAILURE;
    }

    if (!chunk->pixel_data) {
        fprintf(stderr, "Error: pixel_data is NULL\n");
        return EXIT_FAILURE;
    }

    int width = chunk->width; 
    int height = chunk->height;
    int channels = chunk->channels;

    for (int i=0; i<width*height*channels; i+=channels) {
        unsigned char* pixel = chunk->pixel_data;
        unsigned char gray = (unsigned char)(0.299 * pixel[i+0] + 0.587 * pixel[i+1] + 0.114 * pixel[i+2]); // assuming RGB
        pixel[i+0] = gray;
        pixel[i+1] = gray;
        pixel[i+2] = gray;
    }

    return EXIT_SUCCESS;
}

int directional_blur(image_chunk_t* chunk, int line_size) {
    if (!chunk) {
        fprintf(stderr, "Error: chunk is NULL\n");
        return EXIT_FAILURE;
    }
    if (!chunk->pixel_data) {
        fprintf(stderr, "Error: pixel_data is NULL\n");
        return EXIT_FAILURE;
    }

    int width = chunk->width;
    int height = chunk->height;
    int channels = chunk->channels;
    unsigned char* src = chunk->pixel_data;
    unsigned char* dst = malloc(width * height * channels);
    if (!dst) {
        fprintf(stderr, "Error: failed to allocate blur buffer\n");
        return EXIT_FAILURE;
    }

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int total_r = 0, total_g = 0, total_b = 0, count = 0;
            for (int k = 0; k < line_size; ++k) {
                int nx = x + k;
                if (nx >= width) break;
                int idx = (y * width + nx) * channels;
                total_r += src[idx + 0];
                total_g += src[idx + 1];
                total_b += src[idx + 2];
                count++;
            }
            int out_idx = (y * width + x) * channels;
            dst[out_idx + 0] = total_r / count;
            dst[out_idx + 1] = total_g / count;
            dst[out_idx + 2] = total_b / count;
        }
    }

    memcpy(chunk->pixel_data, dst, width * height * channels);
    free(dst);
    return EXIT_SUCCESS;
}

int posterize(image_chunk_t* chunk, int levels) {
    if (!chunk) {
        fprintf(stderr, "Error: chunk is NULL\n");
        return EXIT_FAILURE;
    }
    if (!chunk->pixel_data) {
        fprintf(stderr, "Error: pixel_data is NULL\n");
        return EXIT_FAILURE;
    }

    int width = chunk->width;
    int height = chunk->height;
    int channels = chunk->channels;
    unsigned char* pixel = chunk->pixel_data;

    int step = 256 / levels;

    for (int i = 0; i < width * height * channels; i += channels) {
        for (int c = 0; c < channels; ++c) {
            pixel[i + c] = (pixel[i + c] / step) * step;
        }
    }

    return EXIT_SUCCESS;
}