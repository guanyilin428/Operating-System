#include <os/list.h>
#include <os/mm.h>
#include <os/lock.h>
#include <os/sched.h>
#include <os/time.h>
#include <os/irq.h>
#include <screen.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <assert.h>
#include <asm/regs.h>

#define PAGE_SIZE 4096
extern void ret_from_exception();

pcb_t pcb[NUM_MAX_TASK];
const ptr_t pid0_stack = INIT_KERNEL_STACK + PAGE_SIZE;
pcb_t pid0_pcb = {
    .pid = 0,
    .kernel_sp = (ptr_t)pid0_stack,
    .user_sp = (ptr_t)pid0_stack,
    .preempt_count = 0,
    .status = TASK_RUNNING
};

LIST_HEAD(ready_queue);

/* current running task PCB */
pcb_t * volatile current_running;

/* global process id */
pid_t process_id = 1;

int do_fork()
{
    if(process_id > NUM_MAX_TASK)
        return -1;            
    pcb_t* new = (pcb_t*)kmalloc(sizeof(pcb_t));
    new->pid = process_id++;
    new->status = current_running->status;
    new->type = current_running->type;
    new->preempt_count = current_running->preempt_count;
    new->user_sp = allocPage(1) + PAGE_SIZE;
    new->kernel_sp = allocPage(1) + PAGE_SIZE;
    uint64_t usp_sz = PAGE_SIZE - current_running->user_sp % PAGE_SIZE;
    uint64_t ksp_sz = PAGE_SIZE - current_running->kernel_sp % PAGE_SIZE;
    new->user_sp -= usp_sz;
    new->kernel_sp -= ksp_sz;
    memcpy((void*)new->user_sp, (void*)current_running->user_sp, (int)usp_sz);
    memcpy((void*)new->kernel_sp, (void*)current_running->kernel_sp, (int)ksp_sz);
    regs_context_t *pt_regs =
        (regs_context_t *)(new->kernel_sp + sizeof(switchto_context_t));
    pt_regs->regs[10] = 0;  //a0
    regs_context_t *parent_context = (regs_context_t *)(current_running->kernel_sp + SWITCH_TO_SIZE);
    pt_regs->regs[8] = new->user_sp - current_running->user_sp + parent_context->regs[8];//s0
    switchto_context_t *sw_regs =
        (switchto_context_t *)(new->kernel_sp);
    if((new->type == USER_PROCESS)| (new->type == USER_THREAD))
        sw_regs->regs[0] = (reg_t)ret_from_exception;
    // else 
    //    sw_regs->regs[0] = (reg_t)entry_point;
    sw_regs->regs[1] = new->kernel_sp;
    list_add_tail(&(new->list), &ready_queue);
    return new->pid;    
}


void do_scheduler(void)
{
    // TODO schedule
    // Modify the current_running pointer.
    assert_supervisor_mode();
    pcb_t *prev, *del;
    prev = current_running;
    uint64_t sched_time;
    if(prev != &pid0_pcb && prev->status != TASK_BLOCKED && prev->status != TASK_EXITED){
        list_add_tail(&(prev->list), &ready_queue);
        prev->status = TASK_READY;
    }
    
    current_running = prior_sched();
    //current_running = container_of(ready_queue.next, pcb_t, list);
    __list_del_entry(&(current_running->list));
    current_running -> status = TASK_RUNNING;
    sched_time = get_ticks();
    current_running->sched_time = sched_time;
    
    // restore the current_running's cursor_x and cursor_y
    vt100_move_cursor(current_running->cursor_x,
                      current_running->cursor_y);
    screen_cursor_x = current_running->cursor_x;
    screen_cursor_y = current_running->cursor_y;
    // TODO: switch_to current_running
    switch_to(prev, current_running);
}

void do_sleep(uint32_t sleep_time)
{
    // TODO: sleep(seconds)
    // note: you can assume: 1 second = `timebase` ticks
    // 1. block the current_running
    // 2. create a timer which calls `do_unblock` when timeout
    // 3. reschedule because the current_running is blocked.
    current_running->status = TASK_BLOCKED;
    uint64_t begin = get_timer();
    uint64_t wake_time = begin + (uint64_t)(sleep_time);
    current_running->wake_time = wake_time;
    add_time_queue(&(current_running->list), &timers, wake_time);

    do_scheduler();
}

uint64_t check_sleep(list_head *queue)
{
    pcb_t* p = container_of(queue->next, pcb_t, list);
    return p->wake_time;
}
void add_time_queue(list_node_t *pcb_node, list_head *queue, uint64_t wake_time)
{   
    list_node_t *tmp;
    pcb_t* p;
    int added = 0;
    
    for(tmp = queue->next; tmp != queue; tmp = tmp->next){
        p = container_of(tmp, pcb_t, list);
        if(p->wake_time > wake_time){
            list_add_tail(pcb_node, tmp);
            added++;
            break;
        }
    }
    if(!added)
        list_add_tail(pcb_node, queue);
}

void do_block(list_node_t *pcb_node, list_head *queue)
{
    list_add_tail(pcb_node, queue);
}

void do_unblock(list_node_t *pcb_node)
{
    pcb_t *pcb;

    __list_del_entry(pcb_node);             // dequeue from block_queue
    pcb = container_of(pcb_node, pcb_t, list);
    pcb->status = TASK_READY;   
    list_add_tail(pcb_node, &ready_queue);  // enqueue to ready_queue
}

void do_prior(int prior){
    current_running->prior = prior;
}

pcb_t* prior_sched()
{
    pcb_t *highest, *p;
    list_node_t *tmp;
    int hi_score = 0;
    int score;
    highest = container_of(ready_queue.next, pcb_t, list);
    for(tmp = ready_queue.next; tmp != &ready_queue; tmp = tmp->next){
        p = container_of(tmp, pcb_t, list);
        score = do_score(p->prior, p->sched_time);
        if(hi_score < score){
            hi_score = score;
            highest = p;
        }
    }
    return highest;
}

int do_score(int prior, uint64_t sched_time)
{
    uint64_t time_interval = get_ticks() - sched_time;
    int time_pr = (time_interval)/(INTERVAL_CLKS);
    return (prior*2 + time_pr);
}