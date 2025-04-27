#include<signal.h>
#include<stdlib.h>
#include<unistd.h>
#include<dirent.h>
#include<errno.h>
#include<stdio.h>
#include<stdbool.h>
#include<sys/stat.h>

#include<image.h>
#include<image_queue.h>
#include<file_tracker.h>
#include<image_chunker.h>
#include<directory_monitor.h>
#include<chunk_threader.h>
#include<stdatomic.h>

#include "reconstruction.h"
#include "macros.h"

image_name_queue_t name_queue;
chunk_queue_t chunker_filtering_queue, filtering_reconstruction_queue;

const char* input_directory = "../images";
const char* out_directory = "../filtered_images";
const char* effects = NULL;

atomic_size_t total_images_read = 0;
atomic_size_t total_images_written = 0;
atomic_size_t total_images_discarded = 0;

void* update_stats(void* param) {    
    printf("Total Images Read:      %zu\033[K\n", total_images_read);
    printf("Total Images Written:   %zu\033[K\n", total_images_written);
    printf("Total Images Discarded: %zu\033[K\n", total_images_discarded);
    fflush(stdout); 

    while (!stop_flag) {

        printf("\033[3A");
        printf("\rTotal Images Read:      %zu\033[K\n", total_images_read);
        printf("Total Images Written:   %zu\033[K\n", total_images_written);
        printf("Total Images Discarded: %zu\033[K\n", total_images_discarded);
        printf("Enter 'e' to Exit: ");
        fflush(stdout); 

        usleep(100000);
    }

    return NULL;
}

void* input_listener(void* param) {
    printf("\nPress 'e' then Enter to initiate shutdown via SIGINT.\n");
    int character;
    while (true) {
        character = getchar(); 

        if (character == 'e') {
            printf("Exiting The process.\n\n");
            fflush(stdout);
            raise(SIGINT);
            break; 
        } else if (character == EOF) {
            printf("Input stream closed. Raising SIGINT...\n");
            fflush(stdout);
            raise(SIGINT);
            break;
        }
    }
    return NULL;
}

bool is_directory(const char* path) {
    struct stat path_stat;
    if (stat(path, &path_stat) != 0) {
        perror("stat failed");
        return false;
    }
    return S_ISDIR(path_stat.st_mode);
}

void arg_parse(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: ppxl <input_directory> -e <effects> -o <output_directory>\n");
        exit(EXIT_FAILURE);
    }

    input_directory = argv[1];

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-e") == 0 && i + 1 < argc) {
            effects = argv[i + 1];
            i++;
        } else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            out_directory = argv[i + 1];
            i++;
        } else {
            fprintf(stderr, "Unknown argument: %s\n", argv[i]);
            fprintf(stderr, "Usage: ppxl <input_directory> -e <effects> -o <output_directory>\n");
            exit(EXIT_FAILURE);
        }
    }

    if (!is_directory(input_directory)) {
        fprintf(stderr, "Error: Input directory '%s' does not exist or is not a directory.\n", input_directory);
        exit(EXIT_FAILURE);
    }

    if (!is_directory(out_directory)) {
        fprintf(stderr, "Error: Output directory '%s' does not exist or is not a directory.\n", out_directory);
        exit(EXIT_FAILURE);
    }

    if (effects == NULL) {
        fprintf(stderr, "Error: Effects not specified. Use -e <effects> to specify effects.\n");
        exit(EXIT_FAILURE);
    }
}

volatile sig_atomic_t stop_flag = 0;

