/*
 * Copyright (C) 2011-2021 Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Intel Corporation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */


#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "util.h"
#include "sethread_internal.h"

/* sgx_thread_rwlock_init:
 *  initialize a reader/writer lock
 */
int sgx_thread_rwlock_init(sgx_thread_rwlock_t *rwlock, const sgx_thread_rwlockattr_t *unused)
{
    UNUSED(unused);
    CHECK_PARAMETER(rwlock);

    rwlock->m_reader_count = 0;
    rwlock->m_writers_waiting = 0;
    rwlock->m_owner = SGX_THREAD_T_NULL;
    rwlock->m_lock = SGX_SPINLOCK_INITIALIZER;

    QUEUE_INIT(&rwlock->m_reader_queue);
    QUEUE_INIT(&rwlock->m_writer_queue);

    return 0;
}


/* sgx_thread_rwlock_destroy:
 *  destroy a reader/writer lock
 *    - this will fail if any threads are holding a lock or are
 *    waiting for the lock
 */
int SGXAPI sgx_thread_rwlock_destroy(sgx_thread_rwlock_t *rwlock)
{
    CHECK_PARAMETER(rwlock);

    SPIN_LOCK(&rwlock->m_lock);
    if ( (rwlock->m_owner != SGX_THREAD_T_NULL )
        || (rwlock->m_reader_count != 0 )
        || (rwlock->m_writers_waiting != 0)
        || (QUEUE_FIRST(&rwlock->m_reader_queue) != SGX_THREAD_T_NULL)
        || (QUEUE_FIRST(&rwlock->m_writer_queue) != SGX_THREAD_T_NULL)) {
        SPIN_UNLOCK(&rwlock->m_lock);
        return EBUSY;
    }

    SPIN_UNLOCK(&rwlock->m_lock);

    return 0;
}

/* sgx_thread_rwlock_rdlock:
 *   acquire a reader lock
 *    - should not be called if the thread is already holding the writer lock
 */
int sgx_thread_rwlock_rdlock(sgx_thread_rwlock_t *rwlock)
{
    CHECK_PARAMETER(rwlock);

    sgx_thread_t self = (sgx_thread_t)get_thread_data();

    SPIN_LOCK(&rwlock->m_lock);

    //if no writers own the lock, then read lock is allowed
    if (rwlock->m_owner == SGX_THREAD_T_NULL)
    {

        //increase the reader count to show that a new thread has the reader lock
        rwlock->m_reader_count++;

        SPIN_UNLOCK(&rwlock->m_lock);
        return 0;
    }
    else
    {
        //still holding the spinlock
        if ( rwlock->m_owner == self )
        {
            SPIN_UNLOCK(&rwlock->m_lock);
            return EDEADLK;
        }

        //insert this thread into the queue
        QUEUE_INSERT_TAIL(&rwlock->m_reader_queue, self);

        while (1)
        {
            int err = 0;
            SPIN_UNLOCK(&rwlock->m_lock);

            //make an OCall to wait for the lock
            sgx_thread_wait_untrusted_event_ocall(&err, TD2TCS(self));

            SPIN_LOCK(&rwlock->m_lock);

            //if no writers own the lock, then read lock is allowed
            if (rwlock->m_owner == SGX_THREAD_T_NULL)
            {
                //increase the reader count to show that this thread has the reader lock
                rwlock->m_reader_count++;

                //remove this thread from the reader queue - if this thread was queued
                QUEUE_REMOVE(&rwlock->m_reader_queue, self);

                SPIN_UNLOCK(&rwlock->m_lock);
                return 0;
            }
        }
    }
    /* NOTREACHED */
}

/* sgx_thread_rwlock_tryrdlock:
 *   try to acquire a reader lock
 *    - return
 *        0     if the lock is acquired
 *        EBUSY if the lock is busy
 */
int sgx_thread_rwlock_tryrdlock(sgx_thread_rwlock_t *rwlock)
{
    CHECK_PARAMETER(rwlock);

    SPIN_LOCK(&rwlock->m_lock);

    //if no writers own the lock, then read lock is allowed
    if (rwlock->m_owner == SGX_THREAD_T_NULL)
    {

        //increase the reader count to show that a new thread has the reader lock
        rwlock->m_reader_count++;

        SPIN_UNLOCK(&rwlock->m_lock);
        return 0;
    }
    else
    {
        SPIN_UNLOCK(&rwlock->m_lock);
        return EBUSY;
    }
    /* NOTREACHED */
}

/* sgx_thread_rwlock_wrlock:
 *   acquire a writer lock
 *    - should not be called if the thread is already holding the writer lock
 */
int sgx_thread_rwlock_wrlock(sgx_thread_rwlock_t *rwlock)
{
    CHECK_PARAMETER(rwlock);

    sgx_thread_t self = (sgx_thread_t)get_thread_data();

    SPIN_LOCK(&rwlock->m_lock);

    //if no writers own the lock and there are no readers
    if ((rwlock->m_owner == SGX_THREAD_T_NULL)
        && (rwlock->m_reader_count == 0))
    {
        //take the writer lock by inserting our thread ID
        rwlock->m_owner = self;

        SPIN_UNLOCK(&rwlock->m_lock);
        return 0;
    }
    else
    {
        //still holding the spinlock
        if ( rwlock->m_owner == self )
        {
            //try to acquire writer lock when we hold it is a deadlock
            SPIN_UNLOCK(&rwlock->m_lock);
            return EDEADLK;
        }
        //insert this thread into the writer queue
        QUEUE_INSERT_TAIL(&rwlock->m_writer_queue, self);

        while (1)
        {
            int err = 0;
            SPIN_UNLOCK(&rwlock->m_lock);

            //make an OCall to wait for the lock
            sgx_thread_wait_untrusted_event_ocall(&err, TD2TCS(self));
            
            SPIN_LOCK(&rwlock->m_lock);

            //if no writers own the lock and there are no readers
            if ((rwlock->m_owner == SGX_THREAD_T_NULL)
                && (rwlock->m_reader_count == 0))
            {
                //take the writer lock by inserting our thread ID
                rwlock->m_owner = self;

                //remove this thread from the reader queue - if this thread was queued
                QUEUE_REMOVE(&rwlock->m_writer_queue, self);

                SPIN_UNLOCK(&rwlock->m_lock);
                return 0;
            }
        }
    }
    /* NOTREACHED */
}

