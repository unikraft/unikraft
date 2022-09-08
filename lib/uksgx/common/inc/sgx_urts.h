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



#ifndef _SGX_URTS_H_
#define _SGX_URTS_H_

#include "sgx_attributes.h"
#include "sgx_error.h"
#include "sgx_eid.h"
#include "sgx_defs.h"
#include "sgx_key.h"
#include "sgx_report.h"

#include <stddef.h>


#define MAX_EX_FEATURES_COUNT 32

#define SGX_CREATE_ENCLAVE_EX_PCL_BIT_IDX           0
#define SGX_CREATE_ENCLAVE_EX_PCL                  (1 << SGX_CREATE_ENCLAVE_EX_PCL_BIT_IDX) // Reserve Bit 0 for the protected code loader
#define SGX_CREATE_ENCLAVE_EX_SWITCHLESS_BIT_IDX    1
#define SGX_CREATE_ENCLAVE_EX_SWITCHLESS           (1 << SGX_CREATE_ENCLAVE_EX_SWITCHLESS_BIT_IDX) // Reserve Bit 1 for Switchless Runtime System


#define SGX_CREATE_ENCLAVE_EX_KSS_BIT_IDX           2U
#define SGX_CREATE_ENCLAVE_EX_KSS                  (1U << SGX_CREATE_ENCLAVE_EX_KSS_BIT_IDX)  // Bit 2 for Key Separation & Sharing 

#pragma pack(push, 1)

/* Structure for KSS feature */
typedef struct _sgx_kss_config_t
{
    sgx_config_id_t config_id;
    sgx_config_svn_t config_svn;
} sgx_kss_config_t;

#pragma pack(pop)


//update the following when adding new extended feature
#define _SGX_LAST_EX_FEATURE_IDX_  SGX_CREATE_ENCLAVE_EX_KSS_BIT_IDX 

#define _SGX_EX_FEATURES_MASK_ (((uint32_t)-1) >> (MAX_EX_FEATURES_COUNT - 1  - _SGX_LAST_EX_FEATURE_IDX_))

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t sgx_launch_token_t[1024];

/* Convenient macro to be passed to sgx_create_enclave(). */
#if !defined(NDEBUG) || defined(EDEBUG)
#define SGX_DEBUG_FLAG 1
#else
#define SGX_DEBUG_FLAG 0
#endif

sgx_status_t SGXAPI sgx_create_enclave(const char *file_name, 
                                       const int debug, 
                                       sgx_launch_token_t *launch_token, 
                                       int *launch_token_updated, 
                                       sgx_enclave_id_t *enclave_id, 
                                       sgx_misc_attribute_t *misc_attr);



sgx_status_t SGXAPI sgx_create_enclave_ex(const char * file_name, 
                                          const int debug, 
                                          sgx_launch_token_t * launch_token, 
                                          int * launch_token_updated, 
                                          sgx_enclave_id_t * enclave_id, 
                                          sgx_misc_attribute_t * misc_attr,  
                                          const uint32_t ex_features, 
                                          const void* ex_features_p[32]);


sgx_status_t SGXAPI sgx_create_enclave_from_buffer_ex(
                                          uint8_t *buffer,
                                          size_t buffer_size,
                                          const int debug,
                                          sgx_enclave_id_t * enclave_id,
                                          sgx_misc_attribute_t * misc_attr,
                                          const uint32_t ex_features,
                                          const void* ex_features_p[32]);
 
 



sgx_status_t SGXAPI sgx_create_encrypted_enclave(
                        const char *file_name,
                        const int debug,
                        sgx_launch_token_t *launch_token,
                        int *launch_token_updated,
                        sgx_enclave_id_t *enclave_id,
                        sgx_misc_attribute_t *misc_attr,
                        uint8_t* sealed_key);

sgx_status_t SGXAPI sgx_destroy_enclave(const sgx_enclave_id_t enclave_id);

sgx_status_t SGXAPI sgx_get_target_info(
	const sgx_enclave_id_t enclave_id,
	sgx_target_info_t* target_info);

#ifdef __cplusplus
}
#endif


#endif
