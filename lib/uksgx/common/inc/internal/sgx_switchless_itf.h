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

#ifndef _SWITCHLESS_ITF_H_
#define _SWITCHLESS_ITF_H_

#include "sgx_eid.h"
#include "sgx_error.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef sgx_status_t(*sl_init_func_t)(sgx_enclave_id_t, const void*, void**);
typedef sgx_status_t(*sl_ecall_func_t)(void*, const unsigned int, void*, int*);
typedef sgx_status_t(*sl_destroy_func_t)(void*);
typedef void(*sl_ocall_fallback_func_t)(void*);
typedef sgx_status_t(*sl_on_first_ecall_func_t)(void*, sgx_enclave_id_t, const void*);

typedef struct
{
    sl_init_func_t sl_init_func_ptr;
    sl_ecall_func_t sl_ecall_func_ptr;
    sl_destroy_func_t sl_destroy_func_ptr;
    sl_ocall_fallback_func_t sl_ocall_fallback_func_ptr;
    sl_on_first_ecall_func_t sl_on_first_ecall_func_ptr;

} sgx_switchless_funcs_t;


typedef void(*sgx_set_switchless_itf_func_t)(const sgx_switchless_funcs_t*);

#define SL_SET_SWITCHLESS_INTERFACE_FUNC_NAME "sgx_set_switchless_itf"

void sgx_set_switchless_itf(const sgx_switchless_funcs_t* sl_funcs);

#ifdef __cplusplus
}
#endif


#endif
