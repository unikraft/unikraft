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
 * File: sgx_dcap_pcs_com.cpp
 *
 * Description: DCAP PCS communication APIs. Dynamically load and call quote provider APIs.
 */

#include "sgx_qve_header.h"
#include "sgx_qve_def.h"
#include "sgx_dcap_pcs_com.h"
#include <stdlib.h>
#include "se_trace.h"


sgx_get_quote_verification_collateral_func_t p_sgx_ql_get_quote_verification_collateral = NULL;
sgx_free_quote_verification_collateral_func_t p_sgx_ql_free_quote_verification_collateral = NULL;

sgx_ql_get_qve_identity_func_t p_sgx_ql_get_qve_identity = NULL;
sgx_ql_free_qve_identity_func_t p_sgx_ql_free_qve_identity = NULL;

sgx_ql_get_root_ca_crl_func_t p_sgx_ql_get_root_ca_crl = NULL;
sgx_ql_free_root_ca_crl_func_t p_sgx_ql_free_root_ca_crl = NULL;

tdx_get_quote_verification_collateral_func_t p_tdx_ql_get_quote_verification_collateral = NULL;
tdx_free_quote_verification_collateral_func_t p_tdx_ql_free_quote_verification_collateral = NULL;


/**
 * Dynamically load sgx_ql_get_quote_verification_collateral symbol and call it.
 *
 * @param fmspc[IN] - Pointer to base 16-encoded representation of FMSPC. (5 bytes).
 * @param pck_ca[IN] - Pointer to Null terminated string identifier of the PCK Cert CA that issued the PCK Certificates. Allowed values {platform, processor}.
 * @param pp_quote_collateral[OUT] - Pointer to a pointer to the PCK quote collateral data needed for quote verification.
 *                                   The provider library will allocate this buffer and it is expected that the Quote Library will free it using the provider library sgx_ql_free_quote_verification_collateral() API.
 *
 * @return Status code of the operation, one of:
 *      - SGX_QL_SUCCESS
 *      - SGX_QL_NO_QUOTE_COLLATERAL_DATA
 *      - SGX_QL_ERROR_INVALID_PARAMETER
 *      - SGX_QL_PLATFORM_LIB_UNAVAILABLE
 **/
quote3_error_t sgx_dcap_retrieve_verification_collateral(
    const char *fmspc,
    uint16_t fmspc_size,
    const char *pck_ca,
    struct _sgx_ql_qve_collateral_t **pp_quote_collateral)
{

    if (fmspc == NULL || pck_ca == NULL || pp_quote_collateral == NULL || *pp_quote_collateral != NULL) {
        return SGX_QL_ERROR_INVALID_PARAMETER;
    }

    if (!sgx_dcap_load_qpl() || !p_sgx_ql_get_quote_verification_collateral) {
        return SGX_QL_PLATFORM_LIB_UNAVAILABLE;
    }

    //call p_sgx_ql_get_quote_verification_collateral to retrieve verification collateral
    //
    return p_sgx_ql_get_quote_verification_collateral(
        fmspc,
        fmspc_size,
        pck_ca,
        pp_quote_collateral);

}

/**
 * Dynamically load sgx_ql_free_quote_verification_collateral symbol and call it.
 *
 * @param pp_quote_collateral[IN] - Pointer to the PCK certification that the sgx_ql_get_quote_verification_collateral() API has allocated.
 *
 * @return Status code of the operation, one of:
 *      - SGX_QL_SUCCESS
 *      - SGX_QL_NO_QUOTE_COLLATERAL_DATA
 *      - SGX_QL_ERROR_INVALID_PARAMETER
 *      - SGX_QL_PLATFORM_LIB_UNAVAILABLE
 **/
quote3_error_t sgx_dcap_free_verification_collateral(struct _sgx_ql_qve_collateral_t *p_quote_collateral)
{
    if (p_quote_collateral == NULL) {
        return SGX_QL_ERROR_INVALID_PARAMETER;
    }

    if (!sgx_dcap_load_qpl() || !p_sgx_ql_free_quote_verification_collateral) {
        return SGX_QL_PLATFORM_LIB_UNAVAILABLE;
    }

    //call p_sgx_ql_free_quote_verification_collateral to free allocated memory
    //
    return p_sgx_ql_free_quote_verification_collateral(p_quote_collateral);
}

