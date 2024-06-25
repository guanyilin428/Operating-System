#include <os/irq.h>
#include <os/time.h>
#include <os/sched.h>
#include <os/string.h>
#include <os/stdio.h>
#include <assert.h>
#include <sbi.h>
#include <screen.h>
#include <csr.h>
#include <os/smp.h>
#include <os/mm.h>
#include <emacps/xemacps_example.h>
#include <plic.h>
#include <net.h>

handler_t irq_table[IRQC_COUNT];
handler_t exc_table[EXCC_COUNT];
int sd_block_id = 1024;

/*
void timer_check()
{
    if(timers.next == &timers) //empty then do nothing
        return;          
    uint64_t sleep_time = check_sleep(&timers);
    uint64_t now = get_timer();
    list_node_t *wake_node;
        
    while(sleep_time < now){
        wake_node = timers.next;
        do_unblock(wake_node);
        if(timers.next == &timers) //empty
            break;
        sleep_time = check_sleep(&timers);
    }
}   
*/
void timer_check(){
    if(timers.next == &timers) //empty then do nothing
        return;
    list_node_t *wake_node, *tmp;
    uint64_t now = get_timer();
    pcb_t* p;

    wake_node = timers.next;
    while(wake_node != &timers && !list_empty(&timers)){
        p = container_of(wake_node, pcb_t, list);
        tmp = wake_node->next;
        if(p->wake_time <= now){
            do_unblock(wake_node);
        }
        wake_node = tmp;
    }
}

void check_net_reg(){
    volatile u32 txsr = XEmacPs_ReadReg(EmacPsInstance.Config.BaseAddress, XEMACPS_TXSR_OFFSET);
    if(!list_empty(&net_send_queue)){
        if(txsr & XEMACPS_TXSR_TXCOMPL_MASK){
            // printk("successfully unblock\n\r");
            pcb_t* p = container_of(net_send_queue.next, pcb_t, list);
            // printk("the pid is %d\n\r", p->pid);
            do_unblock(net_send_queue.next);
        }
    }
    XEmacPs_WriteReg(EmacPsInstance.Config.BaseAddress, XEMACPS_TXSR_OFFSET, txsr | XEMACPS_TXSR_TXCOMPL_MASK);

    volatile u32 rxsr = XEmacPs_ReadReg(EmacPsInstance.Config.BaseAddress,
                                XEMACPS_RXSR_OFFSET);
    if(!list_empty(&net_recv_queue)){
        if (rxsr & XEMACPS_RXSR_FRAMERX_MASK){
            // printk("successfully unblock\n\r");
            do_unblock(net_recv_queue.next);
        }
    }   
    XEmacPs_WriteReg(EmacPsInstance.Config.BaseAddress, XEMACPS_RXSR_OFFSET, rxsr | XEMACPS_RXSR_FRAMERX_MASK);         
}

void reset_irq_timer()
{
    // clock interrupt handler.
    // call following functions when task4
    timer_check();

    // note: use sbi_set_timer
    sbi_set_timer(get_ticks() + INTERVAL_CLKS);
    // remember to reschedule
    do_scheduler();
}

void interrupt_helper(regs_context_t *regs, uint64_t stval, uint64_t cause)
{
   // if(cause & SCAUSE_IRQ_FLAG){
    // static int cnt = 0;
    current_running = get_current_cpu_id()? &current_running_core1 : &current_running_core0;
    load_pcb_cursor();
    screen_reflush();

    // printk("the cause is %lx\n\r", cause);
    // printk("the stval is %lx\n\r", stval);
    // printk("cnt is %d\n\r", cnt++);
    // printk("the cpu id is %d\n\r", get_current_cpu_id());

    if(cause >> 63){
        irq_table[(cause<<1)>>1](regs, stval, cause);
    }
    else{
        exc_table[cause](regs, stval, cause);
    }
}

void handle_int(regs_context_t *regs, uint64_t interrupt, uint64_t cause)
{
    reset_irq_timer();
}

void handle_irq(regs_context_t *regs, int irq)
{
    // printk("enter handle irq\n\r");
    // TODO: 
    // handle external irq from network device
    // let PLIC know that handle_irq has been finished
    u32 RegISR;
    RegISR = XEmacPs_ReadReg(EmacPsInstance.Config.BaseAddress,
				   XEMACPS_ISR_OFFSET);
    XEmacPs_WriteReg(EmacPsInstance.Config.BaseAddress, XEMACPS_ISR_OFFSET,
			   RegISR | XEMACPS_IXR_ALL_MASK);
    
    //printk("the mode is %d\n\r", net_poll_mode);
    //static int cnt = 3;
    //screen_move_cursor(1, cnt++);
    // printk("enter handle irq2\n\r");
    if(net_poll_mode)
        check_net_reg();
    
    plic_irq_eoi(irq);
}

void init_exception()
{
    int i;
    for(i = 0; i < IRQC_COUNT; i++){
        irq_table[i] = handle_int;
    }
    for(i = 0; i < EXCC_COUNT; i++){
        exc_table[i] = handle_other;
    }
    irq_table[IRQC_S_EXT           ] = plic_handle_irq;
    exc_table[EXCC_SYSCALL] = handle_syscall;
    exc_table[EXCC_INST_PAGE_FAULT] = handle_inst_page_fault;
    exc_table[EXCC_LOAD_PAGE_FAULT] = handle_ld_page_fault;
    exc_table[EXCC_STORE_PAGE_FAULT] = handle_st_page_fault;
    setup_exception();
}

