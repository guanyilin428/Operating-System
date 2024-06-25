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

handler_t irq_table[IRQC_COUNT];
handler_t exc_table[EXCC_COUNT];
uintptr_t riscv_dtb;

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

void reset_irq_timer()
{
    // TODO clock interrupt handler.
    // TODO: call following functions when task4
    timer_check();

    // note: use sbi_set_timer
    sbi_set_timer(get_ticks() + INTERVAL_CLKS);
    // remember to reschedule
    do_scheduler();
}

void interrupt_helper(regs_context_t *regs, uint64_t stval, uint64_t cause)
{
   // if(cause & SCAUSE_IRQ_FLAG){
    current_running = get_current_cpu_id()? &current_running_core1 : &current_running_core0;
    //load_pcb_cursor();
    screen_reflush();

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

void init_exception()
{
    int i;
    for(i = 0; i < IRQC_COUNT; i++){
        irq_table[i] = handle_int;
    }
    for(i = 0; i < EXCC_COUNT; i++){
        exc_table[i] = handle_other;
    }
    exc_table[EXCC_SYSCALL] = handle_syscall;
    setup_exception();
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
