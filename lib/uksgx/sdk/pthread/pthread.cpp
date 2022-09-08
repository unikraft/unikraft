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
#include <setjmp.h>
#include <errno.h>
#include "util.h"
#include <assert.h>
#include <stdio.h>

#include "sgx_trts.h"
#include "sgx_spinlock.h"
#include "sgx_thread.h"
#include "pthread_imp.h"

#include "internal/se_cdefs.h"
#include "internal/arch.h"
#include "internal/thread_data.h"
#include "trts_internal.h"
#include "trts_util.h"
#include "sgx_pthread_t.h"

#define SGX_THREAD_QUEUE_FIRST(que)   ((que)->first)
#define SGX_THREAD_QUEUE_NEXT(elm)     ((elm)->next)

#define SGX_THREAD_QUEUE_INSERT_TAIL(que, elm, lock) do {       \
    sgx_spin_lock(lock);                                     \
    (elm)->next = NULL;				                \
    if ((que)->first != NULL)  {                              \
        ((que)->last)->next = (elm);    \
    } else {  \
        (que)->first = (elm);				          \
    }                                                                                                          \
    (que)->last = (elm); 					          \
    sgx_spin_unlock(lock);            \
} while (0)

#define SGX_THREAD_QUEUE_GET_HEADER(que, elm, lock) do {            \
    sgx_spin_lock(lock);                                     \
    if (NULL != (elm = SGX_THREAD_QUEUE_FIRST(que) )) {      \
        if( NULL == (elm)->next)  { \
            (que)->first = NULL;	\
            (que)->last = NULL;  \
        } else {  \
            (que)->first = (elm)->next;   \
        } \
    } \
    sgx_spin_unlock(lock);            \
} while (0)

static inline void asm_pause(void)
{
    __asm__ __volatile__( "pause" : : : "memory" );
}

static volatile sgx_pthread_common_queue_t  g_work_queue = {NULL, NULL};
static volatile sgx_spinlock_t g_work_queue_lock = SGX_SPINLOCK_INITIALIZER;
/* This TLS variable is used to store every thread's unique Identity */
__thread pthread_info pthread_info_tls = {NULL, NULL, {0, 0, 0, 0, 0, 0, 0, 0}, SGX_SUCCESS};

bool _pthread_enabled(void)
{
    if(is_tcs_binding_mode() == false)
    {
        pthread_info_tls.m_local_storage = NULL;
    }
    pthread_info_tls.m_pthread = NULL;
    pthread_info_tls.m_state = SGX_SUCCESS;
    memset((char*)&pthread_info_tls.m_mark, 0x00, sizeof(jmp_buf));
    return true;
}

void _pthread_tls_store_state(sgx_status_t state)
{
    pthread_info_tls.m_state = state;
    return;
}
sgx_status_t _pthread_tls_get_state(void)
{
    return pthread_info_tls.m_state;
}
 void _pthread_tls_store_context(void* context)
 {
     //Store the context in TLS variable which will be used by pthread_exit()
     memcpy((char*)&pthread_info_tls.m_mark, (char*)context, sizeof(jmp_buf));
     return;
 }

void _pthread_wakeup_join(void* ms)
{
    pthread_t thread = pthread_info_tls.m_pthread;
    if(NULL == thread)
        //do nothing
        return;

    if(NULL == ms || !sgx_is_outside_enclave(ms, sizeof(uint64_t)))
        abort();

    sgx_spin_lock(&thread->lock);
    if(thread->joiner_td != WAITER_TD_NULL) {
        //Wake the pthread_join thread.
        uint64_t *waiter = (uint64_t*)ms;
        *waiter = (uint64_t)TD2TCS(thread->joiner_td);
    }
    thread->state = _STATE_WAKEDUP;
    thread->tid = NULL;
    sgx_spin_unlock(&thread->lock);
    return;
}

/*
 * timeout: Seconds
 * Note: timeout == 0 means wait without timeout.
 */
int _pthread_wait_timeout(sgx_thread_t waiter_td, uint64_t timeout)
{
    sgx_status_t ret = SGX_ERROR_UNEXPECTED;
    pthread_wait_timeout_ocall((int*)&ret, (unsigned long long)TD2TCS(waiter_td), timeout);
    if(SGX_SUCCESS != ret) {
        //ERROR
        return -1;
    }
    return 0;
}

