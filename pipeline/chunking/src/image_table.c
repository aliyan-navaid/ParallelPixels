#include<image_table.h>

processed_file_t *processed_files = NULL; // <--- Definition without extern

// Function to add a filename to the hash table
void add_processed_file(const char *filename) {
    processed_file_t *entry = malloc(sizeof(processed_file_t));
    if (!entry) {
        perror("Failed to allocate memory for hash entry");
        return; // Or handle error more robustly
    }

    strncpy(entry->name, filename, sizeof(entry->name) - 1);
    entry->name[sizeof(entry->name) - 1] = '\0'; // Ensure null termination
    HASH_ADD_STR(processed_files, name, entry);
}

// Function to check if a filename is in the hash table
bool was_file_processed(const char *filename) {
    processed_file_t *entry;
    HASH_FIND_STR(processed_files, filename, entry);
    return entry != NULL;
}

// Function to free the hash table (call this at cleanup)
void free_processed_files() {
    processed_file_t *current_entry, *tmp;
    HASH_ITER(hh, processed_files, current_entry, tmp) {
        HASH_DEL(processed_files, current_entry); // Remove from hash
        free(current_entry); // Free the structure
    }
}