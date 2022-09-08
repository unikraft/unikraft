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
 * File: sgx_default_quote_provider.cpp
 *
 * Description: Quote Provider Library
 */

#include "sgx_default_quote_provider.h"
#include "se_memcpy.h"
#include "sgx_base64.h"
#include "sgx_default_qcnl_wrapper.h"
#include "x509.h"
#include <cstdarg>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

using namespace std;

#ifndef _MSC_VER
#define __unaligned
#endif

static const char *X509_DELIMITER = "-----BEGIN CERTIFICATE-----";
static sgx_ql_logging_callback_t logger_callback = nullptr;

void qpl_log(sgx_ql_log_level_t level, const char *fmt, ...) {
    if (logger_callback != nullptr) {
        char message[512];
        va_list args;
        va_start(args, fmt);
        vsnprintf(message, sizeof(message), fmt, args);
        va_end(args);

        // ensure buf is always null-terminated
        message[sizeof(message) - 1] = 0;

        logger_callback(level, message);
    }
}

static quote3_error_t qcnl_error_to_ql_error(sgx_qcnl_error_t ret) {
    switch (ret) {
    case SGX_QCNL_SUCCESS:
        return SGX_QL_SUCCESS;
    case SGX_QCNL_UNEXPECTED_ERROR:
        return SGX_QL_ERROR_UNEXPECTED;
    case SGX_QCNL_INVALID_PARAMETER:
        return SGX_QL_ERROR_INVALID_PARAMETER;
    case SGX_QCNL_OUT_OF_MEMORY:
        return SGX_QL_ERROR_OUT_OF_MEMORY;
    case SGX_QCNL_NETWORK_ERROR:
    case SGX_QCNL_NETWORK_PROXY_FAIL:
    case SGX_QCNL_NETWORK_HOST_FAIL:
    case SGX_QCNL_NETWORK_COULDNT_CONNECT:
    case SGX_QCNL_NETWORK_HTTP2_ERROR:
    case SGX_QCNL_NETWORK_WRITE_ERROR:
    case SGX_QCNL_NETWORK_OPERATION_TIMEDOUT:
    case SGX_QCNL_NETWORK_HTTPS_ERROR:
    case SGX_QCNL_NETWORK_UNKNOWN_OPTION:
    case SGX_QCNL_NETWORK_INIT_ERROR:
        return SGX_QL_NETWORK_ERROR;
    case SGX_QCNL_MSG_ERROR:
        return SGX_QL_ERROR_MESSAGE_PARSING_ERROR;
    case SGX_QCNL_ERROR_STATUS_NO_CACHE_DATA:
        return SGX_QL_NO_QUOTE_COLLATERAL_DATA;
    case SGX_QCNL_ERROR_STATUS_PLATFORM_UNKNOWN:
        return SGX_QL_PLATFORM_UNKNOWN;
    case SGX_QCNL_ERROR_STATUS_CERTS_UNAVAILABLE:
        return SGX_QL_CERTS_UNAVAILABLE;
    case SGX_QCNL_ERROR_STATUS_UNEXPECTED:
        return SGX_QL_UNKNOWN_MESSAGE_RESPONSE;
    case SGX_QCNL_ERROR_STATUS_SERVICE_UNAVAILABLE:
        return SGX_QL_SERVICE_UNAVAILABLE;
    default:
        return SGX_QL_ERROR_UNEXPECTED;
    }
}

quote3_error_t sgx_ql_get_quote_config(const sgx_ql_pck_cert_id_t *p_cert_id, sgx_ql_config_t **pp_quote_config) {
    sgx_qcnl_error_t ret = sgx_qcnl_get_pck_cert_chain(p_cert_id, pp_quote_config);

    if (ret == SGX_QCNL_ERROR_STATUS_NO_CACHE_DATA) {
        qpl_log(SGX_QL_LOG_ERROR, "[QPL] No certificate data for this platform.\n");
        return SGX_QL_NO_PLATFORM_CERT_DATA;
    } else {
        if (ret != SGX_QCNL_SUCCESS)
            qpl_log(SGX_QL_LOG_ERROR, "[QPL] Failed to get quote config. Error code is 0x%04x\n", ret);
        return qcnl_error_to_ql_error(ret);
    }
}