/*
 * Wakeup the target TD
 */
int _pthread_wakeup(sgx_thread_t td)
{
    if(WAITER_TD_NULL == td) {
        //do nothing
        return 0;
    }
    sgx_status_t ret = SGX_ERROR_UNEXPECTED;
    pthread_wakeup_ocall((int*)&ret, (unsigned long long)TD2TCS(td));
    if(ret != SGX_SUCCESS)
        return -1;
    return 0;
}

/*
 * Note: This function is the entry point of sgx_pthread
 */
sgx_status_t _pthread_thread_run(void* ms)
{
    UNUSED(ms);
    volatile sgx_pthread_common_queue_elm_t *elm = NULL;
    /*
      * Note: Every time entry into _pthread_thread_run(), it can get an task from "g_work_queue" successfully.
      *             Because the task was added into the queue before pthread_create_ocall() called.
      */
    SGX_THREAD_QUEUE_GET_HEADER(&g_work_queue, elm, &g_work_queue_lock);
    if(NULL != elm) {
        pthread_t thread = container_of(elm, pthread, common_queue_elm);
        sgx_spin_lock(&thread->lock);
        //Fill the thread's TLS
        pthread_info_tls.m_pthread = thread;
        pthread_info_tls.m_local_storage = NULL;
        thread->tid = &pthread_info_tls;
        thread->state = _STATE_EXECUTION;
        sgx_spin_unlock(&thread->lock);
        _pthread_wakeup(thread->creater_td);
        //Execute the thread function
        thread->start_routine(thread->arg);
    }
    return SGX_SUCCESS;
}

/*
 *: function: create a new thread
 *  limitation: Currently the "attr" is unused inside SGX pthread_create(). And the thread is created with "PTHREAD_CREATE_JOINABLE" attribute.
 *                      So user need to call pthread_join() to free pthread_create() created thread's resource.
 */
int pthread_create(pthread_t *threadp, const pthread_attr_t *attr,
                   void *(*start_routine)(void *), void *arg)
{
    if (threadp == NULL)
        return EINVAL;
    if (start_routine == NULL ||!sgx_is_within_enclave((void *)start_routine, sizeof(void*)))
        return EINVAL;
    if (arg != NULL && !sgx_is_within_enclave((void *)arg, sizeof(void *)))
        return EINVAL;
    UNUSED(attr);

    pthread_t thread = (pthread_t )malloc(sizeof(pthread));
    if(NULL == thread) {
        *threadp = NULL;
        return ENOMEM;
    }
    //inital thread structure
    thread->lock = SGX_SPINLOCK_INITIALIZER;
    thread->tid = NULL;
    thread->retval = NULL;
    thread->joiner_td = WAITER_TD_NULL;   /* clear the waiter's TD, the  joiner_td value is dynamic according to the joiner thread*/
    thread->creater_td = (sgx_thread_t)get_thread_data();
    thread->state = _STATE_PREPARING;
    thread->start_routine = start_routine;   //Hook the func
    thread->arg = arg;
    SGX_THREAD_QUEUE_INSERT_TAIL(&g_work_queue, &thread->common_queue_elm, &g_work_queue_lock);

    int retValue = EINVAL;
    sgx_status_t ret = SGX_ERROR_UNEXPECTED;
    pthread_create_ocall((int*)&ret, (unsigned long long)TD2TCS((sgx_thread_t)get_thread_data()));
    if(SGX_SUCCESS != ret) {
        //ERROR
        //Delete an element from the header of the work queue
        volatile sgx_pthread_common_queue_elm_t *elm = NULL;
        SGX_THREAD_QUEUE_GET_HEADER(&g_work_queue, elm, &g_work_queue_lock);
        if(NULL != elm) {
            pthread_t target_thread = container_of(elm, pthread, common_queue_elm);
            if(pthread_equal(target_thread, thread)) {
                retValue = (SGX_ERROR_OUT_OF_TCS == ret) ? EAGAIN : EINVAL;
                goto pthread_create_error;
            }else {
                //Notify the remote thread/local thread that need exit with an ERROR code.
                sgx_spin_lock(&target_thread->lock);
                target_thread->state = (SGX_ERROR_OUT_OF_TCS == ret) ? _STATE_ERROR_OUT_OF_TCS : _STATE_ERROR_UNEXPECTED;
                sgx_spin_unlock(&target_thread->lock);
                _pthread_wakeup(target_thread->creater_td);
            }
        }
    } 

    //Wait until the "start_routine" has been executed by new created thread.
    while(1) {
        if(_pthread_wait_timeout(thread->creater_td, PTHREAD_WAIT_TIMEOUT_SECONDS) != 0) {
            retValue = EINVAL;
            goto pthread_create_error;
        }

        sgx_spin_lock(&thread->lock);
        if(_STATE_PREPARING != thread->state) {
             sgx_spin_unlock(&thread->lock);
             if(thread->state == _STATE_ERROR_OUT_OF_TCS ||thread->state == _STATE_ERROR_UNEXPECTED ) {
                 //ERROR
                 retValue = (_STATE_ERROR_OUT_OF_TCS == thread->state) ? EAGAIN : EINVAL;
                 goto pthread_create_error;
             }
            //SUCCESS
            break;
        }
        sgx_spin_unlock(&thread->lock);

    }

    //SUCCESS
    *threadp = thread;
    return 0;

pthread_create_error:
    *threadp = NULL;
    free(thread);
    return retValue;
}

