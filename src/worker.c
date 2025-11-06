#include <stdio.h>

#include "tasks_implem.h"
#include "tasks_queue.h"
#include "debug.h"
#include <unistd.h>
#include <pthread.h>
#include "tasks.h"

pthread_mutex_t mutex=PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond1=PTHREAD_COND_INITIALIZER;

void * worker(void * arg){
    pthread_mutex_lock(&mutex);
    while(sys_state.task_counter==0){
        pthread_cond_wait(&cond1,&mutex);
    }
    
    task_t *task=get_task_to_execute();
    pthread_mutex_unlock(&mutex);
    task_return_value_t ret = exec_task(active_task);
    if (ret == TASK_COMPLETED){
            terminate_task(active_task);
    }
    pthread_cond_signal(&cond2);



}