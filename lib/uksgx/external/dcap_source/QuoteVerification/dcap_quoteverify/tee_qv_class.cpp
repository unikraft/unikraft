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
  * File: tee_qv_class.cpp
  *
  * Description: Implementation of SGX and TDX QVL/QvE wrapper class
  */

#include "tee_qv_class.h"
#include "sgx_dcap_qv_internal.h"
#include "sgx_dcap_pcs_com.h"
#include "se_trace.h"
#ifndef _MSC_VER
#include "linux/qve_u.h"
#else //_MSC_VER
#include "win/qve_u.h"
#endif //_MSC_VER

//SGX untrusted quote verification
//
quote3_error_t sgx_qv::tee_verify_evidence(
    const uint8_t *p_quote,
    uint32_t quote_size,
    const struct _sgx_ql_qve_collateral_t *p_quote_collateral,
    const time_t expiration_check_date,
    uint32_t *p_collateral_expiration_status,
    sgx_ql_qv_result_t *p_quote_verification_result,
    sgx_ql_qe_report_info_t *p_qve_report_info,
    uint32_t supplemental_data_size,
    uint8_t *p_supplemental_data) {

    return sgx_qvl_verify_quote(
        p_quote,
        quote_size,
        p_quote_collateral,
        expiration_check_date,
        p_collateral_expiration_status,
        p_quote_verification_result,
        p_qve_report_info,
        supplemental_data_size,
        p_supplemental_data);
}

quote3_error_t sgx_qv::tee_get_supplemental_data_size(uint32_t *p_data_size)
{
    return sgx_qvl_get_quote_supplemental_data_size(p_data_size);
}

quote3_error_t sgx_qv::tee_get_supplemental_data_version(uint32_t *p_version)
{
    return sgx_qvl_get_quote_supplemental_data_version(p_version);
}

quote3_error_t sgx_qv::tee_get_fmspc_ca_from_quote(
    const uint8_t* p_quote,
    uint32_t quote_size,
    unsigned char* p_fmsp_from_quote,
    uint32_t fmsp_from_quote_size,
    unsigned char* p_ca_from_quote,
    uint32_t ca_from_quote_size) {

    return qvl_get_fmspc_ca_from_quote(
        p_quote,
        quote_size,
        p_fmsp_from_quote,
        fmsp_from_quote_size,
        p_ca_from_quote,
        ca_from_quote_size);
}

quote3_error_t sgx_qv::tee_get_verification_endorsement(
        const char *fmspc,
        uint16_t fmspc_size,
        const char *pck_ca,
        sgx_ql_qve_collateral_t **pp_quote_collateral) {

    return sgx_dcap_retrieve_verification_collateral(
        fmspc,
        fmspc_size,
        pck_ca,
        pp_quote_collateral);
}

quote3_error_t sgx_qv::tee_free_verification_endorsement(
    sgx_ql_qve_collateral_t *p_quote_collateral) {

    return sgx_dcap_free_verification_collateral(p_quote_collateral);
}

quote3_error_t sgx_qv::tee_get_qve_identity(
    uint8_t **pp_qveid,
    uint32_t *p_qveid_size,
    uint8_t **pp_qveid_issue_chain,
    uint32_t *p_qveid_issue_chain_size,
    uint8_t **pp_root_ca_crl,
    uint16_t *p_root_ca_crl_size) {

    return sgx_dcap_retrieve_qve_identity(
        pp_qveid,
        p_qveid_size,
        pp_qveid_issue_chain,
        p_qveid_issue_chain_size,
        pp_root_ca_crl,
        p_root_ca_crl_size);
}

quote3_error_t sgx_qv::tee_free_qve_identity(
    uint8_t *p_qveid,
    uint8_t *p_qveid_issue_chain,
    uint8_t *p_root_ca_crl) {

    return sgx_dcap_free_qve_identity(
        p_qveid,
        p_qveid_issue_chain,
        p_root_ca_crl);
}

//SGX trusted quote verification
//
quote3_error_t sgx_qv_trusted::tee_verify_evidence(
    const uint8_t *p_quote,
    uint32_t quote_size,
    const struct _sgx_ql_qve_collateral_t *p_quote_collateral,
    const time_t expiration_check_date,
    uint32_t *p_collateral_expiration_status,
    sgx_ql_qv_result_t *p_quote_verification_result,
    sgx_ql_qe_report_info_t *p_qve_report_info,
    uint32_t supplemental_data_size,
    uint8_t *p_supplemental_data) {

    sgx_status_t ret = SGX_ERROR_UNEXPECTED;
    quote3_error_t qv_ret = SGX_QL_ERROR_UNEXPECTED;

    if (m_qve_id == 0)
        return SGX_QL_ERROR_UNEXPECTED;

    ret = sgx_qve_verify_quote(
        m_qve_id,
        &qv_ret,
        p_quote,
        quote_size,
        p_quote_collateral,
        expiration_check_date,
        p_collateral_expiration_status,
        p_quote_verification_result,
        p_qve_report_info,
        supplemental_data_size,
        p_supplemental_data);

    if (qv_ret == SGX_QL_SUCCESS && ret == SGX_SUCCESS) {
        SE_TRACE(SE_TRACE_DEBUG, "Info: sgx_qve_verify_quote successfully returned.\n");
    }
    else {
        SE_TRACE(SE_TRACE_DEBUG, "Error: sgx_qve_verify_quote failed. ecall return 0x%04x, \
            function return 0x%04x\n", ret, qv_ret);
    }

    return qv_ret;
}

