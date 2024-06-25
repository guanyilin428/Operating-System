#include <os/lock.h>
#include <os/sched.h>
#include <atomic.h>
#include <os/list.h>
#include <assert.h>

mutex_lock_t lock[LOCK_NUM];
int lock_allocated[LOCK_NUM];
int lock_first = 0;
int find_lock_id(){
    int i;
    if(!lock_first){
        for(i = 0; i < LOCK_NUM; i++)
            lock_allocated[i] = 0;
        lock_first++;
    }
    for(i = 1; i < LOCK_NUM; i++)
        if(!lock_allocated[i])
            return i;
}

int do_mutex_lock_init(void* handle)
{   
    if(*(int*)handle)
        return 0;
    int id = find_lock_id();
    lock_allocated[id] = 1; 
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
        do_block(*current_running, &(lock[id].block_queue));
        lock[id].guard.status = UNGUARDED; 
        do_scheduler();
    }
    lock[id].lock.status = LOCKED;     
    lock[id].guard.status = UNGUARDED;
    lock_allocated[id] = 1; 
    (*current_running)->lock[id] = 1;
    (*current_running)->lock_num++;
}

void do_mutex_lock_release(int id)
{
   /* while( atomic_cmpxchg_d(UNGUARDED, GUARDED, (ptr_t)&(lock->guard.status))== GUARDED )
        ;   //acquire guard_lock by spinning
    */
    lock[id].lock.status = UNLOCKED;
    lock[id].guard.status = UNGUARDED;
    (*current_running)->lock[id] = 0;
    (*current_running)->lock_num--;
    lock_allocated[id] = 0;

    while( !list_empty(&(lock[id].block_queue)) )
        do_unblock(lock[id].block_queue.next);
}

void do_spin_lock_init(spin_lock_t* lk){
    atomic_swap_d(UNLOCKED, &(lk->status));
}

void do_spin_lock_acquire(spin_lock_t *lk){
    //while(atomic_cmpxchg(UNLOCKED, LOCKED, lk->status) == LOCKED)
    while(atomic_swap_d(LOCKED, &(lk->status)) == LOCKED)
        ;
}

void do_spin_lock_release(spin_lock_t *lk){
    atomic_swap_d(UNLOCKED, &(lk->status));
}