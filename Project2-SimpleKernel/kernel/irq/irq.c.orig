#include <os/irq.h>
#include <os/time.h>
#include <os/sched.h>
#include <os/string.h>
#include <stdio.h>
#include <assert.h>
#include <sbi.h>
#include <screen.h>
#include <csr.h>

handler_t irq_table[IRQC_COUNT];
handler_t exc_table[EXCC_COUNT];
uintptr_t riscv_dtb;

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

void reset_irq_timer()
{
    // TODO clock interrupt handler.
    // TODO: call following functions when task4
    screen_reflush();
    timer_check();

    // note: use sbi_set_timer
    sbi_set_timer(get_ticks() + INTERVAL_CLKS);
    // remember to reschedule
    do_scheduler();
}

void interrupt_helper(regs_context_t *regs, uint64_t stval, uint64_t cause)
{
   // if(cause & SCAUSE_IRQ_FLAG){
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
    printk("sstatus: 0x%lx sbadaddr: 0x%lx scause: %lu\n\r",
           regs->sstatus, regs->sbadaddr, regs->scause);
    printk("sepc: 0x%lx\n\r", regs->sepc);
    assert(0);
}
