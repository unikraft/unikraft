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

#ifndef PTHREAD_IMP_H__
#define PTHREAD_IMP_H__

#include "sgx_thread.h"
#include "setjmp.h"
#include "sgx_spinlock.h"
#include "pthread.h"

#define WAITER_TD_NULL 0

#define PTHREAD_WAIT_TIMEOUT_SECONDS 5

typedef struct _sgx_pthread_common_queue_elm_t
{
    volatile struct _sgx_pthread_common_queue_elm_t *   next;
}sgx_pthread_common_queue_elm_t;

typedef struct _sgx_pthread_common_queue_t
{
    volatile struct _sgx_pthread_common_queue_elm_t *   first;  /* first element */
    volatile struct _sgx_pthread_common_queue_elm_t *   last;   /* last element */
}sgx_pthread_common_queue_t;

struct sgx_pthread_storage {
    int keyid;
    struct sgx_pthread_storage *next;
    void *data;
};

struct sgx_thread_key {
    int used;
    void (*destructor)(void *);
};

//_pthread->state
typedef enum {
    _STATE_NONE = 0,
    _STATE_PREPARING,
    _STATE_EXECUTION,
    _STATE_WAKEDUP,
    _STATE_ERROR_OUT_OF_TCS,
    _STATE_ERROR_UNEXPECTED,
} _pthread_state;

typedef struct _pthread
{
    volatile sgx_spinlock_t    lock;
    volatile _pthread_state    state;
    sgx_thread_t         joiner_td;    /* The thread's TD who execute join for this target thread */
    sgx_thread_t         creater_td;
    struct _sgx_pthread_common_queue_elm_t   common_queue_elm;
    void *(*start_routine)(void *);
    void *               arg;	
    void *               retval;  /* Used by sgx_thread_exit() and sgx_thread_join() to pass the exit message */
    void *               tid;
}pthread;

typedef struct _pthread_attr
{
    char    reserved;
}pthread_attr;

typedef struct _pthread_info
{
    pthread *         m_pthread;
    struct sgx_pthread_storage  *m_local_storage;
    jmp_buf         m_mark;
    sgx_status_t    m_state;
}pthread_info;

#endif
