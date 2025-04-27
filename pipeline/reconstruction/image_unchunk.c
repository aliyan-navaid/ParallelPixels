#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "image_unchunk.h"
#include <stb_image_write.h>

char *generate_suffix(const char **effects, int num_effects) {
    return strdup("processed"); // simple for now
}

/*
* @brief Get the directory part of a given path.
* @param path The file path.
* @return The directory part of the path.
* @note The function will return a copy of the directory part,
* so you need to manage it separately.
*/
static inline char* get_dir(const char* path) {
    // given a path, return the directory part
    // if no directory is found, return "."

    char* dir = strdup(path); // make a copy to avoid modifying the original
    char* last_slash = strrchr(dir, '/'); // find the last slash

    if (last_slash) {
        *last_slash = '\0'; // terminate the string at the last slash
    } else {
        free(dir); // no slash found, free the copy
        dir = strdup("."); // return current directory
    }

    return dir;
}

/*
* @brief Get the file name (not the extension) from a given path.
* @param path The file path.
* @return The file name part of the path.
* @note The function will return a copy of the file name part, 
* so you need to manage it separately.
*/
static inline char* get_filename(const char* path) {
    // given a path, return the file name part
    // if no file name is found, return the original path

    const char* base_name = strrchr(path, '/'); // find the last slash

    if (base_name) {
        base_name++; // move past the '/'
    } else {
        base_name = path; // no '/' found, use the whole filename
    }

    // now create a copy of the basename to keep it separate from input path
    char* base_name_copy = strdup(base_name);

    // now remove extension
    char* dot = strrchr(base_name_copy, '.'); // find the last dot
    if (dot) {
        *dot = '\0'; // terminate the string at the last dot
    }

    return base_name_copy;
}

/*
* @brief Get the extension from a given path.
* @param path The file path.
* @return The extension part of the path.
* @note The function will return a copy of the file name part, 
* so you need to manage it separately.
*/
static inline char* get_extension(const char* path) {
    // given a path, return the file extension part
    // if no extension is found, return NULL

    const char* dot = strrchr(path, '.'); // find the last dot

    if (dot) {
        return strdup(dot + 1); // move past the '.'
    } else {
        return ""; // no extension found, return empty string
    }
}

/* Generates a new path for the output file.
* @param output_dir The directory where the output file will be saved.
* @param original_path The original file path.
* @param suffix The suffix to be added to the output file name.
* @return A new path for the output file.
* @note The function will create a new path by combining the output directory, original file name, and suffix.
* The output file name will be in the format: `<output_dir>/<original_file_name>_<suffix>.<extension>`.
*/
char *result_path(const char *output_directory, const char *original_path, const char *suffix) {
    assert(original_path != NULL);
    
    char* output_dir = NULL;
    if (output_directory) {
        output_dir = strdup(output_directory);
    } else {
        output_dir = get_dir(original_path);
    }

    char *output_filename = get_filename(original_path);
    assert(strlen(output_filename) > 0 && suffix != NULL);

    char* extension = get_extension(original_path);

    char *new_path = (char *)malloc(strlen(output_dir) + 1 + strlen(output_filename) + 1 + strlen(suffix) + 1 + strlen(extension) + 1);
    
    sprintf(new_path, "%s/%s_%s.%s", output_dir, output_filename, suffix, extension);
    // add null terminator
    new_path[strlen(output_dir) + 1 + strlen(output_filename) + 1 + strlen(suffix) + 1 + strlen(extension)] = '\0';

    free(output_filename);
    free(output_dir);
    free(extension);

    return new_path;
}

/* Creates an empty image with the specified `width`, `height`, and number of `channels`.
* @note The pixel data is initialized to zero.
*/
static inline image_t create_empty_image(int width, int height, int channels) {
    image_t image;
    image.width = width;
    image.height = height;
    image.channels = channels;

    image.pixel_data = (unsigned char *)malloc(sizeof(unsigned char) * width * height * channels);
    memset(image.pixel_data, 0, width * height * channels);

    return image;
}

/* given `x`, `y`, row width, and cell size, converts the `(x,y)` coordinate from 
* 2d to 1d index `i`. 
*/
static inline size_t convert_to_index(int x, int y, size_t width, size_t cell_size) {
    return (y * width + x) * cell_size;
}

void write_image(image_t image, const char *path) {
    // write the image to a file
    assert(path != NULL);

    int result = stbi_write_jpg(path, image.width, image.height, image.channels, image.pixel_data, 100);
    // int result = stbi_write_png(path, image.width, image.height, image.channels, image.pixel_data, image.width * image.channels);
    assert(result != 0);
}

image_t image_from_chunks(dlist_t *chunks) {
    assert(chunks != NULL && chunks->size > 0);

    dlist_node_t *node = chunks->head;
    size_t num_chunks = chunks->size;

    int width = get_image_chunk(node->data)->original_image_width;
    int height = get_image_chunk(node->data)->original_image_height;
    int channels = get_image_chunk(node->data)->channels;
    image_t image = create_empty_image(width, height, channels);

    size_t cell_size = channels;

    while (node != NULL) {
        image_chunk_t *chunk = get_image_chunk(node->data);
        assert(chunk->pixel_data != NULL);

        for (int y = 0; y < chunk->height; ++y) {
            for (int x = 0; x < chunk->width; ++x) {

                size_t src_index = convert_to_index(x, y, chunk->width, cell_size);
                size_t dst_index = convert_to_index(chunk->offset_x + x, chunk->offset_y + y, width, cell_size);

                memcpy(image.pixel_data + dst_index, chunk->pixel_data + src_index, cell_size);
            }
        }

        node = node->next;
    }

    return image;
}

void cleanup_image(image_t *image) {
    // free the image
    assert(image != NULL);

    free(image->pixel_data);
    image->pixel_data = NULL;
    image->width = 0;
    image->height = 0;
    image->channels = 0;
}