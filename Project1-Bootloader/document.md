# Document

## debug record

1. 无论输入0/1输出的都是kernel1
> - 原因：bootblock.S中，在判断输入值是0/1前就直接`SBI_CALL SD_CONSOLE_PUTCHAR`进行了交互，导致寄存器a0的值发生变化，用户的选择输入失效。
> - 修改：在根据输入进行判断，并决定是否跳转执行后，再
系统调用putchar进行交互。
2. kernel 1输出后PC值跑飞
> - 原因：一开始把kernel1.c中的死循环getch的部分删除了，导致没有循环将kernel卡住，结果pc跑飞，持续输出truly_illegal_insn
> - 解决：类似于kernel0.c中的实现，将这一部分加上。

3. loadboot后反复输出"it's a bootloader..."
> - 原因：经检查，跳转到new bootloader的过程是没有问题的，问题出现在createimage_ccore.c中，由于逻辑上的疏忽，导致将write_os_size写进了ph循环中，也即对应每一个ELF，每写入一个segment，都会重新写一遍write_os_size，而实际上应该是在所有的段都写完后再统一写入。
> - 解决：将write_os_size函数移出ph循环，调整为正确的逻辑，即每个ELF文件最后写入。

4. os_size_loc位置关于kernel的相关信息写入错误：
> - 原因：hexdump image后，发现对应部分内容关于kernel的扇区信息写入有误。错误原因是——首先写入2bytes关于起始扇区ID的信息后，再写入时，fseek中的参数是SEEK_CUR，然而fwrite后img指针会偏移，导致SEEK_CUR有误。
> - 解决：将扇区起始id和扇区大小两个信息算出后，一并写入，不分两次写入。

## 实验感想
虽然写代码和debug的过程很艰辛，但是也通过这些bug收获了许多之前不明白的知识，或者是未注意到的细节。并且通过这次实验，我也更加清楚了：计算机从插电到OS开始接管，这一段时间内机器是如何自动化实现的，更加深入理解了这一过程。
