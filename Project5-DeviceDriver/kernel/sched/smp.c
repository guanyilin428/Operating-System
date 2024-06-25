#include <sbi.h>
#include <atomic.h>
#include <os/sched.h>
#include <os/smp.h>
#include <os/lock.h>

void smp_init()
{
    do_spin_lock_init(&kernel_lock);
}

void wakeup_other_hart()
{
    sbi_send_ipi(NULL);
}

void lock_kernel()
{
    do_spin_lock_acquire(&kernel_lock);
}

void unlock_kernel()
{
    do_spin_lock_release(&kernel_lock);
}

