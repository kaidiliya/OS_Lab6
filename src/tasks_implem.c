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

tasks_queue_t *queues[THREAD_COUNT];
extern pthread_mutex_t mutexs_q[THREAD_COUNT];

int round_robin=0;
pthread_mutex_t mutex_rr=PTHREAD_MUTEX_INITIALIZER;
__thread int thread_id;
extern __thread task_t *active_task;
extern pthread_mutex_t mutex2;
pthread_mutex_t mutex_rand=PTHREAD_MUTEX_INITIALIZER;;
pthread_mutex_t mutex_task=PTHREAD_MUTEX_INITIALIZER;
extern pthread_cond_t  checkfinished;
extern pthread_cond_t  emptyqueue;
extern int submitted;
extern int finished;

void * worker(void * arg){
    thread_id=  *(int*)arg;
    
    for(;;){
        
        active_task = get_task_to_execute(thread_id);
        if(active_task!=NULL){
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
    pthread_mutex_lock(&mutex_rr);
    int index = round_robin % THREAD_COUNT;
    round_robin++;
    pthread_mutex_unlock(&mutex_rr);

    enqueue_task(queues[index], t,index);


}

void dispatch_task_worker(task_t *t)
{
    enqueue_task(queues[thread_id], t,thread_id);


}



task_t* get_task_to_execute(int worker_id) {

    pthread_mutex_lock(&mutexs_q[worker_id]); //mutex of the queue of worker_id

    task_t* t=dequeue_task(queues[worker_id],worker_id);
    pthread_mutex_unlock(&mutexs_q[worker_id]);
    pthread_mutex_lock(&mutex_rand);    //mutex of variable r

    if(t==NULL){
        int r=rand()%THREAD_COUNT;  
        while(r==worker_id){
            r=rand()%THREAD_COUNT;
        }
         pthread_mutex_lock(&mutexs_q[r]); //mutex of the queue of worker_id
        if(!(queues[r]->index==queues[r]->steal_p)){
            
           
 
            steal(queues[r],worker_id);
            PRINT_DEBUG(10, "               Steal from %i in  queue %i\n", worker_id,r);

            
            

        }
        pthread_mutex_unlock(&mutexs_q[r]);
        
    }
    pthread_mutex_unlock(&mutex_rand);

    return t;

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
    pthread_mutex_lock(&mutex2); // mutex global
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
    
    pthread_cond_signal(&checkfinished); // condition signal send to main thread to check if all task are finished
    
    pthread_mutex_unlock(&mutex2);


}

void task_check_runnable(task_t *t)
{
    pthread_mutex_lock(&mutex_task); //mutex of a task
#ifdef WITH_DEPENDENCIES
    if(t->task_dependency_done == t->task_dependency_count &&(t->status==WAITING)){
        t->status = READY;
        dispatch_task_worker(t);
    }
#endif
pthread_mutex_unlock(&mutex_task);
}
