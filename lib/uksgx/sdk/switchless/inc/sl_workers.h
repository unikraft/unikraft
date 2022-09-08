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

#ifndef _SL_WORKERS_H_
#define _SL_WORKERS_H_

#include <stdbool.h>
#include <sgx_uswitchless.h>
#ifndef SL_INSIDE_ENCLAVE /* Untrusted */
#include <pthread.h>
#endif

/*
 * Abbreviations
 */
#define sl_worker_type_t                sgx_uswitchless_worker_type_t
#define sl_worker_event_t               sgx_uswitchless_worker_event_t
#define sl_worker_stats_t               sgx_uswitchless_worker_stats_t
#define sl_worker_callback_t            sgx_uswitchless_worker_callback_t

#define SL_WORKER_TYPE_UNTRUSTED        SGX_USWITCHLESS_WORKER_TYPE_UNTRUSTED
#define SL_WORKER_TYPE_TRUSTED          SGX_USWITCHLESS_WORKER_TYPE_TRUSTED

#define SL_WORKER_EVENT_START           SGX_USWITCHLESS_WORKER_EVENT_START
#define SL_WORKER_EVENT_IDLE            SGX_USWITCHLESS_WORKER_EVENT_IDLE
#define SL_WORKER_EVENT_MISS            SGX_USWITCHLESS_WORKER_EVENT_MISS
#define SL_WORKER_EVENT_EXIT            SGX_USWITCHLESS_WORKER_EVENT_EXIT


struct sl_uswitchless;

struct sl_workers
{
    struct sl_uswitchless*              handle;
    sl_worker_type_t                    type;
    sl_worker_stats_t                   stats;
    volatile int32_t                    should_wake;
    uint64_t                            num_all;
    uint64_t                            num_running;
    uint64_t                            num_sleeping;
#ifndef SL_INSIDE_ENCLAVE /* Untrusted */
    pthread_t*                          threads;
#else /* Trusted */
    void*                               __unused;
#endif /* SL_INSIDE_ENCLAVE */
};

#ifdef __cplusplus
extern "C" {
#endif

uint32_t sl_workers_init(struct sl_workers* workers,
                        sl_worker_type_t type,
                        struct sl_uswitchless* handle);

void sl_workers_destroy(struct sl_workers* workers);

uint32_t sl_workers_init_threads(struct sl_workers* workers);

uint32_t sl_workers_run_threads(struct sl_workers* workers);

void sl_workers_kill_threads(struct sl_workers* workers);

void sl_workers_notify_event(struct sl_workers* workers, sl_worker_event_t event);

void wake_all_threads(struct sl_workers* workers);

#ifdef __cplusplus
}
#endif

#endif /* _SL_WORKERS_H_ */
