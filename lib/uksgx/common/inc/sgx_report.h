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



/*
 *  This file is to define Enclave's Report
*/

#ifndef _SGX_REPORT_H_
#define _SGX_REPORT_H_

#include "sgx_attributes.h"
#include "sgx_key.h"

#define SGX_HASH_SIZE        32              /* SHA256 */
#define SGX_MAC_SIZE         16              /* Message Authentication Code - 16 bytes */

#define SGX_REPORT_DATA_SIZE    64

#define SGX_ISVEXT_PROD_ID_SIZE 16
#define SGX_ISV_FAMILY_ID_SIZE  16

typedef struct _sgx_measurement_t
{
    uint8_t                 m[SGX_HASH_SIZE];
} sgx_measurement_t;

typedef uint8_t             sgx_mac_t[SGX_MAC_SIZE];

typedef struct _sgx_report_data_t
{
    uint8_t                 d[SGX_REPORT_DATA_SIZE];
} sgx_report_data_t;

typedef uint16_t            sgx_prod_id_t;

typedef uint8_t sgx_isvext_prod_id_t[SGX_ISVEXT_PROD_ID_SIZE];
typedef uint8_t sgx_isvfamily_id_t[SGX_ISV_FAMILY_ID_SIZE];

#define SGX_TARGET_INFO_RESERVED1_BYTES 2
#define SGX_TARGET_INFO_RESERVED2_BYTES 8
#define SGX_TARGET_INFO_RESERVED3_BYTES 384


typedef struct _target_info_t
{
    sgx_measurement_t       mr_enclave;     /* (  0) The MRENCLAVE of the target enclave */
    sgx_attributes_t        attributes;     /* ( 32) The ATTRIBUTES field of the target enclave */
    uint8_t                 reserved1[SGX_TARGET_INFO_RESERVED1_BYTES];   /* ( 48) Reserved */
    sgx_config_svn_t        config_svn;     /* ( 50) CONFIGSVN field */
    sgx_misc_select_t       misc_select;    /* ( 52) The MISCSELECT of the target enclave */
    uint8_t                 reserved2[SGX_TARGET_INFO_RESERVED2_BYTES]; /* ( 56) Reserved */
    sgx_config_id_t         config_id;      /* ( 64) CONFIGID */
    uint8_t                 reserved3[SGX_TARGET_INFO_RESERVED3_BYTES]; /* (128) Struct size is 512 bytes */
} sgx_target_info_t;


#define SGX_REPORT_BODY_RESERVED1_BYTES 12
#define SGX_REPORT_BODY_RESERVED2_BYTES 32
#define SGX_REPORT_BODY_RESERVED3_BYTES 32
#define SGX_REPORT_BODY_RESERVED4_BYTES 42


typedef struct _report_body_t
{
    sgx_cpu_svn_t           cpu_svn;        /* (  0) Security Version of the CPU */
    sgx_misc_select_t       misc_select;    /* ( 16) Which fields defined in SSA.MISC */
    uint8_t                 reserved1[SGX_REPORT_BODY_RESERVED1_BYTES];  /* ( 20) */
    sgx_isvext_prod_id_t    isv_ext_prod_id;/* ( 32) ISV assigned Extended Product ID */
    sgx_attributes_t        attributes;     /* ( 48) Any special Capabilities the Enclave possess */
    sgx_measurement_t       mr_enclave;     /* ( 64) The value of the enclave's ENCLAVE measurement */
    uint8_t                 reserved2[SGX_REPORT_BODY_RESERVED2_BYTES];  /* ( 96) */
    sgx_measurement_t       mr_signer;      /* (128) The value of the enclave's SIGNER measurement */
    uint8_t                 reserved3[SGX_REPORT_BODY_RESERVED3_BYTES];  /* (160) */
    sgx_config_id_t         config_id;      /* (192) CONFIGID */
    sgx_prod_id_t           isv_prod_id;    /* (256) Product ID of the Enclave */
    sgx_isv_svn_t           isv_svn;        /* (258) Security Version of the Enclave */
    sgx_config_svn_t        config_svn;     /* (260) CONFIGSVN */
    uint8_t                 reserved4[SGX_REPORT_BODY_RESERVED4_BYTES];  /* (262) */
    sgx_isvfamily_id_t      isv_family_id;  /* (304) ISV assigned Family ID */
    sgx_report_data_t       report_data;    /* (320) Data provided by the user */
} sgx_report_body_t;

typedef struct _report_t                    /* 432 bytes */
{
    sgx_report_body_t       body;
    sgx_key_id_t            key_id;         /* (384) KeyID used for diversifying the key tree */
    sgx_mac_t               mac;            /* (416) The Message Authentication Code over this structure. */
} sgx_report_t;

#endif
