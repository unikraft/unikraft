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
/**
 * File: sgx_urts_wrapper.h
 *
 * Description: SGX uRTS wrapper APIs, for loading uRTS library dynamically
 *
 */

#ifndef _SGX_URTS_WRAPPER_H_
#define _SGX_URTS_WRAPPER_H_

#include "sgx_urts.h"

#if defined(__cplusplus)
extern "C" {
#endif


#if defined(_MSC_VER)
#ifdef UNICODE
#define SGX_URTS_API_CREATE_ENCLAVE "sgx_create_enclavew"

typedef sgx_status_t (SGXAPI *sgx_create_enclave_func_t)(const LPCWSTR file_name,
    const int debug,
    sgx_launch_token_t* launch_token,
    int* launch_token_updated,
    sgx_enclave_id_t* enclave_id,
    sgx_misc_attribute_t* misc_attr);
#else

#define SGX_URTS_API_CREATE_ENCLAVE "sgx_create_enclavea"

typedef sgx_status_t (SGXAPI *sgx_create_enclave_func_t)(const LPCSTR file_name,
    const int debug,
    sgx_launch_token_t* launch_token,
    int* launch_token_updated,
    sgx_enclave_id_t* enclave_id,
    sgx_misc_attribute_t* misc_attr);

#endif /* !UNICODE */

#elif defined(__GNUC__)
#define SGX_URTS_API_CREATE_ENCLAVE "sgx_create_enclave"

typedef sgx_status_t (SGXAPI *sgx_create_enclave_func_t)(const char* file_name,
    const int debug,
    sgx_launch_token_t* launch_token,
    int* launch_token_updated,
    sgx_enclave_id_t* enclave_id,
    sgx_misc_attribute_t* misc_attr);

#define SGX_URTS_API_OCALL_PTHREAD_CREATE "pthread_create_ocall"
#define SGX_URTS_API_OCALL_PTHREAD_TIMEOUT "pthread_wait_timeout_ocall"
#define SGX_URTS_API_OCALL_PTHREAD_WAKEUP "pthread_wakeup_ocall"

typedef int (*pthread_create_ocall_func_t)(unsigned long long self);
typedef int (*pthread_wait_timeout_ocall_func_t)(unsigned long long waiter, unsigned long long timeout);
typedef int (*pthread_wakeup_ocall_func_t)(unsigned long long waiter);

#endif

#define SGX_URTS_API_DESTROY_ENCLAVE "sgx_destroy_enclave"
#define SGX_URTS_API_ECALL "sgx_ecall"
#define SGX_URTS_API_OCALL_CPUID "sgx_oc_cpuidex"
#define SGX_URTS_API_OCALL_WAIT_EVENT "sgx_thread_wait_untrusted_event_ocall"
#define SGX_URTS_API_OCALL_SET_EVENT "sgx_thread_set_untrusted_event_ocall"
#define SGX_URTS_API_OCALL_WAITSET_EVENT "sgx_thread_setwait_untrusted_events_ocall"
#define SGX_URTS_API_OCALL_SET_MULTIPLE_EVENT "sgx_thread_set_multiple_untrusted_events_ocall"



typedef sgx_status_t (SGXAPI* sgx_destroy_enclave_func_t)(const sgx_enclave_id_t enclave_id);

typedef sgx_status_t (SGXAPI* sgx_ecall_func_t)(const sgx_enclave_id_t eid, const int index, const void* ocall_table, void* ms);

typedef void (*sgx_oc_cpuidex_func_t)(int cpuinfo[4], int leaf, int subleaf);

typedef int (*sgx_thread_wait_untrusted_event_ocall_func_t)(const void *self);

typedef int (*sgx_thread_set_untrusted_event_ocall_func_t)(const void *waiter);

typedef int (*sgx_thread_setwait_untrusted_events_ocall_func_t)(const void *waiter, const void *self);

typedef int (*sgx_thread_set_multiple_untrusted_events_ocall_func_t)(const void **waiters, size_t total);


bool sgx_dcap_load_urts();


#if defined(__cplusplus)
}
#endif

#endif /* !_SGX_URTS_WRAPPER_H_*/
