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
* File: sgx_ql_ecdsa.h
*
 *Description: The quoting library's instantiation of the the quoting interface to support ECDSA-P256 quotes.
*
*/

/* User defined types */
#ifndef _SGX_QL_ECDSA_QUOTE_H_
#define _SGX_QL_ECDSA_QUOTE_H_

#include "sgx_ql_quote.h"

/**
    Class definition of the reference ECDSA-P256 quoting code which implements the quoting interface, IQuote.
*/
class ECDSA256Quote :public IQuote {
public:
    virtual quote3_error_t set_enclave_load_policy(sgx_ql_request_policy_t policy);

    virtual quote3_error_t init_quote(sgx_ql_att_key_id_t* p_att_key_id,
                                      sgx_ql_cert_key_type_t certification_key_type,
                                      sgx_target_info_t *p_target_info,
                                      bool refresh_att_key,
                                      size_t* p_pub_key_id_size,
                                      uint8_t* p_pub_key_id);

    virtual quote3_error_t get_quote_size(sgx_ql_att_key_id_t* p_att_key_id,
                                          sgx_ql_cert_key_type_t certification_key_type,
                                          uint32_t* p_quote_size);

    virtual quote3_error_t get_quote(const sgx_report_t *p_app_report,
                                     sgx_ql_att_key_id_t* p_att_key_id,
                                     sgx_ql_qe_report_info_t *p_qe_report_info,
                                     sgx_quote3_t *p_quote,
                                     uint32_t quote_size);

private:
    quote3_error_t ecdsa_set_enclave_load_policy(sgx_ql_request_policy_t policy);

    quote3_error_t ecdsa_init_quote(sgx_ql_cert_key_type_t certification_key_type,
                                    sgx_target_info_t *p_target_info,
                                    bool refresh_att_key,
                                    ref_sha256_hash_t *p_pub_key_id);

    quote3_error_t ecdsa_get_quote_size(sgx_ql_cert_key_type_t certification_key_type,
                                        uint32_t* p_quote_size);

    quote3_error_t ecdsa_get_quote(const sgx_report_t *p_app_report,
                                   sgx_ql_qe_report_info_t *p_qe_report_info,
                                   sgx_quote3_t *p_quote,
                                   uint32_t quote_size);
};
#endif  //_SGX_QL_ECDSA_QUOTE_H_

