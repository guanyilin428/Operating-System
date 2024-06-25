#include <sys/syscall.h>
#include <stdatomic.h>
#include <time.h>
#include <mthread.h>


#define ADD_MAX_TIMES 10000

int lock;
int sum;

static inline uint32_t atomic_swap(uint32_t val, uint64_t mem_addr)
{
    uint32_t ret;
    __asm__ __volatile__ (
        "amoswap.w.aqrl %0, %2, %1\n"
        : "=r"(ret), "+A" (*(uint32_t*)mem_addr)
        : "r"(val)
        : "memory");
    return ret;
}

static inline uint64_t atomic_swap_d(uint64_t val, uint64_t mem_addr)
{
    uint64_t ret;
    __asm__ __volatile__ (
                          "amoswap.d.aqrl %0, %2, %1\n"
                          : "=r"(ret), "+A" (*(uint64_t*)mem_addr)
                          : "r"(val)
                          : "memory");
    return ret;
}

/* if *mem_addr == old_val, then *mem_addr = new_val, else return *mem_addr */
static inline uint32_t atomic_cmpxchg(uint32_t old_val, uint32_t new_val, uint64_t mem_addr)
{
    uint32_t ret;
    register unsigned int __rc;
    __asm__ __volatile__ (
          "0:	lr.w %0, %2\n"	
          "	bne  %0, %z3, 1f\n"
          "	sc.w.rl %1, %z4, %2\n"
          "	bnez %1, 0b\n"
          "	fence rw, rw\n"
          "1:\n"
          : "=&r" (ret), "=&r" (__rc), "+A" (*(uint32_t*)mem_addr)
          : "rJ" (old_val), "rJ" (new_val)
          : "memory");
    return ret;
}


void thread_lock_init(){
    atomic_swap(0, &lock);
}

void thread_lock_acquire(){
    while(atomic_swap(1, &lock) == 1)
        ;
}

void thread_lock_release(){
    lock = 0;
}

void add_thread(void* arg){
    sys_move_cursor(1,1);
    printf("thread1 starts calculating\n");
    while(sum < ADD_MAX_TIMES){
        thread_lock_acquire();
        sum++;
        thread_lock_release();
    }
    sys_exit();
}

int main(){
    thread_lock_init();
    sum = 0;
    clock_t start = clock();

    mthread_t add;

    mthread_create(&add, add_thread, 0);

    sys_move_cursor(1,2);
    printf("thread2 starts calculating\n");

    while(sum < ADD_MAX_TIMES){
        thread_lock_acquire();
        sum++;
        thread_lock_release();
    }
   
    clock_t end = clock();
    printf("result: sum is %d\n", sum);
    printf("the multi-threads uses: %ld ticks\n", end - start);
    
    return 0;
}