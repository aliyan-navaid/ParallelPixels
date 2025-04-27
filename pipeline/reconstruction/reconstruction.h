#pragma once

#include <stdlib.h>
#include <string.h>
#include <xxhash.h>
#include <pthread.h>
#include <signal.h>
#include <assert.h>
#include <stdbool.h>

#include "Object.h"
#include "dict.h"
#include "dlist.h"
#include "image.h"
#include "image_unchunk.h"
#include "thread_pool.h"

#define RECONSTRUCTION_THREADS 4

extern volatile sig_atomic_t stop_flag;
extern const char* out_directory;

// initialize reconstruction module in a separate thread.
pthread_t* init_reconstruction(chunk_queue_t *processed_queue);