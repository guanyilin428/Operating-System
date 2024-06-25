#ifndef MM_H
#define MM_H

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *            Copyright (C) 2018 Institute of Computing Technology, CAS
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *                                   Memory Management
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

#include <type.h>
#include <os/sched.h>
#include <os/list.h>
#include <pgtable.h>

#define MEM_SIZE 32
#define PAGE_SIZE 4096 // 4K
#define INIT_KERNEL_STACK 0xffffffc051000000lu
#define FREEMEM (INIT_KERNEL_STACK + 3*PAGE_SIZE)
#define USER_STACK_ADDR 0xf00010000lu
#define SHM_VA_START    0xf00011000lu
#define USER_VA_TOP     0x3ffffffffflu

typedef struct pg_node{
    ptr_t page_vaddr;
    list_node_t list;
}pg_node_t;

typedef struct shmpg_node{
    int key;
    uintptr_t shmpg_paddr;
    list_node_t list;
    int procnum;
    list_head va_info;
}shmpg_node_t;

typedef struct shmpg_info
{
    pid_t pid;
    uintptr_t va;
    list_node_t list;
}shmpg_info_t;


/* Rounding; only works for n = power of two */
#define ROUND(a, n)     (((((uint64_t)(a))+(n)-1)) & ~((n)-1))
#define ROUNDDOWN(a, n) (((uint64_t)(a)) & ~((n)-1))

extern ptr_t memCurr;
extern list_head RecyclePages;
extern list_head ShmPage;
extern ptr_t allocPage(struct pcb* pcb);
extern void freePage(uintptr_t kva);
extern void* kmalloc(size_t size);
extern void share_pgtable(uintptr_t dest_pgdir, uintptr_t src_pgdir);

extern int search_last_pte(uintptr_t va, uintptr_t pgdir, PTE** pte);
extern uintptr_t check_alloc(uintptr_t va, uintptr_t pgdir);
extern uintptr_t alloc_page_helper(uintptr_t va, uintptr_t pgdir, int is_user, struct pcb* pcb);
//extern uintptr_t (uintptr_t va, uintptr_t pgdir);
extern void add_to_pg_list(uintptr_t va, struct pcb* p);
uintptr_t shm_page_get(int key);
void shm_page_dt(uintptr_t addr);

#endif /* MM_H */