int Initialization(void) {
    if (image_name_queue_init(&name_queue) != 0) {
        FPRINTF(stderr, "Failed to initialize name queue.\n");
        return EXIT_FAILURE;
    }

    if (chunk_queue_init(&chunker_filtering_queue) != 0) {
        FPRINTF(stderr, "Failed to initialize filtering->reconstruction queue.\n");
        image_name_queue_destroy(&name_queue); 
        return EXIT_FAILURE;
    }

    if (chunk_queue_init(&filtering_reconstruction_queue) != 0) {
        FPRINTF(stderr, "Failed to initialize chunker->filtering queue.\n");
        image_name_queue_destroy(&name_queue); 
        chunk_queue_destroy(&chunker_filtering_queue);
        return EXIT_FAILURE;
    }

    if (discarded_images_init() != 0) {
        FPRINTF(stderr, "Failed to initialize discarded images table.\n");
        image_name_queue_destroy(&name_queue);
        chunk_queue_destroy(&chunker_filtering_queue);
        chunk_queue_destroy(&filtering_reconstruction_queue);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

void cleanup_resources(void) {
    free_processed_files(); 
    image_name_queue_destroy(&name_queue);
    chunk_queue_destroy(&chunker_filtering_queue);
    chunk_queue_destroy(&filtering_reconstruction_queue);
    free_discarded_images_table();
}

void ExitHandler(int signum) {
    stop_flag = 1;
/*     const char msg[] = "\nSignal received, initiating shutdown...\n";
    write(STDOUT_FILENO, msg, sizeof(msg) - 1); */
}

int main(int argc, char* argv[]) {

    // For Debugging purposes only!
    //argc = 6; argv[1] = "../images"; argv[2] = "-e"; argv[3] = "greyscale"; argv[4] = "-o"; argv[5] = "../filtered_images";

    arg_parse(argc, argv);

    const char *directoryPath = input_directory;
    int exit_status = 0;

    long cores = sysconf(_SC_NPROCESSORS_ONLN);
    const size_t num_chunker_threads = (cores > 1)? (size_t)cores: 2; 

    pthread_t watcher_thread, stats_updater, input_thread;
    pthread_t *chunker_threads = NULL;

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = ExitHandler; 
    sigfillset(&sa.sa_mask);       
    sa.sa_flags = 0;                

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("Failed to register SIGINT handler");
        return EXIT_FAILURE;
    }

    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        perror("Failed to register SIGTERM handler");
        return EXIT_FAILURE;
    }

    DIR *dir_check = opendir(directoryPath);
    if (dir_check == NULL) {
        char err_msg[512];
        snprintf(err_msg, sizeof(err_msg), "Target directory '%s' does not exist or cannot be opened", directoryPath);
        perror(err_msg);
        FPRINTF(stderr, "Please ensure the '../images' directory exists relative to the build directory.\n");
        return EXIT_FAILURE;
    }

    closedir(dir_check);

    if (Initialization() != EXIT_SUCCESS)
        return EXIT_FAILURE;

    chunker_threads = malloc(num_chunker_threads * sizeof(pthread_t));
    if (chunker_threads == NULL) {
        perror("Failed to allocate memory for chunker thread IDs");
        exit_status = EXIT_FAILURE;
        goto Cleanup;
    }

    PRINTF("Starting input listener thread...\n");
    if (pthread_create(&input_thread, NULL, input_listener, NULL) != 0) { // <<< Create input thread
        perror("Failed to create input listener thread");
        exit_status = EXIT_FAILURE;
        goto Cleanup;
    }

    PRINTF("Starting stats updated thread\n");
    if (pthread_create(&stats_updater, NULL, update_stats, (void *)NULL) != 0) { // Removed watcher_attr
        perror("Failed to create stats_updater thread");
        exit_status = EXIT_FAILURE;
        goto Cleanup;
    }
    
    PRINTF("Starting image watcher thread for directory: %s\n", directoryPath);
    if (pthread_create(&watcher_thread, NULL, read_images_from_directory, (void *)directoryPath) != 0) { // Removed watcher_attr
        perror("Failed to create watcher thread");
        exit_status = EXIT_FAILURE;
        goto Cleanup;
    }

    PRINTF("Starting %zu chunker threads...\n", num_chunker_threads);
    for (size_t i = 0; i < num_chunker_threads; i++) {
        if (pthread_create(&chunker_threads[i], NULL, chunk_image_thread, NULL) != 0) {
            perror("Failed to create a chunker thread");

            stop_flag = 1; 

            broadcast_image_name_queue(&name_queue);
            broadcast_chunk_queue(&chunker_filtering_queue);
            broadcast_chunk_queue(&filtering_reconstruction_queue);

            for (size_t j = 0; j < i; j++) 
                pthread_join(chunker_threads[j], NULL);
            
            pthread_join(watcher_thread, NULL);
            exit_status = EXIT_FAILURE;
            goto Cleanup;
        }
    }
    
    // Aliyan TODO: instead of a function call, create threads here (Use thread pool).
    // Join them later
    unsigned int num_filter_threads = num_chunker_threads;
    pthread_t *filter_threads = (pthread_t *) malloc(num_filter_threads * sizeof(pthread_t));
    PRINTF("Starting %zu image processing threads...\n", num_filter_threads);
    for (size_t i = 0; i < num_filter_threads; i++) {
        if (pthread_create(&filter_threads[i], NULL, process_chunk, NULL) != 0) {
            perror("Failed to create a filter thread");

            stop_flag = 1; 

            broadcast_image_name_queue(&name_queue);
            broadcast_chunk_queue(&chunker_filtering_queue);
            broadcast_chunk_queue(&filtering_reconstruction_queue);

            for (size_t j = 0; j < i; j++) 
                pthread_join(filter_threads[j], NULL);
            for (size_t j = 0; j < num_chunker_threads; j++) 
                pthread_join(chunker_threads[j], NULL);
            
            pthread_join(watcher_thread, NULL);
            exit_status = EXIT_FAILURE;
            goto Cleanup;
        }
    }

    // #################################################################
    // calls the reconstruction thread which recieves the chunks from the filtering 
    // stage and reconstructs the image.
    // The reconstruction thread is not a part of the chunker threads.
    //
    pthread_t* recon_thread = init_reconstruction(&filtering_reconstruction_queue);
    // 
    // #################################################################

    PRINTF("Watcher thread started. Waiting for signal (SIGINT/SIGTERM)...\n");
    while (!stop_flag) 
        sleep(1);

    PRINTF("\nShutdown signal received.\n");
    PRINTF("Attempting to cancel watcher & chunker thread (if possible)...\n");
    pthread_join(watcher_thread, NULL); 

    PRINTF("Broadcasting to chunker threads...\n");
    broadcast_image_name_queue(&name_queue);
    broadcast_chunk_queue(&chunker_filtering_queue);
    broadcast_chunk_queue(&filtering_reconstruction_queue);

    PRINTF("Waiting for chunker threads to finish...\n");
    for (size_t i = 0; i < num_chunker_threads; i++) {
        pthread_join(chunker_threads[i], NULL);
        PRINTF("Chunker thread %zu finished.\n", i);
    }

    // join filtering threads here. 
    PRINTF("Waiting for filtering threads to finish...\n");
    for (size_t i = 0; i < num_filter_threads; i++) {
        pthread_join(filter_threads[i], NULL);
        PRINTF("Filtering thread %zu finished.\n", i);
    }   

    // join reconstructio threads here.
    pthread_join(*recon_thread, NULL);
    free(recon_thread);

    pthread_join(stats_updater, NULL);

    pthread_join(input_thread, NULL);

    PRINTF("All chunker threads finished.\n");

    PRINTF("Cleaning up resources...\n");

    Cleanup:
        cleanup_resources();
        free(chunker_threads);
        chunker_threads = NULL;

    PRINTF("Cleanup complete. Exiting.\n");

    return exit_status; 
}