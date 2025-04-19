#pragma once

#include "image_chunker.h" 

void greyscale(image_chunk_t* chunk);
void posterize(image_chunk_t* chunk, int levels);
void directional_blur(image_chunk_t* chunk, int line_size);