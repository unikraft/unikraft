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
* File: sgx_ql_quote.h
*
* Description: Generic SGX quote reference code definitions.
*
*/

/* User defined types */
#ifndef _SGX_QL_QUOTE_H_
#define _SGX_QL_QUOTE_H_
#include <stddef.h>
#include "sgx_ql_lib_common.h"
#include "sgx_quote.h"
#include "sgx_quote_3.h"


#pragma pack(push, 1)
/** Describes the algorithm parameters needed to generate the given algorithm's signature.  Used for quote generation
 *  APIs. */
typedef struct _sgx_ql_att_key_id_param_t {
    uint32_t    algorithm_param_size;               ///< Size of additional attestation key information.  0 is valid.
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning ( disable:4200 )
#endif
    uint8_t     algorithm_param[];                  ///< Additional attestation algorithm information.For example, SigRL for EPID.
#ifdef _MSC_VER
#pragma warning(pop)
#endif
}sgx_ql_att_key_id_param_t;

/** The full data structure passed to the platform by the verifier. It will list all of the attestation algorithms and
 *  QE's supported by the verifier */
typedef struct _sgx_ql_att_id_list_t {
    sgx_ql_att_key_id_list_header_t   header;       ///< Header for the attestation key ID list provided by the quote verifier.
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning ( disable:4200 )
#endif
    sgx_att_key_id_ext_t              ext_id_list[];///< Place holder for the extended attestation ID list.
#ifdef _MSC_VER
#pragma warning(pop)
#endif
}sgx_ql_att_key_id_list_t;

typedef struct _sgx_ql_qe_report_info_t {
    sgx_quote_nonce_t nonce;
    sgx_target_info_t app_enclave_target_info;
    sgx_report_t qe_report;
}sgx_ql_qe_report_info_t;

#pragma pack(pop)

#ifdef __cplusplus
/** Describes the generic Quoting API used by all attestation keys/algorithms.  A particular quoting implementer will implement this interface.
    Application can use this interface to remain agnostic to the attestation key used to generate a quote. */
class IQuote {
public:
    virtual ~IQuote() {}

    virtual quote3_error_t init_quote(sgx_ql_att_key_id_t* p_att_key_id,
                                      sgx_ql_cert_key_type_t certification_key_type,
                                      sgx_target_info_t *p_target_info,
                                      bool refresh_att_key,
                                      size_t* p_pub_key_id_size,
                                      uint8_t* p_pub_key_id) = 0;

    virtual quote3_error_t get_quote_size(sgx_ql_att_key_id_t* p_att_key_id,
                                          sgx_ql_cert_key_type_t certification_key_type,
                                          uint32_t* p_quote_size) = 0;

    virtual quote3_error_t get_quote(const sgx_report_t *p_app_report,
                                     sgx_ql_att_key_id_t* p_att_key_id,
                                     sgx_ql_qe_report_info_t *p_qe_report_info,
                                     sgx_quote3_t *p_quote,
                                     uint32_t quote_size) = 0;
};
#endif //#ifdef __cplusplus
#endif //_SGX_QL_QUOTE_H_
