#pragma once

#include<pthread.h> 

struct image_name_queue_node;

typedef struct image_name_queue_node {
    struct image_name_queue_node* next; 
    char* name;                         
} image_name_queue_node_t;

typedef struct {
    image_name_queue_node_t* head;          
    image_name_queue_node_t* tail;          
    pthread_mutex_t lock;               
    pthread_cond_t cond_not_empty;      
} image_name_queue_t;

int image_name_queue_init(image_name_queue_t* q);
int enqueue_image_name(image_name_queue_t *q, const char *name);
char* dequeue_image_name(image_name_queue_t *q);
void broadcast_image_name_queue(image_name_queue_t* q);
void image_name_queue_destroy(image_name_queue_t* q);