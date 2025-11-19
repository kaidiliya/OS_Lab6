#ifndef __TASKS_IMPLEM_H__
#define __TASKS_IMPLEM_H__

#include "tasks_types.h"

void create_queues(void);
void delete_queues(void);

void create_thread_pool(void);

void dispatch_task(task_t *t);
<<<<<<< HEAD
task_t* get_task_to_execute(int i);
=======
// task_t* get_task_to_execute(void);
task_t* get_task_to_execute(int worker_id);
>>>>>>> 63ebe0a (finish create queue and round-robin)
unsigned int exec_task(task_t *t);
void terminate_task(task_t *t);

void task_check_runnable(task_t *t);

#endif
