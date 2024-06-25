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

extern void ret_from_exception();
extern void __global_pointer$();

int init_task(task_info_t **task){
    int i, j = 0;
    /*
    for(i = 0; i < num_sched1_tasks; i++)
        task[j++] = sched1_tasks[i];
    for(i = 0; i < num_lock_tasks; i++)
        task[j++] = lock_tasks[i];
    */
   /*
    for(i = 0; i < num_timer_tasks; i++)
        task[j++] = timer_tasks[i];
    for(i = 0; i < num_sched2_tasks; i++)
        task[j++] = sched2_tasks[i];
    for(i = 0; i < num_lock2_tasks; i++)
        task[j++] = lock2_tasks[i];
    */
    for(i = 0; i < num_fork_tasks; i++)
        task[j++] = fork_tasks[i];
    
    return j;
}

static void init_pcb_stack(
    ptr_t kernel_stack, ptr_t user_stack, ptr_t entry_point,
    pcb_t *pcb)
{
    regs_context_t *pt_regs =
        (regs_context_t *)(kernel_stack - sizeof(regs_context_t));
    pcb->kernel_sp -= sizeof(regs_context_t);
    for(int i = 0; i < 32; i++)
        pt_regs->regs[i] = 0;
    pt_regs->regs[1] = entry_point;
    pt_regs->regs[2] = pcb->user_sp;
    //pt_regs->regs[3] = __global_pointer$();//gp
    pt_regs->sepc = entry_point;
    pt_regs->sstatus = 0;

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

static void init_pcb()
{
     /* initialize all of your pcb and add them into ready_queue
     * TODO:
     */
    pcb_t *p;
    task_info_t *task[NUM_MAX_TASK];
    int ntasks = init_task(task);
    task_info_t **t = task;

    for(p = pcb; p - pcb < ntasks; p++){
        p->pid = process_id++;
        p->status = TASK_READY;
        p->type = (*t)->type; 
        p->preempt_count = 0;
        p->user_sp = allocPage(1) + PAGE_SIZE;
        p->kernel_sp = allocPage(1) + PAGE_SIZE;
        p->sched_time = 0;
    
        init_pcb_stack(p->kernel_sp, p->user_sp, (*t)->entry_point, p);
        list_add_tail(&(p->list), &ready_queue);
        t++;
    }

    /* remember to initialize `current_running */
    current_running = &pid0_pcb;
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
    syscall[SYSCALL_GET_TIME] = get_time;
    syscall[SYSCALL_GET_TIMEBASE] = get_time_base;
    syscall[SYSCALL_GET_TICK] = get_ticks;
}

// jump from bootloader.
// The beginning of everything >_< ~~~~~~~~~~~~~~
int main()
{
    // init Process Control Block (-_-!)
    init_pcb();
    printk("> [INIT] PCB initialization succeeded.\n\r");

    // read CPU frequency
    time_base = sbi_read_fdt(TIMEBASE);

    // init interrupt (^_^)
    init_exception();
    printk("> [INIT] Interrupt processing initialization succeeded.\n\r");

    // init system call table (0_0)
    init_syscall();
    printk("> [INIT] System call initialized successfully.\n\r");

    // fdt_print(riscv_dtb);

    // init screen (QAQ)
    init_screen();
    printk("> [INIT] SCREEN initialization succeeded.\n\r");

    // TODO:
    // Setup timer interrupt and enable all interrupt
    reset_irq_timer();
    //enable_interrupt();

    while (1) {
        // (QAQQQQQQQQQQQ)
        // If you do non-preemptive scheduling, you need to use it
        // to surrender control do_scheduler();
        //__asm__ __volatile__("wfi\n\r":::);
        do_scheduler();
    };
    return 0;
}