quote3_error_t sgx_qv_trusted::tee_get_supplemental_data_size(uint32_t *p_data_size)
{
    sgx_status_t ret = SGX_ERROR_UNEXPECTED;
    quote3_error_t qv_ret = SGX_QL_ERROR_UNEXPECTED;

    if (m_qve_id == 0)
        return SGX_QL_ERROR_UNEXPECTED;

    ret = sgx_qve_get_quote_supplemental_data_size(
        m_qve_id,
        &qv_ret,
        p_data_size);

    if (qv_ret == SGX_QL_SUCCESS && ret == SGX_SUCCESS) {
        SE_TRACE(SE_TRACE_DEBUG, "Info: sgx_qve_get_quote_supplemental_data_size successfully returned.\n");
    }
    else {
        SE_TRACE(SE_TRACE_DEBUG, "Error: sgx_qve_get_quote_supplemental_data_size failed. ecall return 0x%04x, \
            function return 0x%04x\n", ret, qv_ret);
    }

    return qv_ret;
}

quote3_error_t sgx_qv_trusted::tee_get_supplemental_data_version(uint32_t *p_version)
{
    sgx_status_t ret = SGX_ERROR_UNEXPECTED;
    quote3_error_t qv_ret = SGX_QL_ERROR_UNEXPECTED;

    if (m_qve_id == 0)
        return SGX_QL_ERROR_UNEXPECTED;

    ret = sgx_qve_get_quote_supplemental_data_version(
        m_qve_id,
        &qv_ret,
        p_version);

    if (qv_ret == SGX_QL_SUCCESS && ret == SGX_SUCCESS) {
        SE_TRACE(SE_TRACE_DEBUG, "Info: sgx_qve_get_quote_supplemental_data_version successfully returned.\n");
    }
    else {
        SE_TRACE(SE_TRACE_DEBUG, "Error: sgx_qve_get_quote_supplemental_data_version failed. ecall return 0x%04x, \
            function return 0x%04x\n", ret, qv_ret);
    }

    return qv_ret;
}

quote3_error_t sgx_qv_trusted::tee_get_fmspc_ca_from_quote(
    const uint8_t* p_quote,
    uint32_t quote_size,
    unsigned char* p_fmsp_from_quote,
    uint32_t fmsp_from_quote_size,
    unsigned char* p_ca_from_quote,
    uint32_t ca_from_quote_size) {

    sgx_status_t ret = SGX_ERROR_UNEXPECTED;
    quote3_error_t qv_ret = SGX_QL_ERROR_UNEXPECTED;

    if (m_qve_id == 0)
        return SGX_QL_ERROR_UNEXPECTED;

    ret = get_fmspc_ca_from_quote(
        m_qve_id,
        &qv_ret,
        p_quote,
        quote_size,
        p_fmsp_from_quote,
        fmsp_from_quote_size,
        p_ca_from_quote,
        ca_from_quote_size);

    if (qv_ret == SGX_QL_SUCCESS && ret == SGX_SUCCESS) {
        SE_TRACE(SE_TRACE_DEBUG, "Info: get_fmspc_ca_from_quote successfully returned.\n");
    }
    else {
        SE_TRACE(SE_TRACE_DEBUG, "Error: get_fmspc_ca_from_quote failed. ecall return 0x%04x, \
            function return 0x%04x\n", ret, qv_ret);
    }

    return qv_ret;
}

quote3_error_t tdx_qv::tee_get_verification_endorsement(
        const char *fmspc,
        uint16_t fmspc_size,
        const char *pck_ca,
        struct _sgx_ql_qve_collateral_t **pp_quote_collateral) {

    return tdx_dcap_retrieve_verification_collateral(
        fmspc,
        fmspc_size,
        pck_ca,
        pp_quote_collateral);
}

quote3_error_t tdx_qv::tee_free_verification_endorsement(
    sgx_ql_qve_collateral_t *p_quote_collateral) {

    return tdx_dcap_free_verification_collateral(p_quote_collateral);
}

