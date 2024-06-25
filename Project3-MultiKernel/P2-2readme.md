#### P2-2 readme

##### 1.验证方法

- 如果需要验证`timer_tasks`, `sched2_tasks`, `lock2_tasks`则修改代码如下
  - `main.c` 中的`init_task`函数

  ```c
  int init_task(task_info_t **task){
      int i, j = 0;
  
      for(i = 0; i < num_timer_tasks; i++)
          task[j++] = timer_tasks[i];
      for(i = 0; i < num_sched2_tasks; i++)
          task[j++] = sched2_tasks[i];
      for(i = 0; i < num_lock2_tasks; i++)
          task[j++] = lock2_tasks[i];
      /*
      for(i = 0; i < num_fork_tasks; i++)
          task[j++] = fork_tasks[i];
      */
      return j;
  }
  ```

  ​	不可以将fork_tasks初始化，因为其余tasks并未添加优先级，所以此时测试是无优先级的。

  - `sched.c`中的`do_scheduler()`函数

  ```c
  //current_running = prior_sched();
  current_running = container_of(ready_queue.next, pcb_t, list);
  ```

  由于调度不加入优先级选择，所以current_running直接由`ready_queue`产生，而不由`prior_sched()`函数产生。

- 如果需要验收`fork_task()`，则修改代码如下：

  - `main.c` 中的`init_task`函数

    ```c
    int init_task(task_info_t **task){
        int i, j = 0;
    	/*
        for(i = 0; i < num_timer_tasks; i++)
            task[j++] = timer_tasks[i];
        for(i = 0; i < num_sched2_tasks; i++)
            task[j++] = sched2_tasks[i];
        for(i = 0; i < num_lock2_tasks; i++)
            task[j++] = lock2_tasks[i];
        */
        for(i = 0; i < num_fork_tasks; i++)
            task[j++] = fork_tasks[i];
       
        return j;
    }
    ```

    由于前面的任务并未添加优先级，所以不能和`fork_task`同时跑

  - `sched.c`中的`do_scheduler()`函数

    ```c
    current_running = prior_sched();
    //current_running = container_of(ready_queue.next, pcb_t, list);
    ```

    此时调度需要根据优先级来调度，所以`current_running`的产生应当由优先级调度函数`prior_sched()`产生

- 注意：由于实现了附加题，若需要测试是否处于用户态，本代码框架是在`test_timer.c`文件中添加了`assert_supervisor_mode()`函数，若要正确执行，应当将其中对应的函数注释掉。

- 修改上述文件后：

  - `make clean`
  - `make`
  - `make floppy`
  - `sudo minicom`
  - `loadboot`
  - 注：在测试`fork`实现时，屏幕上会显示“this is father process (num)"，随后输入希望输入的子进程优先级，此处只能键入1--9，数字越大对应优先级越高。父进程的优先级默认设置为0，优先级最低。



