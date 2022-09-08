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
 * File: sgx_dcap_qpl.cpp 
 *  
 * Description: Quote Provider Library
 */

#include <string.h>
#include <stdlib.h>
#include "sgx_pce.h"
#include "sgx_quote_provider.h"

using namespace std;

#define MAX_URL_LENGTH  2083
#define QE3_ID_SIZE     16
#define ENC_PPID_SIZE   384
#define CPUSVN_SIZE     16
#define PCESVN_SIZE     2
#define PCEID_SIZE      2
#define FMSPC_SIZE      6
#define MIN_CERT_DATA_SIZE (500)


quote3_error_t sgx_ql_free_quote_config(sgx_ql_config_t *p_quote_config)
{
    if (p_quote_config){
        if (p_quote_config->p_cert_data){
            free(p_quote_config->p_cert_data);
            p_quote_config->p_cert_data = NULL;
        }
        memset(p_quote_config, 0, sizeof(sgx_ql_config_t));
        free(p_quote_config);
    }

    return SGX_QL_SUCCESS;
}

static uint8_t encrypted_ppid[ENC_PPID_SIZE];
quote3_error_t sgx_ql_get_quote_config(const sgx_ql_pck_cert_id_t *p_cert_id, sgx_ql_config_t **pp_quote_config)
{
    quote3_error_t ret = SGX_QL_ERROR_UNEXPECTED;
    // Check input parameters
    if (p_cert_id == NULL || pp_quote_config == NULL) {
        return SGX_QL_ERROR_INVALID_PARAMETER;
    }
    if (p_cert_id->p_qe3_id == NULL || p_cert_id->qe3_id_size != QE3_ID_SIZE) {
        return SGX_QL_ERROR_INVALID_PARAMETER;
    }
    if (p_cert_id->p_platform_cpu_svn == NULL || p_cert_id->p_platform_pce_isv_svn == NULL) {
        return SGX_QL_ERROR_INVALID_PARAMETER;
    }
    if (p_cert_id->crypto_suite != PCE_ALG_RSA_OAEP_3072) {
        return SGX_QL_ERROR_INVALID_PARAMETER;
    }

    if(p_cert_id->p_encrypted_ppid != NULL) {
        if(p_cert_id->encrypted_ppid_size != ENC_PPID_SIZE) {
            // Allow ENCRYPTED_PPID to be NULL, but if it is not NULL, the size must match ENC_PPID_SIZE
            return SGX_QL_ERROR_INVALID_PARAMETER;
        }
        memcpy(encrypted_ppid, p_cert_id->p_encrypted_ppid, p_cert_id->encrypted_ppid_size);
    }


    do {
        // allocate output buffer
        *pp_quote_config = (sgx_ql_config_t*)malloc(sizeof(sgx_ql_config_t));
        if (*pp_quote_config == NULL) {
            ret = SGX_QL_ERROR_OUT_OF_MEMORY;
            break;
        }
        memset(*pp_quote_config, 0, sizeof(sgx_ql_config_t));

        // set version
        (*pp_quote_config)->version = SGX_QL_CONFIG_VERSION_1;

        // set cpusvn and pcesvn)
        memcpy(&(*pp_quote_config)->cert_cpu_svn,reinterpret_cast<const uint8_t*>(p_cert_id->p_platform_cpu_svn) , CPUSVN_SIZE);
        memcpy(&(*pp_quote_config)->cert_pce_isv_svn,reinterpret_cast<const uint8_t*>(p_cert_id->p_platform_pce_isv_svn) , PCESVN_SIZE);

        uint32_t data_size = p_cert_id->encrypted_ppid_size + p_cert_id->qe3_id_size + PCEID_SIZE + CPUSVN_SIZE + PCESVN_SIZE;
        if(data_size <  MIN_CERT_DATA_SIZE )
                data_size = MIN_CERT_DATA_SIZE;
        (*pp_quote_config)->cert_data_size = data_size;
        (*pp_quote_config)->p_cert_data = (uint8_t*)malloc((*pp_quote_config)->cert_data_size);
        if (!(*pp_quote_config)->p_cert_data) {
            ret = SGX_QL_ERROR_OUT_OF_MEMORY;
            break;
        }
        uint8_t* p_cert_data = (*pp_quote_config)->p_cert_data;
        uint32_t data_index = 0;
        memcpy(p_cert_data + data_index, encrypted_ppid, ENC_PPID_SIZE);
        data_index = data_index + ENC_PPID_SIZE;
        memcpy(p_cert_data + data_index, reinterpret_cast<const uint8_t*>(&( p_cert_id->pce_id)), sizeof(p_cert_id->pce_id));
        data_index = data_index + PCEID_SIZE;
        memcpy(p_cert_data + data_index, reinterpret_cast<const uint8_t*>(p_cert_id->p_platform_cpu_svn) , sizeof(sgx_cpu_svn_t));
        data_index = data_index + CPUSVN_SIZE;
        memcpy(p_cert_data + data_index, reinterpret_cast<const uint8_t*>(p_cert_id->p_platform_pce_isv_svn) , sizeof(sgx_isv_svn_t));
        data_index = data_index + PCESVN_SIZE;
        memcpy(p_cert_data + data_index, p_cert_id->p_qe3_id, p_cert_id->qe3_id_size);

        ret = SGX_QL_SUCCESS;
    } while(0);

    if (ret != SGX_QL_SUCCESS) {
        sgx_ql_free_quote_config(*pp_quote_config);
    }

    return ret;
}

