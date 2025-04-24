#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

#include "Object.h"
#include "dlist.h"
#include "image.h"

const char *generate_suffix(const char **effects, int num_effects);

/* Generates a new path for the output file.
* @param output_dir The directory where the output file will be saved.
* @param original_path The original file path.
* @param suffix The suffix to be added to the output file name.
* @return A new path for the output file.
* @note The function will create a new path by combining the output directory, original file name, and suffix.
* The output file name will be in the format: `<output_dir>/<original_file_name>_<suffix>.<extension>`.
*/
char *result_path(const char *output_directory, const char *original_path, const char *suffix);

/*
* @brief Reconstruct an image from a list of chunks.
* @param chunks A list of image chunks implemented as dlist_t (doubly linked list).
* @return An image_t structure containing the reconstructed image.
* @note The order of the chunks doesn't matter. 
* @note The function assumes that the chunks are valid and completes the image reconstruction.
*/
image_t image_from_chunks(dlist_t *chunks);

/*
* @brief Write an image to a file.
* @param *image The image to write.
* @param *path The path to the output file.
* @note The function uses stb_image_write to write the image in PNG format.
*/
void write_image(image_t image, const char *path);

/*
* @brief Given the image_t structure, it frees the data contained with in it. 
* @note The function assumes that the image_t structure is valid and has been initialized. 
* @note You would need to manage the location of image_t structure separately.
*/
void cleanup_image(image_t *image);