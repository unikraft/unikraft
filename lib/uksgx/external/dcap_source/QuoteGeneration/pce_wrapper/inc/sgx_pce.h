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
 * File: sgx_pce.h
 * Description: Definition for pce interface.
 *
 * PCE interface and supporting structure definitions.
 */
#ifndef _SGX_PCE_H_
#define _SGX_PCE_H_

#include "sgx_key.h"
#include "sgx_report.h"

#define SGX_PCE_MK_ERROR(x)          (0x0000F000|(x))
typedef enum _sgx_pce_error_t
{
    SGX_PCE_SUCCESS                  = SGX_PCE_MK_ERROR(0x0000),
    SGX_PCE_UNEXPECTED               = SGX_PCE_MK_ERROR(0x0001),      /* Unexpected error */
    SGX_PCE_INVALID_PARAMETER        = SGX_PCE_MK_ERROR(0x0002),      /* The parameter is incorrect */
    SGX_PCE_OUT_OF_EPC               = SGX_PCE_MK_ERROR(0x0003),      /* Not enough memory is available to complete this operation */
    SGX_PCE_INTERFACE_UNAVAILABLE    = SGX_PCE_MK_ERROR(0x0004),      /* SGX API is unavailable */
    SGX_PCE_INVALID_REPORT           = SGX_PCE_MK_ERROR(0x0005),      /* the report cannot be verified */
    SGX_PCE_CRYPTO_ERROR             = SGX_PCE_MK_ERROR(0x0006),      /* Cannot decrypt or verify ciphertext */
    SGX_PCE_INVALID_PRIVILEGE        = SGX_PCE_MK_ERROR(0x0007),      /* Not enough privilege to perform the operation */
    SGX_PCE_INVALID_TCB              = SGX_PCE_MK_ERROR(0x0008),      /* PCE could not sign at the requested TCB */
} sgx_pce_error_t;


/* PCE ID for the PCE in this library */
#define PCE_ID 0

/* Crypto_suite */
#define PCE_ALG_RSA_OAEP_3072 1

/* Signature_scheme */
#define PCE_NIST_P256_ECDSA_SHA256 0


//TODO: in qe pce common header
/** Typedef enum _sgx_ql_request_policy */
typedef enum _sgx_ql_request_policy
{
    SGX_QL_PERSISTENT, ///< QE is initialized on first use and reused until process ends.
    SGX_QL_EPHEMERAL,  ///< QE is initialized and terminated on every quote.
                       ///< If a previous QE exists, it is stopped & restarted before quoting.
    SGX_QL_DEFAULT = SGX_QL_PERSISTENT
} sgx_ql_request_policy_t;

#pragma pack(push, 1)
/** Structure for the Platform Certificate Enclave identity information   */
typedef struct _sgx_pce_info_t {
    sgx_isv_svn_t pce_isv_svn;  ///< PCE ISVSVN
    uint16_t      pce_id;       ///< PCE ID.  It will change when something in the PCE would cause the PPID generation to change on the same platform
}sgx_pce_info_t;
#pragma pack(pop)

#if defined(__cplusplus)
extern "C" {
#endif

sgx_pce_error_t sgx_set_pce_enclave_load_policy(
                              sgx_ql_request_policy_t policy);

sgx_pce_error_t sgx_pce_get_target(
                              sgx_target_info_t *p_pce_target,
                              sgx_isv_svn_t *p_pce_isv_svn);

sgx_pce_error_t sgx_get_pce_info(
                              const sgx_report_t* p_report,
                              const uint8_t *p_public_key,
                              uint32_t key_size,
                              uint8_t crypto_suite,
                              uint8_t *p_encrypted_ppid,
                              uint32_t encrypted_ppid_buf_size,
                              uint32_t *p_encrypted_ppid_out_size,
                              sgx_isv_svn_t* p_pce_isvn,
                              uint16_t* p_pce_id,
                              uint8_t *p_signature_scheme);

sgx_pce_error_t sgx_pce_sign_report(
                              const sgx_isv_svn_t* isv_svn,
                              const sgx_cpu_svn_t* cpu_svn,
                              const sgx_report_t* p_report,
                              uint8_t *p_signature,
                              uint32_t signature_buf_size,
                              uint32_t *p_signature_out_size);

sgx_pce_error_t sgx_get_pce_info_without_ppid(
                             sgx_isv_svn_t* p_pce_isvsvn,
                             uint16_t* p_pce_id);

sgx_pce_error_t sgx_set_pce_path(
                             const char* p_path);
#if defined(__cplusplus)
}
#endif

#endif


