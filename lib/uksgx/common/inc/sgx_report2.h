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
 *  This file is to define Report Type2
 */

#ifndef _SGX_REPORT2_H_
#define _SGX_REPORT2_H_

#define TEE_HASH_384_SIZE       48 /* SHA384 */
#define TEE_MAC_SIZE            32 /* Message SHA 256 HASH Code - 32 bytes */

#define SGX_REPORT2_DATA_SIZE   64
#define TEE_CPU_SVN_SIZE        16

#pragma pack(push, 1)

typedef uint8_t tee_mac_t[TEE_MAC_SIZE];

typedef struct _tee_cpu_svn_t {
    uint8_t svn[TEE_CPU_SVN_SIZE];
} tee_cpu_svn_t;

typedef struct _tee_measurement_t {
    uint8_t m[TEE_HASH_384_SIZE];
} tee_measurement_t;

typedef struct _tee_report_data_t {
    uint8_t d[SGX_REPORT2_DATA_SIZE];
} tee_report_data_t;

typedef struct _tee_attributes_t
{
    uint32_t                a[2];
} tee_attributes_t;

#define SGX_LEGACY_REPORT_TYPE  0x0     /* SGX Legacy Report Type */
#define TEE_REPORT2_TYPE        0x81    /* TEE Report Type2 */
#define TEE_REPORT2_SUBTYPE     0x0     /* SUBTYPE for Report Type2 is 0 */
#define TEE_REPORT2_VERSION     0x0     /* VERSION for Report Type2 is 0 */

typedef struct _tee_report_type_t {
    uint8_t type;       /* Trusted Execution Environment(TEE) type:
                            0x00:      SGX Legacy REPORT TYPE
                            0x7F-0x01: Reserved
                            0x80:      Reserved
                            0x81:      TEE Report type 2
                            0xFF-0x82: Reserved
                        */
    uint8_t subtype;    /* TYPE-specific subtype, Stage1: value is 0 */
    uint8_t version;    /* TYPE-specific version, Stage1: value is 0 */
    uint8_t reserved;   /* Reserved, must be zero */
} tee_report_type_t;

#define SGX_REPORT2_MAC_STRUCT_RESERVED1_BYTES 12
#define SGX_REPORT2_MAC_STRUCT_RESERVED2_BYTES 32
typedef struct _sgx_report2_mac_struct_t /* 256 bytes */
{
    tee_report_type_t   report_type;                                        /* (  0) TEE Report type.*/
    uint8_t             reserved1[SGX_REPORT2_MAC_STRUCT_RESERVED1_BYTES];  /* (  4) Reserved, must be zero */
    tee_cpu_svn_t       cpu_svn;                                            /* ( 16) Security Version of the CPU */
    tee_measurement_t   tee_tcb_info_hash;                                  /* ( 32) SHA384 of TEE_TCB_INFO for TEEs */
    tee_measurement_t   tee_info_hash;                                      /* ( 80) SHA384 of TEE_INFO */
    tee_report_data_t   report_data;                                        /* (128) Data provided by the user */
    uint8_t             reserved2[SGX_REPORT2_MAC_STRUCT_RESERVED2_BYTES];  /* (192) Reserved, must be zero */
    tee_mac_t           mac;                                                /* (224) The Message Authentication Code over this structure */
} sgx_report2_mac_struct_t;

#define TEE_TCB_INFO_SIZE 239
#define SGX_REPORT2_RESERVED_BYTES 17
#define TEE_INFO_SIZE 512
typedef struct _sgx_report2_t /* 1024 bytes */
{
    sgx_report2_mac_struct_t    report_mac_struct;                      /* (  0) Report mac struct for SGX report type 2 */
    uint8_t                     tee_tcb_info[TEE_TCB_INFO_SIZE];        /* (256) Struct contains details about extra TCB elements not found in CPUSVN */
    uint8_t                     reserved[SGX_REPORT2_RESERVED_BYTES];   /* (495) Reserved, must be zero */
    uint8_t                     tee_info[TEE_INFO_SIZE];                /* (512) Struct contains the TEE Info */
} sgx_report2_t;
#pragma pack(pop)

#endif
