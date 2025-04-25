#include<stdio.h> 
#include<dirent.h>         
#include<string.h>         
#include<unistd.h>        
#include<pthread.h>    
#include<stdbool.h>      
#include<signal.h>        
#include<stdlib.h>     
#include<directory_monitor.h>
#include<file_tracker.h>
#include<image_queue.h>       

extern volatile sig_atomic_t stop_flag; 
extern image_name_queue_t name_queue;

void *read_images_from_directory(void *arg) {
    const char *directoryPath = (const char *)arg;
    struct dirent *entry;
    DIR *dir = NULL;

    dir = opendir(directoryPath);
    if (dir == NULL) {
        perror("read_images_from_directory - Cannot open directory");
        return NULL; 
    }

    while ((entry = readdir(dir)) != NULL) {
        if (stop_flag) break; 

        bool should_process = false;
        char current_filename[256];
        strncpy(current_filename, entry->d_name, sizeof(current_filename) - 1);
        current_filename[sizeof(current_filename) - 1] = '\0';

        if (strstr(current_filename, ".jpg") || strstr(current_filename, ".png")) {
            if (!was_file_processed(current_filename)) {
                add_processed_file(current_filename);
                should_process = true;
            }
        }

        if (should_process) {
            char imagePath[1024];
            snprintf(imagePath, sizeof(imagePath), "%s/%s", directoryPath, current_filename);
            if (enqueue_image_name(&name_queue, imagePath) != 0) 
                fprintf(stderr, "read_images_from_directory: Image name enqueue failed");
        }
    }

    closedir(dir);
    dir = NULL; 
    if (stop_flag) return NULL; 

    while (!stop_flag) { 
        dir = opendir(directoryPath);
        if (dir == NULL) {
            perror("read_images_from_directory - Cannot open directory for monitoring");
            if (stop_flag) break;
            sleep(5); 
            continue; 
        }

        while ((entry = readdir(dir)) != NULL) {
            if (stop_flag) break; 
            
            bool should_process = false;
            char current_filename[256];
            strncpy(current_filename, entry->d_name, sizeof(current_filename) - 1);
            current_filename[sizeof(current_filename) - 1] = '\0';

            if (strstr(current_filename, ".jpg") || strstr(current_filename, ".png")) {
                if (!was_file_processed(current_filename)) {
                    add_processed_file(current_filename);
                    should_process = true;
                }
            }

            if (should_process) {
                char imagePath[1024];
                snprintf(imagePath, sizeof(imagePath), "%s/%s", directoryPath, current_filename);
                if (enqueue_image_name(&name_queue, imagePath) != 0) 
                    fprintf(stderr, "read_images_from_directory: Cannot enqueue image name");
            }
        }
        
        closedir(dir);
        dir = NULL; 

        if (stop_flag) break;

        sleep(5); 
    }

    if (dir != NULL) closedir(dir);

    return NULL;
}