//TDX trusted quote verification
//
quote3_error_t tdx_qv_trusted::tee_verify_evidence(
    const uint8_t *p_quote,
    uint32_t quote_size,
    const struct _sgx_ql_qve_collateral_t *p_quote_collateral,
    const time_t expiration_check_date,
    uint32_t *p_collateral_expiration_status,
    sgx_ql_qv_result_t *p_quote_verification_result,
    sgx_ql_qe_report_info_t *p_qve_report_info,
    uint32_t supplemental_data_size,
    uint8_t *p_supplemental_data) {

    sgx_status_t ret = SGX_ERROR_UNEXPECTED;
    quote3_error_t qv_ret = SGX_QL_ERROR_UNEXPECTED;

    if (m_qve_id == 0)
        return SGX_QL_ERROR_UNEXPECTED;

    ret = sgx_qve_verify_quote(
        m_qve_id,
        &qv_ret,
        p_quote,
        quote_size,
        (const tdx_ql_qve_collateral_t*) p_quote_collateral,
        expiration_check_date,
        p_collateral_expiration_status,
        (sgx_ql_qv_result_t *)p_quote_verification_result,
        p_qve_report_info,
        supplemental_data_size,
        p_supplemental_data);

    if (qv_ret == SGX_QL_SUCCESS && ret == SGX_SUCCESS) {
        SE_TRACE(SE_TRACE_DEBUG, "Info: tdx_qve_verify_quote successfully returned.\n");
    }
    else {
        SE_TRACE(SE_TRACE_DEBUG, "Error: tdx_qve_verify_quote failed. ecall return 0x%04x, \
            function return 0x%04x\n", ret, qv_ret);
    }

    return qv_ret;
}

quote3_error_t tdx_qv_trusted::tee_get_supplemental_data_size(uint32_t *p_data_size)
{
    sgx_status_t ret = SGX_ERROR_UNEXPECTED;
    quote3_error_t qv_ret = SGX_QL_ERROR_UNEXPECTED;

    if (m_qve_id == 0)
        return SGX_QL_ERROR_UNEXPECTED;

    ret = sgx_qve_get_quote_supplemental_data_size(
        m_qve_id,
        &qv_ret,
        p_data_size);

    if (qv_ret == SGX_QL_SUCCESS && ret == SGX_SUCCESS) {
        SE_TRACE(SE_TRACE_DEBUG, "Info: tdx_qve_get_quote_supplemental_data_size successfully returned.\n");
    }
    else {
        SE_TRACE(SE_TRACE_DEBUG, "Error: tdx_qve_get_quote_supplemental_data_size failed. ecall return 0x%04x, \
            function return 0x%04x\n", ret, qv_ret);
    }

    return qv_ret;
}

quote3_error_t tdx_qv_trusted::tee_get_supplemental_data_version(uint32_t *p_version)
{
    sgx_status_t ret = SGX_ERROR_UNEXPECTED;
    quote3_error_t qv_ret = SGX_QL_ERROR_UNEXPECTED;

    if (m_qve_id == 0)
        return SGX_QL_ERROR_UNEXPECTED;

    ret = sgx_qve_get_quote_supplemental_data_version(
        m_qve_id,
        &qv_ret,
        p_version);

    if (qv_ret == SGX_QL_SUCCESS && ret == SGX_SUCCESS) {
        SE_TRACE(SE_TRACE_DEBUG, "Info: tdx_qve_get_quote_supplemental_data_version successfully returned.\n");
    }
    else {
        SE_TRACE(SE_TRACE_DEBUG, "Error: tdx_qve_get_quote_supplemental_data_version failed. ecall return 0x%04x, \
            function return 0x%04x\n", ret, qv_ret);
    }

    return qv_ret;
}

quote3_error_t tdx_qv_trusted::tee_get_fmspc_ca_from_quote(
    const uint8_t* p_quote,
    uint32_t quote_size,
    unsigned char* p_fmsp_from_quote,
    uint32_t fmsp_from_quote_size,
    unsigned char* p_ca_from_quote,
    uint32_t ca_from_quote_size) {

    sgx_status_t ret = SGX_ERROR_UNEXPECTED;
    quote3_error_t qv_ret = SGX_QL_ERROR_UNEXPECTED;

    if (m_qve_id == 0)
        return SGX_QL_ERROR_UNEXPECTED;

    ret = get_fmspc_ca_from_quote(
        m_qve_id,
        &qv_ret,
        p_quote,
        quote_size,
        p_fmsp_from_quote,
        fmsp_from_quote_size,
        p_ca_from_quote,
        ca_from_quote_size);

    if (qv_ret == SGX_QL_SUCCESS && ret == SGX_SUCCESS) {
        SE_TRACE(SE_TRACE_DEBUG, "Info: tdx_get_fmspc_ca_from_quote successfully returned.\n");
    }
    else {
        SE_TRACE(SE_TRACE_DEBUG, "Error: tdx_get_fmspc_ca_from_quote failed. ecall return 0x%04x, \
            function return 0x%04x\n", ret, qv_ret);
    }

    return qv_ret;
}