/*
 *: terminate calling thread
 */
__attribute__((noreturn)) void pthread_exit(void *retval)
{
    pthread_t  thread = pthread_self();
    if(NULL != thread)
        thread->retval = retval;
    longjmp(pthread_info_tls.m_mark, 0x01);

    abort();
}

extern uint32_t volatile g_uninit_flag;
/*
 *: join with a terminated thread
 *Note: 1.The results of multiple simultaneous calls to pthread_join() specifying the same target thread are undefined.
 *            2.Joining with a thread that has previously been joined results in undefined behavior.
 */
int pthread_join(pthread_t thread, void **retval)
{
    if (thread == NULL ||!sgx_is_within_enclave((void *)thread, sizeof(*thread)))
        return EINVAL;

    pthread_t self_thread = pthread_self();
    sgx_thread_t self_td = (sgx_thread_t)get_thread_data();

    if(NULL != self_thread && pthread_equal(thread, self_thread)) {
        //Can't wait ourself
        return EDEADLK;  /* Resource deadlock would occur */
    }

    // SKip to join if pthread_join() is called durig UNINIT ecall.
    //
    // No need to actually join the threads if pthread_join() is called 
    // during UNINIT ecall. Urts would sync the threads instead.
    if(__sync_and_and_fetch(&g_uninit_flag, 1))
    {
        return 0;
    }

    sgx_spin_lock(&thread->lock);
    //check whether the target thread has already been waited by other thread
    if( WAITER_TD_NULL != thread->joiner_td) {
        sgx_spin_unlock(&thread->lock);
        return EDEADLK;  /* Resource deadlock would occur */
    }
    sgx_spin_unlock(&thread->lock);

    int retValue = EINVAL;
    while(1) {
        sgx_spin_lock(&thread->lock);
        if( _STATE_WAKEDUP == thread->state) {
             sgx_spin_unlock(&thread->lock);
              /*
                * pthread_join() enter here means the target thread has already exited.
                * So it's safe to access 'thread' without spin lock.
                */
            if(NULL != retval)
                  *retval = thread->retval;
            free(thread);
            retValue = 0;  //SUCCESS
            break;
        }
        if(WAITER_TD_NULL == thread->joiner_td)
            thread->joiner_td = self_td;
        sgx_spin_unlock(&thread->lock);

        if(_pthread_wait_timeout(self_td, PTHREAD_WAIT_TIMEOUT_SECONDS) != 0) {
            return EINVAL;
        }
    }

    return retValue;
}

/*
 *: obtain ID of the calling thread
 */
pthread_t pthread_self(void)
{
    pthread_t t = (pthread_t)(pthread_info_tls.m_pthread);
    // For threads that are created outside enclave t value is NULL
    if (!t) { 
        return (pthread_t)(sgx_thread_self());
    }
    return t;
}

/*
 *: compare thread IDs
 */
int pthread_equal(pthread_t t1, pthread_t t2)
{
     return (t1 == t2);
}

