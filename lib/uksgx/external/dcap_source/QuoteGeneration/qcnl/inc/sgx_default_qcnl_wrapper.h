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
/** File: sgx_default_qcnl_wrapper.h 
 *  
 * Description: Definitions and prototypes for the PCK Collateral Network Library APIs
 *
 */
#ifndef _SGX_DEFAULT_QCNL_WRAPPER_H_
#define _SGX_DEFAULT_QCNL_WRAPPER_H_
#pragma once

#include "sgx_ql_lib_common.h"
#include <stddef.h>

#define SGX_QCNL_MK_ERROR(x) (0x0000B000 | (x))
typedef enum _sgx_qcnl_error_t {
    SGX_QCNL_SUCCESS = 0x0000,                                             ///< Success
    SGX_QCNL_UNEXPECTED_ERROR = SGX_QCNL_MK_ERROR(0x0001),                 ///< Unexpected errors
    SGX_QCNL_INVALID_PARAMETER = SGX_QCNL_MK_ERROR(0x0002),                ///< Invalid parameters
    SGX_QCNL_NETWORK_ERROR = SGX_QCNL_MK_ERROR(0x0003),                    ///< Network error
    SGX_QCNL_NETWORK_PROXY_FAIL = SGX_QCNL_MK_ERROR(0x0004),               ///< Network error : Couldn't resolve proxy
    SGX_QCNL_NETWORK_HOST_FAIL = SGX_QCNL_MK_ERROR(0x0005),                ///< Network error : Couldn't resolve host
    SGX_QCNL_NETWORK_COULDNT_CONNECT = SGX_QCNL_MK_ERROR(0x0006),          ///< Network error : Failed to connect() to host or proxy.
    SGX_QCNL_NETWORK_HTTP2_ERROR = SGX_QCNL_MK_ERROR(0x0007),              ///< Network error : A problem was detected in the HTTP2 framing layer
    SGX_QCNL_NETWORK_WRITE_ERROR = SGX_QCNL_MK_ERROR(0x0008),              ///< Network error : an error was returned to libcurl from a write callback.
    SGX_QCNL_NETWORK_OPERATION_TIMEDOUT = SGX_QCNL_MK_ERROR(0x0009),       ///< Network error : Operation timeout.
    SGX_QCNL_NETWORK_HTTPS_ERROR = SGX_QCNL_MK_ERROR(0x000A),              ///< Network error : A problem occurred somewhere in the SSL/TLS handshake
    SGX_QCNL_NETWORK_UNKNOWN_OPTION = SGX_QCNL_MK_ERROR(0x000B),           ///< Network error : An option passed to libcurl is not recognized/known.
    SGX_QCNL_NETWORK_INIT_ERROR = SGX_QCNL_MK_ERROR(0x000C),               ///< Failed to initialize CURL library
    SGX_QCNL_MSG_ERROR = SGX_QCNL_MK_ERROR(0x000D),                        ///< HTTP message error
    SGX_QCNL_OUT_OF_MEMORY = SGX_QCNL_MK_ERROR(0x000E),                    ///< Out of memory error
    SGX_QCNL_ERROR_STATUS_NO_CACHE_DATA = SGX_QCNL_MK_ERROR(0x000F),       ///< No cache data
    SGX_QCNL_ERROR_STATUS_PLATFORM_UNKNOWN = SGX_QCNL_MK_ERROR(0x0010),    ///< Platform unknown
    SGX_QCNL_ERROR_STATUS_UNEXPECTED = SGX_QCNL_MK_ERROR(0x0011),          ///< Unexpected cache error
    SGX_QCNL_ERROR_STATUS_CERTS_UNAVAILABLE = SGX_QCNL_MK_ERROR(0x0012),   ///< Certs not available
    SGX_QCNL_ERROR_STATUS_SERVICE_UNAVAILABLE = SGX_QCNL_MK_ERROR(0x0013), ///< Service is currently not available
    SGX_QCNL_INVALID_CONFIG = SGX_QCNL_MK_ERROR(0x0030),                   ///< Error in configuration file
} sgx_qcnl_error_t;

typedef enum _sgx_qe_type_t {
    SGX_QE_TYPE_ECDSA = 0,
    SGX_QE_TYPE_TD = 1,
} sgx_qe_type_t;

#if defined(__cplusplus)
extern "C" {
#endif

sgx_qcnl_error_t sgx_qcnl_get_pck_cert_chain(const sgx_ql_pck_cert_id_t *p_pck_cert_id,
                                             sgx_ql_config_t **pp_quote_config);

void sgx_qcnl_free_pck_cert_chain(sgx_ql_config_t *p_quote_config);

sgx_qcnl_error_t sgx_qcnl_get_pck_crl_chain(const char *ca,
                                            uint16_t ca_size,
                                            const char* custom_param_b64_string,
                                            uint8_t **p_crl_chain,
                                            uint16_t *p_crl_chain_size);

void sgx_qcnl_free_pck_crl_chain(uint8_t *p_crl_chain);

sgx_qcnl_error_t sgx_qcnl_get_tcbinfo(const char *fmspc,
                                      uint16_t fmspc_size,
                                      const char* custom_param_b64_string,
                                      uint8_t **p_tcbinfo,
                                      uint16_t *p_tcbinfo_size);

void sgx_qcnl_free_tcbinfo(uint8_t *p_tcbinfo);

sgx_qcnl_error_t tdx_qcnl_get_tcbinfo(const char *fmspc,
                                      uint16_t fmspc_size,
                                      const char* custom_param_b64_string,
                                      uint8_t **p_tcbinfo,
                                      uint16_t *p_tcbinfo_size);

void tdx_qcnl_free_tcbinfo(uint8_t *p_tcbinfo);

sgx_qcnl_error_t sgx_qcnl_get_qe_identity(sgx_qe_type_t qe_type,
                                          const char* custom_param_b64_string,
                                          uint8_t **p_qe_identity,
                                          uint16_t *p_qe_identity_size);

void sgx_qcnl_free_qe_identity(uint8_t *p_qe_identity);

sgx_qcnl_error_t sgx_qcnl_get_qve_identity(const char* custom_param_b64_string,
                                           char **pp_qve_identity,
                                           uint32_t *p_qve_identity_size,
                                           char **pp_qve_identity_issuer_chain,
                                           uint32_t *p_qve_identity_issuer_chain_size);

void sgx_qcnl_free_qve_identity(char *p_qve_identity, char *p_qve_identity_issuer_chain);

sgx_qcnl_error_t sgx_qcnl_get_root_ca_crl(const char *root_ca_cdp_url,
                                          const char* custom_param_b64_string,
                                          uint8_t **p_root_ca_crl,
                                          uint16_t *p_root_ca_crl_size);

void sgx_qcnl_free_root_ca_crl(uint8_t *p_root_ca_crl);

bool sgx_qcnl_get_api_version(uint16_t *p_major_ver, uint16_t *p_minor_ver);

sgx_qcnl_error_t sgx_qcnl_set_logging_callback(sgx_ql_logging_callback_t logger);

void qcnl_log(sgx_ql_log_level_t level, const char *fmt, ...);

#if defined(__cplusplus)
}
#endif

#endif /* !_SGX_DEFAULT_QCNL_WRAPPER_H_ */
