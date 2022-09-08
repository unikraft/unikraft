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

#ifndef _SL_USWITCHLESS_H_
#define _SL_USWITCHLESS_H_

#include <sgx_uswitchless.h>
#include <sl_fcall_mngr.h>
#include <sl_workers.h>

#ifndef SL_INSIDE_ENCLAVE /* Untrusted */
#include <pthread.h>
#endif

/*
 * Abbreviations
 */
#define sl_config_t                     sgx_uswitchless_config_t
#define SL_CONFIG_INITIALIZER           SGX_USWITCHLESS_CONFIG_INITIALIZER


/*
 * Data structure that stores the per-enclave state for Switchless SGX.
 *
 * Security note: This data structure is untrusted to the enclave code.
 * Security measures like sanity checks and object "clone" are deployed.
 *
 * @see sl_fcall_mngr.c
 */


#include "sl_once.h"

typedef struct _ocall_table_t
{
    uint32_t count;
    void *ocall[];
} sgx_ocall_table_t;


#pragma pack(push, 1)

struct sl_uswitchless 
{
    sl_config_t                 us_config;
    const sgx_ocall_table_t*    us_ocall_table;
    sl_once_t                   us_init_tswitchless;
    sgx_enclave_id_t            us_enclave_id;
    volatile uint64_t           us_has_new_ocall_fallback;
    struct sl_call_mngr         us_ocall_mngr;
    struct sl_call_mngr         us_ecall_mngr;
    volatile int64_t            us_should_stop;
    struct sl_workers           us_uworkers;
    struct sl_workers           us_tworkers;
#ifndef SL_INSIDE_ENCLAVE /* Untrusted */
    pthread_t                   us_waker;
#else
    void*                       __unused__;
#endif
    volatile uint64_t           us_wake_workers;
    volatile uint64_t           us_init_finished;

};

#pragma pack(pop)

/* Public APIs of sl_uswitchless */
#include <uswitchless.h>

/*Internal APIs of sl_uswitchless */

#endif /* _SL_USWITCHLESS_H_ */
