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
 * File: sgx_dcap_pcs_com.h
 *
 * Description: Definitions and prototypes for the PCS/PCCS communication APIs.
 *
 */

#ifndef _SGX_DCAP_PCS_COM_H_
#define _SGX_DCAP_PCS_COM_H_

#include "sgx_ql_lib_common.h"

#if defined(__cplusplus)
extern "C" {
#endif

#define QL_API_GET_QUOTE_VERIFICATION_COLLATERAL "sgx_ql_get_quote_verification_collateral"
#define QL_API_FREE_QUOTE_VERIFICATION_COLLATERAL "sgx_ql_free_quote_verification_collateral"

#define QL_API_GET_QVE_IDENTITY "sgx_ql_get_qve_identity"
#define QL_API_FREE_QVE_IDENTITY "sgx_ql_free_qve_identity"

#define QL_API_GET_ROOT_CA_CRL "sgx_ql_get_root_ca_crl"
#define QL_API_FREE_ROOT_CA_CRL "sgx_ql_free_root_ca_crl"

#define TDX_QL_API_GET_QUOTE_VERIFICATION_COLLATERAL "tdx_ql_get_quote_verification_collateral"
#define TDX_QL_API_FREE_QUOTE_VERIFICATION_COLLATERAL "tdx_ql_free_quote_verification_collateral"

typedef quote3_error_t(*sgx_get_quote_verification_collateral_func_t)(const char *fmspc,
        uint16_t fmspc_size,
        const char *pck_ca,
        struct _sgx_ql_qve_collateral_t **pp_quote_collateral);

typedef quote3_error_t(*sgx_free_quote_verification_collateral_func_t)(struct _sgx_ql_qve_collateral_t *p_quote_collateral);

typedef quote3_error_t(*sgx_ql_get_qve_identity_func_t)(char **pp_qve_identity,
        uint32_t *p_qve_identity_size,
        char **pp_qve_identity_issuer_chain,
        uint32_t *p_qve_identity_issuer_chain_size);

typedef quote3_error_t(*sgx_ql_free_qve_identity_func_t)(char *p_qve_identity, char *p_qve_identity_issue_chain);

typedef quote3_error_t(*sgx_ql_get_root_ca_crl_func_t)(uint8_t **pp_root_ca_crl, uint16_t *p_root_ca_cal_size);

typedef quote3_error_t(*sgx_ql_free_root_ca_crl_func_t)(uint8_t *p_root_ca_crl);

typedef quote3_error_t(*tdx_get_quote_verification_collateral_func_t)(const char *fmspc,
        uint16_t fmspc_size,
        const char *pck_ca,
        struct _sgx_ql_qve_collateral_t **pp_quote_collateral);

typedef quote3_error_t(*tdx_free_quote_verification_collateral_func_t)(struct _sgx_ql_qve_collateral_t *p_quote_collateral);


bool sgx_dcap_load_qpl();

quote3_error_t sgx_dcap_retrieve_verification_collateral(
        const char *fmspc,
        uint16_t fmspc_size,
        const char *pck_ca,
        struct _sgx_ql_qve_collateral_t **pp_quote_collateral);

quote3_error_t sgx_dcap_free_verification_collateral(struct _sgx_ql_qve_collateral_t *pp_quote_collateral);

quote3_error_t sgx_dcap_retrieve_qve_identity(
        uint8_t **pp_qveid,
        uint32_t *p_qveid_size,
        uint8_t **pp_qveid_issue_chain,
        uint32_t *p_qveid_issue_chain_size,
        uint8_t **pp_root_ca_crl,
        uint16_t *p_root_ca_crl_size);

quote3_error_t sgx_dcap_free_qve_identity(uint8_t *p_qveid,
                                          uint8_t *p_qveid_issue_chain,
                                          uint8_t *p_root_ca_crl);


quote3_error_t tdx_dcap_retrieve_verification_collateral(
        const char *fmspc,
        uint16_t fmspc_size,
        const char *pck_ca,
        struct _sgx_ql_qve_collateral_t **pp_quote_collateral);

quote3_error_t tdx_dcap_free_verification_collateral(struct _sgx_ql_qve_collateral_t *pp_quote_collateral);


#ifndef _MSC_VER
bool sgx_qv_set_qpl_path(const char* p_path);
bool sgx_qv_set_qve_path(const char* p_path);
#endif

#if defined(__cplusplus)
}
#endif

#endif /* !_SGX_DCAP_PCS_COM_H_*/
