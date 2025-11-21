#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "tasks_implem.h"
#include <stdbool.h>
#include "tasks_queue.h"

pthread_mutex_t mutexs_q[THREAD_COUNT];
extern tasks_queue_t *queues[THREAD_COUNT];

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
    q[i].steal_p = 0;
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

  
void enqueue_task(tasks_queue_t *q, task_t *t,int th_nb) { 
    pthread_mutex_lock(&mutexs_q[th_nb]); 
    int next = (q->index + 1) % q->task_buf_size;
    if (next == q->steal_p) {
        // le buffer est plein → agrandir
        int old_size = q->task_buf_size;
        int new_size = old_size * 2;
        task_t **new_buf = malloc(sizeof(task_t*) * new_size);

        int n = (q->index - q->steal_p + old_size) % old_size;
        for (int i = 0; i < n; i++)
            new_buf[i] = q->task_buffer[(q->steal_p + i) % old_size];

        free(q->task_buffer);
        q->task_buffer = new_buf;
        q->task_buf_size = new_size;
        q->steal_p = 0;
        q->index = n;}
    q->task_buffer[q->index] = t; 
    q->index=(q->index+1)% q->task_buf_size; 
    pthread_cond_broadcast(&emptyqueue); 
    pthread_mutex_unlock(&mutexs_q[th_nb]); } 


task_t* dequeue_task(tasks_queue_t *q,int th_nb)
{
    if (q->index == q->steal_p) {
        return NULL;
    }
    q->index=(q->index-1)% q->task_buf_size; 

    task_t *t = q->task_buffer[q->index]; // LIFO
    return t;
}


void steal(tasks_queue_t *q,int th_nb){
    task_t *t=q->task_buffer[q->steal_p];
    q->steal_p=(q->steal_p+1)% q->task_buf_size; 
    enqueue_task(queues[th_nb],t,th_nb);
    


}