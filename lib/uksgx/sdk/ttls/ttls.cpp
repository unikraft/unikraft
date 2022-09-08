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

#include "sgx_ttls.h"
#include "sgx_utils.h"
#include "sgx_tcrypto.h"
#include "sgx_quote_3.h"
#include "cert_header.h"
#include "sgx_dcap_tvl.h"
#include <string.h>
#include <sgx_trts.h>
#include "sgx_ttls_t.h"

static const char* oid_sgx_quote = X509_OID_FOR_QUOTE_STRING;

//The ISVSVN threshold of Intel signed QvE
const sgx_isv_svn_t qve_isvsvn_threshold = 6;

extern "C" quote3_error_t SGXAPI tee_get_certificate_with_evidence(
    const unsigned char *p_subject_name,
    const uint8_t *p_prv_key,
    size_t private_key_size,
    const uint8_t *p_pub_key,
    size_t public_key_size,
    uint8_t **pp_output_cert,
    size_t *p_output_cert_size)
{
    sgx_status_t ret = SGX_ERROR_UNEXPECTED;
    quote3_error_t func_ret = SGX_QL_ERROR_UNEXPECTED;
    sgx_report_t app_report;
    sgx_target_info_t target_info;
    uint8_t *p_quote = NULL;
    uint32_t quote_size = 0;
    sgx_sha_state_handle_t sha_handle = NULL;
    sgx_report_data_t report_data = { 0 };

    if (p_subject_name == NULL ||
        p_prv_key == NULL || private_key_size <= 0 ||
        p_pub_key == NULL || public_key_size <= 0 ||
        pp_output_cert == NULL || p_output_cert_size == NULL)
        return SGX_QL_ERROR_INVALID_PARAMETER;

    // only support PEM format key
    if (strnlen(reinterpret_cast<const char*>(p_pub_key), public_key_size) != public_key_size - 1 ||
        strnlen(reinterpret_cast<const char*>(p_prv_key), private_key_size) != private_key_size -1)
        return SGX_QL_ERROR_INVALID_PARAMETER;

    do {
        //OCALL to get target info of QE
        ret = sgx_tls_get_qe_target_info_ocall(&func_ret, &target_info, sizeof(sgx_target_info_t));
        if (ret != SGX_SUCCESS) {
            func_ret = SGX_QL_ERROR_UNEXPECTED;
            break;
        }
        if (func_ret != SGX_QL_SUCCESS)
            break;

        //Use user provided input as report data
        //report data = sha256(public key) || 0s
        ret = sgx_sha256_init(&sha_handle);
        if (ret != SGX_SUCCESS) {
            func_ret = SGX_QL_ERROR_UNEXPECTED;
            break;
        }

        ret = sgx_sha256_update(p_pub_key, (uint32_t)public_key_size, sha_handle);
        if (ret != SGX_SUCCESS) {
            func_ret = SGX_QL_ERROR_UNEXPECTED;
            break;
        }

        ret = sgx_sha256_get_hash(sha_handle, reinterpret_cast<sgx_sha256_hash_t *>(&report_data));
        if (ret != SGX_SUCCESS) {
            func_ret = SGX_QL_ERROR_UNEXPECTED;
            break;
        }

        //generate report with returned QE target info
        ret = sgx_create_report(&target_info, &report_data, &app_report);
        if (ret != SGX_SUCCESS) {
            func_ret = SGX_QL_ERROR_UNEXPECTED;
            break;
        }

        //OCALL to get quote size
        ret = sgx_tls_get_quote_size_ocall(&func_ret, &quote_size);
        if (ret != SGX_SUCCESS) {
            func_ret = SGX_QL_ERROR_UNEXPECTED;
            break;
        }
        if (func_ret != SGX_QL_SUCCESS)
            break;

        p_quote = (uint8_t *) malloc (quote_size);
        if (p_quote == NULL) {
            func_ret = SGX_QL_ERROR_OUT_OF_MEMORY;
            break;
        }
        memset (p_quote, 0, quote_size);

        //OCALL to get quote
        ret = sgx_tls_get_quote_ocall(&func_ret, &app_report, sizeof(sgx_report_t), p_quote, quote_size);
        if (ret != SGX_SUCCESS) {
            func_ret = SGX_QL_ERROR_UNEXPECTED;
            break;
        }
        if (func_ret != SGX_QL_SUCCESS)
            break;

        //Generate self-signed X.509 certiciate
        //Make SGX quote as an extension
        ret = generate_x509_self_signed_certificate(
            (const unsigned char*) oid_sgx_quote,
            strlen(oid_sgx_quote),
            p_subject_name,
            p_prv_key,
            private_key_size,
            p_pub_key,
            public_key_size,
            p_quote,
            quote_size,
            pp_output_cert,
            p_output_cert_size);
        if (ret != SGX_SUCCESS) {
            func_ret = SGX_QL_ERROR_UNEXPECTED;
            break;
        }

        func_ret = SGX_QL_SUCCESS;

    } while (0);

    SGX_TLS_SAFE_FREE(p_quote);

    if (sha_handle)
        sgx_sha256_close(sha_handle);

    return func_ret;
}