/* sgx_thread_rwlock_trywrlock:
 *   try to acquire a writer lock
 *    - return
 *        0     if the lock is acquired
 *        EBUSY if the lock is busy
 */
int sgx_thread_rwlock_trywrlock(sgx_thread_rwlock_t *rwlock)
{
    CHECK_PARAMETER(rwlock);

    sgx_thread_t self = (sgx_thread_t)get_thread_data();

    SPIN_LOCK(&rwlock->m_lock);

    //if no writers own the lock and there are no readers
    if ((rwlock->m_owner == SGX_THREAD_T_NULL)
        && (rwlock->m_reader_count == 0))
    {
        //take the writer lock by inserting our thread ID
        rwlock->m_owner = self;

        SPIN_UNLOCK(&rwlock->m_lock);
        return 0;
    }
    else
    {
        SPIN_UNLOCK(&rwlock->m_lock);
        return EBUSY;
    }
    /* NOTREACHED */
}

/* sgx_thread_rwlock_rdunlock:
 *  remove a reader lock.  If there are no more readers and there is a 
 *  writer waiting, then wake it up.
 */
int sgx_thread_rwlock_rdunlock(sgx_thread_rwlock_t *rwlock)
{
    int ret = 0;

    SPIN_LOCK(&rwlock->m_lock);

    /* if the lock is not held by anyone */
    if (rwlock->m_reader_count == 0) {
        SPIN_UNLOCK(&rwlock->m_lock);
        return EPERM;
    }

    rwlock->m_reader_count--;
    
    /* if the lock is now not held by anyone */
    if (rwlock->m_reader_count == 0)
    {   
        /* If there is a writer waiting, then
         * the thread should be awakened up by the caller.
         */
        sgx_thread_t waiter = QUEUE_FIRST(&rwlock->m_writer_queue);

        SPIN_UNLOCK(&rwlock->m_lock);
        
        if (waiter != SGX_THREAD_T_NULL) /* wake the waiter up*/
            sgx_thread_set_untrusted_event_ocall(&ret, TD2TCS(waiter));
    }
    else
    {
        SPIN_UNLOCK(&rwlock->m_lock);
    }
    return ret;
}

/* sgx_thread_rwlock_wrunlock:
 *  remove the writer lock.  If there are reader threads waiting
 *  then wake them, otherwise, if there are writer thread waiting, 
 *  then wake them.
 */
int sgx_thread_rwlock_wrunlock(sgx_thread_rwlock_t *rwlock)
{
    int ret = 0;
    
    sgx_thread_t self = (sgx_thread_t)get_thread_data();

    SPIN_LOCK(&rwlock->m_lock);

    /* if the lock is not held by this thread */
    if (rwlock->m_owner != self ) {
        SPIN_UNLOCK(&rwlock->m_lock);
        return EPERM;
    }

    rwlock->m_owner = SGX_THREAD_T_NULL;
    
    /* if there are reader locks waiting, then wake them first */
    sgx_thread_t waiter = QUEUE_FIRST(&rwlock->m_reader_queue);
    if (waiter != SGX_THREAD_T_NULL)
    {
        size_t iCount = 0;
        QUEUE_COUNT_ALL(waiter, &rwlock->m_reader_queue, iCount);
 
        const void** ppWaiters = (const void**)malloc(iCount * sizeof(const void*));
        if (!ppWaiters)
        {
            SPIN_UNLOCK(&rwlock->m_lock);
            return ENOMEM;
        }
        const void** ppWaitersTemp = ppWaiters;
        QUEUE_FOREACH(waiter, &rwlock->m_reader_queue)
        {
            *ppWaitersTemp++ = TD2TCS(waiter);
        }
        SPIN_UNLOCK(&rwlock->m_lock);
        
        /* wake all the queued threads up*/
        sgx_thread_set_multiple_untrusted_events_ocall(&ret, ppWaiters, iCount);
        free(ppWaiters);
    }
    else
    { 
        /* If we didn't wake any readers, then look for a waiting writer to wake */
        waiter = QUEUE_FIRST(&rwlock->m_writer_queue);
        SPIN_UNLOCK(&rwlock->m_lock);
        if (waiter != SGX_THREAD_T_NULL)
        {
            /* wake the waiter up*/
            sgx_thread_set_untrusted_event_ocall(&ret, TD2TCS(waiter));
        }
    }

    return ret;
}

/* sgx_thread_rwlock_unlock:
 *  if the thread holds a writer lock, then release it;
 *  otherwise release a reader lock.  
 */
int sgx_thread_rwlock_unlock(sgx_thread_rwlock_t *rwlock)
{
    sgx_thread_t self = (sgx_thread_t)get_thread_data();

    //no need to hold a spinlock here - if it is our write lock, then nobody will change it
    if (rwlock->m_owner == self)
    {
        /* if the lock is held by this thread, then we are releasing a writer lock */
        return sgx_thread_rwlock_wrunlock(rwlock);
    }
    else
    {
        /* otherwise, we are releasing a reader lock */
        return sgx_thread_rwlock_rdunlock(rwlock);
    }
}