quote3_error_t sgx_ql_free_quote_config(sgx_ql_config_t *p_quote_config) {
    sgx_qcnl_free_pck_cert_chain(p_quote_config);

    return SGX_QL_SUCCESS;
}

static quote3_error_t split_buffer(uint8_t *in_buf, uint16_t in_buf_size, char **__unaligned out_buf1, uint32_t *__unaligned out_buf1_size,
                                   char **__unaligned out_buf2, uint32_t *__unaligned out_buf2_size) {
    const string delimiter = "-----BEGIN CERTIFICATE-----";

    string s0((char *)in_buf, in_buf_size);
    size_t pos = s0.find(delimiter);
    if (pos == string::npos) {
        qpl_log(SGX_QL_LOG_ERROR, "[QPL] Invalid certificate chain.\n");
        return SGX_QL_MESSAGE_ERROR;
    }

    *out_buf1_size = (uint32_t)(pos);
    *out_buf1 = reinterpret_cast<char *>(malloc(*out_buf1_size));
    if (!(*out_buf1)) {
        qpl_log(SGX_QL_LOG_ERROR, "[QPL] Out of memory.\n");
        return SGX_QL_ERROR_OUT_OF_MEMORY;
    }
    if (memcpy_s(*out_buf1, pos, s0.data(), pos) != 0) {
        qpl_log(SGX_QL_LOG_ERROR, "[QPL] Unexpected error in memcpy_s.\n");
        free(*out_buf1);
        *out_buf1 = NULL;
        return SGX_QL_ERROR_UNEXPECTED;
    }

    *out_buf2_size = (uint32_t)(in_buf_size - pos);
    *out_buf2 = reinterpret_cast<char *>(malloc(*out_buf2_size));
    if (!(*out_buf2)) {
        qpl_log(SGX_QL_LOG_ERROR, "[QPL] Out of memory.\n");
        free(*out_buf1);
        *out_buf1 = NULL;
        return SGX_QL_ERROR_OUT_OF_MEMORY;
    }
    if (memcpy_s(*out_buf2, *out_buf2_size, s0.data() + pos, in_buf_size - pos) != 0) {
        qpl_log(SGX_QL_LOG_ERROR, "[QPL] Unexpected error in memcpy_s.\n");
        free(*out_buf1);
        free(*out_buf2);
        *out_buf1 = NULL;
        *out_buf2 = NULL;
        return SGX_QL_ERROR_UNEXPECTED;
    }

    return SGX_QL_SUCCESS;
}

