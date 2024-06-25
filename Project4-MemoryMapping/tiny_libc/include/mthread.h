/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *            Copyright (C) 2018 Institute of Computing Technology, CAS
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *                              A Mini PThread-like library
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

#ifndef MTHREAD_H_
#define MTHREAD_H_

#include <stdint.h>
#include <stdatomic.h>
/* on success, these functions return zero. Otherwise, return an error number */
#define EBUSY  1 /* the lock is busy(for example, it is locked by another thread) */
#define EINVAL 2 /* the lock is invalid */

typedef int mthread_mutex_t;

int mthread_mutex_init(mthread_mutex_t* handle);
int mthread_mutex_lock(void* handle);
int mthread_mutex_unlock(void* handle);
int mthread_mutex_trylock(void *handle);
int mthread_mutex_destroy(void *handle);

int mutex_get();
void mutex_block(int id);
void mutex_unlock(int id);

typedef struct mthread_barrier
{
    int id;
    int count;
} mthread_barrier_t;

int mthread_barrier_init(mthread_barrier_t *handle, unsigned count);
int mthread_barrier_wait(mthread_barrier_t *handle);
int mthread_barrier_destroy(mthread_barrier_t *handle);

typedef int mthread_semaphore_t;

int mthread_semaphore_init(mthread_semaphore_t *handle, int val);
int mthread_semaphore_up(mthread_semaphore_t *handle);
int mthread_semaphore_down(mthread_semaphore_t *handle);
int mthread_semaphore_destroy(mthread_semaphore_t *handle);
typedef int32_t mthread_t;
int mthread_create(mthread_t *thread,
                   void (*start_routine)(void*),
                   void *arg);
int mthread_join(mthread_t thread);
#endif
