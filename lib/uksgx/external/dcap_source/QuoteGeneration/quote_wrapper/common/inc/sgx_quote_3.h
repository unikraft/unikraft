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
 * File: sgx_quote_3.h
 * Description: Definition for quote structure.
 *
 * Quote structure and all relative structure will be defined in this file.
 */

#ifndef _SGX_QUOTE_3_H_
#define _SGX_QUOTE_3_H_

#include "sgx_quote.h"
#include "sgx_pce.h"

#define REF_QUOTE_MAX_AUTHENTICATON_DATA_SIZE 64
#define USE_PCEID

/** Enumerates the different attestation key algorithms */
typedef enum {
    SGX_QL_ALG_EPID = 0,       ///< EPID 2.0 - Anonymous
    SGX_QL_ALG_RESERVED_1 = 1, ///< Reserved
    SGX_QL_ALG_ECDSA_P256 = 2, ///< ECDSA-256-with-P-256 curve, Non - Anonymous
    SGX_QL_ALG_ECDSA_P384 = 3, ///< ECDSA-384-with-P-384 curve (Note: currently not supported), Non-Anonymous
    SGX_QL_ALG_MAX = 4
} sgx_ql_attestation_algorithm_id_t;

/** Enumerates the different certification data types used to describe the signer of the attestation key */
typedef enum {
    PPID_CLEARTEXT = 1,         ///< Clear PPID + CPU_SVN, PvE_SVN, PCE_SVN, PCE_ID
    PPID_RSA2048_ENCRYPTED = 2, ///< RSA-2048-OAEP Encrypted PPID + CPU_SVN, PvE_SVN, PCE_SVN, PCE_ID
    PPID_RSA3072_ENCRYPTED = 3, ///< RSA-3072-OAEP Encrypted PPID + CPU_SVN, PvE_SVN, PCE_SVN, PCE_ID
    PCK_CLEARTEXT = 4,          ///< Clear PCK Leaf Cert
    PCK_CERT_CHAIN = 5,         ///< Full PCK Cert chain (PCK Leaf Cert|| Intermediate CA Cert || Root CA Cert)
    ECDSA_SIG_AUX_DATA = 6,     ///< Indicates the contents of the CERTIFICATION_INFO_DATA contains the ECDSA_SIG_AUX_DATA of another Quote.
    QL_CERT_KEY_TYPE_MAX = 16,
} sgx_ql_cert_key_type_t;

#pragma pack(push, 1)

#ifndef USE_PCEID
/** TEMP!!! Structure for the Platform Certificate Enclave identity information.  The first release of the reference
 *  does not contain the PCEID in the quote. */
typedef struct _sgx_pce_info_no_pce_id_t {
    sgx_isv_svn_t pce_isv_svn;  ///< PCE ISVSVN
}sgx_pce_info_no_pce_id_t;
#endif

/** Describes the header that contains the list of attestation keys supported by a given verifier */
typedef struct _sgx_ql_att_key_id_list_header_t {
    uint16_t       id;          ///< Structure ID
    uint16_t       version;     ///< Structure version
    uint32_t       num_att_ids; ///< Number of 'Attestation Key Identifier' Elements
}sgx_ql_att_key_id_list_header_t;

/** This is the data structure of the CERTIFICATION_INFO_DATA in the Quote when the certification type is
 *  PPID_CLEARTTEXT. It identifies the PCK Cert required to verify the certification signature. */
typedef struct _sgx_ql_ppid_cleartext_cert_info_t {
    uint8_t         ppid[16];   ///< PPID of this platform
    sgx_cpu_svn_t   cpu_svn;    ///< The CPUSVN TCB used to generate the PCK signature.
    #ifdef USE_PCEID
    sgx_pce_info_t  pce_info;   ///< The PCE ISVSVN used to generate the PCK signature.
    #else
    sgx_pce_info_no_pce_id_t  pce_info;
    #endif
}sgx_ql_ppid_cleartext_cert_info_t;

/** This is the data structure of the CERTIFICATION_INFO_DATA in the Quote when the certification type is
 *  PPID_RSA2048_ENCRYPTED. It identifies the PCK Cert required to verify the certification signature. */
typedef struct _sgx_ql_ppid_rsa2048_encrypted_cert_info_t {
    uint8_t         enc_ppid[256];   ///< Encrypted PPID of this platform
    sgx_cpu_svn_t   cpu_svn;         ///< The CPUSVN TCB used to generate the PCK signature.
    #ifdef USE_PCEID
    sgx_pce_info_t  pce_info;   ///< The PCE ISVSVN used to generate the PCK signature.
    #else
    sgx_pce_info_no_pce_id_t  pce_info;
    #endif
}sgx_ql_ppid_rsa2048_encrypted_cert_info_t;