quote3_error_t ql_get_quote_verification_collateral_internal(sgx_prod_type_t prod_type,
                                                             const uint8_t *fmspc,
                                                             const uint16_t fmspc_size,
                                                             const char *pck_ca,
                                                             const void* custom_param,
                                                             const uint16_t custom_param_length,
                                                             sgx_ql_qve_collateral_t **pp_quote_collateral) {
    if (fmspc == NULL || pck_ca == NULL || pp_quote_collateral == NULL ||
        (custom_param != NULL && custom_param_length == 0) ||
        (custom_param == NULL && custom_param_length != 0)) {
        qpl_log(SGX_QL_LOG_ERROR, "[QPL] Invalid parameter.\n");
        return SGX_QL_ERROR_INVALID_PARAMETER;
    }

    uint8_t *p_pck_crl_chain = NULL;
    uint16_t pck_crl_chain_size = 0;
    uint8_t *p_tcbinfo = NULL;
    uint16_t tcbinfo_size = 0;
    uint8_t *p_qe_identity = NULL;
    uint16_t qe_identity_size = 0;
    char *base64_string = NULL;

    sgx_qcnl_error_t qcnl_ret = SGX_QCNL_UNEXPECTED_ERROR;
    quote3_error_t ret = SGX_QL_ERROR_UNEXPECTED;

    do {
        // Allocate buffer
        *pp_quote_collateral = (sgx_ql_qve_collateral_t *)malloc(sizeof(sgx_ql_qve_collateral_t));
        if (!(*pp_quote_collateral)) {
            qpl_log(SGX_QL_LOG_ERROR, "[QPL] Out of memory.\n");
            return SGX_QL_ERROR_OUT_OF_MEMORY;
        }
        memset(*pp_quote_collateral, 0, sizeof(sgx_ql_qve_collateral_t));

        // Encode custom_param with Base64
        if (custom_param != NULL && custom_param_length != 0) {
            base64_string = base64_encode(reinterpret_cast<const char *>(custom_param), custom_param_length);
            if (!base64_string) {
                ret = SGX_QL_ERROR_OUT_OF_MEMORY;
                break;
            }
        }

        // Set version
        if (!sgx_qcnl_get_api_version(&(*pp_quote_collateral)->major_version, &(*pp_quote_collateral)->minor_version)) {
            qpl_log(SGX_QL_LOG_ERROR, "[QPL] QCNL is configured with an unsupported API version.\n");
            ret = SGX_QL_ERROR_UNEXPECTED;
            break;
        }

        // Set PCK CRL and certchain
        qcnl_ret = sgx_qcnl_get_pck_crl_chain(pck_ca, (uint16_t)strnlen(pck_ca, USHRT_MAX), base64_string, &p_pck_crl_chain, &pck_crl_chain_size);
        if (qcnl_ret != SGX_QCNL_SUCCESS) {
            qpl_log(SGX_QL_LOG_ERROR, "[QPL] Failed to get PCK CRL and certchain : 0x%04x\n", qcnl_ret);
            ret = qcnl_error_to_ql_error(qcnl_ret);
            break;
        }

        ret = split_buffer(p_pck_crl_chain, pck_crl_chain_size, &(*pp_quote_collateral)->pck_crl, &(*pp_quote_collateral)->pck_crl_size,
                           &(*pp_quote_collateral)->pck_crl_issuer_chain, &(*pp_quote_collateral)->pck_crl_issuer_chain_size);
        if (ret != SGX_QL_SUCCESS) {
            qpl_log(SGX_QL_LOG_ERROR, "[QPL] Failed to process PCK CRL.\n");
            break;
        }

        // Set TCBInfo and certchain
        if (prod_type == SGX_PROD_TYPE_SGX) {
            qcnl_ret = sgx_qcnl_get_tcbinfo(reinterpret_cast<const char *>(fmspc), fmspc_size, base64_string, &p_tcbinfo, &tcbinfo_size);
        } else {
            qcnl_ret = tdx_qcnl_get_tcbinfo(reinterpret_cast<const char *>(fmspc), fmspc_size, base64_string, &p_tcbinfo, &tcbinfo_size);
        } 

        if (qcnl_ret != SGX_QCNL_SUCCESS) {
            qpl_log(SGX_QL_LOG_ERROR, "[QPL] Failed to get TCBInfo : 0x%04x\n", qcnl_ret);
            ret = qcnl_error_to_ql_error(qcnl_ret);
            break;
        }

        ret = split_buffer(p_tcbinfo, tcbinfo_size, &(*pp_quote_collateral)->tcb_info, &(*pp_quote_collateral)->tcb_info_size,
                           &(*pp_quote_collateral)->tcb_info_issuer_chain, &(*pp_quote_collateral)->tcb_info_issuer_chain_size);
        if (ret != SGX_QL_SUCCESS) {
            qpl_log(SGX_QL_LOG_ERROR, "[QPL] Failed to process TCBInfo.\n");
            break;
        }

        // Set QEIdentity and certchain
        sgx_qe_type_t qe_type;
        if (prod_type == SGX_PROD_TYPE_SGX)
            qe_type = SGX_QE_TYPE_ECDSA;
        else qe_type = SGX_QE_TYPE_TD;

        qcnl_ret = sgx_qcnl_get_qe_identity(qe_type, base64_string, &p_qe_identity, &qe_identity_size);
        if (qcnl_ret != SGX_QCNL_SUCCESS) {
            qpl_log(SGX_QL_LOG_ERROR, "[QPL] Failed to get QE identity : 0x%04x\n", qcnl_ret);
            ret = qcnl_error_to_ql_error(qcnl_ret);
            break;
        }

        ret = split_buffer(p_qe_identity, qe_identity_size, &(*pp_quote_collateral)->qe_identity, &(*pp_quote_collateral)->qe_identity_size,
                           &(*pp_quote_collateral)->qe_identity_issuer_chain, &(*pp_quote_collateral)->qe_identity_issuer_chain_size);
        if (ret != SGX_QL_SUCCESS) {
            qpl_log(SGX_QL_LOG_ERROR, "[QPL] Failed to process QE identity.\n");
            break;
        }

        // The second certificate in the chain is root CA, so we skip the first cert
        string str_issuer_chain = (*pp_quote_collateral)->qe_identity_issuer_chain;
        size_t pos = str_issuer_chain.find(X509_DELIMITER, 1);
        if (pos == string::npos) {
            qpl_log(SGX_QL_LOG_ERROR, "[QPL] Failed to get root certificate.\n");
            break;
        }
        string root_ca_cdp_url = get_cdp_url_from_pem_cert(str_issuer_chain.substr(pos).c_str());
        if (root_ca_cdp_url.empty()) {
            qpl_log(SGX_QL_LOG_ERROR, "[QPL] Failed to get root CA CDP Point.\n");
            break;
        }

        // Set Root CA CRL
        qcnl_ret = sgx_qcnl_get_root_ca_crl(root_ca_cdp_url.c_str(), base64_string, reinterpret_cast<uint8_t **>(&(*pp_quote_collateral)->root_ca_crl),
                                            reinterpret_cast<uint16_t *>(&(*pp_quote_collateral)->root_ca_crl_size));
        if (qcnl_ret != SGX_QCNL_SUCCESS) {
            qpl_log(SGX_QL_LOG_ERROR, "[QPL] Failed to get root CA CRL : 0x%04x\n", qcnl_ret);
            ret = qcnl_error_to_ql_error(qcnl_ret);
            break;
        }
        // Add NULL terminator to Root CA CRL
        (*pp_quote_collateral)->root_ca_crl_size++;
        char *p_root_ca_crl = (char *)realloc((*pp_quote_collateral)->root_ca_crl, (*pp_quote_collateral)->root_ca_crl_size);
        if (p_root_ca_crl == NULL) {
            ret = SGX_QL_ERROR_OUT_OF_MEMORY;
            break;
        }
        (*pp_quote_collateral)->root_ca_crl = p_root_ca_crl;
        (*pp_quote_collateral)->root_ca_crl[(*pp_quote_collateral)->root_ca_crl_size - 1] = 0;

        ret = SGX_QL_SUCCESS;
    } while (0);

    sgx_qcnl_free_pck_crl_chain(p_pck_crl_chain);
    if (prod_type == SGX_PROD_TYPE_SGX) {
        sgx_qcnl_free_tcbinfo(p_tcbinfo);
    } else {
        tdx_qcnl_free_tcbinfo(p_tcbinfo);
    }
    sgx_qcnl_free_qe_identity(p_qe_identity);
    if (base64_string) {
        free(base64_string);
        base64_string = NULL;
    }

    if (ret != SGX_QL_SUCCESS) {
        sgx_ql_free_quote_verification_collateral(*pp_quote_collateral);
        *pp_quote_collateral = NULL;
    }

    return ret;
}

