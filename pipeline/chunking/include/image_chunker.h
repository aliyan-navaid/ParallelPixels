#pragma once

#include<image.h> 

unsigned char *load_image(const char *filename, int *width, int *height, int *channels);
void *chunk_image_thread(void *arg);