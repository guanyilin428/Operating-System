/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *            Copyright (C) 2018 Institute of Computing Technology, CAS
 *               Author : Han Shukai (email : hanshukai@ict.ac.cn)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *                       System call related processing
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * */

#ifndef OS_SYSCALL_NUMBER_H_
#define OS_SYSCALL_NUMBER_H_


#define IGNORE 0
#define NUM_SYSCALLS 64

/* define */
#define SYSCALL_EXEC 0
#define SYSCALL_EXIT 1
#define SYSCALL_SLEEP 2
#define SYSCALL_KILL 3
#define SYSCALL_WAITPID 4
#define SYSCALL_PS 5
#define SYSCALL_GETPID 6
#define SYSCALL_YIELD 7

#define SYSCALL_MUTEX_GET 8
#define SYSCALL_MUTEX_LOCK 9
#define SYSCALL_MUTEX_UNLOCK 10
#define SYSCALL_FORK 11
#define SYSCALL_GETCHAR 12
#define SYSCALL_PRIOR 13

#define SYSCALL_PUTCHAR 14

#define SYSCALL_FUTEX_WAIT 17
#define SYSCALL_FUTEX_WAKEUP 18
#define SYSCALL_WRITE 19
#define SYSCALL_READ 20
#define SYSCALL_CURSOR 21
#define SYSCALL_REFLUSH 22
#define SYSCALL_SCREEN_CLEAR 23

#define SYSCALL_SERIAL_READ 24
#define SYSCALL_SERIAL_WRITE 25
#define SYSCALL_READ_SHELL_BUFF 26

#define SYSCALL_GET_TIME 29
#define SYSCALL_GET_TIMEBASE 30
#define SYSCALL_GET_TICK 31

#define SYSCALL_SEMAPHORE_INIT 32
#define SYSCALL_SEMAPHORE_DOWN 33
#define SYSCALL_SEMAPHORE_UP 34
#define SYSCALL_SEMAPHORE_DESTROY 35

#define SYSCALL_BARRIER_INIT 36
#define SYSCALL_BARRIER_WAIT 37
#define SYSCALL_BARRIER_DESTROY 38

#define SYSCALL_MBOX_OPEN 39
#define SYSCALL_MBOX_CLOSE 40
#define SYSCALL_MBOX_SEND 41
#define SYSCALL_MBOX_RECV 42
#define SYSCALL_MBOX_OP 43

#define SYSCALL_MTHREAD_CREATE 44
#define SYSCALL_SHMPAGEGET 45
#define SYSCALL_SHMPAGEDT 46
#define SYSCALL_SHOW_EXEC 47

#define SYSCALL_BINSEMGET 48
#define SYSCALL_BINSEMOP 49
#define SYSCALL_NET_RECV 50
#define SYSCALL_NET_SEND 51
#define SYSCALL_NET_IRQ_MODE 52

#endif