quote3_error_t ql_free_quote_verification_collateral_internal(sgx_ql_qve_collateral_t *p_quote_collateral) {
    if (p_quote_collateral) {
        if (p_quote_collateral->pck_crl_issuer_chain) {
            free(p_quote_collateral->pck_crl_issuer_chain);
            p_quote_collateral->pck_crl_issuer_chain = NULL;
        }
        if (p_quote_collateral->root_ca_crl) {
            free(p_quote_collateral->root_ca_crl);
            p_quote_collateral->root_ca_crl = NULL;
        }
        if (p_quote_collateral->pck_crl) {
            free(p_quote_collateral->pck_crl);
            p_quote_collateral->pck_crl = NULL;
        }
        if (p_quote_collateral->tcb_info_issuer_chain) {
            free(p_quote_collateral->tcb_info_issuer_chain);
            p_quote_collateral->tcb_info_issuer_chain = NULL;
        }
        if (p_quote_collateral->tcb_info) {
            free(p_quote_collateral->tcb_info);
            p_quote_collateral->tcb_info = NULL;
        }
        if (p_quote_collateral->qe_identity_issuer_chain) {
            free(p_quote_collateral->qe_identity_issuer_chain);
            p_quote_collateral->qe_identity_issuer_chain = NULL;
        }
        if (p_quote_collateral->qe_identity) {
            free(p_quote_collateral->qe_identity);
            p_quote_collateral->qe_identity = NULL;
        }
        free(p_quote_collateral);
        p_quote_collateral = NULL;
    }

    return SGX_QL_SUCCESS;
}

