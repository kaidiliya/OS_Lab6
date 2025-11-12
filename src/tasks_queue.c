#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>


#include "tasks_queue.h"

static pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  cond1 = PTHREAD_COND_INITIALIZER;



tasks_queue_t* create_tasks_queue(void)
{
    tasks_queue_t *q = (tasks_queue_t*) malloc(sizeof(tasks_queue_t));

    q->task_buf_size = QUEUE_CAPACITY;
    q->task_buffer = (task_t**) malloc(sizeof(task_t*) * q->task_buf_size);

    q->index = 0;

    return q;
}


void free_tasks_queue(tasks_queue_t *q)
{
    /* IMPORTANT: We chose not to free the queues to simplify the
     * termination of the program (and make debugging less complex) */
    
    /* free(q->task_buffer); */
    /* free(q); */
}



void enqueue_task(tasks_queue_t *q, task_t *t)
{
    pthread_mutex_lock(&mutex1);
    if(q->index == q->task_buf_size){
        fprintf(stderr,"ERROR: the queue of tasks is full\n");
        exit(EXIT_FAILURE);
    }
    q->task_buffer[q->index] = t;
    q->index++;
    pthread_cond_signal(&cond1);
    pthread_mutex_unlock(&mutex1);
}


task_t* dequeue_task(tasks_queue_t *q)
{
    pthread_mutex_lock(&mutex1);
    while (q->index == 0) {
        pthread_cond_wait(&cond1, &mutex1);
    }
    task_t *t = q->task_buffer[--q->index]; // LIFO
    pthread_mutex_unlock(&mutex1);
    return t;
}