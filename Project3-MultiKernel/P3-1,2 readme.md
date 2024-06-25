### P3-1, 3-2 readme

#### 验证

- loadboot后会打印shell页面
- P3的验证程序顺序并未更改
  - exec 0: 执行test_kill的一系列任务，可以通过kill对应pid观察行为
  - exec 1: semaphore的任务，已实现
  - exec 2: barrier的任务，已实现
  - exec 5: 生成strGenerator的进程，可以反复exec 5生成多个
  - exec 4: 生成strServer进程，生成一个，进行str的接收，并向Client传递position
- 此处的ps还只会显示current_running的信息，并非打印整个pcb_table的process信息，之后会修改为打印整个pcb_table的相关进程信息
- 双核实现待更新