quote3_error_t sgx_ql_get_quote_verification_collateral(const uint8_t *fmspc, 
                                                        uint16_t fmspc_size, 
                                                        const char *pck_ca,
                                                        sgx_ql_qve_collateral_t **pp_quote_collateral) {
    quote3_error_t ret = ql_get_quote_verification_collateral_internal(SGX_PROD_TYPE_SGX,
                                                                       fmspc,
                                                                       fmspc_size,
                                                                       pck_ca,
                                                                       NULL,
                                                                       0,
                                                                       pp_quote_collateral);
    if (ret == SGX_QL_SUCCESS) {
        (*pp_quote_collateral)->tee_type = 0x0; // SGX
    } else {
        qpl_log(SGX_QL_LOG_ERROR, "[QPL] Failed to get SGX quote verification collateral : %d\n", ret);
    }
    return ret;
}

quote3_error_t sgx_ql_get_quote_verification_collateral_with_params(const uint8_t *fmspc,
                                                                    const uint16_t fmspc_size,
                                                                    const char *pck_ca,
                                                                    const void* custom_param,
                                                                    const uint16_t custom_param_length,
                                                                    sgx_ql_qve_collateral_t **pp_quote_collateral) {
    quote3_error_t ret = ql_get_quote_verification_collateral_internal(SGX_PROD_TYPE_SGX,
                                                                       fmspc,
                                                                       fmspc_size,
                                                                       pck_ca,
                                                                       custom_param,
                                                                       custom_param_length,
                                                                       pp_quote_collateral);
    if (ret == SGX_QL_SUCCESS) {
        (*pp_quote_collateral)->tee_type = 0x0; // SGX
    } else {
        qpl_log(SGX_QL_LOG_ERROR, "[QPL] Failed to get SGX quote verification collateral : %d\n", ret);
    }
    return ret;
}
quote3_error_t tdx_ql_get_quote_verification_collateral(const uint8_t *fmspc, uint16_t fmspc_size, const char *pck_ca,
                                                        sgx_ql_qve_collateral_t **pp_quote_collateral) {
    quote3_error_t ret = ql_get_quote_verification_collateral_internal(SGX_PROD_TYPE_TDX,
                                                                       fmspc,
                                                                       fmspc_size,
                                                                       pck_ca,
                                                                       NULL,
                                                                       0,
                                                                       pp_quote_collateral);
    if (ret == SGX_QL_SUCCESS) {
        (*pp_quote_collateral)->tee_type = 0x0; // SGX
    } else {
        qpl_log(SGX_QL_LOG_ERROR, "[QPL] Failed to get SGX quote verification collateral : %d\n", ret);
    }
    return ret;
}
quote3_error_t sgx_ql_free_quote_verification_collateral(sgx_ql_qve_collateral_t *p_quote_collateral) {
    return ql_free_quote_verification_collateral_internal(p_quote_collateral);
}

quote3_error_t tdx_ql_free_quote_verification_collateral(tdx_ql_qve_collateral_t *p_quote_collateral) {
    return ql_free_quote_verification_collateral_internal((tdx_ql_qve_collateral_t *)p_quote_collateral);
}

