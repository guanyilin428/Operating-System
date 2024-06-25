/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *            Copyright (C) 2018 Institute of Computing Technology, CAS
 *               Author : Han Shukai (email : hanshukai@ict.ac.cn)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *         The kernel's entry, where most of the initialization work is done.
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this
 * software and associated documentation files (the "Software"), to deal in the Software
 * without restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * */

#include <common.h>
#include <os/irq.h>
#include <os/mm.h>
#include <os/sched.h>
#include <screen.h>
#include <sbi.h>
#include <stdio.h>
#include <os/time.h>
#include <os/syscall.h>
#include <test.h>
#include <csr.h>
#include <os/lock.h>
#include <os/sync.h>
#include <os/smp.h>

extern void ret_from_exception();
extern void __global_pointer$();
reg_t kernel_stack[NUM_MAX_TASK];
reg_t user_stack[NUM_MAX_TASK];
pid_t pid_allocated[NUM_MAX_TASK + 1]; //0 represents not allocated;
spin_lock_t kernel_lock;

void init_allocated(){
    for(int i = 0; i <= NUM_MAX_TASK; i++)
        pid_allocated[i] = 0;
}

pid_t find_pid(){
    for(int i = 1; i <= NUM_MAX_TASK; i++){
        if(!pid_allocated[i])
            return (pid_t)i;
    }
    return -1; // all pids have been allocated
}

static void init_all_stacks(){
    for(int i = 1; i <= NUM_MAX_TASK; i++){
        user_stack[i] = allocPage(1) + PAGE_SIZE;
        kernel_stack[i] = allocPage(1) + PAGE_SIZE;
    }
}

static void init_pcb_stack(
    ptr_t kernel_stack, ptr_t user_stack, ptr_t entry_point,
    pcb_t *pcb, void *args)
{
    regs_context_t *pt_regs =
        (regs_context_t *)(kernel_stack - sizeof(regs_context_t));
    pcb->kernel_sp -= sizeof(regs_context_t);
    for(int i = 0; i < 32; i++)
        pt_regs->regs[i] = 0;
    pt_regs->regs[1] = entry_point;
    pt_regs->regs[2] = pcb->user_sp;
    pt_regs->regs[3] = __global_pointer$;
    pt_regs->regs[4] = (reg_t)pcb;
    pt_regs->regs[10] = args;
    pt_regs->sepc = entry_point;
    pt_regs->sstatus = 0;
    pt_regs->scause = 0;
    pt_regs->sbadaddr = 0;

    if((pcb->type == KERNEL_PROCESS) | (pcb->type == KERNEL_THREAD)){
        pt_regs->sstatus |= SR_SPP;
        pt_regs->regs[2] = pcb->kernel_sp;
    }
    
    pcb->kernel_sp -= sizeof(switchto_context_t);
    switchto_context_t *sw_regs =
        (switchto_context_t *)(pcb->kernel_sp);
    
    for(int i = 0; i < 14; i++){
        sw_regs->regs[i] = 0;
    }
    if((pcb->type == USER_PROCESS)| (pcb->type == USER_THREAD))
        sw_regs->regs[0] = (reg_t)ret_from_exception;
    else 
        sw_regs->regs[0] = (reg_t)entry_point;
    sw_regs->regs[1] = pcb->kernel_sp;    
}

static pid_t init_pcb(task_info_t* info, void* arg, spawn_mode_t mode)
{
    int pid = find_pid();
    pid_allocated[pid] = 1;
    pcb_t* p = &pcb[pid - 1];
    p->pid = pid;
    p->parent = 0;
    p->status = TASK_READY;
    p->type = info->type;
    p->preempt_count = 0;
    for(int i = 0; i < LOCK_NUM; i++)
        p->lock[i] = 0;
    p->lock_num = 0;
    p->user_sp = allocPage(1) + PAGE_SIZE;
    p->kernel_sp = allocPage(1) + PAGE_SIZE;
    p->sched_time = 0;
    p->mode = mode;
    p->mask = (*current_running)->mask;

    init_pcb_stack(p->kernel_sp, p->user_sp, info->entry_point, p, arg);
    list_add_tail(&(p->list), &ready_queue);
    return pid;
}

void init_bubble_task(){
    task_info_t task_bubble = {(ptr_t)&null_task, USER_PROCESS};
    init_pcb(&task_bubble, NULL, AUTO_CLEANUP_ON_EXIT);
}

void init_shell(){
    task_info_t shell = {(ptr_t)&test_shell, USER_PROCESS};
    init_pcb(&shell, NULL, AUTO_CLEANUP_ON_EXIT);
}

