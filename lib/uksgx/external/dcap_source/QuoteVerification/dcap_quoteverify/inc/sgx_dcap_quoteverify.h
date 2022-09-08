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
 * File: sgx_dcap_quoteverify.h
 *
 * Description: Definitions and prototypes for Intel(R) SGX/TDX DCAP Quote Verification Library
 *
 */

#ifndef _SGX_DCAP_QV_H_
#define _SGX_DCAP_QV_H_

#include "sgx_qve_header.h"
#include "sgx_ql_quote.h"

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * When the Quoting Verification Library is linked to a process, it needs to know the proper enclave loading policy.
 * The library may be linked with a long lived process, such as a service, where it can load the enclaves and leave
 * them loaded (persistent). This better ensures that the enclaves will be available upon quote requests and not subject
 * to EPC limitations if loaded on demand. However, if the Quoting library is linked with an application process, there
 * may be many applications with the Quoting library and a better utilization of EPC is to load and unloaded the quoting
 * enclaves on demand (ephemeral).  The library will be shipped with a default policy of loading enclaves and leaving
 * them loaded until the library is unloaded (PERSISTENT). If the policy is set to EPHEMERAL, then the QE and PCE will
 * be loaded and unloaded on-demand.  If either enclave is already loaded when the policy is change to EPHEMERAL, the
 * enclaves will be unloaded before returning.
 *
 * @param policy Sets the requested enclave loading policy to either SGX_QL_PERSISTENT, SGX_QL_EPHEMERAL or SGX_QL_DEFAULT.
 *
 * @return SGX_QL_SUCCESS Successfully set the enclave loading policy for the quoting library's enclaves.
 * @return SGX_QL_UNSUPPORTED_LOADING_POLICY The selected policy is not support by the quoting library.
 * @return SGX_QL_ERROR_UNEXPECTED Unexpected internal error.
 *
 **/
quote3_error_t sgx_qv_set_enclave_load_policy(sgx_ql_request_policy_t policy);


/**
 * Get supplemental data required size.
 * @param p_data_size[OUT] - Pointer to hold the size of the buffer in bytes required to contain all of the supplemental data.
 *
 * @return Status code of the operation, one of:
 *      - SGX_QL_SUCCESS
 *      - SGX_QL_ERROR_INVALID_PARAMETER
 *      - SGX_QL_ERROR_QVL_QVE_MISMATCH
 *      - SGX_QL_ENCLAVE_LOAD_ERROR
 **/
quote3_error_t sgx_qv_get_quote_supplemental_data_size(uint32_t *p_data_size);


/**
 * Perform ECDSA quote verification.
 *
 * @param p_quote[IN] - Pointer to SGX Quote.
 * @param quote_size[IN] - Size of the buffer pointed to by p_quote (in bytes).
 * @param p_quote_collateral[IN] - This is a pointer to the Quote Certification Collateral provided by the caller.
 * @param expiration_check_date[IN] - This is the date that the QvE will use to determine if any of the inputted collateral have expired.
 * @param p_collateral_expiration_status[OUT] - Address of the outputted expiration status.  This input must not be NULL.
 * @param p_quote_verification_result[OUT] - Address of the outputted quote verification result.
 * @param p_qve_report_info[IN/OUT] - This parameter can be used in 2 ways.
 *        If p_qve_report_info is NOT NULL, the API will use Intel QvE to perform quote verification, and QvE will generate a report using the target_info in sgx_ql_qe_report_info_t structure.
 *        if p_qve_report_info is NULL, the API will use QVL library to perform quote verification, note that the results can not be cryptographically authenticated in this mode.
 * @param supplemental_data_size[IN] - Size of the buffer pointed to by p_quote (in bytes).
 * @param p_supplemental_data[OUT] - The parameter is optional.  If it is NULL, supplemental_data_size must be 0.
 *
 * @return Status code of the operation, one of:
 *      - SGX_QL_SUCCESS
 *      - SGX_QL_ERROR_INVALID_PARAMETER
 *      - SGX_QL_QUOTE_FORMAT_UNSUPPORTED
 *      - SGX_QL_QUOTE_CERTIFICATION_DATA_UNSUPPORTED
 *      - SGX_QL_UNABLE_TO_GENERATE_REPORT
 *      - SGX_QL_CRL_UNSUPPORTED_FORMAT
 *      - SGX_QL_ERROR_UNEXPECTED
 **/
quote3_error_t sgx_qv_verify_quote(
    const uint8_t *p_quote,
    uint32_t quote_size,
    const sgx_ql_qve_collateral_t *p_quote_collateral,
    const time_t expiration_check_date,
    uint32_t *p_collateral_expiration_status,
    sgx_ql_qv_result_t *p_quote_verification_result,
    sgx_ql_qe_report_info_t *p_qve_report_info,
    uint32_t supplemental_data_size,
    uint8_t *p_supplemental_data);