/**
 * Dynamically load sgx_ql_get_qve_identity & sgx_ql_get_root_ca_crl symbol and call it.
 *
 * @param pp_qveid[OUT] - Pointer to pointer of the QvE Identity
 * @param p_qveid_size[OUT] - Pointer to the QvE Identity size
 * @param pp_qveid_issue_chain[OUT] - Pointer to pointer of the QvE Identity issue chain
 * @param p_qveid_issue_chain_size[OUT] - Pointer to the QvE Identity issue chain size
 * @param pp_root_ca_crl[OUT] - Pointer to pointer of the Intel ROOT CA CRL
 * @param p_root_ca_crl_size[OUT] - Pointer to the Intel ROOT CA CRL size
 *
 * @return Status code of the operation, one of:
 *      - SGX_QL_SUCCESS
 *      - SGX_QL_NO_QUOTE_COLLATERAL_DATA
 *      - SGX_QL_ERROR_INVALID_PARAMETER
 *      - SGX_QL_PLATFORM_LIB_UNAVAILABLE
 **/

quote3_error_t sgx_dcap_retrieve_qve_identity(
         uint8_t **pp_qveid,
         uint32_t *p_qveid_size,
         uint8_t **pp_qveid_issue_chain,
         uint32_t *p_qveid_issue_chain_size,
         uint8_t **pp_root_ca_crl,
         uint16_t *p_root_ca_crl_size)
{
    quote3_error_t ret = SGX_QL_ERROR_INVALID_PARAMETER;

    if (pp_qveid == NULL || p_qveid_size == NULL ||
            pp_qveid_issue_chain == NULL || p_qveid_issue_chain_size == NULL) {
        return SGX_QL_ERROR_INVALID_PARAMETER;
    }

    if (!sgx_dcap_load_qpl() || !p_sgx_ql_get_qve_identity) {
        return SGX_QL_PLATFORM_LIB_UNAVAILABLE;
    }

    //call sgx_ql_get_qve_identity to retrieve QvE Identity and Signing Chain
    //
    ret = p_sgx_ql_get_qve_identity(
        (char **)pp_qveid,
        p_qveid_size,
        (char **)pp_qveid_issue_chain,
        p_qveid_issue_chain_size);

    if (ret != SGX_QL_SUCCESS)
        return ret;

    //call sgx_ql_get_root_ca_crl to retrieve Intel ROOT CA CRL
    //
    ret = p_sgx_ql_get_root_ca_crl(
        pp_root_ca_crl,
        p_root_ca_crl_size);

    return ret;
}


/**
 * Dynamically load sgx_ql_free_qve_identity & sgx_ql_free_root_ca_crl symbol and call it.
 *
 * @param pp_qveid[OUT] - Pointer to pointer of the QvE Identity
 * @param p_qveid_size[OUT] - Pointer to the QvE Identity size
 * @param pp_qveid_issue_chain[OUT] - Pointer to pointer of the QvE Identity issue chain
 * @param p_qveid_issue_chain_size[OUT] - Pointer to the QvE Identity issue chain size
 * @param pp_root_ca_crl[OUT] - Pointer to pointer of the Intel ROOT CA CRL
 * @param p_root_ca_crl_size[OUT] - Pointer to the Intel ROOT CA CRL size
 *
 * @return Status code of the operation, one of:
 *      - SGX_QL_SUCCESS
 *      - SGX_QL_NO_QUOTE_COLLATERAL_DATA
 *      - SGX_QL_ERROR_INVALID_PARAMETER
 *      - SGX_QL_PLATFORM_LIB_UNAVAILABLE
 **/