/** This is the data structure of the CERTIFICATION_INFO_DATA in the Quote when the certification type is
 *  PPID_RSA2072_ENCRYPTED. It identifies the PCK Cert required to verify the certification signature. */
typedef struct _sgx_ql_ppid_rsa3072_encrypted_cert_info_t {
    uint8_t         enc_ppid[384];   ///< Encrypted PPID of this platform
    sgx_cpu_svn_t   cpu_svn;         ///< The CPUSVN TCB used to generate the PCK signature.
    sgx_pce_info_t  pce_info;        ///< The PCE ISVSVN used to generate the PCK signature.
}sgx_ql_ppid_rsa3072_encrypted_cert_info_t;

/** Structure to hold the size of the authentication data and the place holder for
    the authentication data itself.*/
typedef struct _sgx_ql_auth_data_t {
    uint16_t size;    ///< Size in bytes contained the auth_data buffer.
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning ( disable:4200 )
#endif
    uint8_t auth_data[];   ///< Additional data provided by Att key owner to be signed by the certification key
#ifdef _MSC_VER
#pragma warning(pop)
#endif
} sgx_ql_auth_data_t;

/** Data that will be signed by the ECDSA described in the CERTIFICATION_* fields.
    This will be SHA256 hashed along with the ECDSA PUBLIC KEY and put in
    QE3_REPORT.ReportData. */
typedef struct _sgx_ql_certification_data_t {
    uint16_t cert_key_type;  ///< The type of certification key used to sign the QE3 Report and Att key hash (ECDSA_ID+Authentication Data).
    uint32_t size;  ///< Size of the data structure for the cert_key_type information.
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning ( disable:4200 )
#endif
    uint8_t certification_data[];  ///< Certification data associated with the cert_key_type
#ifdef _MSC_VER
#pragma warning(pop)
#endif
} sgx_ql_certification_data_t;

/** The SGX_QL_SGX_QL_ALG_ECDSA_P256 specific data structure.  Appears in the signature_data[] of the sgx_quote3_t
 *  structure. */
typedef struct _sgx_ql_ecdsa_sig_data_t {
    uint8_t               sig[32*2];            ///< Signature over the Quote using the ECDSA Att key. Big Endian.
    uint8_t               attest_pub_key[32*2]; ///< ECDSA Att Public Key.  Hash in QE3Report.ReportData.  Big Endian
    sgx_report_body_t     qe_report;            ///< QE3 Report of the QE when the Att key was generated.  The ReportData will contain the ECDSA_ID
    uint8_t               qe_report_sig[32*2];  ///< Signature of QE Report using the Certification Key (PCK for root signing). Big Endian
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning ( disable:4200 )
#endif
    uint8_t               auth_certification_data[];               ///< Place holder for both the auth_data_t and certification_data_t.  Concatenated in that order.
#ifdef _MSC_VER
#pragma warning(pop)
#endif
} sgx_ql_ecdsa_sig_data_t;

/** The quote header.  It is designed to compatible with earlier versions of the quote. */
typedef struct _sgx_quote_header_t {
    uint16_t            version;             ///< 0:  The version this quote structure.
    uint16_t            att_key_type;        ///< 2:  sgx_attestation_algorithm_id_t.  Describes the type of signature in the signature_data[] field.
    uint32_t            att_key_data_0;      ///< 4:  Optionally stores additional data associated with the att_key_type.
    sgx_isv_svn_t       qe_svn;              ///< 8:  The ISV_SVN of the Quoting Enclave when the quote was generated.
    sgx_isv_svn_t       pce_svn;             ///< 10: The ISV_SVN of the PCE when the quote was generated.
    uint8_t             vendor_id[16];       ///< 12: Unique identifier of QE Vendor.
    uint8_t             user_data[20];       ///< 28: Custom attestation key owner data.
} sgx_quote_header_t;

/** The generic quote data structure.  This is the common part of the quote.  The signature_data[] contains the signature and supporting
 *  information of the key used to sign the quote and the contents depend on the sgx_quote_sign_type_t value. */
typedef struct _sgx_quote3_t {
    sgx_quote_header_t  header;              ///< 0:   The quote header.
    sgx_report_body_t   report_body;         ///< 48: The REPORT of the app that is attesting remotely.
    uint32_t            signature_data_len;  ///< 432: The length of the signature_data.  Varies depending on the type of sign_type.
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning ( disable:4200 )
#endif
    uint8_t             signature_data[];    ///< 436: Contains the variable length containing the quote signature and support data for the signature.
#ifdef _MSC_VER
#pragma warning(pop)
#endif
} sgx_quote3_t;

#pragma pack(pop)

#endif //_SGX_QUOTE_3_H_

