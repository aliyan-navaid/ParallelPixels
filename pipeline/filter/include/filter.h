#pragma once

#include "image_chunker.h" 

int greyscale(image_chunk_t* chunk);
int posterize(image_chunk_t* chunk, int levels);
int directional_blur(image_chunk_t* chunk, int line_size);