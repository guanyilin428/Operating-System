#include <stdatomic.h>
#include <mthread.h>
#include <sys/syscall.h>
#include <sys/syscall_number.h>

int mthread_mutex_init(mthread_mutex_t* handle)
{
    mutex_get(handle);
    return 0;
}

int mthread_mutex_lock(void* handle) 
{
    mutex_block(*(int*)handle);
    return 0;
}
int mthread_mutex_unlock(void* handle)
{   
    mutex_unlock(*(int*)handle);
    return 0;
}

int mutex_get(mthread_mutex_t* handle)
{
    return invoke_syscall(SYSCALL_MUTEX_GET, handle, IGNORE, IGNORE);
}

void mutex_block(int id)
{
    invoke_syscall(SYSCALL_MUTEX_LOCK, id, IGNORE, IGNORE);
}

void mutex_unlock(int id)
{
    invoke_syscall(SYSCALL_MUTEX_UNLOCK, id, IGNORE, IGNORE);
}

int mthread_barrier_init(mthread_barrier_t* handle, unsigned count)
{
    handle->count = 0;
    invoke_syscall(SYSCALL_BARRIER_INIT, handle, count, IGNORE);
    return 0;
}
int mthread_barrier_wait(mthread_barrier_t* handle)
{
    handle->count++;
    invoke_syscall(SYSCALL_BARRIER_WAIT, handle, IGNORE, IGNORE);
    return 0;
}
int mthread_barrier_destroy(mthread_barrier_t* handle)
{
    invoke_syscall(SYSCALL_BARRIER_DESTROY, handle, IGNORE, IGNORE);
    return 0;
}

int mthread_semaphore_init(mthread_semaphore_t *handle, int val)
{
    return invoke_syscall(SYSCALL_SEMAPHORE_INIT, handle, val, IGNORE);
}

int mthread_semaphore_up(mthread_semaphore_t *handle)
{
    invoke_syscall(SYSCALL_SEMAPHORE_UP, handle, IGNORE, IGNORE);
    return 0;
}

int mthread_semaphore_down(mthread_semaphore_t *handle)
{
    invoke_syscall(SYSCALL_SEMAPHORE_DOWN, handle, IGNORE, IGNORE);
    return 0;
}

int mthread_semaphore_destroy(mthread_semaphore_t *handle)
{
    invoke_syscall(SYSCALL_SEMAPHORE_DESTROY, handle, IGNORE, IGNORE);
    return 0;
}

int mthread_create(mthread_t *thread,
                   void (*start_routine)(void*),
                   void *arg)
{
    invoke_syscall(SYSCALL_MTHREAD_CREATE, thread, start_routine, arg);
}

int mthread_join(mthread_t thread)
{
    //TODO:
}
