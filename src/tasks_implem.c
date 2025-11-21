#include <stdio.h>
#include <stdlib.h>
#include "tasks_implem.h"
#include "tasks_queue.h"
#include "debug.h"
#include <unistd.h>
#include <pthread.h>
#include "tasks_io.h"
#include "tasks.h"
  


pthread_t tids[THREAD_COUNT];

tasks_queue_t *tqueue= NULL;

pthread_mutex_t mutex_runable = PTHREAD_MUTEX_INITIALIZER;

extern __thread task_t *active_task;
extern pthread_mutex_t mutex_task_op_count;

extern pthread_cond_t  checkfinished;
extern pthread_cond_t  emptyqueue;
extern int submitted;
extern int finished;

void * worker(void * arg){

    for(;;){
        
        active_task = get_task_to_execute();
       
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
    
}

void create_queues(void)
{
    tqueue = create_tasks_queue();
}

void delete_queues(void)
{
    free_tasks_queue(tqueue);
}    

void create_thread_pool(void)
{
    for (int i=0;i<THREAD_COUNT;i++){


        if (pthread_create(&tids[i], NULL, worker, NULL)!=0) {
            perror("pthread_create"); 
            exit(EXIT_FAILURE);
        }
    }
    return ;
}




void dispatch_task(task_t *t)
{
    enqueue_task(tqueue, t);
}

task_t* get_task_to_execute(void)
{
    return dequeue_task(tqueue);
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
    pthread_mutex_lock(&mutex_task_op_count);
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
    pthread_cond_signal(&checkfinished);
    
    pthread_mutex_unlock(&mutex_task_op_count);
}

void task_check_runnable(task_t *t)
{
//pthread_mutex_lock(&mutex_runable);
#ifdef WITH_DEPENDENCIES
    if(t->task_dependency_done == t->task_dependency_count &&(t->status==WAITING)){
        t->status = READY;
        dispatch_task(t);
    }
#endif
//pthread_mutex_lock(&mutex_runable);
}