/**
 * Call quote provider library to get QvE identity.
 *
 * @param pp_qveid[OUT] - Pointer to the pointer of QvE identity
 * @param p_qveid_size[OUT] -  Pointer to the size of QvE identity
 * @param pp_qveid_issue_chain[OUT] - Pointer to the pointer QvE identity certificate chain
 * @param p_qveid_issue_chain_size[OUT] - Pointer to the QvE identity certificate chain size
 * @param pp_root_ca_crl[OUT] - Pointer to the pointer of Intel Root CA CRL
 * @param p_root_ca_crl_size[OUT] - Pointer to the Intel Root CA CRL size
 *
 * @return Status code of the operation, one of:
 *      - SGX_QL_SUCCESS
 *      - SGX_QL_ERROR_INVALID_PARAMETER
 *      - SGX_QL_NO_QVE_IDENTITY_DATA
 *      - SGX_QL_ERROR_OUT_OF_MEMORY
 *      - SGX_QL_NETWORK_ERROR
 *      - SGX_QL_MESSAGE_ERROR
 *      - SGX_QL_ERROR_UNEXPECTED
 **/
quote3_error_t sgx_qv_get_qve_identity(
        uint8_t **pp_qveid,
        uint32_t *p_qveid_size,
        uint8_t **pp_qveid_issue_chain,
        uint32_t *p_qveid_issue_chain_size,
        uint8_t **pp_root_ca_crl,
        uint16_t *p_root_ca_crl_size);

/**
 * Call quote provider library to free the p_qve_id, p_qveid_issuer_chain buffer and p_root_ca_crl allocated by sgx_qv_get_qve_identity
 **/
quote3_error_t sgx_qv_free_qve_identity(uint8_t *p_qveid,
                                        uint8_t *p_qveid_issue_chain,
                                        uint8_t *p_root_ca_crl);


#ifndef _MSC_VER
typedef enum
{
    SGX_QV_QVE_PATH,
    SGX_QV_QPL_PATH
} sgx_qv_path_type_t;

quote3_error_t sgx_qv_set_path(sgx_qv_path_type_t path_type,
                                   const char *p_path);
#endif



/**
 * Get TDX supplemental data required size.
 * @param p_data_size[OUT] - Pointer to hold the size of the buffer in bytes required to contain all of the supplemental data.
 *
 * @return Status code of the operation, one of:
 *      - SGX_QL_SUCCESS
 *      - SGX_QL_ERROR_INVALID_PARAMETER
 *      - SGX_QL_ERROR_QVL_QVE_MISMATCH
 *      - SGX_QL_ENCLAVE_LOAD_ERROR
 **/
quote3_error_t tdx_qv_get_quote_supplemental_data_size(uint32_t *p_data_size);

/**
 * Perform TDX ECDSA quote verification.
 *
 * @param p_quote[IN] - Pointer to TDX Quote.
 * @param quote_size[IN] - Size of the buffer pointed to by p_quote (in bytes).
 * @param p_quote_collateral[IN] - This is a pointer to the Quote Certification Collateral provided by the caller.
 * @param expiration_check_date[IN] - This is the date that the QvE will use to determine if any of the inputted collateral have expired.
 * @param p_collateral_expiration_status[OUT] - Address of the outputted expiration status.  This input must not be NULL.
 * @param p_quote_verification_result[OUT] - Address of the outputted quote verification result.
 * @param p_qve_report_info[IN/OUT] - This parameter can be used in 2 ways.
 *        If p_qve_report_info is NOT NULL, the API will use Intel QvE to perform quote verification, and QvE will generate a report using the target_info in sgx_ql_qe_report_info_t structure.
 *        if p_qve_report_info is NULL, the API will use QVL library to perform quote verification, not that the results can not be cryptographically authenticated in this mode.
 * @param supplemental_data_size[IN] - Size of the buffer pointed to by p_quote (in bytes).
 * @param p_supplemental_data[OUT] - The parameter is optional.  If it is NULL, supplemental_data_size must be 0.
 *
 * @return Status code of the operation, one of:
 *      - SGX_QL_SUCCESS
 *      - SGX_QL_ERROR_INVALID_PARAMETER
 *      - SGX_QL_QUOTE_FORMAT_UNSUPPORTED
 *      - SGX_QL_QUOTE_CERTIFICATION_DATA_UNSUPPORTED
 *      - SGX_QL_UNABLE_TO_GENERATE_REPORT
 *      - SGX_QL_CRL_UNSUPPORTED_FORMAT
 *      - SGX_QL_ERROR_UNEXPECTED
 **/
quote3_error_t tdx_qv_verify_quote(
    const uint8_t *p_quote,
    uint32_t quote_size,
    const tdx_ql_qve_collateral_t *p_quote_collateral,
    const time_t expiration_check_date,
    uint32_t *p_collateral_expiration_status,
    sgx_ql_qv_result_t *p_quote_verification_result,
    sgx_ql_qe_report_info_t *p_qve_report_info,
    uint32_t supplemental_data_size,
    uint8_t *p_supplemental_data);


#if defined(__cplusplus)
}
#endif

#endif /* !_SGX_DCAP_QV_H_*/
