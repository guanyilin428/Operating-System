#include <os/time.h>
#include <os/mm.h>
#include <os/irq.h>
#include <type.h>

uint64_t time_elapsed = 0;
uint32_t time_base = 0;

LIST_HEAD(timers);

uint64_t get_ticks()
{
    __asm__ __volatile__(
        "rdtime %0"
        : "=r"(time_elapsed));
    return time_elapsed;
}

uint64_t get_timer()
{
    return get_ticks() / time_base;
}

uint64_t get_time_base()
{
    return time_base;
}

void latency(uint64_t time)
{
    uint64_t begin_time = get_timer();

    while (get_timer() - begin_time < time);
    return;
}

uint32_t get_time(uint32_t* time_elapsed)
{
    *time_elapsed = get_ticks();
    return time_base;
}