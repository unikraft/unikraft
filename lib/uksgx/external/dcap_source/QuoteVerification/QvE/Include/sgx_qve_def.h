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

#ifndef _SGX_QVE_DEF_H_
#define _SGX_QVE_DEF_H_

#include "sgx_ql_quote.h"
#include "sgx_report.h"
#include "sgx_quote.h"


#ifndef DEBUG_MODE
#define DEBUG_MODE 0
#endif //DEBUG_MODE

#define SUPPLEMENTAL_DATA_VERSION 3
#define QVE_COLLATERAL_VERSION1 0x1
#define QVE_COLLATERAL_VERSION3 0x3
#define QVE_COLLATERAL_VERSOIN31 0x00010003
#define QVE_COLLATERAL_VERSION4 0x4
#define FMSPC_SIZE 6
#define CA_SIZE 10
#define SGX_CPUSVN_SIZE   16
//
#define QUOTE_MIN_SIZE 1020
#define QUOTE_CERT_TYPE 5
#define CRL_MIN_SIZE 300
#define PROCESSOR_ISSUER "Processor"
#define PLATFORM_ISSUER "Platform"
#define PROCESSOR_ISSUER_ID "processor"
#define PLATFORM_ISSUER_ID "platform"
#define PEM_CRL_PREFIX "-----BEGIN X509 CRL-----"
#define PEM_CRL_PREFIX_SIZE 24

#define UNUSED_PARAM(x) (void)(x)
#define CHECK_MANDATORY_PARAMS(param, param_size) (param == NULL || param_size == 0)
#define CHECK_OPT_PARAMS(param, param_size) ((param == NULL && param_size != 0) || (param != NULL && param_size == 0))

#define NULL_POINTER(x) x==NULL
#define NULL_BREAK(x) if (x == NULL) {break;}
#define BREAK_ERR(x) {if (x != STATUS_OK) break;}
#define SGX_ERR_BREAK(x) {if (x != SGX_SUCCESS) break;}
#ifndef CLEAR_FREE_MEM
#include <string.h>
#define CLEAR_FREE_MEM(address, size) {             \
    if (address != NULL) {                          \
        if (size > 0) {                             \
            (void)memset_s(address, size, 0, size); \
        }                                           \
        free(address);                              \
     }                                              \
}
#endif //CLEAR_FREE_MEM

#define EXPECTED_CERTIFICATE_COUNT_IN_PCK_CHAIN 3

#endif //_SGX_QVE_DEF_H_
