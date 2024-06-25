#include <os/lock.h>
#include <os/sched.h>
#include <atomic.h>
#include <os/list.h>
#include <assert.h>

mutex_lock_t lock[LOCK_NUM];
int lock_id = 1;

int do_mutex_lock_init(void* handle)
{   
    assert_supervisor_mode();
    if(*(int*)handle)
        return 0;
    int id = lock_id++;
    lock[id].lock.status = UNLOCKED;   //init spin_lock
    lock[id].guard.status = UNGUARDED; //init guard
    INIT_LIST_HEAD(&(lock[id].block_queue));
    *(int*)handle = id;
    return id;
}

void do_mutex_lock_acquire(int id)
{  /* while( atomic_cmpxchg_d(UNGUARDED, GUARDED, (ptr_t)&(lock->guard.status))== GUARDED )
        ;   //acquire guard_lock by spinning
  */
    if(lock[id].lock.status == LOCKED) {
        current_running->status = TASK_BLOCKED;
        do_block(&(current_running->list), &(lock[id].block_queue));
        lock[id].guard.status = UNGUARDED; 
        do_scheduler();
    }
    lock[id].lock.status = LOCKED;     
    lock[id].guard.status = UNGUARDED; 
}

void do_mutex_lock_release(int id)
{
   /* while( atomic_cmpxchg_d(UNGUARDED, GUARDED, (ptr_t)&(lock->guard.status))== GUARDED )
        ;   //acquire guard_lock by spinning
    */
    lock[id].lock.status = UNLOCKED;

    list_node_t *tmp = lock[id].block_queue.next;
    while( !list_empty(&(lock[id].block_queue)) )
    {
        do_unblock(tmp);
        tmp = tmp->next;
    }
    
    lock[id].guard.status = UNGUARDED;
}

