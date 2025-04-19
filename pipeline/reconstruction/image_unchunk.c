#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <assert.h>
#include "stb_image_write.h"
#include "image.h"
#include "image_unchunk.h"
#include "image_chunker.h"

const char *generate_suffix(const char **effects, int num_effects) {
    return "processed"; // simple for now
}

static inline const char* get_dir(const char* path) {
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

static inline const char* get_filename(const char* path) {
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
    return base_name_copy;
}

const char *result_path(const char *output_dir, const char *original_path, const char *suffix) {
    assert(original_path != NULL);
    
    if (!output_dir) {
        output_dir = get_dir(original_path);
    } 

    char *output_filename = get_filename(original_path);
    assert(output_filename != NULL && output_dir != NULL 
        && strlen(output_filename) > 0 && suffix != NULL
    );
    
    char *new_path = (char *)malloc(strlen(output_dir) + strlen(output_filename) + strlen(suffix) + 3);
    sprintf(new_path, "%s/%s_%s", output_dir, output_filename, suffix);

    free(output_filename);

    return new_path;
}

static inline image_t create_empty_image(int width, int height, int channels) {
    image_t image;
    image.width = width;
    image.height = height;
    image.channels = channels;

    image.pixel_data = (unsigned char *)malloc(width * height * channels);
    memset(image.pixel_data, 0, width * height * channels);

    return image;
}

static inline size_t convert_to_index(int x, int y, size_t width, size_t cell_size) {
    return (y * width + x) * cell_size;
}

static inline void write_image(image_t *image, const char *path) {
    // write the image to a file
    assert(image != NULL && path != NULL);

    int result = stbi_write_png(path, image->width, image->height, image->channels, image->pixel_data, image->width * image->channels);
    assert(result != 0);
}

image_t image_from_chunks(image_chunk_t *chunks, int num_chunks) {
    // given a list of chunks, reconstruct the image
    assert(chunks != NULL && num_chunks > 0);

    // first, consturct an empty image from original_image information
    int width = chunks[0].original_image_width;
    int height = chunks[0].original_image_height;
    int channels = chunks[0].channels;
    image_t image = create_empty_image(width, height, channels);

    size_t cell_size = channels;

    // now, fill the image with the pixel data from the chunks
    for (int i = 0; i < num_chunks; ++i) {
        image_chunk_t *chunk = &chunks[i];
        assert(chunk->pixel_data != NULL);

        for (int y = 0; y < chunk->height; ++y) {
            for (int x = 0; x < chunk->width; ++x) {

                size_t src_index = convert_to_index(x, y, chunk->width, cell_size);
                size_t dst_index = convert_to_index(chunk->offset_x + x, chunk->offset_y + y, width, cell_size);

                memcpy(image.pixel_data + dst_index, chunk->pixel_data + src_index, cell_size);
            }
        }
    }

    return image;
}

void cleanup_chunks(image_chunk_t *chunks, int num_chunks) {
    // free the chunks
    assert(chunks != NULL && num_chunks > 0);

    for (int i = 0; i < num_chunks; ++i) {
        free_image_chunk(&chunks[i]);
    }

    free(chunks);
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