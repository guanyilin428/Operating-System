#include <os/list.h>
#include <mthread.h>
#include <os/sync.h>

#define MAX_BARRIER_SIZE 20

barrier_t arr[MAX_BARRIER_SIZE];
int bar_first = 0;
int allocated[MAX_BARRIER_SIZE];

void bar_init_allocated(){
    for(int i = 0; i < MAX_BARRIER_SIZE; i++)
        allocated[i] = 0;
}

int bar_find_id(){
    if(!bar_first){
        bar_init_allocated();
        bar_first++;
    }
    int i;
    for(i = 0; i < MAX_BARRIER_SIZE; i++){
        if(!allocated[i])
            return i;
    }
}

void do_barrier_init(mthread_barrier_t* handle, int count){
    int id = bar_find_id();
    handle->id = id;
    allocated[id] = 1;

    arr[id].count = 0;
    arr[id].max = count;
    __list_init(&(arr[id].queue));   
}

void do_barrier_wait(mthread_barrier_t *handle){
    barrier_t *tmp = &arr[handle->id];
    tmp->count++;
    if(tmp->count < tmp->max){
        do_block(*current_running, &(tmp->queue));
        do_scheduler();
    }
    else{
        while(!list_empty(&(tmp->queue)))
            do_unblock(tmp->queue.next);
        tmp->count = 0;
    }
}

void do_barrier_destroy(mthread_barrier_t *handle){
    barrier_t *tmp = &arr[handle->id];
    allocated[handle->id] = 0;
    while(!list_empty(&(tmp->queue)))
        do_unblock(tmp->queue.next);
}