## OS P4 rpt

#### 1. 初始化

```c
static pid_t init_pcb(void* arg, spawn_mode_t mode)
{
    int pid = find_pid();
    pid_allocated[pid] = 1;
    pcb_t* p = &pcb[pid - 1];
    p->pid = pid;
    p->parent = 0;
    p->status = TASK_READY;
    //p->type = info->type;
    p->preempt_count = 0;
    for(int i = 0; i < LOCK_NUM; i++)
        p->lock[i] = 0;
    p->lock_num = 0;
    p->sched_time = 0;
    p->mode = mode;
    p->pgdir = allocPage();
    __list_init(&(p->pg_list));
    share_pgtable(p->pgdir, pa2kva(PGDIR_PA));
    p->user_sp = alloc_page_helper(USER_STACK_ADDR - PAGE_SIZE, p->pgdir, 1) + PAGE_SIZE; 
    p->user_sp = ROUND(p->user_sp, 128); //128?
    add_pg_node(p->user_sp, &(p->pg_list));
    p->kernel_sp = alloc_page_helper(KERNEL_STACK_VADDR - PAGE_SIZE, p->pgdir, 0) + PAGE_SIZE;
    p->kernel_sp = ROUND(p->kernel_sp, 128);
    add_pg_node(p->kernel_sp, &(p->pg_list));
    p->user_sp = USER_STACK_ADDR;
    //init_pcb_stack(p->kernel_sp, p->user_sp, info->entry_point, p, arg);
    list_add_tail(&(p->list), &ready_queue);
    return pid;
}

```

重点关注用户栈和内核栈的分配问题

```c
 p->user_sp = alloc_page_helper(USER_STACK_ADDR - PAGE_SIZE, p->pgdir, 1) + PAGE_SIZE; 
    p->user_sp = ROUND(p->user_sp, 128); //128?
    add_pg_node(p->user_sp, &(p->pg_list));
    p->kernel_sp = alloc_page_helper(KERNEL_STACK_VADDR - PAGE_SIZE, p->pgdir, 0) + PAGE_SIZE;
    p->kernel_sp = ROUND(p->kernel_sp, 128);
    add_pg_node(p->kernel_sp, &(p->pg_list));
    p->user_sp = USER_STACK_ADDR;
```



#### 2. 物理页的分配和回收

- 数据结构：

  - 物理页

  ```c
  typedef struct pg_node{
      ptr_t page_vaddr;
      list_node_t list;
  }pg_node_t;
  ```

  在分配新的一个物理页框时，会给该页框赋予一个结构体，如上，其中`page_vaddr`负责记录该页的虚拟基地址（线性映射）

  

  - 回收页的链表

  `LIST_HEAD(RecyclePages);`

  该链表为全局变量，记录被回收的页表。在`release_sources`函数中会将释放的空闲页加入此链表



- 分配和回收算法：

  分配算法：

  ```c
  //return vaddr
  ptr_t allocPage()
  {
      // align PAGE_SIZE
      if(!list_empty(&RecyclePages)){
          pg_node_t* p = container_of(RecyclePages.next, pg_node_t, list);
          list_add_tail(&(p->list), &((*current_running)->pg_list));
          return p->page_vaddr;
      }
      ptr_t ret = ROUND(memCurr, PAGE_SIZE); 
      memCurr = ret + PAGE_SIZE;
      pg_node_t pg;
      pg.page_vaddr = ret;
      list_add_tail(&(pg.list), &((*current_running)->pg_list));
      return ret;
  }s
  ```

  在分配一个新的物理页时，首先会判断`RecyclePages`链表是否为空，如果不为空，则分配已回收的物理页；否则分配新页。

  该函数返回物理页的虚拟基地址。

  

  回收算法：

  `pcb`结构体中新加入的数据结构`list_head pg_list;`用于记录已分配给该进程的页表。

  当释放资源时，将此链表指针传入`freePage`函数，将其中的结点从该链表删除，并加入`RecyclePages`链表中。

  ``` c
  void freePage(list_head* head){
      while(!list_empty(head)){
          list_node_t* node = head->next;
          __list_del_entry(node);
          list_add_tail(node, &RecyclePages);
      }
  }
  ```



#### 2. exec修改

- 目标：格式变化为 exec 可执行文件名 arg0 arg1 ... （q：这几个参数是有什么用的呢？？？？）
- exec：传参放在用户栈的栈顶
- **init pcb传递exec的参数，将其放置在用户栈的栈顶**

#### 3.对ls的支持

- 要求：ls要求显示所有可以exec的名字，这些名字会由`generateMapping`程序自动生成





思考 pgdir应该是指针还是？？涉及initpcb和loadelf