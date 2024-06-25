#include <os/list.h>
#include <os/sched.h>
#include <os/sync.h>


#define MAX_SEM_SIZE 20

semaphore_t sem_arr[MAX_SEM_SIZE];
int allocated[MAX_SEM_SIZE];
int sem_first = 0;

void sem_init_allocated(){
    for(int i = 0; i < MAX_SEM_SIZE; i++)
        allocated[i] = 0;
}

int sem_find_id(){
    if(!sem_first){
        sem_init_allocated(allocated, MAX_SEM_SIZE);
        sem_first++;
    }
    int i;
    for(i = 0; i < MAX_SEM_SIZE; i++)
        if(!allocated[i])
            return i;
}

int do_semaphore_init(mthread_semaphore_t* handle, int val){
    int id = sem_find_id();
    *handle = id;
    allocated[id] = 1;

    sem_arr[id].sem = val;
    __list_init(&(sem_arr[id].queue));
    return *handle;
}

void do_semaphore_up(mthread_semaphore_t* handle){
    semaphore_t *tmp = &sem_arr[*handle];
    (tmp->sem)++;

    if(tmp->sem <= 0){
        do_unblock(&((*current_running)->list));
    }
}

void do_semaphore_down(mthread_semaphore_t* handle){
    semaphore_t *tmp = &sem_arr[*handle];
    (tmp->sem)--;
    if(tmp->sem < 0)
        do_block(*current_running, &(tmp->queue));
}

void do_semaphore_destroy(mthread_semaphore_t* handle){
    allocated[*handle] = 0;
    semaphore_t *tmp = &sem_arr[*handle];

    while(!list_empty(&(tmp->queue)))
        do_unblock(tmp->queue.next);
}