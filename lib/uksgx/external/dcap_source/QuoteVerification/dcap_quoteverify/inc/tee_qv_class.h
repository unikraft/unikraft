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
 * File: tee_qv_class.h
 *
 * Description: Definitions of SGX and TDX QVL/QvE wrapper class
 *
 */

#ifndef _TEE_CLASS_H_
#define _TEE_CLASS_H_

#include "sgx_qve_header.h"
#include "sgx_qve_def.h"
#include "sgx_urts.h"

#if defined(__cplusplus)
extern "C" {
#endif

class tee_qv_base {
public:
    virtual quote3_error_t tee_verify_evidence(
        const uint8_t *p_quote,
        uint32_t quote_size,
        const struct _sgx_ql_qve_collateral_t *p_quote_collateral,
        const time_t expiration_check_date,
        uint32_t *p_collateral_expiration_status,
        sgx_ql_qv_result_t *p_quote_verification_result,
        sgx_ql_qe_report_info_t *p_qve_report_info,
        uint32_t supplemental_data_size,
        uint8_t *p_supplemental_data) = 0;

    virtual quote3_error_t tee_get_supplemental_data_size(uint32_t *p_data_size) = 0;

    virtual quote3_error_t tee_get_supplemental_data_version(uint32_t *p_version) = 0;

    virtual quote3_error_t tee_get_fmspc_ca_from_quote(
        const uint8_t* p_quote,
        uint32_t quote_size,
        unsigned char* p_fmsp_from_quote,
        uint32_t fmsp_from_quote_size,
        unsigned char* p_ca_from_quote,
        uint32_t ca_from_quote_size) = 0;

    virtual quote3_error_t tee_get_verification_endorsement(
        const char *fmspc,
        uint16_t fmspc_size,
        const char *pck_ca,
        struct _sgx_ql_qve_collateral_t **pp_quote_collateral) = 0;

    virtual quote3_error_t tee_free_verification_endorsement(
        struct _sgx_ql_qve_collateral_t *pp_quote_collateral) = 0;

    virtual quote3_error_t tee_get_qve_identity(
        uint8_t **pp_qveid,
        uint32_t *p_qveid_size,
        uint8_t **pp_qveid_issue_chain,
        uint32_t *p_qveid_issue_chain_size,
        uint8_t **pp_root_ca_crl,
        uint16_t *p_root_ca_crl_size) = 0;

    virtual quote3_error_t tee_free_qve_identity(
        uint8_t *p_qveid,
        uint8_t *p_qveid_issue_chain,
        uint8_t *p_root_ca_crl) = 0;

    virtual ~tee_qv_base() {};
};

class sgx_qv : public tee_qv_base {
public:
    virtual quote3_error_t tee_verify_evidence(
        const uint8_t *p_quote,
        uint32_t quote_size,
        const struct _sgx_ql_qve_collateral_t *p_quote_collateral,
        const time_t expiration_check_date,
        uint32_t *p_collateral_expiration_status,
        sgx_ql_qv_result_t *p_quote_verification_result,
        sgx_ql_qe_report_info_t *p_qve_report_info,
        uint32_t supplemental_data_size,
        uint8_t *p_supplemental_data);

    virtual quote3_error_t tee_get_supplemental_data_size(uint32_t *p_data_size);

    virtual quote3_error_t tee_get_supplemental_data_version(uint32_t *p_version);

    virtual quote3_error_t tee_get_fmspc_ca_from_quote(
        const uint8_t* p_quote,
        uint32_t quote_size,
        unsigned char* p_fmsp_from_quote,
        uint32_t fmsp_from_quote_size,
        unsigned char* p_ca_from_quote,
        uint32_t ca_from_quote_size);

    virtual quote3_error_t tee_get_verification_endorsement(
        const char *fmspc,
        uint16_t fmspc_size,
        const char *pck_ca,
        sgx_ql_qve_collateral_t **pp_quote_collateral);

    virtual quote3_error_t tee_free_verification_endorsement(
        sgx_ql_qve_collateral_t *pp_quote_collateral);

    virtual quote3_error_t tee_get_qve_identity(
        uint8_t **pp_qveid,
        uint32_t *p_qveid_size,
        uint8_t **pp_qveid_issue_chain,
        uint32_t *p_qveid_issue_chain_size,
        uint8_t **pp_root_ca_crl,
        uint16_t *p_root_ca_crl_size);

    virtual quote3_error_t tee_free_qve_identity(
        uint8_t *p_qveid,
        uint8_t *p_qveid_issue_chain,
        uint8_t *p_root_ca_crl);
};

class sgx_qv_trusted : public sgx_qv {
public:
    sgx_qv_trusted(sgx_enclave_id_t id) : m_qve_id(id) {};
    ~sgx_qv_trusted() { m_qve_id = 0; };

    virtual quote3_error_t tee_verify_evidence(
        const uint8_t *p_quote,
        uint32_t quote_size,
        const struct _sgx_ql_qve_collateral_t *p_quote_collateral,
        const time_t expiration_check_date,
        uint32_t *p_collateral_expiration_status,
        sgx_ql_qv_result_t *p_quote_verification_result,
        sgx_ql_qe_report_info_t *p_qve_report_info,
        uint32_t supplemental_data_size,
        uint8_t *p_supplemental_data);

    virtual quote3_error_t tee_get_supplemental_data_size(uint32_t *p_data_size);

    virtual quote3_error_t tee_get_supplemental_data_version(uint32_t *p_version);

    virtual quote3_error_t tee_get_fmspc_ca_from_quote(
        const uint8_t* p_quote,
        uint32_t quote_size,
        unsigned char* p_fmsp_from_quote,
        uint32_t fmsp_from_quote_size,
        unsigned char* p_ca_from_quote,
        uint32_t ca_from_quote_size);

private:
    sgx_enclave_id_t m_qve_id;
};

class tdx_qv : public sgx_qv {
public:

    virtual quote3_error_t tee_get_verification_endorsement(
        const char *fmspc,
        uint16_t fmspc_size,
        const char *pck_ca,
        struct _sgx_ql_qve_collateral_t **pp_quote_collateral);

    virtual quote3_error_t tee_free_verification_endorsement(
        struct _sgx_ql_qve_collateral_t *pp_quote_collateral);

};

class tdx_qv_trusted : public tdx_qv {
public:
    tdx_qv_trusted(sgx_enclave_id_t id) : m_qve_id(id) {};
    ~tdx_qv_trusted() { m_qve_id = 0; };

    virtual quote3_error_t tee_verify_evidence(
        const uint8_t *p_quote,
        uint32_t quote_size,
        const struct _sgx_ql_qve_collateral_t *p_quote_collateral,
        const time_t expiration_check_date,
        uint32_t *p_collateral_expiration_status,
        sgx_ql_qv_result_t *p_quote_verification_result,
        sgx_ql_qe_report_info_t *p_qve_report_info,
        uint32_t supplemental_data_size,
        uint8_t *p_supplemental_data);

    virtual quote3_error_t tee_get_supplemental_data_size(uint32_t *p_data_size);

    virtual quote3_error_t tee_get_supplemental_data_version(uint32_t *p_version);

    virtual quote3_error_t tee_get_fmspc_ca_from_quote(
        const uint8_t* p_quote,
        uint32_t quote_size,
        unsigned char* p_fmsp_from_quote,
        uint32_t fmsp_from_quote_size,
        unsigned char* p_ca_from_quote,
        uint32_t ca_from_quote_size);

private:
    sgx_enclave_id_t m_qve_id;
};


#if defined(__cplusplus)
}
#endif

#endif /* !_TEE_CLASS_H_*/