quote3_error_t sgx_dcap_free_qve_identity(
         uint8_t *p_qveid,
         uint8_t *p_qveid_issue_chain,
         uint8_t *p_root_ca_crl)
{
    quote3_error_t ret = SGX_QL_ERROR_INVALID_PARAMETER;

    if (p_qveid == NULL || p_qveid_issue_chain == NULL || p_root_ca_crl == NULL) {
        return SGX_QL_ERROR_INVALID_PARAMETER;
    }

    if (!sgx_dcap_load_qpl() || !p_sgx_ql_free_qve_identity || !p_sgx_ql_free_root_ca_crl) {
        return SGX_QL_PLATFORM_LIB_UNAVAILABLE;
    }

    //call p_sgx_ql_free_qve_identity to free allocated memory
    //ignore error
    //
    ret =  p_sgx_ql_free_qve_identity((char *)p_qveid, (char *)p_qveid_issue_chain);


    //call p_sgx_ql_free_root_ca_crl to free allocated memory
    //ignore error
    //
    ret =  p_sgx_ql_free_root_ca_crl(p_root_ca_crl);

    return ret;
}

/**
 * Dynamically load tdx_ql_get_quote_verification_collateral symbol and call it.
 *
 * @param fmspc[IN] - Pointer to base 16-encoded representation of FMSPC. (5 bytes).
 * @param pck_ca[IN] - Pointer to Null terminated string identifier of the PCK Cert CA that issued the PCK Certificates. Allowed values {platform, processor}.
 * @param pp_quote_collateral[OUT] - Pointer to a pointer to the PCK quote collateral data needed for quote verification.
 *                                   The provider library will allocate this buffer and it is expected that the Quote Library will free it using the provider library sgx_ql_free_quote_verification_collateral() API.
 *
 * @return Status code of the operation, one of:
 *      - SGX_QL_SUCCESS
 *      - SGX_QL_NO_QUOTE_COLLATERAL_DATA
 *      - SGX_QL_ERROR_INVALID_PARAMETER
 *      - SGX_QL_PLATFORM_LIB_UNAVAILABLE
 **/
quote3_error_t tdx_dcap_retrieve_verification_collateral(
    const char *fmspc,
    uint16_t fmspc_size,
    const char *pck_ca,
    struct _sgx_ql_qve_collateral_t **pp_quote_collateral)
{

    if (fmspc == NULL || pck_ca == NULL || pp_quote_collateral == NULL || *pp_quote_collateral != NULL) {
        return SGX_QL_ERROR_INVALID_PARAMETER;
    }

    if (!sgx_dcap_load_qpl() || !p_tdx_ql_get_quote_verification_collateral) {
        return SGX_QL_PLATFORM_LIB_UNAVAILABLE;
    }

    //call p_sgx_ql_get_quote_verification_collateral to retrieve verification collateral
    //
    return p_tdx_ql_get_quote_verification_collateral(
        fmspc,
        fmspc_size,
        pck_ca,
        pp_quote_collateral);

}

/**
 * Dynamically load tdx_ql_free_quote_verification_collateral symbol and call it.
 *
 * @param pp_quote_collateral[IN] - Pointer to the PCK certification that the sgx_ql_get_quote_verification_collateral() API has allocated.
 *
 * @return Status code of the operation, one of:
 *      - SGX_QL_SUCCESS
 *      - SGX_QL_NO_QUOTE_COLLATERAL_DATA
 *      - SGX_QL_ERROR_INVALID_PARAMETER
 *      - SGX_QL_PLATFORM_LIB_UNAVAILABLE
 **/
quote3_error_t tdx_dcap_free_verification_collateral(struct _sgx_ql_qve_collateral_t *p_quote_collateral)
{
    if (p_quote_collateral == NULL) {
        return SGX_QL_ERROR_INVALID_PARAMETER;
    }

    if (!sgx_dcap_load_qpl() || !p_tdx_ql_free_quote_verification_collateral) {
        return SGX_QL_PLATFORM_LIB_UNAVAILABLE;
    }

    //call p_sgx_ql_free_quote_verification_collateral to free allocated memory
    //
    return p_tdx_ql_free_quote_verification_collateral(p_quote_collateral);
}
