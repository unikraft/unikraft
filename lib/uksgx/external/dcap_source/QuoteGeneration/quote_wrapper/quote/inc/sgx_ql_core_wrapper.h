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
/** File: sgx_ql_core_wrapper.h
 *
 * Description: Definitions and prototypes for the core quoting API's for genererting generic attestaion key
 *              quotes.
 *
 */
#ifndef _SGX_QL_CORE_WRAPPER_H_
#define _SGX_QL_CORE_WRAPPER_H_

#include "sgx_quote_3.h"
#include "sgx_ql_quote.h"


#define SGX_QL_MAX_ATT_KEY_IDS 10

#define SGX_QL_CERT_TYPE PPID_RSA3072_ENCRYPTED

#if defined(__cplusplus)
extern "C" {
#endif

quote3_error_t sgx_ql_set_enclave_load_policy(sgx_ql_request_policy_t policy);

quote3_error_t sgx_ql_init_quote(sgx_ql_att_key_id_t *p_att_key_id,
                                 sgx_target_info_t *p_qe_target_info,
                                 bool refresh_att_key,
                                 size_t *p_pub_key_id_size,
                                 uint8_t *p_pub_key_id);

quote3_error_t sgx_ql_get_quote_size(sgx_ql_att_key_id_t *p_att_key_id,
                                     uint32_t *p_quote_size);

quote3_error_t sgx_ql_get_quote(const sgx_report_t *p_app_report,
                                sgx_ql_att_key_id_t *p_att_key_id,
                                sgx_ql_qe_report_info_t *p_qe_report_info,
                                uint8_t *p_quote,
                                uint32_t quote_size);

quote3_error_t sgx_set_qe3_path(const char *p_path);
quote3_error_t sgx_set_ide_path(const char *p_path);
quote3_error_t sgx_set_qpl_path(const char *p_path);
quote3_error_t sgx_ql_get_keyid(sgx_att_key_id_ext_t *p_att_key_id_ext);
#if defined(__cplusplus)
}
#endif

#endif /* !_SGX_QL_WRAPPER_H_ */

