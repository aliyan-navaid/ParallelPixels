#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "image.h"

image_t image_from_chunks(image_chunk_t *chunks, int num_chunks);
void write_image(image_t *image, const char *path);
void cleanup_chunks(image_chunk_t *chunks, int num_chunks);
void cleanup_image(image_t *image);