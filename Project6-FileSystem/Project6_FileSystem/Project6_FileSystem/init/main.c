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
#include <plic.h>
#include <emacps/xemacps_example.h>
#include <net.h>

#include <common.h>
#include <type.h>
#include <os/irq.h>
#include <os/mm.h>
#include <os/sched.h>
#include <screen.h>
#include <sbi.h>
#include <stdio.h>
#include <os/time.h>
#include <os/syscall.h>
#include <csr.h>
#include <os/lock.h>
#include <os/sync.h>
#include <os/smp.h>
#include <os/elf.h>
#include <os/fs.h>
#include <user_programs.h>
#include <assert.h>

#define KERNEL_STACK_VADDR 0xffffffffffff0000
#define MAX_ARG_LENGTH 20

extern void ret_from_exception();
extern void __global_pointer$();
reg_t kernel_stack[NUM_MAX_TASK];
reg_t user_stack[NUM_MAX_TASK];
pid_t pid_allocated[NUM_MAX_TASK + 1]; //0 represents not allocated;
spin_lock_t kernel_lock;

uintptr_t user_alloc_page_helper(uintptr_t va, uintptr_t pgdir, pcb_t* pcb){
    return alloc_page_helper(va, pgdir, 1, pcb);
}

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

static void init_pcb_stack(
    ptr_t kernel_stack, ptr_t entry_point,
    pcb_t *pcb, int argc, char *argv[], reg_t pr_gp)
{
    regs_context_t *pt_regs =
        (regs_context_t *)(kernel_stack - sizeof(regs_context_t));
    pcb->kernel_sp -= sizeof(regs_context_t);
    for(int i = 0; i < 32; i++)
        pt_regs->regs[i] = 0;
    pt_regs->regs[1] = entry_point;
    pt_regs->regs[2] = pcb->user_sp;
    if(pcb->type == USER_THREAD){
        pt_regs->regs[3] = pr_gp;
    }else{
        pt_regs->regs[3] = __global_pointer$;
    }
    pt_regs->regs[4] = (reg_t)pcb;
    pt_regs->regs[10] = argc;
    pt_regs->regs[11] = argv;
    pt_regs->sepc = entry_point;
    pt_regs->sstatus = SR_SUM;
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


static pid_t init_pcb(void* arg, spawn_mode_t mode, int is_thread)
{
    int pid = find_pid();
    pid_allocated[pid] = 1;
    pcb_t* p = &pcb[pid - 1];
    p->pid = pid;
    p->parent = 0;
    p->status = TASK_READY;
    p->preempt_count = 0;
    for(int i = 0; i < LOCK_NUM; i++)
        p->lock[i] = 0;
    p->lock_num = 0;
    p->sched_time = 0;
    p->mode = mode;
    p->pg_num = 0;

    __list_init(&(p->pg_list));
    __list_init(&(p->pin_pg_list));
    reg_t offset = 0;
    if(!is_thread){
        p->pgdir = allocPage(p); //vaddr
        // add to pin_pg_list;
        clear_pgdir(p->pgdir);
        share_pgtable((char*)p->pgdir, (char*)pa2kva(PGDIR_PA));
    }else{
        p->pgdir = (*current_running)->pgdir;
        offset = (*current_running)->thread_num * PAGE_SIZE;
    }

    p->user_sp = alloc_page_helper(USER_STACK_ADDR - PAGE_SIZE + offset, p->pgdir, 1, p) + PAGE_SIZE;   
    p->user_stack_base = p->user_sp;
    p->kernel_sp = alloc_page_helper(KERNEL_STACK_VADDR - PAGE_SIZE + offset, p->pgdir, 0, p) + PAGE_SIZE;
    p->kernel_stack_base = p->kernel_sp;
    __list_init(&(p->pg_list));

 
    net_poll_mode = 1;
    // xemacps_example_main();
    p->user_sp -= 0x100;
    list_add_tail(&(p->list), &ready_queue);
    return pid;
}


pid_t do_mthread_create(int32_t *thread,
                   void (*start_routine)(void*),
                   void *arg)
{
    (*current_running)->thread_num++;
    pid_t pid = init_pcb(NULL, AUTO_CLEANUP_ON_EXIT, 1); //1 represents thread
    pcb_t* p = &pcb[pid - 1];
    reg_t offset = (*current_running)->thread_num * PAGE_SIZE;

    p->user_sp = USER_STACK_ADDR - 0x100 + offset;
    p->type = USER_THREAD;

    // cpy gp_value
    reg_t pr_ksp = (*current_running)->kernel_stack_base;
    regs_context_t *pr_regs =
        (regs_context_t *)(pr_ksp - sizeof(regs_context_t));
    reg_t pr_gp = pr_regs->regs[3];

    init_pcb_stack(p->kernel_sp, start_routine, p, arg, NULL, pr_gp);
    return pid;
}

pid_t do_exec(char* file_name, int argc, char* argv[]){
    int elf_length;
    char* elf_binary;
    if(!get_elf_file(file_name, &elf_binary, &elf_length))
        return -1;
        
    pid_t pid = init_pcb(NULL, AUTO_CLEANUP_ON_EXIT, 0); //0 represents process
    pcb_t* p = &pcb[pid - 1];
    p->thread_num = 0;
    
    //cpy the argvs onto user stack
    uintptr_t new_argv = USER_STACK_ADDR - 0x100;
    uintptr_t *kargv = p->user_sp;
    uintptr_t argv_offset = 0;
    uintptr_t addr_offset = argc * sizeof(uintptr_t);
    for (int i = 0; i < argc; i++){
        *(kargv + i) = new_argv + addr_offset + argv_offset;
        memcpy(p->user_sp + addr_offset  + argv_offset, argv[i], MAX_ARG_LENGTH);
        argv_offset += MAX_ARG_LENGTH;
    }

    p->user_sp = USER_STACK_ADDR - 0x100;
    uintptr_t elf_entry = load_elf(elf_binary, elf_length, p->pgdir, p, user_alloc_page_helper);
    p->type = USER_PROCESS;
    __list_init(&(p->pg_list));
    p->pg_num = 0;
    init_pcb_stack(p->kernel_sp, elf_entry, p, argc, (char**)new_argv, 0);
    return pid;
}

void init_pid0(){
    pid0_pcb_core0.kernel_sp = pid0_pcb_core0.kernel_stack_base - (sizeof(switchto_context_t) + sizeof(regs_context_t));
    pid0_pcb_core1.kernel_sp = pid0_pcb_core1.kernel_stack_base - (sizeof(switchto_context_t) + sizeof(regs_context_t));
}


void init_shell(){
    pid_t pid = init_pcb(NULL, AUTO_CLEANUP_ON_EXIT, 0);
    pcb_t* p = &pcb[pid - 1];
    p->user_sp = USER_STACK_ADDR - 0x100;
    uintptr_t shell_entry = load_elf(_elf___test_test_shell_elf,    \
                                     _length___test_test_shell_elf, \
                                     p->pgdir, p, user_alloc_page_helper);
    p->type = USER_PROCESS;
    __list_init(&(p->pg_list));
    p->pg_num = 0;
    init_pcb_stack(p->kernel_sp, shell_entry, p, 0, NULL, 0);
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
    syscall[SYSCALL_EXEC] = do_exec;
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
    syscall[SYSCALL_MTHREAD_CREATE] = do_mthread_create;
    syscall[SYSCALL_SHMPAGEGET] = shm_page_get;
    syscall[SYSCALL_SHMPAGEDT] = shm_page_dt;
    syscall[SYSCALL_SHOW_EXEC] = do_show_exec;
    syscall[SYSCALL_BINSEMGET] = do_binsemget;
    syscall[SYSCALL_BINSEMOP] = do_binsemop;
    syscall[SYSCALL_NET_RECV] = do_net_recv;
    syscall[SYSCALL_NET_SEND] = do_net_send;
    syscall[SYSCALL_NET_IRQ_MODE] = do_net_irq_mode;
    syscall[SYSCALL_MKFS] = do_mkfs;
    syscall[SYSCALL_STATFS] = do_statfs;
    syscall[SYSCALL_CD] = do_cd;
    syscall[SYSCALL_MKDIR] = do_mkdir;
    syscall[SYSCALL_RMDIR] = do_rmdir;
    syscall[SYSCALL_LS] = do_ls;
    syscall[SYSCALL_TOUCH] = do_touch;
    syscall[SYSCALL_CAT] = do_cat;
    syscall[SYSCALL_FOPEN] = do_fopen;
    syscall[SYSCALL_FWRITE] = do_fwrite;
    syscall[SYSCALL_FCLOSE] = do_fclose;
    syscall[SYSCALL_FREAD] = do_fread;  
    syscall[SYSCALL_LSEEK] = do_lseek;  
    syscall[SYSCALL_LN] = do_ln;
    syscall[SYSCALL_RM] = do_rm;
}

void load_pcb_cursor(){
    screen_cursor_x = (*current_running)->cursor_x;
    screen_cursor_y = (*current_running)->cursor_y;
}

void cancel_map()
{
    uint64_t va = 0x50200000;
    uint64_t pgdir = 0xffffffc05e000000;
    uint64_t vpn2 = 
        va >> (NORMAL_PAGE_SHIFT + PPN_BITS + PPN_BITS);
    uint64_t vpn1 = (vpn2 << PPN_BITS) ^
                    (va >> (NORMAL_PAGE_SHIFT + PPN_BITS));
    PTE *fst_pg = (PTE *)pgdir + vpn2;
    PTE *snd_pte = (PTE *)pa2kva(get_pa(*fst_pg)) + vpn1;
    *fst_pg = 0;
    *snd_pte = 0;
}
void init_ethernet(){
    uint32_t slcr_bade_addr = 0, ethernet_addr = 0;

    // get_prop_u32(_dtb, "/soc/slcr/reg", &slcr_bade_addr);
    slcr_bade_addr = sbi_read_fdt(SLCR_BADE_ADDR);
    printk("[slcr] phy: 0x%x\n\r", slcr_bade_addr);

    // get_prop_u32(_dtb, "/soc/ethernet/reg", &ethernet_addr);
    ethernet_addr = sbi_read_fdt(ETHERNET_ADDR);
    printk("[ethernet] phy: 0x%x\n\r", ethernet_addr);

    uint32_t plic_addr = 0;
    // get_prop_u32(_dtb, "/soc/interrupt-controller/reg", &plic_addr);
    plic_addr = sbi_read_fdt(PLIC_ADDR);
    printk("[plic] plic: 0x%x\n\r", plic_addr);

    uint32_t nr_irqs = sbi_read_fdt(NR_IRQS);
    // get_prop_u32(_dtb, "/soc/interrupt-controller/riscv,ndev", &nr_irqs);
    printk("[plic] nr_irqs: 0x%x\n\r", nr_irqs);

    XPS_SYS_CTRL_BASEADDR =
        (uintptr_t)ioremap((uint64_t)slcr_bade_addr, NORMAL_PAGE_SIZE);
    xemacps_config.BaseAddress =
        (uintptr_t)ioremap((uint64_t)ethernet_addr, NORMAL_PAGE_SIZE);
    uintptr_t _plic_addr =
        (uintptr_t)ioremap((uint64_t)plic_addr, 0x4000*NORMAL_PAGE_SIZE);
    // XPS_SYS_CTRL_BASEADDR = slcr_bade_addr;
    // xemacps_config.BaseAddress = ethernet_addr;
    xemacps_config.DeviceId        = 0;
    xemacps_config.IsCacheCoherent = 0;

    printk(
        "[slcr_bade_addr] phy:%x virt:%lx\n\r", slcr_bade_addr,
        XPS_SYS_CTRL_BASEADDR);
    printk(
        "[ethernet_addr] phy:%x virt:%lx\n\r", ethernet_addr,
        xemacps_config.BaseAddress);
    printk("[plic_addr] phy:%x virt:%lx\n\r", plic_addr, _plic_addr);
    plic_init(_plic_addr, nr_irqs);
    
    long status = EmacPsInit(&EmacPsInstance);
    if (status != XST_SUCCESS) {
        printk("Error: initialize ethernet driver failed!\n\r");
        assert(0);
    }
}
// jump from bootloader.
// The beginning of everything >_< ~~~~~~~~~~~~~~
int main()
{
    if(get_current_cpu_id() == 0){  // main_kernel
        smp_init();
        lock_kernel();
        init_allocated();
        init_shell();
        init_pid0();
        current_running = &current_running_core0;
        *current_running = &pid0_pcb_core0;
        //load_pcb_cursor();
        init_ethernet();
        
        printk("> [INIT] Shell initialization succeeded.\n\r");

        init_exception();
        printk("> [INIT] Interrupt Processing initialization succeeded.\n\r");
        
        init_syscall();
        printk("> [INIT] System Call initialized successfully.\n\r");

        init_fs();
        printk("> [INIT] File System initialization succeeded.\n\r");

        init_screen();
        printk("> [INIT] SCREEN initialization succeeded.\n\r");
        share_pgtable(pcb[0].pgdir, pa2kva(PGDIR_PA));


        printk("This is cpu_kernel 0\n\r");

        wakeup_other_hart();
    }else{
        lock_kernel();
        cancel_map();
        current_running = &current_running_core1;
        *current_running = &pid0_pcb_core1;
        //load_pcb_cursor();

        printk("This is cpu_kernel 1\n\r");
        setup_exception();
    }

    // read CPU frequency
    time_base = sbi_read_fdt(TIMEBASE);

    // Setup timer interrupt and enable all interrupt
    sbi_set_timer(get_ticks() + INTERVAL_CLKS);
    unlock_kernel();
    while (1) {
        enable_interrupt();
        __asm__ __volatile__("wfi\n\r");
    }
    return 0;
}