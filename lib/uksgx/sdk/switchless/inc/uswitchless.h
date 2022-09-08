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

#ifndef _USWITCHLESS_H_
#define _USWITCHLESS_H_

#include "sl_uswitchless.h"


// struct sl_uswitchless maintains the per-enclave state for Switchless SGX,
// which are used by both outside and inside the enclave. This struct is
// intentionally defined as an opaque type to hide the internals of
// the implementation of Switchless SGX from uRTS and tRTS.
struct sl_uswitchless;

#ifdef __cplusplus
extern "C" {
#endif


sgx_status_t sl_init_uswitchless(sgx_enclave_id_t enclave_id, const void* config, void** switchless_pp);

//sgx_status_t sl_destroy_uswitchless(sgx_enclave_id_t enclave_id);
sgx_status_t sl_destroy_uswitchless(void* _switchless);

sgx_status_t sl_uswitchless_new(const sgx_uswitchless_config_t* config,
                                const sgx_enclave_id_t enclave_id,
                                struct sl_uswitchless** uswitchless_pp);

void sl_uswitchless_free(struct sl_uswitchless* handle);

uint32_t sl_uswitchless_init_workers(struct sl_uswitchless* handle);

uint32_t sl_uswitchless_start_workers(struct sl_uswitchless* handle,
                                 const sgx_ocall_table_t* ocall_table);

void sl_uswitchless_stop_workers(struct sl_uswitchless* handle);

//void sl_uswitchless_check_switchless_ocall_fallback(sgx_enclave_id_t enclave_id);
void sl_uswitchless_check_switchless_ocall_fallback(void* _switchless);

sgx_status_t sl_uswitchless_do_switchless_ecall(void* _switchless,
                                                const unsigned int ecall_id,
                                                void* ecall_ms,
                                                int* need_fallback);

sgx_status_t sl_uswitchless_on_first_ecall(void* _switchless, sgx_enclave_id_t enclave_id, const void* ocall_table);

sgx_status_t sl_ocall_wake_workers(void* ms);

#ifdef __cplusplus
}
#endif

#endif /* _USWITCHLESS_H_*/