static void init_syscall(void)
{
    // initialize system call table.
    syscall[SYSCALL_SLEEP] = do_sleep;
    syscall[SYSCALL_YIELD] = do_scheduler;
    syscall[SYSCALL_MUTEX_GET] = do_mutex_lock_init;
    syscall[SYSCALL_MUTEX_LOCK] = do_mutex_lock_acquire;
    syscall[SYSCALL_MUTEX_UNLOCK] = do_mutex_lock_release;
    syscall[SYSCALL_FORK] = do_fork;
    syscall[SYSCALL_GETCHAR] = sbi_console_getchar;
    syscall[SYSCALL_PRIOR] = do_prior;
    syscall[SYSCALL_WRITE] = screen_write;
    syscall[SYSCALL_CURSOR] = screen_move_cursor;
    syscall[SYSCALL_REFLUSH] = screen_reflush;
    syscall[SYSCALL_SCREEN_CLEAR] = screen_clear;
    syscall[SYSCALL_PS] = do_process_show;
    syscall[SYSCALL_GET_TIME] = get_time;
    syscall[SYSCALL_GET_TIMEBASE] = get_time_base;
    syscall[SYSCALL_GET_TICK] = get_ticks;
    syscall[SYSCALL_SPAWN] = init_pcb;
    syscall[SYSCALL_EXIT] = do_exit;
    syscall[SYSCALL_WAITPID] = do_waitpid;
    syscall[SYSCALL_KILL] = do_kill;
    syscall[SYSCALL_GETPID] = do_getpid;
    syscall[SYSCALL_PUTCHAR] = screen_putchar;
    syscall[SYSCALL_SEMAPHORE_INIT] = do_semaphore_init;
    syscall[SYSCALL_SEMAPHORE_DOWN] = do_semaphore_down;
    syscall[SYSCALL_SEMAPHORE_UP] = do_semaphore_up;
    syscall[SYSCALL_SEMAPHORE_DESTROY] = do_semaphore_destroy;
    syscall[SYSCALL_BARRIER_INIT] = do_barrier_init;
    syscall[SYSCALL_BARRIER_WAIT] = do_barrier_wait;
    syscall[SYSCALL_BARRIER_DESTROY] = do_barrier_destroy;
    syscall[SYSCALL_MBOX_OPEN] = do_mailbox_open;
    syscall[SYSCALL_MBOX_CLOSE] = do_mailbox_close;
    syscall[SYSCALL_MBOX_SEND] = do_mailbox_send;
    syscall[SYSCALL_MBOX_RECV] = do_mailbox_recv;    
    syscall[SYSCALL_MBOX_OP]   = do_mailbox_op;
    syscall[SYSCALL_PID_MASK_SET] = do_pid_task_set;
}

void load_pcb_cursor(){
    screen_cursor_x = (*current_running)->cursor_x;
    screen_cursor_y = (*current_running)->cursor_y;
}
// jump from bootloader.
// The beginning of everything >_< ~~~~~~~~~~~~~~
int main()
{
    if(get_current_cpu_id() == 0){  // main_kernel
        smp_init();
        lock_kernel();
        init_allocated();
        init_all_stacks();
        init_shell();
        init_bubble_task();
        current_running = &current_running_core0;
        *current_running = &pid0_pcb_core0;
        init_exception();
        //prints("> [INIT] Interrupt processing initialization succeeded.\n\r");
        
        init_syscall();
        //prints("> [INIT] System call initialized successfully.\n\r");
       // prints("This is cpu_kernel 0\n\r");

        init_screen();

        //prints("> [INIT] SCREEN initialization succeeded.\n\r");
        //prints("> [INIT] shell initialization succeeded.\n\r");
        wakeup_other_hart();
    }else{
        lock_kernel();
        current_running = &current_running_core1;
        *current_running = &pid0_pcb_core1;

        prints("This is cpu_kernel 1\n\r");
        setup_exception();
    }

    // read CPU frequency
    time_base = sbi_read_fdt(TIMEBASE);

    // TODO:
    // Setup timer interrupt and enable all interrupt
    //sbi_set_timer(get_ticks() + INTERVAL_CLKS);
    reset_irq_timer();
    
    while (1) {
        // (QAQQQQQQQQQQQ)
        // If you do non-preemptive scheduling, you need to use it
        // to surrender control do_scheduler();
        //__asm__ __volatile__("wfi\n\r":::);
    };
    return 0;
}