quote3_error_t sgx_ql_get_qve_identity(char **pp_qve_identity,
                                       uint32_t *p_qve_identity_size,
                                       char **pp_qve_identity_issuer_chain,
                                       uint32_t *p_qve_identity_issuer_chain_size) {
    sgx_qcnl_error_t ret = sgx_qcnl_get_qve_identity(NULL, pp_qve_identity, p_qve_identity_size, pp_qve_identity_issuer_chain, p_qve_identity_issuer_chain_size);

    if (ret != SGX_QCNL_SUCCESS) {
        qpl_log(SGX_QL_LOG_ERROR, "[QPL] Failed to get QVE identity. Error code is 0x%04x\n", ret);
    }

    if (ret == SGX_QCNL_ERROR_STATUS_NO_CACHE_DATA)
        return SGX_QL_NO_QVE_IDENTITY_DATA;
    else
        return qcnl_error_to_ql_error(ret);
}

quote3_error_t sgx_ql_free_qve_identity(char *p_qve_identity, char *p_qve_identity_issuer_chain) {
    sgx_qcnl_free_qve_identity(p_qve_identity, p_qve_identity_issuer_chain);

    return SGX_QL_SUCCESS;
}

quote3_error_t sgx_ql_get_root_ca_crl(uint8_t **pp_root_ca_crl, uint16_t *p_root_ca_crl_size) {
    uint8_t *p_qe_identity = NULL;
    uint16_t qe_identity_size = 0;
    quote3_error_t ret = SGX_QL_ERROR_UNEXPECTED;

    // Get QEIdentity and certchain
    sgx_qcnl_error_t qcnl_ret = sgx_qcnl_get_qe_identity(SGX_QE_TYPE_ECDSA, NULL, &p_qe_identity, &qe_identity_size);
    if (qcnl_ret != SGX_QCNL_SUCCESS) {
        qpl_log(SGX_QL_LOG_ERROR, "[QPL] Failed to get QE identity : 0x%04x\n", qcnl_ret);
        return qcnl_error_to_ql_error(qcnl_ret);
    }

    do {
        size_t pos;
        string str_qe_identity(reinterpret_cast<const char *>(p_qe_identity), qe_identity_size);
        pos = str_qe_identity.find(X509_DELIMITER);
        if (pos == string::npos) {
            qpl_log(SGX_QL_LOG_ERROR, "[QPL] Invalid QE identity.");
            break;
        }

        pos = str_qe_identity.find(X509_DELIMITER, pos + 1);
        if (pos == string::npos) {
            qpl_log(SGX_QL_LOG_ERROR, "[QPL] Invalid QE identity.");
            break;
        }

        string root_ca_cdp_url = get_cdp_url_from_pem_cert(str_qe_identity.substr(pos).c_str());
        if (root_ca_cdp_url.empty()) {
            qpl_log(SGX_QL_LOG_ERROR, "[QPL] Failed to get root CA CDP Point.\n");
            break;
        }

        // Set Root CA CRL
        qcnl_ret = sgx_qcnl_get_root_ca_crl(root_ca_cdp_url.c_str(), NULL, pp_root_ca_crl, p_root_ca_crl_size);
        if (qcnl_ret != SGX_QCNL_SUCCESS) {
            qpl_log(SGX_QL_LOG_ERROR, "[QPL] Failed to get root CA CRL : 0x%04x\n", qcnl_ret);
            ret = qcnl_error_to_ql_error(qcnl_ret);
            break;
        }

        ret = SGX_QL_SUCCESS;
    } while (0);

    sgx_qcnl_free_qe_identity(p_qe_identity);

    return ret;
}

quote3_error_t sgx_ql_free_root_ca_crl(uint8_t *p_root_ca_crl) {
    sgx_qcnl_free_root_ca_crl(p_root_ca_crl);

    return SGX_QL_SUCCESS;
}

quote3_error_t sgx_ql_set_logging_callback(sgx_ql_logging_callback_t logger) {
    logger_callback = logger;
    sgx_qcnl_set_logging_callback(logger);
    return SGX_QL_SUCCESS;
}
