#include "filter.h"

#include <stdio.h>

extern chunk_queue_t filtering_reconstruction_queue;

void greyscale(image_chunk_t* chunk) {
    
    if (!chunk) {
        fprintf(stderr, "Error: chunk is NULL\n");
        return;
    }

    if (!chunk->pixel_data) {
        fprintf(stderr, "Error: pixel_data is NULL\n");
        return;
    }

    int width = chunk->width; 
    int height = chunk->height;
    int channels = chunk->channels;

    for (int i=0; i<width*height*channels; i+=channels) {
        unsigned char* pixel = chunk->pixel_data;
        unsigned char gray = (unsigned char)(0.299 * pixel[i+0] + 0.587 * pixel[i+1] + 0.114 * pixel[i+2]); // assuming RGB
        pixel[0] = gray;
        pixel[1] = gray;
        pixel[2] = gray;
    }
}

void directional_blur(image_chunk_t* chunk, int line_size) {
    
    if (!chunk) {
        fprintf(stderr, "Error: chunk is NULL\n");
        return;
    }

    if (!chunk->pixel_data) {
        fprintf(stderr, "Error: pixel_data is NULL\n");
        return;
    }

    int width = chunk->width; 
    int height = chunk->height;
    int channels = chunk->channels;
    unsigned char* pixel = chunk->pixel_data;

    for (long long int i = 0; i < width * height * channels; i+=channels*line_size) {
        int total_r = 0;
        int total_g = 0;
        int total_b = 0;

        for (int j=0; j<line_size*channels; j+=channels) {
            total_r += pixel[i+j+0];
            total_g += pixel[i+j+1];
            total_b += pixel[i+j+2];
        }
        
        unsigned char r = total_r/line_size;
        unsigned char b = total_b/line_size;
        unsigned char g = total_g/line_size;
        
        for (int j=0; j<line_size*channels; j+=channels) {
            pixel[i+j+0] =r;
            pixel[i+j+1] = g;
            pixel[i+j+2] = b;
        }
    }
}

void posterize(image_chunk_t* chunk, int levels) {
    if (!chunk) {
        fprintf(stderr, "Error: chunk is NULL\n");
        return;
    }
    if (!chunk->pixel_data) {
        fprintf(stderr, "Error: pixel_data is NULL\n");
        return;
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
}