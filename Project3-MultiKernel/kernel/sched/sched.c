#include <os/list.h>
#include <os/mm.h>
#include <os/lock.h>
#include <os/sched.h>
#include <os/time.h>
#include <os/irq.h>
#include <screen.h>
#include <os/stdio.h>
#include <assert.h>
#include <string.h>
#include <assert.h>
#include <os.h>
#include <os/stdio.h>
#include <asm/regs.h>
#include <os/smp.h>

#define PAGE_SIZE 4096
extern void ret_from_exception();

pcb_t pcb[NUM_MAX_TASK];
const ptr_t pid0_stack_core0 = INIT_KERNEL_STACK + PAGE_SIZE;
const ptr_t pid0_stack_core1 = INIT_KERNEL_STACK + 2 * PAGE_SIZE;
pcb_t pid0_pcb_core0 = {
    .pid = 0,
    .kernel_sp = (ptr_t)pid0_stack_core0,
    .user_sp = (ptr_t)pid0_stack_core0,
    .preempt_count = 0,
    .status = TASK_RUNNING
};
pcb_t pid0_pcb_core1 = {
    .pid = 0,
    .kernel_sp = (ptr_t)pid0_stack_core1,
    .user_sp = (ptr_t)pid0_stack_core1,
    .preempt_count = 0,
    .status = TASK_RUNNING
};

LIST_HEAD(ready_queue);
LIST_HEAD(wait_queue);

/* current running task PCB */
pcb_t ** volatile current_running;
pcb_t * volatile current_running_core0;
pcb_t * volatile current_running_core1;
/* global process id */
pid_t process_id = 1;

int do_fork()
{
    if(process_id > NUM_MAX_TASK)
        return -1;            
    pcb_t* new = (pcb_t*)kmalloc(sizeof(pcb_t));
    new->pid = process_id++;
    new->status = (*current_running)->status;
    new->type = (*current_running)->type;
    new->preempt_count = (*current_running)->preempt_count;
    new->user_sp = allocPage(1) + PAGE_SIZE;
    new->kernel_sp = allocPage(1) + PAGE_SIZE;
    uint64_t usp_sz = PAGE_SIZE - (*current_running)->user_sp % PAGE_SIZE;
    uint64_t ksp_sz = PAGE_SIZE - (*current_running)->kernel_sp % PAGE_SIZE;
    new->user_sp -= usp_sz;
    new->kernel_sp -= ksp_sz;
    memcpy((void*)new->user_sp, (void*)(*current_running)->user_sp, (int)usp_sz);
    memcpy((void*)new->kernel_sp, (void*)(*current_running)->kernel_sp, (int)ksp_sz);
    regs_context_t *pt_regs =
        (regs_context_t *)(new->kernel_sp + sizeof(switchto_context_t));
    pt_regs->regs[10] = 0;  //a0
    regs_context_t *parent_context = (regs_context_t *)((*current_running)->kernel_sp + SWITCH_TO_SIZE);
    pt_regs->regs[8] = new->user_sp - (*current_running)->user_sp + parent_context->regs[8];//s0
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
    pcb_t *prev;
    prev = *current_running;
    uint64_t sched_time;

    int cpu_id = get_current_cpu_id();
    
    if(list_empty(&ready_queue)){
        if(prev != &pid0_pcb_core0 && prev != &pid0_pcb_core1 && prev->status == TASK_RUNNING)
            return;
        *current_running = &pcb[1]; //bubble_task
    }
    else{
        if(prev != &pid0_pcb_core0 && prev != &pid0_pcb_core1 && prev != &pcb[1] && prev->status == TASK_RUNNING){
            list_add_tail(&(prev->list), &ready_queue);
            prev->status = TASK_READY;
        }
        (*current_running) = container_of(ready_queue.next, pcb_t, list);
        if((*current_running)->mask & (1<<cpu_id) == 0)
            *current_running = &pcb[1];
        __list_del_entry(&((*current_running)->list));
        __list_init(&((*current_running)->list));
    }
    
    //current_running = prior_sched();
    (*current_running)-> status = TASK_RUNNING;
    sched_time = get_ticks();
    (*current_running)->sched_time = sched_time;
    
    // restore the current_running's cursor_x and cursor_y
    
    vt100_move_cursor((*current_running)->cursor_x,
                      (*current_running)->cursor_y);
    //screen_cursor_x = (*current_running)->cursor_x;
    //screen_cursor_y = (*current_running)->cursor_y;   
   
    // TODO: switch_to current_running
    switch_to(prev, *current_running);
}

