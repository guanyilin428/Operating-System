# README
## 1. 验证过程
> 1. make clean
> 2. make：make结束会打印kernel的相关信息
> 3. ./run_qemu.sh 
> 4. loadboot
> 5. 随后可以看到小飞机，以及两个任务轮流拿锁的字符串行。

## 2. 整体设计
### task1 任务调度
#### init_pcb
- 一次性初始化所有pcb（16个）
- 将需要执行的任务放入task数组
- pcb中的部分参数，如：entry_point等，均根据task数组的相关信息初始化
```c
  for(p = pcb; p - pcb < NUM_MAX_TASK; p++){
        p->pid = process_id++;
        p->status = TASK_READY;
        p->type = (*t)->type; 
        p->preempt_count = 0;

        p->kernel_sp = allocPage(1) + PAGE_SIZE;
        p->user_sp = allocPage(1) + PAGE_SIZE;

        if(p - pcb < ntasks){
            init_pcb_stack(p->kernel_sp, p->user_sp, (*t)->entry_point, p);
            list_add_tail(&(p->list), &ready_queue);
            t++;
        }
    }
```
- 即main中的for循环会一次性初始化所有pcb，但是只有对应任务数量的pcb会初始化pcb_stack，并加入ready_queue队列。

#### switch_to
- 先根据a0和PCB_KERNEL_SP拿到栈指针
- 内核栈指针存放到t0中，因为t0是caller-saved reg，且若直接存入sp会覆盖掉当前的sp，导致数据丢失
- 在存上文中的callee寄存器时，需要先将栈指针的值加上-(SWITCH_TO_SIZE)，再存储寄存器值
- 先存储sp，此时sp可用，将t0中的内核栈指针值转移到sp中
- 后根据sp和SWITCH_TO_XX偏移量存储剩余13个寄存器的值
- 将最新的栈指针值存入PCB的对应位置

```asm
ld t0, PCB_KERNEL_SP(a0)
  //save
  addi t0, t0, -(SWITCH_TO_SIZE)
  sd sp, SWITCH_TO_SP(t0)
  mv sp, t0
  sd ra, SWITCH_TO_RA(sp)
  sd s0, SWITCH_TO_S0(sp)
  sd s1, SWITCH_TO_S1(sp)
  sd s2, SWITCH_TO_S2(sp)
  sd s3, SWITCH_TO_S3(sp)
  sd s4, SWITCH_TO_S4(sp)
  sd s5, SWITCH_TO_S5(sp)
  sd s6, SWITCH_TO_S6(sp)
  sd s7, SWITCH_TO_S7(sp)
  sd s8, SWITCH_TO_S8(sp)
  sd s9, SWITCH_TO_S9(sp)
  sd s10, SWITCH_TO_S10(sp)
  sd s11, SWITCH_TO_S11(sp)
  sd sp, PCB_KERNEL_SP(a0)
  
  // restore next
  ld sp, PCB_KERNEL_SP(a1)
  ld ra, SWITCH_TO_RA(sp)
  ld s0, SWITCH_TO_S0(sp)
  ld s1, SWITCH_TO_S1(sp)
  ld s2, SWITCH_TO_S2(sp)
  ld s3, SWITCH_TO_S3(sp)
  ld s4, SWITCH_TO_S4(sp)
  ld s5, SWITCH_TO_S5(sp)
  ld s6, SWITCH_TO_S6(sp)
  ld s7, SWITCH_TO_S7(sp)
  ld s8, SWITCH_TO_S8(sp)
  ld s9, SWITCH_TO_S9(sp)
  ld s10, SWITCH_TO_S10(sp)
  ld s11, SWITCH_TO_S11(sp)
  mv t0, sp
  addi sp, sp, SWITCH_TO_SIZE
  sd sp, PCB_KERNEL_SP(a1)
  ld sp, SWITCH_TO_SP(t0)
  mv tp, a1
  jr ra
```
- 取寄存器中的值时，先根据a1和PCB_KERNEL_SP拿到栈指针，存放到sp中
- 根据sp和SWICH_TO_XX偏移量得到其余的寄存器值
- 将sp中内核栈指针的值转移到t0中，再取此前所存储的栈指针值到sp中
- 恢复栈指针，并修改PCB中内核栈指针的值
- 保持current_running和tp一致
> 存储时先存sp后存其余寄存器，取数时先取其余寄存器最后取sp值
> 存数时将寄存器值存入PCB中的kernel_sp所指位置的下方，并更新kernel_sp指向context的下方
> 取数时context在ksp的上方，取完后恢复kernel_sp

#### ready_queue
实验中的设计效仿了linux中关于链表的设计，将链表抽象出来，所以需要借助两个宏得到链表节点对应的PCB地址
```c
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE*)0)->MEMBER)
#define container_of(ptr, type, member) ({ \
    const typeof( ((type *)0)->member) *__mptr = (ptr); \
    (type *)( (char*)__mptr - offsetof(type, member));})
```

### task2 互斥锁
- 修改了结构体的定义，增加了guard变量
```c
typedef enum{
    UNGUARDED,
    GUARDED
}guard_status_t;

typedef struct mutex_lock
{
    spin_lock_t lock;
    guard_t     guard;
    list_head block_queue;
} mutex_lock_t;
```
- 由于本实验中的锁不为自旋锁，没有变量使得lock和unlock两个动作成为原子化的动作，所以增加了guard变量，使得两个动作原子化
```c
void do_mutex_lock_release(mutex_lock_t *lock)
{
    while( atomic_cmpxchg_d(UNGUARDED, GUARDED, (ptr_t)&(lock->guard.status))== GUARDED )
        ;   //acquire guard_lock by spinning
    lock->lock.status = UNLOCKED;

    list_node_t *tmp = lock->block_queue.next;
    while( !list_empty(&(lock->block_queue)) )
    {
        do_unblock(tmp);
        tmp = tmp->next;
    }
    
    lock->guard.status = UNGUARDED;
}
```
- 以unlock中封装的lock_release为例，在函数开始时原子化地compare&exchange了guard变量的状态，函数最后再将guard变量设置为UNGUARDED，使得unlock动作原子化
