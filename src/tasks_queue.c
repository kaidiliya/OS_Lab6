#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>


#include "tasks_queue.h"
//Protects the queue operations
pthread_mutex_t mutex_queue = PTHREAD_MUTEX_INITIALIZER;
//Signaled when the queue becomes non-empty
pthread_cond_t  emptyqueue = PTHREAD_COND_INITIALIZER;
//Signaled when there is space in the queue
pthread_cond_t  fullqueue = PTHREAD_COND_INITIALIZER;



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
    pthread_mutex_lock(&mutex_queue);
    while(q->index == q->task_buf_size){
       pthread_cond_wait(&fullqueue, &mutex_queue);
    }
    q->task_buffer[q->index] = t;
    q->index++;

    pthread_cond_signal(&emptyqueue);
    pthread_mutex_unlock(&mutex_queue);
}


task_t* dequeue_task(tasks_queue_t *q)
{
    pthread_mutex_lock(&mutex_queue);
    //If queue is empty, workers must wait
    while (q->index == 0) {
        pthread_cond_wait(&emptyqueue, &mutex_queue);
    }
    task_t *t = q->task_buffer[q->index-1];
    q->index--;
    pthread_cond_signal(&fullqueue);
    pthread_mutex_unlock(&mutex_queue);
    return t;
}