/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *            Copyright (C) 2018 Institute of Computing Technology, CAS
 *               Author : Han Shukai (email : hanshukai@ict.ac.cn)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *        Process scheduling related content, such as: scheduler, process blocking,
 *                 process wakeup, process creation, process kill, etc.
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
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

#ifndef INCLUDE_SCHEDULER_H_
#define INCLUDE_SCHEDULER_H_
 
#include <context.h>

#include <type.h>
#include <os/list.h>
#include <os/mm.h>
#include <os/lock.h>
#include <pgtable.h>

#define NUM_MAX_TASK 16
#define MAX_PAGE_NUM 1000

#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE*)0)->MEMBER)
#define container_of(ptr, type, member) ({ \
    const typeof( ((type *)0)->member) *__mptr = (ptr); \
    (type *)( (char*)__mptr - offsetof(type, member));})

typedef enum {
    TASK_BLOCKED,
    TASK_RUNNING,
    TASK_READY,
    TASK_EXITED,
    TASK_ZOMBIE,
} task_status_t;

typedef enum {
    KERNEL_PROCESS,
    KERNEL_THREAD,
    USER_PROCESS,
    USER_THREAD,
} task_type_t;

typedef enum {
    ENTER_ZOMBIE_ON_EXIT,
    AUTO_CLEANUP_ON_EXIT,
} spawn_mode_t;

/* Process Control Block */
typedef struct pcb
{   
    /* register context */
    // this must be this order!! The order is defined in regs.h
    reg_t kernel_sp;
    reg_t user_sp;

    // count the number of disable_preempt
    // enable_preempt enables CSR_SIE only when preempt_count == 0
    reg_t preempt_count;

    ptr_t kernel_stack_base;
    ptr_t user_stack_base;

    /* previous, next pointer */
    list_node_t list;
    list_head wait_list;

    /* process id */
    pid_t pid;
    pid_t parent;
    /* kernel/user thread/process */
    task_type_t type;
    spawn_mode_t mode;
    /* BLOCK | READY | RUNNING */
    task_status_t status;

    int lock[LOCK_NUM]; //record lock id
    int lock_num;
    uint64_t wake_time;
    int prior;
    uint64_t sched_time;

    PTE *pgdir;
    /* allocated pages */
    list_head pg_list;
    list_head pin_pg_list;
    int thread_num;
    int pg_num;

    /* cursor position */
    int cursor_x;
    int cursor_y;
    
} pcb_t;

/* task information, used to init PCB */
typedef struct task_info{
    ptr_t entry_point;
    task_type_t type;
    int prior;
} task_info_t;

/* ready queue to run */
extern list_head ready_queue;

/* current running task PCB */
extern pcb_t ** volatile current_running;
extern pcb_t * volatile current_running_core0;
extern pcb_t * volatile current_running_core1;
extern pid_t process_id;

extern pcb_t pcb[NUM_MAX_TASK];
extern reg_t kernel_stack[NUM_MAX_TASK];
extern reg_t user_stack[NUM_MAX_TASK];
extern pid_t pid_allocated[NUM_MAX_TASK + 1];
extern pcb_t pid0_pcb_core0;
extern pcb_t pid0_pcb_core1;
extern const ptr_t pid0_stack_core0;
extern const ptr_t pid0_stack_core1;

extern void switch_to(pcb_t *prev, pcb_t *next);
void do_scheduler(void);
void do_sleep(uint32_t);
//void add_time_queue(list_node_t *pcb_node, list_head *queue, uint64_t wake_time);
uint64_t check_sleep(list_head *queue);
void do_block(pcb_t* pcb, list_head *queue);
void do_unblock(list_node_t *);
void do_prior(int prior);
int do_fork();
pcb_t* prior_sched();
int do_score(int prior, uint64_t sched_time);

void release_sources(pcb_t* p);
void do_zombie(pcb_t *p);

extern pid_t find_pid();

extern pid_t do_spawn(task_info_t *task, void* arg, spawn_mode_t mode);
extern void do_exit(void);
extern int do_kill(pid_t pid);
extern int do_waitpid(pid_t pid);
extern void do_process_show();
extern pid_t do_getpid();
extern pid_t do_exec(char* file_name, int argc, char* argv[]);
extern void do_show_exec();
 
#endif
