#pragma once

#include<image.h> 

unsigned char *load_image(const char *filename, int *width, int *height, int *channels);
void *chunk_image_thread(void *arg);
void free_image_chunk(image_chunk_t *chunk);