#include<stdio.h>     
#include<stdlib.h>     
#include<string.h>     
#include<stdbool.h>    
#include<file_tracker.h> 

processed_file_t *processed_files = NULL; 

void add_processed_file(const char *filename) {
    processed_file_t *entry = malloc(sizeof(processed_file_t));
    if (!entry) {
        perror("add_processed_file - Failed to allocate memory for hash entry");
        return; 
    }

    strncpy(entry->name, filename, sizeof(entry->name) - 1);
    entry->name[sizeof(entry->name) - 1] = '\0'; 
    HASH_ADD_STR(processed_files, name, entry);
}

bool was_file_processed(const char *filename) {
    processed_file_t *entry;
    HASH_FIND_STR(processed_files, filename, entry);
    return entry != NULL;
}

void free_processed_files(void) {
    processed_file_t *current_entry, *tmp;
    HASH_ITER(hh, processed_files, current_entry, tmp) {
        HASH_DEL(processed_files, current_entry); 
        free(current_entry); 
    }
}