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
 * File: sgx_quote_4.h
 * Description: Definition for quote structure.
 *
 * Quote structure and all relative structure will be defined in this file.
 */

#ifndef _SGX_QUOTE_4_H_
#define _SGX_QUOTE_4_H_

#include "sgx_quote_3.h"
#include "sgx_report2.h"


#pragma pack(push, 1)

#define TD_INFO_RESERVED_BYTES 112
typedef struct _tee_info_t                    /* 512 bytes */
{
    tee_attributes_t     attributes;          /* (  0) TD's attributes */
    tee_attributes_t     xfam;                /* (  8) TD's XFAM */
    tee_measurement_t    mr_td;               /* ( 16) Measurement of the initial contents of the TD */
    tee_measurement_t    mr_config_id;        /* ( 64) Software defined ID for non-owner-defined configuration on the guest TD. e.g., runtime or OS configuration */
    tee_measurement_t    mr_owner;            /* (112) Software defined ID for the guest TD's owner */
    tee_measurement_t    mr_owner_config;     /* (160) Software defined ID for owner-defined configuration of the guest TD, e.g., specific to the workload rather than the runtime or OS */
    tee_measurement_t    rt_mr[4];            /* (208) Array of 4(TDX1: NUM_RTMRS is 4) runtime extendable measurement registers */
    uint8_t reserved[TD_INFO_RESERVED_BYTES]; /* (400) Reserved, must be zero */
} tee_info_t;


#define TEE_TCB_SVN_SIZE        16
typedef struct _tee_tcb_svn_t
{
    uint8_t  tcb_svn[TEE_TCB_SVN_SIZE];
} tee_tcb_svn_t;

#define TD_TEE_TCB_INFO_RESERVED_BYTES 111
typedef struct _tee_tcb_info_t
{
    uint8_t           valid[8];                                   /* (  0) Indicates TEE_TCB_INFO fields which are valid
                                                                           - 1 in the i-th significant bit reflects that the field starting at byte offset(8*i)
                                                                           - 0 in the i-th significant bit reflects that either no field start by byte offset(8*i) or that
                                                                               field is not populated and is set to zero. */
    tee_tcb_svn_t     tee_tcb_svn;                                /* (  8) TEE_TCB_SVN Array */
    tee_measurement_t mr_seam;                                    /* ( 24) Measurement of the SEAM module */
    tee_measurement_t mr_seam_signer;                             /* ( 72) Measurement of SEAM module signer. (Not populated for Intel SEAM modules) */
    tee_attributes_t  attributes;                                 /* (120) Additional configuration attributes.(Not populated for Intel SEAM modules) */
    uint8_t           reserved[TD_TEE_TCB_INFO_RESERVED_BYTES];   /* (128) Reserved, must be zero */
} tee_tcb_info_t;

/** The SGX_QL_SGX_QL_ALG_ECDSA_P256 specific data structure.  Appears in the signature_data[] of the sgx_quote3_t
 *  structure. */
typedef struct _sgx_qe_report_certification_data_t {
     sgx_report_body_t   qe_report;           ///< QE Report of the QE when the Att key was generated.  The ReportData will contain the ECDSA_ID
     uint8_t             qe_report_sig[32*2]; ///< Signature of QE Report using the Certification Key (PCK for root signing). Big Endian
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning ( disable:4200 )
#endif
     uint8_t             auth_certification_data[];               ///< Place holder for both the auth_data_t and certification_data_t.  Concatenated in that order.
#ifdef _MSC_VER
#pragma warning(pop)
#endif
} sgx_qe_report_certification_data_t;

typedef struct _sgx_ecdsa_sig_data_v4_t {
     uint8_t             sig[32*2];            ///< Signature over the Quote using the ECDSA Att key. Big Endian.
     uint8_t             attest_pub_key[32*2]; ///< ECDSA Att Public Key. Hash in QE Report.ReportData. Big Endian
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning ( disable:4200 )
#endif
     uint8_t             certification_data[]; ///< Certification data associated with the cert_key_type
#ifdef _MSC_VER
#pragma warning(pop)
#endif
} sgx_ecdsa_sig_data_v4_t;

/** The quote header.  It is designed to compatible with earlier versions of the quote. */
typedef struct _sgx_quote4_header_t {
    uint16_t            version;              ///< 0:  The version this quote structure.
    uint16_t            att_key_type;         ///< 2:  sgx_attestation_algorithm_id_t.  Describes the type of signature in the signature_data[] field.
    uint32_t            tee_type;             ///< 4:  Type of Trusted Execution Environment for which the Quote has been generated.
                                              ///      Supported values: 0 (SGX), 0x81(TDX)
    uint32_t            reserved;             ///< 8:  Reserved field.
    uint8_t             vendor_id[16];        ///< 12: Unique identifier of QE Vendor.
    uint8_t             user_data[20];        ///< 28: Custom attestation key owner data.
} sgx_quote4_header_t;

/** SGX Report2 body */
typedef struct _sgx_report2_body_t {
    tee_tcb_svn_t       tee_tcb_svn;          ///<  0:  TEE_TCB_SVN Array
    tee_measurement_t   mr_seam;              ///< 16:  Measurement of the SEAM module
    tee_measurement_t   mrsigner_seam;        ///< 64:  Measurement of a 3rd party SEAM module’s signer (SHA384 hash). 
                                              ///       The value is 0’ed for Intel SEAM module
    tee_attributes_t    seam_attributes;      ///< 112: MBZ: TDX 1.0
    tee_attributes_t    td_attributes;        ///< 120: TD's attributes
    tee_attributes_t    xfam;                 ///< 128: TD's XFAM
    tee_measurement_t   mr_td;                ///< 136: Measurement of the initial contents of the TD
    tee_measurement_t   mr_config_id;         ///< 184: Software defined ID for non-owner-defined configuration on the guest TD. e.g., runtime or OS configuration
    tee_measurement_t   mr_owner;             ///< 232: Software defined ID for the guest TD's owner
    tee_measurement_t   mr_owner_config;      ///< 280: Software defined ID for owner-defined configuration of the guest TD, e.g., specific to the workload rather than the runtime or OS
    tee_measurement_t   rt_mr[4];             ///< 328: Array of 4(TDX1: NUM_RTMRS is 4) runtime extendable measurement registers
    tee_report_data_t   report_data;          ///< 520: Additional report data
}sgx_report2_body_t;

/** The generic TD quote data structure.  This is the common part of the quote.  The signature_data[] contains the signature and supporting
 *  information of the key used to sign the quote and the contents depend on the sgx_quote_sign_type_t value. */
typedef struct _sgx_quote4_t {
    sgx_quote4_header_t header;               ///< 0:   The quote header.
    sgx_report2_body_t  report_body;          ///< 48:  The REPORT of the TD that is attesting remotely.
    uint32_t            signature_data_len;   ///< 656: The length of the signature_data.  Varies depending on the type of sign_type.
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning ( disable:4200 )
#endif
    uint8_t             signature_data[];     ///< 660: Contains the variable length containing the quote signature and support data for the signature.
#ifdef _MSC_VER
#pragma warning(pop)
#endif
} sgx_quote4_t;

#pragma pack(pop)

#endif //_SGX_QUOTE_4_H_
