#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>


#include "tasks_queue.h"

pthread_mutex_t mutexs_q[THREAD_COUNT];

pthread_cond_t  emptyqueue = PTHREAD_COND_INITIALIZER;


tasks_queue_t*create_tasks_queue(void)
{
    for (int i = 0; i < THREAD_COUNT; i++) {
    pthread_mutex_init(&mutexs_q[i], NULL);
    }

    
    tasks_queue_t *q= (tasks_queue_t*) malloc(sizeof(tasks_queue_t)*THREAD_COUNT);
    for(int i=0;i<THREAD_COUNT;i++){

    q[i].task_buf_size = QUEUE_CAPACITY;
    q[i].task_buffer = (task_t**) malloc(sizeof(task_t*) * q[i].task_buf_size);
    q[i].index = 0;
    }

    return q;
}



void free_tasks_queue(tasks_queue_t *q)
{
    /* IMPORTANT: We chose not to free the queues to simplify the
     * termination of the program (and make debugging less complex) */
    
    /* free(q->task_buffer); */
    /* free(q); */
}

  

void enqueue_task(tasks_queue_t *q, task_t *t,int th_nb)
{
    pthread_mutex_lock(&mutexs_q[th_nb]);

    if(q->index +1 == q->task_buf_size){
             q->task_buf_size = q->task_buf_size*2;
             q->task_buffer = (task_t**) realloc(q->task_buffer,sizeof(task_t*) * q->task_buf_size);
         }
    q->task_buffer[q->index] = t;
    q->index++;
    pthread_cond_broadcast(&emptyqueue);
    pthread_mutex_unlock(&mutexs_q[th_nb]);
}


task_t* dequeue_task(tasks_queue_t *q,int th_nb)
{
    pthread_mutex_lock(&mutexs_q[th_nb]);
    while (q->index == 0) {
        pthread_cond_wait(&emptyqueue, &mutexs_q[th_nb]);
    }
    task_t *t = q->task_buffer[--q->index];
    pthread_mutex_unlock(&mutexs_q[th_nb]);
    return t;
}