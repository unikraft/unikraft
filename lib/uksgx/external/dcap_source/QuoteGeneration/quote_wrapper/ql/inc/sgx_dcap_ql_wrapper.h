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
 * File: sgx_dcap_ql_wrapper.h
 *
 * Description: Definitions and prototypes for SGX's quote library for
 * use in the DCAP SDK.
 *
 */
#ifndef _SGX_DCAP_QL_WRAPPER_H_
#define _SGX_DCAP_QL_WRAPPER_H_

#include "sgx_report.h"
#include "sgx_pce.h"
#include "sgx_ql_lib_common.h"

#if defined(__cplusplus)
extern "C" {
#endif

quote3_error_t sgx_qe_set_enclave_load_policy(sgx_ql_request_policy_t policy);

quote3_error_t sgx_qe_get_target_info(sgx_target_info_t *p_qe_target_info);

quote3_error_t sgx_qe_get_quote_size(uint32_t *p_quote_size);

quote3_error_t sgx_qe_get_quote(const sgx_report_t *p_app_report,
                                uint32_t quote_size,
                                uint8_t *p_quote);

quote3_error_t sgx_qe_cleanup_by_policy();

#ifndef _MSC_VER
typedef enum
{
    SGX_QL_QE3_PATH,
    SGX_QL_PCE_PATH,
    SGX_QL_QPL_PATH,
    SGX_QL_IDE_PATH,
} sgx_ql_path_type_t;
quote3_error_t sgx_ql_set_path(sgx_ql_path_type_t path_type, const char *p_path);
#endif

#if defined(__cplusplus)
}
#endif

#endif /* !_SGX_DCAP_QL_WRAPPER_H_ */

