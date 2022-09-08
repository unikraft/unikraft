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
 * File: td_ql_logic.h 
 *  
 * Description: API definitions for TD quote library 
 *
 */
#ifndef _TD_QL_LOGIC_H_
#define _TD_QL_LOGIC_H_
#include "sgx_ql_lib_common.h"
#include "sgx_quote_4.h"

#if defined(__cplusplus)
extern "C" {
#endif
quote3_error_t td_set_enclave_load_policy(sgx_ql_request_policy_t policy);

quote3_error_t td_init_quote(sgx_ql_cert_key_type_t certification_key_type,
                             bool refresh_att_key);
quote3_error_t td_get_quote_size(sgx_ql_cert_key_type_t certification_key_type,
                                 uint32_t *p_quote_size);
quote3_error_t td_get_quote(const sgx_report2_t *p_app_report,
        sgx_quote4_t *p_quote,
        uint32_t quote_size);

quote3_error_t td_set_qe_path(const char *p_path);
quote3_error_t td_set_qpl_path(const char *p_path);
void *get_qpl_handle();
#if defined(__cplusplus)
}
#endif


#endif

