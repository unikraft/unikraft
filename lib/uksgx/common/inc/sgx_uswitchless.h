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

#ifndef _SGX_USWITCHLESS_H_
#define _SGX_USWITCHLESS_H_

/*
 * Switchless SGX
 *
 * Motivation. Enclave switches are very expensive. They are mostly triggered
 * by making ECalls or OCalls. For Intel(R)SGX applications that rely on ECalls/OCalls
 * heavily, enclave switches can degrade the performance significantly.
 *
 * Overview. Switchless SGX offers switchless features that eliminiate enclave
 * switches from SGX applications. Switchless SGX addresses this issue in two
 * approaches: 1) make ECalls/OCalls themselves switchless, which are called
 * switchless ECalls/OCalls; and 2) get rid of OCalls by providing switchless
 * implementation for common operations inside enclaves (e.g., thread-related
 * operations, file I/O operations, system clock, etc.).
 *
 * Note. This version of implementation only provides switchless ECalls/OCalls.
 *
 * switchless ECalls/OCalls. In EDL file, ECalls/OCalls that are annotated with
 * keyword transition_using_threads can be accelerated by executing them in a switchless way. SGX
 * application developers are recommended to only label ECalls/OCalls as switchless
 * on condition that they are short, non-blocking and may be called frequently.
 * To be switchless, switchless ECalls/OCalls (when appropriate) are executed by
 * worker threads.
 *
 * Worker threads. Each enclave has a set of worker threads, each of which is
 * either trusted or untrusted. The trusted worker threads are responsible for
 * handling requests made by the non-enclave code; the untrusted ones are *
 * responsible for handling requests made by the enclave code. The requests
 * and responses are passed through some shared data structures that are
 * accessible by both the non-enclave and enclave halves of a Intel(R)SGX program.
 * Thanks to these worker threads, requests can be processed without performing
 * enclave switches. While a on-going request is submitted, the
 * caller (trusted or untrusted) thread will busy wait for the completion of
 * the request by an worker. Worker threads are managed by Switchless SGX; that
 * is, Intel(R)SGX application developers do not have to start, terminate, schedule
 * these threads manually.
 */

#include "sgx_error.h"
#include "sgx_eid.h"
#include "sgx_defs.h"

/*
 * A worker can be either trusted (executed inside enclave) or untrusted
 * (executed outside enclave)
 */
typedef enum {
    SGX_USWITCHLESS_WORKER_TYPE_UNTRUSTED,
    SGX_USWITCHLESS_WORKER_TYPE_TRUSTED
} sgx_uswitchless_worker_type_t;

/*
 * There are 4 worker events that may be of interest to users.
 */
typedef enum {
    SGX_USWITCHLESS_WORKER_EVENT_START, /* a worker thread starts */
    SGX_USWITCHLESS_WORKER_EVENT_IDLE, /* a worker thread is idle */
    SGX_USWITCHLESS_WORKER_EVENT_MISS, /* a worker thread misses some tasks */
    SGX_USWITCHLESS_WORKER_EVENT_EXIT, /* a worker thread exits */
    _SGX_USWITCHLESS_WORKER_EVENT_NUM,
} sgx_uswitchless_worker_event_t;

/*
 * Statistics of workers
 */
typedef struct {
    uint64_t processed; /* # of tasks that all workers have processed */
    uint64_t missed;    /* # of tasks that all workers have missed */
} sgx_uswitchless_worker_stats_t;

/*
 * Worker callbacks enables user to customize some behaviours of workers.
 */
typedef void (*sgx_uswitchless_worker_callback_t)(
                sgx_uswitchless_worker_type_t /* type */,
                sgx_uswitchless_worker_event_t /* event */,
                const sgx_uswitchless_worker_stats_t* /* stats */);


/*
 * Configuration for Switchless SGX
 */

#define SL_DEFAULT_FALLBACK_RETRIES  20000
#define SL_DEFAULT_SLEEP_RETRIES     20000
#define SL_DEFUALT_MAX_TASKS_QWORDS  1   //64
#define SL_MAX_TASKS_MAX_QWORDS      8   //512

typedef struct 
{
	uint32_t                            switchless_calls_pool_size_qwords; //number of qwords to use for outstanding calls. (actual number is x 64)

    uint32_t                            num_uworkers;  //number of untrusted (for ocalls) worker threads

    uint32_t                            num_tworkers;  //number of trusted (for ecalls) worker threads

	uint32_t                            retries_before_fallback; //how many times to execute assembly pause instruction 
                                                                 //while waiting for worker thread to start executing switchless call 
                                                                 //before falling back to direct ECall/OCall. 

	uint32_t                            retries_before_sleep;    //how many times a worker thread executes assembly pause instruction
                                                                 //while waiting for switchless call request 
                                                                 //before going to sleep

    sgx_uswitchless_worker_callback_t   callback_func[_SGX_USWITCHLESS_WORKER_EVENT_NUM]; //array of pointers to callback functions.
} sgx_uswitchless_config_t;

#define SGX_USWITCHLESS_CONFIG_INITIALIZER    {0, 1, 1, 0, 0, { 0 } }


#endif /* _SGX_USWITCHLESS_H_ */
