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

#ifndef _SGX_QVE_HEADER_H_
#define _SGX_QVE_HEADER_H_

#include "sgx_key.h"
#include "time.h"

#ifndef SGX_QL_QV_MK_ERROR
#define SGX_QL_QV_MK_ERROR(x)              (0x0000A000|(x))
#endif //SGX_QL_QV_MK_ERROR
/** Contains the possible values of the quote verification result. */
typedef enum _sgx_ql_qv_result_t
{
   SGX_QL_QV_RESULT_OK = 0x0000,                                            ///< The Quote verification passed and is at the latest TCB level
   SGX_QL_QV_RESULT_MIN = SGX_QL_QV_MK_ERROR(0x0001),
   SGX_QL_QV_RESULT_CONFIG_NEEDED = SGX_QL_QV_MK_ERROR(0x0001),             ///< The Quote verification passed and the platform is patched to
                                                                            ///< the latest TCB level but additional configuration of the SGX
                                                                            ///< platform may be needed
   SGX_QL_QV_RESULT_OUT_OF_DATE = SGX_QL_QV_MK_ERROR(0x0002),               ///< The Quote is good but TCB level of the platform is out of date.
                                                                            ///< The platform needs patching to be at the latest TCB level
   SGX_QL_QV_RESULT_OUT_OF_DATE_CONFIG_NEEDED = SGX_QL_QV_MK_ERROR(0x0003), ///< The Quote is good but the TCB level of the platform is out of
                                                                            ///< date and additional configuration of the SGX Platform at its
                                                                            ///< current patching level may be needed. The platform needs
                                                                            ///< patching to be at the latest TCB level
   SGX_QL_QV_RESULT_INVALID_SIGNATURE = SGX_QL_QV_MK_ERROR(0x0004),         ///< The signature over the application report is invalid
   SGX_QL_QV_RESULT_REVOKED = SGX_QL_QV_MK_ERROR(0x0005),                   ///< The attestation key or platform has been revoked
   SGX_QL_QV_RESULT_UNSPECIFIED = SGX_QL_QV_MK_ERROR(0x0006),               ///< The Quote verification failed due to an error in one of the input
   SGX_QL_QV_RESULT_SW_HARDENING_NEEDED = SGX_QL_QV_MK_ERROR(0x0007),       ///< The TCB level of the platform is up to date, but SGX SW Hardening
                                                                            ///< is needed
   SGX_QL_QV_RESULT_CONFIG_AND_SW_HARDENING_NEEDED = SGX_QL_QV_MK_ERROR(0x0008),   ///< The TCB level of the platform is up to date, but additional
                                                                                   ///< configuration of the platform at its current patching level
                                                                                   ///< may be needed. Moreove, SGX SW Hardening is also needed

   SGX_QL_QV_RESULT_MAX = SGX_QL_QV_MK_ERROR(0x00FF),                              ///< Indicate max result to allow better translation

} sgx_ql_qv_result_t;


typedef enum _pck_cert_flag_enum_t {
    PCK_FLAG_FALSE = 0,
    PCK_FLAG_TRUE,
    PCK_FLAG_UNDEFINED
} pck_cert_flag_enum_t;


#define ROOT_KEY_ID_SIZE    48
#define PLATFORM_INSTANCE_ID_SIZE   16


/** Contains data that will allow an alternative quote verification policy. */
typedef struct _sgx_ql_qv_supplemental_t
{
    uint32_t version;                     ///< Supplemental data version
    time_t earliest_issue_date;           ///< Earliest issue date of all the collateral (UTC)
    time_t latest_issue_date;             ///< Latest issue date of all the collateral (UTC)
    time_t earliest_expiration_date;      ///< Earliest expiration date of all the collateral (UTC)
    time_t tcb_level_date_tag;            ///< The SGX TCB of the platform that generated the quote is not vulnerable
                                          ///< to any Security Advisory with an SGX TCB impact released on or before this date.
                                          ///< See Intel Security Center Advisories
    uint32_t pck_crl_num;                 ///< CRL Num from PCK Cert CRL
    uint32_t root_ca_crl_num;             ///< CRL Num from Root CA CRL
    uint32_t tcb_eval_ref_num;            ///< Lower number of the TCBInfo and QEIdentity
    uint8_t root_key_id[ROOT_KEY_ID_SIZE];              ///< ID of the collateral's root signer (hash of Root CA's public key SHA-384)
    sgx_key_128bit_t pck_ppid;            ///< PPID from remote platform.  Can be used for platform ownership checks
    sgx_cpu_svn_t tcb_cpusvn;             ///< CPUSVN of the remote platform's PCK Cert
    sgx_isv_svn_t tcb_pce_isvsvn;         ///< PCE_ISVNSVN of the remote platform's PCK Cert
    uint16_t pce_id;                      ///< PCE_ID of the remote platform
    uint32_t tee_type;                    ///< 0x00000000: SGX or 0x00000081: TDX
    uint8_t sgx_type;                     ///< Indicate the type of memory protection available on the platform, it should be one of
                                          ///< Standard (0), Scalable (1) and Scalable with Integrity (2)

    // Multi-Package PCK cert related flags, they are only relevant to PCK Certificates issued by PCK Platform CA
    uint8_t platform_instance_id[PLATFORM_INSTANCE_ID_SIZE];           ///< Value of Platform Instance ID, 16 bytes
    pck_cert_flag_enum_t dynamic_platform;      ///< Indicate whether a platform can be extended with additional packages - via Package Add calls to SGX Registration Backend
    pck_cert_flag_enum_t cached_keys;           ///< Indicate whether platform root keys are cached by SGX Registration Backend
    pck_cert_flag_enum_t smt_enabled;           ///< Indicate whether a plat form has SMT (simultaneous multithreading) enabled

} sgx_ql_qv_supplemental_t;


#endif //_QVE_HEADER_H_
