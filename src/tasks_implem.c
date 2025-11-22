#include <stdio.h>
#include <stdlib.h>
#include "tasks_implem.h"
#include "tasks_queue.h"
#include "debug.h"
#include <unistd.h>
#include <pthread.h>
#include "tasks_io.h"
#include "tasks.h"
  
pthread_mutex_t mutex_runable = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_rr = PTHREAD_MUTEX_INITIALIZER;


pthread_t tids[THREAD_COUNT];

tasks_queue_t *queues[THREAD_COUNT];

int round_robin=0;

extern __thread task_t *active_task;
extern pthread_mutex_t mutex_task_op_count;
extern pthread_cond_t  checkfinished;
extern pthread_cond_t  emptyqueue;
extern int submitted;
extern int finished;

void * worker(void * arg){
    int worker_id=  *(int*)arg;

    for(;;){
        
        active_task = get_task_to_execute(worker_id);
        task_return_value_t ret = exec_task(active_task);
        
            if (ret == TASK_COMPLETED){
                terminate_task(active_task);
            }

    #ifdef WITH_DEPENDENCIES
            else{
                active_task->status = WAITING;
                task_check_runnable(active_task);

            }
    #endif
            
    }
    free(arg);
    
}


void create_queues(void)
{
    for (int i = 0; i < THREAD_COUNT; i++) {
        queues[i] = create_tasks_queue();
    }
}

void delete_queues(void)
{
    for (int i = 0; i < THREAD_COUNT; i++) {
        free_tasks_queue(queues[i]);
    }
}    

void create_thread_pool(void)
{
    for (int i=0;i<THREAD_COUNT;i++){
        int *arg=malloc(sizeof(int));
        *arg=i;

        if (pthread_create(&tids[i], NULL, worker, arg)!=0) {
            perror("pthread_create"); 
            exit(EXIT_FAILURE);
        }

    }

    return ;
}




void dispatch_task(task_t *t)
{
    pthread_mutex_lock(&mutex_rr); // mutex of round robin
    int index = round_robin % THREAD_COUNT;
    round_robin++;
    pthread_mutex_unlock(&mutex_rr);

    enqueue_task(queues[index], t,index);


}



task_t* get_task_to_execute(int worker_id) {
    return dequeue_task(queues[worker_id],worker_id);
}

unsigned int exec_task(task_t *t)
{
    t->step++;
    t->status = RUNNING;

    PRINT_DEBUG(10, "Execution of task %u (step %u)\n", t->task_id, t->step);
    
    unsigned int result = t->fct(t, t->step);
    
    return result;
}

void terminate_task(task_t *t)
{   
    pthread_mutex_lock(&mutex_task_op_count);  // mutex global
    t->status = TERMINATED;
    
    PRINT_DEBUG(10, "Task terminated: %u\n", t->task_id);

#ifdef WITH_DEPENDENCIES
    if(t->parent_task != NULL){
        task_t *waiting_task = t->parent_task;
        waiting_task->task_dependency_done++;
        
        task_check_runnable(waiting_task);
    }
#endif

    
    finished++;
    
    pthread_cond_signal(&checkfinished);// condition signal send to main thread to check if all task are finished
    
    pthread_mutex_unlock(&mutex_task_op_count);


}

void task_check_runnable(task_t *t)
{
pthread_mutex_lock(&mutex_runable); //mutex of a task
#ifdef WITH_DEPENDENCIES
    if(t->task_dependency_done == t->task_dependency_count &&(t->status==WAITING)){
        t->status = READY;
        dispatch_task(t);
    }
#endif
pthread_mutex_unlock(&mutex_runable);
}