void handle_inst_page_fault(regs_context_t *regs, uint64_t stval, uint64_t cause){
    uintptr_t kva;
    uintptr_t va = stval;
    uintptr_t pgdir = (*current_running)->pgdir;

    if(check_alloc(va, pgdir)){
        PTE* pte;
        search_last_pte(va, (*current_running)->pgdir, &pte);
        set_attribute(pte, _PAGE_ACCESSED|_PAGE_DIRTY); 
        local_flush_tlb_all();
        local_flush_icache_all();
    }else{
        prints("error inst!!%lx\n", stval);
        while(1) ;
    }
}

void handle_ld_page_fault(regs_context_t *regs, uint64_t stval, uint64_t cause){
    handle_st_ld_page_fault(regs, stval, cause, 1);
}

void handle_st_page_fault(regs_context_t *regs, uint64_t stval, uint64_t cause){
    handle_st_ld_page_fault(regs, stval, cause, 0);
}

//to be checked
PTE* find_swap_pte(){
    list_node_t* node = (*current_running)->pg_list.next;
    pg_node_t * pg = container_of(node, pg_node_t, list);

    PTE* pte;
    search_last_pte(pg->page_vaddr, (*current_running)->pgdir, &pte);
    if((*pte) & _PAGE_PRESENT){ // in mem
        __list_del_entry(node);
        list_add_tail(node, &RecyclePages);
        return pte;
    }
}

void mem2sd_swap(PTE* pte){
    uintptr_t paddr = get_pa(*pte);
    sbi_sd_write(paddr, 8, sd_block_id);

    *pte = sd_block_id<<_PAGE_PFN_SHIFT;
    *pte &= ~_PAGE_PRESENT;
    sd_block_id += 8;
    (*current_running)->pg_num--;
}

// type: 1--ld(A) 0--st(D)
void handle_st_ld_page_fault(regs_context_t *regs, uint64_t stval, uint64_t cause, int type){
    uintptr_t va = stval;
    uintptr_t pgdir = (*current_running)->pgdir;
    
    //swap
    if((*current_running)->pg_num >= MAX_PAGE_NUM){
        PTE* pte = find_swap_pte();
        mem2sd_swap(pte);
    } 
    
    PTE* thr_pg;
    int res = search_last_pte(va, pgdir, &thr_pg);
    if(!res || *thr_pg == 0){ // not allocated yet
        alloc_page_helper(va, pgdir, 1, *current_running);
        search_last_pte(va, pgdir, &thr_pg);
        set_attribute(thr_pg, type ? _PAGE_ACCESSED : (_PAGE_ACCESSED|_PAGE_DIRTY));
        local_flush_tlb_all();
        local_flush_icache_all();
        return;
    }else if(*thr_pg & _PAGE_PRESENT){ // allocated and in mem
        set_attribute(thr_pg, type ? _PAGE_ACCESSED : (_PAGE_ACCESSED|_PAGE_DIRTY));
        local_flush_tlb_all();
        local_flush_icache_all();
    }else{ // allocated and in disk
        int block_id = (*thr_pg) >> _PAGE_PFN_SHIFT;
        *thr_pg = 0;
        uintptr_t kva = alloc_page_helper(va, pgdir, 1, *current_running);
        sbi_sd_read(kva2pa(kva), 8, block_id);
        set_attribute(thr_pg, type ? _PAGE_ACCESSED : (_PAGE_ACCESSED|_PAGE_DIRTY));
        local_flush_tlb_all();
        local_flush_icache_all();
    }
}

void handle_other(regs_context_t *regs, uint64_t stval, uint64_t cause)
{
    char* reg_name[] = {
        "zero "," ra  "," sp  "," gp  "," tp  ",
        " t0  "," t1  "," t2  ","s0/fp"," s1  ",
        " a0  "," a1  "," a2  "," a3  "," a4  ",
        " a5  "," a6  "," a7  "," s2  "," s3  ",
        " s4  "," s5  "," s6  "," s7  "," s8  ",
        " s9  "," s10 "," s11 "," t3  "," t4  ",
        " t5  "," t6  "
    };
    for (int i = 0; i < 32; i += 3) {
        for (int j = 0; j < 3 && i + j < 32; ++j) {
            printk("%s : %016lx ",reg_name[i+j], regs->regs[i+j]);
        }
        printk("\n\r");
    }
    printk("sstatus: 0x%lx sbadaddr: 0x%lx scause: %lx\n\r",
           regs->sstatus, regs->sbadaddr, regs->scause);
    printk("stval: 0x%lx cause: %lx\n\r",
           stval, cause);
    printk("sepc: 0x%lx\n\r", regs->sepc);
    // printk("mhartid: 0x%lx\n\r", get_current_cpu_id());

    uintptr_t fp = regs->regs[8], sp = regs->regs[2];
    printk("[Backtrace]\n\r");
    printk("  addr: %lx sp: %lx fp: %lx\n\r", regs->regs[1] - 4, sp, fp);
    // while (fp < USER_STACK_ADDR && fp > USER_STACK_ADDR - PAGE_SIZE) {
    while (fp > 0x10000) {
        uintptr_t prev_ra = *(uintptr_t*)(fp-8);
        uintptr_t prev_fp = *(uintptr_t*)(fp-16);

        printk("  addr: %lx sp: %lx fp: %lx\n\r", prev_ra - 4, fp, prev_fp);

        fp = prev_fp;
    }

    assert(0);
}
