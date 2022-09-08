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

#ifndef _UAE_SERVICE_INTERNAL_H_
#define _UAE_SERVICE_INTERNAL_H_

#include <stdint.h>
#include "arch.h"
#include "sgx_urts.h"

#ifdef  __cplusplus
extern "C" {
#endif
/**
 * Function to get launch token of a enclave
 *
 * @param signature[in] Signature of enclave to be launched.
 * @param attribute[in] Attribute of enclave to be launched.
 * @param launch_token[out] Vontains launch token.
 * @return if a launch token is generated,return SGX_SUCCESS, otherwise return general error code SGX_ERROR_SERVICE_UNAVAILABLE
 *         SGX_ERROR_SERVICE_TIMEOUT, or SGX_ERROR_SERVICE_INVALID_PRIVILEGE, SGX_ERROR_INVALID_PARAMETER
 *         to indicate special error condition.
 */
sgx_status_t SGXAPI get_launch_token(const enclave_css_t* signature, const sgx_attributes_t* attribute, sgx_launch_token_t* launch_token);

typedef sgx_status_t (*func_get_launch_token_t)(const enclave_css_t*,
                                                const sgx_attributes_t*,
                                                sgx_launch_token_t*);

#ifdef  __cplusplus
}
#endif

#endif
