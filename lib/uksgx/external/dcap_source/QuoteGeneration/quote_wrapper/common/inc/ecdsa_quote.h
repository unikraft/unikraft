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
 * File: esdsa_quote.h 
 *  
 * Description: General ECDSA quote reference code definitions.
 *
 */

/* User defined types */
#ifndef _ECDSA_QUOTE_H_
#define _ECDSA_QUOTE_H_

#include "sgx_tseal.h"
#include "sgx_pce.h"
#include "sgx_quote_3.h"

#define REF_ECDSDA_AUTHENTICATION_DATA_SIZE 32

#define SGX_QL_SEAL_ECDSA_KEY_BLOB 0
#define SGX_QL_ECDSA_KEY_BLOB_VERSION_0 0

#define SGX_QL_TRUSTED_ECDSA_BLOB_SIZE_SDK ((uint32_t)(sizeof(sgx_sealed_data_t) + \
                                                      sizeof(ref_ciphertext_ecdsa_data_sdk_t) + \
                                                      sizeof(ref_plaintext_ecdsa_data_sdk_t)))

#pragma pack(push, 1)

/** Used to store the plaintext data of the ECDSA key and the certification data in a sealed blob.  This portion of the blob is
 *  authenticated but not encrypted. Allows for reuse of a generated ECDSA Attestation key. The init_quote() will
 *  generate and store it on disk.  It can be used later for the get_quote() to generate quotes using a pre-generated
 *  and certified ECDSA Attestation Key. */
typedef struct _ref_plaintext_ecdsa_data_sdk_t {
    uint8_t                seal_blob_type;           ///< Enclave-specific Sealblob Type, currently only one Sealblob type defined: SGX_QL_SEAL_ECDSA_KEY_BLOB=0
    uint8_t                ecdsa_key_version;        ///< Describes the version of the structure of this ECDSA blob.
    sgx_cpu_svn_t          cert_cpu_svn;             ///< The CPUSVN used to certify the ECDSA att key.
    sgx_isv_svn_t          cert_qe_isv_svn;          ///< The QE's ISV_SVN when the ECDSA att key was generated.
    sgx_pce_info_t         cert_pce_info;            ///< PCE ISVSVN and PCEID used to certify.
    sgx_ql_cert_key_type_t certification_key_type;   ///< Certification key type of this blob.
    sgx_cpu_svn_t          raw_cpu_svn;              ///< The platform's raw CPUSVN when the ECDSA att key was certified.
    sgx_pce_info_t         raw_pce_info;             ///< The platform's raw PCE ISV_SVN and PCIED when the ECDSA att key was certified.
    uint8_t                signature_scheme;         ///< Indicates the crypto algorithm used to sign the qe_report.
    sgx_target_info_t      pce_target_info;          ///< PCE Target info used when the key was certified.
    sgx_report_t           qe_report;                ///< Report of the QE used to generate the ECDSA Att Key.  REPORTDATA = ECDSA_ID || AuthenticationData
    sgx_ec256_signature_t  qe_report_cert_key_sig;   ///< The ECDSA signature using the certification key (PCK for root signing).  x and y each stored in Big Endian
    sgx_ec256_public_t     ecdsa_att_public_key;     ///< The public portion of the generated ECDSA Att Key. x and y each stored in Big Endian
    sgx_sha256_hash_t      ecdsa_id;                 ///< The SHA256 hash of the ecdsa_att_public_key
    sgx_cpu_svn_t          seal_cpu_svn;             ///< The CPUSVN used to seal the blob.
    sgx_isv_svn_t          seal_qe_isv_svn;          ///< The QE ISV_SVN used to seal the blob.
    uint32_t               authentication_data_size; ///< The number of bytes in the authentication_data array.
    uint8_t                authentication_data[REF_ECDSDA_AUTHENTICATION_DATA_SIZE];    ///< authentication_data buffer.
    sgx_key_128bit_t       qe_id;
}ref_plaintext_ecdsa_data_sdk_t;

#pragma pack(pop)

#endif  // _ECDSA_QUOTE_H_

