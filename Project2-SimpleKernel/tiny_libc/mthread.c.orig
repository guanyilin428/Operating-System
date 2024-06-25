#include <stdatomic.h>
#include <mthread.h>
#include <sys/syscall.h>
#include <os/lock.h>

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