void do_sleep(uint32_t sleep_time)
{
    // TODO: sleep(seconds)
    // note: you can assume: 1 second = `timebase` ticks
    // 1. block the current_running
    // 2. create a timer which calls `do_unblock` when timeout
    // 3. reschedule because the current_running is blocked.
    (*current_running)->status = TASK_BLOCKED;
    uint64_t begin = get_timer();
    uint64_t wake_time = begin + (uint64_t)(sleep_time);
    (*current_running)->wake_time = wake_time;
    //add_time_queue(&(current_running->list), &timers, wake_time);
    list_add_tail(&((*current_running)->list), &timers);
    do_scheduler();
}

uint64_t check_sleep(list_head *queue)
{
    pcb_t* p = container_of(queue->next, pcb_t, list);
    return p->wake_time;
}


/*
void add_time_queue(list_node_t *pcb_node, list_head *queue, uint64_t wake_time)
{   
    list_node_t *tmp;
    pcb_t* p;
    int added = 0;
    
    for(tmp = queue->next; tmp != queue; tmp = tmp->next){
        p = container_of(tmp, pcb_t, list);
        if(p->wake_time > wake_time){
            list_add_tail(pcb_node, p);
            added++;
            break;
        }
    }
    if(!added)
        list_add_tail(pcb_node, queue);
}
*/

void do_block(pcb_t* pcb, list_head *queue)
{
    pcb->status = TASK_BLOCKED;
    list_add_tail(&(pcb->list), queue);
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
    (*current_running)->prior = prior;
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
    int time_pr = (int)(get_ticks() - sched_time)/(INTERVAL_CLKS);
    return (prior*10 + time_pr);
}

void do_process_show()
{
    int i;
    for(i = 1; i < NUM_MAX_TASK; i++){
        if(!pid_allocated[i])
            continue;
            
        pcb_t* p =&pcb[i-1];
        switch (p->status)
        {
        case TASK_RUNNING:
        prints("PID:%d STATUS:RUNNING MASK:%d\n\r", i, p->mask);
        break;

        case TASK_READY:
        prints("PID:%d STATUS:READY MASK:%d\n\r", i, p->mask);
        break;

        case TASK_BLOCKED:
        prints("PID:%d STATUS:BLOCKED MASK:%d\n\r", i, p->mask);
        break;

        case TASK_ZOMBIE:
        prints("PID:%d STATUS:ZOMBIE MASK:%d\n\r", i, p->mask);
        break;

        default:
        break;
        }            
    }
}

void release_sources(pcb_t* p){
    int i;
    p->status = TASK_EXITED;

    if(p->lock_num){ // release locks
        for(i = 1; i < LOCK_NUM; i++){
            if(!(p->lock[i]))
                continue;
            lock_allocated[i] = 0;
            while( !list_empty(&(lock[i].block_queue)) )
                do_unblock(lock[i].block_queue.next);
        }
    }
        
    p->status = TASK_EXITED;
    pid_allocated[p->pid] = 0;
    __list_del_entry(&(p->list));
    __list_init(&(p->list));
}

void do_zombie(pcb_t *p){
    p->status = TASK_ZOMBIE;
    __list_del_entry(&(p->list));
    __list_init(&(p->list));
    pcb_t *prt = &(pcb[p->parent - 1]);
    if(p->parent)
        do_unblock(&(prt->list));
    // child_proc
}

void do_exit(){
    if((*current_running)->mode == ENTER_ZOMBIE_ON_EXIT){
        do_zombie(*current_running);
    }else{ // release sources
        release_sources(*current_running);
    }
    do_scheduler();
}

int do_waitpid(pid_t pid){
    pcb_t* p = &pcb[pid - 1]; // process being waited
    if(p->status != TASK_ZOMBIE && p->status != TASK_EXITED){
        p->parent = (*current_running)->pid;
        do_block(*current_running, &wait_queue);
        do_scheduler();
    }
    //while()
    if(p->status == TASK_ZOMBIE) // retrieve sources
        release_sources(p);
    
    return pid;
}

int do_kill(pid_t pid){
    if(pid <= 0 || pid > NUM_MAX_TASK || !pid_allocated[pid])
        return 0;
    if(pid == 1){  //cursor?
        prints("CANNOT kill shell, permission denied!!\n\r");
        return 0;
    }
    pcb_t *p = &pcb[pid - 1];
    if(p->mode == ENTER_ZOMBIE_ON_EXIT)
        do_zombie(p);
    else
        release_sources(p);   
    return 1; 
}

pid_t do_getpid(){
    return (*current_running)->pid;
}

void do_pid_task_set(pid_t pid, int mask){
    pcb_t *p = &pcb[pid-1];
    p->mask = mask;
}