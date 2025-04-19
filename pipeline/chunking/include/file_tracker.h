#pragma once

#include<uthash.h>
#include<stdbool.h>

typedef void (*char_process_function_ptr)(const char*);
typedef void (*void_process_function_ptr)(void);

// Structure for the hash table entries
typedef struct {
    char name[256];
    UT_hash_handle hh; 
/*     char_process_function_ptr add_processed_file;
    char_process_function_ptr was_file_processed;
    void_process_function_ptr free_processed_files; */
} processed_file_t;

void processed_files_init(void);
void add_processed_file(const char *filename);
bool was_file_processed(const char *filename);
void free_processed_files(void);

extern processed_file_t *processed_files; 