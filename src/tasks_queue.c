#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>


#include "tasks_queue.h"

pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  emptyqueue = PTHREAD_COND_INITIALIZER;
pthread_cond_t  fullqueue = PTHREAD_COND_INITIALIZER;
int nb_waiting_th=0;


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
    nb_waiting_th++;
    while(q->index == q->task_buf_size){
        // if(nb_waiting_th==THREAD_COUNT){
        //     q->task_buf_size = QUEUE_CAPACITY*2;
        //     q->task_buffer = (task_t**) realloc(q->task_buffer,sizeof(task_t*) * q->task_buf_size);
        // }
        
        pthread_cond_wait(&fullqueue, &mutex1);
    }
    nb_waiting_th--;
    q->task_buffer[q->index] = t;
    q->index++;
    pthread_cond_broadcast(&emptyqueue);
    pthread_mutex_unlock(&mutex1);
}


task_t* dequeue_task(tasks_queue_t *q)
{
    pthread_mutex_lock(&mutex1);
    while (q->index == 0) {
        pthread_cond_wait(&emptyqueue, &mutex1);
    }
    task_t *t = q->task_buffer[--q->index]; // LIFO
    pthread_cond_signal(&fullqueue);
    pthread_mutex_unlock(&mutex1);
    return t;
}