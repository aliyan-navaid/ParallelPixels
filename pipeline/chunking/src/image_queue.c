#include<stdio.h>      
#include<stdlib.h>    
#include<string.h>    
#include<signal.h>
#include<pthread.h>    
#include<errno.h>      
#include<image_queue.h>       

extern volatile sig_atomic_t stop_flag;

int image_name_queue_init(image_name_queue_t* q) {
    if (q == NULL) 
        return EINVAL; 

    q->head = NULL;
    q->tail = NULL;

    if (pthread_mutex_init(&q->lock, NULL) != 0) {
        perror("image_name_queue_init: Failed to initialize mutex");
        return -1; 
    }

    if (pthread_cond_init(&q->cond_not_empty, NULL) != 0) {
        perror("image_name_queue_init: Failed to initialize condition variable");
        pthread_mutex_destroy(&q->lock); 
        return -1; 
    }
    
    return 0;
}

int enqueue_image_name(image_name_queue_t *q, const char *name) {
    if (q == NULL || name == NULL) 
        return EINVAL;

    image_name_queue_node_t *new_node = (image_name_queue_node_t*)malloc(sizeof(image_name_queue_node_t));
    if (new_node == NULL) {
        perror("enqueue_image_name: Failed to allocate memory for new queue node");
        return -1;
    }
    new_node->next = NULL;

    new_node->name = strdup(name);
    if (new_node->name == NULL) {
        perror("enqueue_image_name: Failed to allocate memory for node name (strdup)");
        free(new_node);
        return -1;
    }

    pthread_mutex_lock(&q->lock);

    if (q->tail == NULL) {
        
        // very unlikely to happen
        if (q->head != NULL) {
            fprintf(stderr, "enqueue_image_name: Queue inconsistency detected (tail is NULL, head is not)\n");
            pthread_mutex_unlock(&q->lock);
            free(new_node->name);
            free(new_node);
            return -1; 
        }

        q->head = new_node;
        q->tail = new_node;
    } 
    
    else { 
        q->tail->next = new_node;
        q->tail = new_node;
    }

    pthread_cond_signal(&q->cond_not_empty);
    pthread_mutex_unlock(&q->lock);

    printf("image name enqueued successfully: %s\n", name); // Debugging

    return 0;
}

char* dequeue_image_name(image_name_queue_t *q) {
    if (q == NULL) 
        return NULL; 

    pthread_mutex_lock(&q->lock);

    while (q->head == NULL && !stop_flag) 
        pthread_cond_wait(&q->cond_not_empty, &q->lock);

    if (stop_flag && q->head == NULL) { 
        pthread_mutex_unlock(&q->lock);
        printf("dequeue: Stop flag detected, returning NULL.\n"); 
        return NULL; 
    }

    image_name_queue_node_t* dequeue_node = q->head;
    char* name = dequeue_node->name; 

    q->head = dequeue_node->next;

    if (q->head == NULL) 
        q->tail = NULL;

    pthread_mutex_unlock(&q->lock);

    free(dequeue_node); 

    printf("image name dequeued successfully: %s\n", name); 

    return name; 
}

void broadcast_image_name_queue(image_name_queue_t* q) {
    pthread_mutex_lock(&q->lock);
    pthread_cond_broadcast(&q->cond_not_empty);
    pthread_mutex_unlock(&q->lock);
}

void image_name_queue_destroy(image_name_queue_t* q) {
    if (q == NULL) 
        return;

    image_name_queue_node_t* curr = q->head;
    image_name_queue_node_t* temp;

    while(curr != NULL) {
        free(curr->name); 
        temp = curr;
        curr = curr->next;
        free(temp);  
    }

    q->head = NULL;
    q->tail = NULL;

    pthread_mutex_destroy(&q->lock);
    pthread_cond_destroy(&q->cond_not_empty);

    printf("Image Name queue destroyed successfully\n");
}