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

#ifndef _SGX_UAE_LAUNCH_H_
#define _SGX_UAE_LAUNCH_H_

#include <stdint.h>

#include "sgx_error.h"
#include "sgx_urts.h"

#ifdef  __cplusplus
extern "C" {
#endif

/**
 * Get the white list's size
 *
 * @param p_whitelist_size Save the size of the white list.
 * @return if OK, return SGX_SUCCESS
 */
sgx_status_t SGXAPI sgx_get_whitelist_size(uint32_t* p_whitelist_size);

/**
 * Get the white list value
 *
 * @param p_whitelist Save the white list value
 * @param whitelist_size The size of the white list and the read data size is whitelist_size
 * @return if OK, return SGX_SUCCESS
 */
sgx_status_t SGXAPI sgx_get_whitelist(uint8_t* p_whitelist, uint32_t whitelist_size);

/**
 * Register white list certificate chain
 *
 * @param p_wl_cert_chain The white list to be registered.
 * @param wl_cert_chain_size The size of the white list.
 * @return If OK, return SGX_SUCCESS
 */
sgx_status_t SGXAPI sgx_register_wl_cert_chain(uint8_t* p_wl_cert_chain, uint32_t wl_cert_chain_size);

#ifdef  __cplusplus
}
#endif

#endif
