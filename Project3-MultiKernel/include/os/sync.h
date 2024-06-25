/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *            Copyright (C) 2018 Institute of Computing Technology, CAS
 *               Author : Han Shukai (email : hanshukai@ict.ac.cn)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *            Synchronous primitive related content implementation,
 *                 such as: locks, barriers, semaphores, etc.
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

#ifndef INCLUDE_SYNC_H_
#define INCLUDE_SYNC_H_

#include <os/lock.h>
#include <mthread.h>
#include <string.h>
#include <mailbox.h>

#define NAME_LENGTH 30
typedef struct semaphore {
    int sem;
    list_head queue;
}semaphore_t;

typedef struct barrier{
    int count;
    int max;
    list_head queue;
}barrier_t;

typedef struct{
    char buff[MAX_MBOX_LENGTH];
    int id;
    char name[NAME_LENGTH];
    int start; // point the start
    int end;   // point the end (end has content)
    int empty;
    list_head send;
    list_head recv;
}kmailbox;

int do_semaphore_init(mthread_semaphore_t* handle, int val);
void do_semaphore_up(mthread_semaphore_t* handle);
void do_semaphore_down(mthread_semaphore_t* handle);
void do_semaphore_destroy(mthread_semaphore_t* handle);

void do_barrier_init(mthread_barrier_t* handle, int count);
void do_barrier_wait(mthread_barrier_t *handle);
void do_barrier_destroy(mthread_barrier_t *handle);

mailbox_t do_mailbox_open(char *name);
void do_mailbox_close(mailbox_t* mailbox);
int do_mailbox_send(mailbox_t* mailbox, char* msg, int msg_len);
int do_mailbox_recv(mailbox_t* mailbox, char *msg, int msg_len);

#endif