extern "C" quote3_error_t tee_free_certificate(uint8_t* p_certificate)
{
    SGX_TLS_SAFE_FREE(p_certificate);
    return SGX_QL_SUCCESS;
}

extern "C" quote3_error_t tee_verify_certificate_with_evidence(
    const uint8_t *p_cert_in_der,
    size_t cert_in_der_len,
    const time_t expiration_check_date,
    sgx_ql_qv_result_t *p_qv_result,
    uint8_t **pp_supplemental_data,
    uint32_t *p_supplemental_data_size)
{
    sgx_status_t ret = SGX_ERROR_UNEXPECTED;
    quote3_error_t func_ret = SGX_QL_ERROR_UNEXPECTED;
    uint8_t *p_quote = NULL;
    uint32_t quote_size = 0;
    sgx_ql_qe_report_info_t qve_report_info;
    uint32_t collateral_expiration_status = 0;

    sgx_cert_t cert = {0};
    uint8_t *pub_key_buff = NULL;
    size_t pub_key_buff_size = KEY_BUFF_SIZE;
    sgx_quote3_t *p_sgx_quote = NULL;
    sgx_report_data_t *p_report_data = NULL;
    sgx_report_data_t cert_pub_hash;
    sgx_sha_state_handle_t sha_handle = NULL;


    memset(&cert_pub_hash, 0, sizeof(sgx_report_data_t));

    if (p_cert_in_der == NULL ||
        p_qv_result == NULL ||
        pp_supplemental_data == NULL ||
        p_supplemental_data_size == NULL)
        return SGX_QL_ERROR_INVALID_PARAMETER;


    do {
        //verify X.509 certificate
        pub_key_buff = (uint8_t*)malloc(KEY_BUFF_SIZE);
        if (!pub_key_buff) {
            func_ret = SGX_QL_OUT_OF_EPC;
            break;
        }
        memset(pub_key_buff, 0, KEY_BUFF_SIZE);

        try {
            ret = sgx_read_cert_in_der(&cert, p_cert_in_der, cert_in_der_len);
            if (ret != SGX_SUCCESS)
                break;

            // validate the certificate signature
            ret = sgx_cert_verify(&cert, NULL, NULL, 0);
            if (ret != SGX_SUCCESS)
                break;

            // try to get quote from cert extension
            if (sgx_cert_find_extension(
                &cert,
                oid_sgx_quote,
                NULL,
                &quote_size) == SGX_ERROR_INVALID_PARAMETER)
            {
                p_quote = (uint8_t*)malloc(quote_size);
                if (!p_quote) {
                    func_ret = SGX_QL_ERROR_OUT_OF_MEMORY;
                    break;
                }
            }

            if (sgx_cert_find_extension(
                &cert,
                oid_sgx_quote,
                p_quote,
                &quote_size) != SGX_SUCCESS)
            {
                func_ret = SGX_QL_ERROR_UNEXPECTED;
                break;
            }
        }

        catch (...) {
            func_ret = SGX_QL_ERROR_UNEXPECTED;
            break;
        }

        //OCALL to get supplemental data size
        ret = sgx_tls_get_supplemental_data_size_ocall(&func_ret, p_supplemental_data_size);
        if (ret != SGX_SUCCESS) {
            func_ret = SGX_QL_ERROR_UNEXPECTED;
            break;
        }
        if (func_ret != SGX_QL_SUCCESS)
            break;

        *pp_supplemental_data = (uint8_t *) malloc (*p_supplemental_data_size);
        if (*pp_supplemental_data == NULL) {
            func_ret = SGX_QL_OUT_OF_EPC;
            break;
        }

        ret = sgx_read_rand(reinterpret_cast<unsigned char *> (&qve_report_info.nonce), sizeof(sgx_quote_nonce_t));
        if (ret != SGX_SUCCESS) {
            func_ret = SGX_QL_ERROR_UNEXPECTED;
            break;
        }

        ret = sgx_self_target(&qve_report_info.app_enclave_target_info);
        if (ret != SGX_SUCCESS) {
            func_ret = SGX_QL_ERROR_UNEXPECTED;
            break;
        }

        //OCALL to verify SGX quote
        ret = sgx_tls_verify_quote_ocall(
            &func_ret,
            p_quote,
            quote_size,
            expiration_check_date,
            p_qv_result,
            &qve_report_info,
            sizeof(sgx_ql_qe_report_info_t),
            *pp_supplemental_data,
            *p_supplemental_data_size);

        if (ret != SGX_SUCCESS) {
            func_ret = SGX_QL_ERROR_UNEXPECTED;
            break;
        }
        if (func_ret != SGX_QL_SUCCESS)
            break;

        //call TVL API to verify the idenity of Intel signed QvE
        func_ret = sgx_tvl_verify_qve_report_and_identity(
            p_quote,
            quote_size,
            &qve_report_info,
            expiration_check_date,
            collateral_expiration_status,
            *p_qv_result,
            *pp_supplemental_data,
            *p_supplemental_data_size,
            qve_isvsvn_threshold);

        if (func_ret != SGX_QL_SUCCESS)
            break;

        // extract public key from cert
        ret = sgx_get_pubkey_from_cert(&cert, pub_key_buff, &pub_key_buff_size);
        if (ret != SGX_SUCCESS) {
            func_ret = SGX_QL_ERROR_UNEXPECTED;
            break;
        }

        // get hash of cert pub key
        ret = sgx_sha256_init(&sha_handle);
        if (ret != SGX_SUCCESS) {
            func_ret = SGX_QL_ERROR_UNEXPECTED;
            break;
        }

        // public key
        ret = sgx_sha256_update(pub_key_buff, (uint32_t)pub_key_buff_size, sha_handle);
        if (ret != SGX_SUCCESS) {
            func_ret = SGX_QL_ERROR_UNEXPECTED;
            break;
        }

        ret = sgx_sha256_get_hash(sha_handle, reinterpret_cast<sgx_sha256_hash_t *>(&cert_pub_hash));
        if (ret != SGX_SUCCESS) {
            func_ret = SGX_QL_ERROR_UNEXPECTED;
            break;
        }

        // get report data from quote
        p_sgx_quote = (sgx_quote3_t *) p_quote;

        if (p_sgx_quote != NULL) {
            p_report_data = &(p_sgx_quote->report_body.report_data);
        }
        else {
            func_ret = SGX_QL_ERROR_UNEXPECTED;
            break;
        }

        // compare hash, only compare the first 32 bytes
        if (memcmp(p_report_data, &cert_pub_hash, SGX_HASH_SIZE) != 0) {
            func_ret = SGX_QL_ERROR_PUB_KEY_ID_MISMATCH;
            break;
        }

    } while(0);

    SGX_TLS_SAFE_FREE(pub_key_buff);
    SGX_TLS_SAFE_FREE(p_quote);
    if (func_ret != SGX_QL_SUCCESS)
        SGX_TLS_SAFE_FREE(*pp_supplemental_data);

    if (sha_handle)
        sgx_sha256_close(sha_handle);

    return func_ret;
}

quote3_error_t tee_free_supplemental_data(uint8_t* p_supplemental_data)
{
    SGX_TLS_SAFE_FREE(p_supplemental_data);
    return SGX_QL_SUCCESS;
}
