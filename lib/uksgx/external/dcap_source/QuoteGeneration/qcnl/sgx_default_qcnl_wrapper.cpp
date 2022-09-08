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
 * File: sgx_default_qcnl_wrapper.cpp 
 *  
 * Description: SGX default PCK Collateral Network Library  
 *
 */

#include "sgx_default_qcnl_wrapper.h"
#include "pckcert_provider.h"
#include "qcnl_config.h"
#include "qcnl_def.h"
#include "qv_collateral_provider.h"
#include "sgx_pce.h"
#include "qcnl_util.h"
#include <cstdarg>
#include <stdexcept>

static sgx_ql_logging_callback_t logger_callback = nullptr;

void qcnl_log(sgx_ql_log_level_t level, const char *fmt, ...) {
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

/**
* This API gets PCK certificate chain and TCBm from PCCS server based on the information provided(QE_ID, TCBr, EncPPID, PCE_ID) 
* The buffer allocated by this function should be freed with sgx_qcnl_free_pck_cert_chain by the caller.
*
* @param p_pck_cert_id PCK cert identity information
* @param pp_quote_config Output buffer for quote configuration data
*
* @return SGX_QCNL_SUCCESS If the PCK certificate chain and TCBm was retrieved from PCCS server successfully.
*/
sgx_qcnl_error_t sgx_qcnl_get_pck_cert_chain(const sgx_ql_pck_cert_id_t *p_pck_cert_id,
                                             sgx_ql_config_t **pp_quote_config) {

    // Check input parameters
    if (p_pck_cert_id == NULL || pp_quote_config == NULL) {
        return SGX_QCNL_INVALID_PARAMETER;
    }
    if (p_pck_cert_id->p_qe3_id == NULL || p_pck_cert_id->qe3_id_size != consts::QE3_ID_SIZE) {
        return SGX_QCNL_INVALID_PARAMETER;
    }
    if (p_pck_cert_id->p_encrypted_ppid != NULL && p_pck_cert_id->encrypted_ppid_size != consts::ENC_PPID_SIZE) {
        // Allow ENCRYPTED_PPID to be NULL, but if it is not NULL, the size must match ENC_PPID_SIZE
        return SGX_QCNL_INVALID_PARAMETER;
    }
    if (p_pck_cert_id->p_platform_cpu_svn == NULL || p_pck_cert_id->p_platform_pce_isv_svn == NULL) {
        return SGX_QCNL_INVALID_PARAMETER;
    }
    if (p_pck_cert_id->crypto_suite != PCE_ALG_RSA_OAEP_3072) {
        return SGX_QCNL_INVALID_PARAMETER;
    }

    PckCertProvider pckCertProvider;
    return pckCertProvider.get_pck_cert_chain(p_pck_cert_id, pp_quote_config);
}

/**
* This API frees the buffer allocated by sgx_qcnl_get_pck_cert_chain
*/
void sgx_qcnl_free_pck_cert_chain(sgx_ql_config_t *p_quote_config) {
    if (p_quote_config) {
        if (p_quote_config->p_cert_data) {
            free(p_quote_config->p_cert_data);
            p_quote_config->p_cert_data = NULL;
        }
        memset(p_quote_config, 0, sizeof(sgx_ql_config_t));
        free(p_quote_config);
        p_quote_config = NULL;
    }
}

/**
* This API gets CRL certificate chain from PCCS server. The p_crl_chain buffer allocated by this API
* must be freed with sgx_qcnl_free_pck_crl_chain upon success.
*
* @param ca Currently only "platform" or "processor"
* @param ca_size Size of the ca buffer
* @param p_crl_chain Output buffer for CRL certificate chain
* @param p_crl_chain_size Size of CRL certificate chain
*
* @return SGX_QCNL_SUCCESS If the CRL certificate chain was retrieved from PCCS server successfully.
*/
sgx_qcnl_error_t sgx_qcnl_get_pck_crl_chain(const char *ca,
                                            uint16_t ca_size,
                                            const char* custom_param_b64_string,
                                            uint8_t **p_crl_chain,
                                            uint16_t *p_crl_chain_size) {
    // Check input parameters
    (void)ca_size; // UNUSED
    if (p_crl_chain == NULL || p_crl_chain_size == NULL) {
        return SGX_QCNL_INVALID_PARAMETER;
    }
    if (ca == NULL || (strcmp(ca, consts::CA_PLATFORM) != 0 && strcmp(ca, consts::CA_PROCESSOR) != 0)) {
        return SGX_QCNL_INVALID_PARAMETER;
    }

    QvCollateralProvider qvCollateralProvider(custom_param_b64_string);
    return qvCollateralProvider.get_pck_crl_chain(ca, ca_size, p_crl_chain, p_crl_chain_size);
}

/**
* This API frees the p_crl_chain buffer allocated by sgx_qcnl_get_pck_crl_chain
*/
void sgx_qcnl_free_pck_crl_chain(uint8_t *p_crl_chain) {
    if (p_crl_chain) {
        free(p_crl_chain);
        p_crl_chain = NULL;
    }
}

/**
* This API gets TCB information from PCCS server. The p_tcbinfo buffer allocated by this API
* must be freed with sgx_qcnl_free_tcbinfo upon success.
*
* @param prod_type SGX or TDX
* @param fmspc Family-Model-Stepping value
* @param fmspc_size Size of the fmspc buffer
* @param p_tcbinfo Output buffer for TCB information
* @param p_tcbinfo_size Size of TCB information
*
* @return SGX_QCNL_SUCCESS If the TCB information was retrieved from PCCS server successfully.
*/
sgx_qcnl_error_t qcnl_get_tcbinfo_internal(sgx_prod_type_t prod_type,
                                           const char *fmspc,
                                           uint16_t fmspc_size,
                                           const char* custom_param_b64_string,
                                           uint8_t **p_tcbinfo,
                                           uint16_t *p_tcbinfo_size) {
    // Check input parameters
    // fmspc is always 6 bytes
    if (p_tcbinfo == NULL || p_tcbinfo_size == NULL) {
        return SGX_QCNL_INVALID_PARAMETER;
    }
    if (fmspc == NULL || fmspc_size != consts::FMSPC_SIZE) {
        return SGX_QCNL_INVALID_PARAMETER;
    }

    QvCollateralProvider qvCollateralProvider(custom_param_b64_string);
    return qvCollateralProvider.get_tcbinfo(prod_type, fmspc, fmspc_size, p_tcbinfo, p_tcbinfo_size);
}

sgx_qcnl_error_t sgx_qcnl_get_tcbinfo(const char *fmspc,
                                      uint16_t fmspc_size,
                                      const char* custom_param_b64_string,
                                      uint8_t **p_tcbinfo,
                                      uint16_t *p_tcbinfo_size) {
    return qcnl_get_tcbinfo_internal(SGX_PROD_TYPE_SGX,
                                     fmspc,
                                     fmspc_size,
                                     custom_param_b64_string,
                                     p_tcbinfo,
                                     p_tcbinfo_size);
}

sgx_qcnl_error_t tdx_qcnl_get_tcbinfo(const char *fmspc,
                                      uint16_t fmspc_size,
                                      const char* custom_param_b64_string,
                                      uint8_t **p_tcbinfo,
                                      uint16_t *p_tcbinfo_size) {
    return qcnl_get_tcbinfo_internal(SGX_PROD_TYPE_TDX,
                                     fmspc,
                                     fmspc_size,
                                     custom_param_b64_string,
                                     p_tcbinfo,
                                     p_tcbinfo_size);
}

/**
* This API frees the p_tcbinfo buffer allocated by sgx_qcnl_get_tcbinfo
*/
void sgx_qcnl_free_tcbinfo(uint8_t *p_tcbinfo) {
    if (p_tcbinfo) {
        free(p_tcbinfo);
        p_tcbinfo = NULL;
    }
}

/**
* This API frees the p_tcbinfo buffer allocated by tdx_qcnl_get_tcbinfo
*/
void tdx_qcnl_free_tcbinfo(uint8_t *p_tcbinfo) {
    if (p_tcbinfo) {
        free(p_tcbinfo);
    }
}

/**
* This API gets QE identity from PCCS server. The p_qe_identity buffer allocated by this API
* must be freed with sgx_qcnl_free_qe_identity upon success.
*
* @param qe_type Type of QE
* @param p_qe_identity Output buffer for QE identity
* @param p_qe_identity_size Size of QE identity
*
* @return SGX_QCNL_SUCCESS If the QE identity was retrieved from PCCS server successfully.
*/
sgx_qcnl_error_t sgx_qcnl_get_qe_identity(sgx_qe_type_t qe_type,
                                          const char* custom_param_b64_string,
                                          uint8_t **p_qe_identity,
                                          uint16_t *p_qe_identity_size) {
    // Check input parameters
    if (p_qe_identity == NULL || p_qe_identity_size == NULL ) {
        return SGX_QCNL_INVALID_PARAMETER;
    }

    QvCollateralProvider qvCollateralProvider(custom_param_b64_string);
    return qvCollateralProvider.get_qe_identity(qe_type, p_qe_identity, p_qe_identity_size);
}

/**
* This API frees the p_qe_identity buffer allocated by sgx_qcnl_get_qe_identity
*/
void sgx_qcnl_free_qe_identity(uint8_t *p_qe_identity) {
    if (p_qe_identity) {
        free(p_qe_identity);
        p_qe_identity = NULL;
    }
}

/**
* This API gets QvE identity from PCCS server. The pp_qve_identity and pp_qve_identity_issuer_chain buffer allocated by this API
* must be freed with sgx_qcnl_free_qve_identity upon success.
*
* @param pp_qve_identity Output buffer for QvE identity
* @param p_qve_identity_size Size of QvE identity
* @param pp_qve_identity_issuer_chain Output buffer for QvE identity certificate chain
* @param p_qve_identity_issuer_chain_size Size of QvE identity certificate chain
*
* @return SGX_QCNL_SUCCESS If the QvE identity was retrieved from PCCS server successfully.
*/
sgx_qcnl_error_t sgx_qcnl_get_qve_identity(const char* custom_param_b64_string,
                                           char **pp_qve_identity,
                                           uint32_t *p_qve_identity_size,
                                           char **pp_qve_identity_issuer_chain,
                                           uint32_t *p_qve_identity_issuer_chain_size) {
    // Check input parameters
    if (pp_qve_identity == NULL || p_qve_identity_size == NULL || pp_qve_identity_issuer_chain == NULL || p_qve_identity_issuer_chain_size == NULL) {
        return SGX_QCNL_INVALID_PARAMETER;
    }

    QvCollateralProvider qvCollateralProvider(custom_param_b64_string);
    return qvCollateralProvider.get_qve_identity(pp_qve_identity, p_qve_identity_size, pp_qve_identity_issuer_chain, p_qve_identity_issuer_chain_size);
}

/**
* This API frees the p_qve_identity and p_qve_identity_issuer_chain buffer allocated by sgx_qcnl_get_qve_identity
*/
void sgx_qcnl_free_qve_identity(char *p_qve_identity, char *p_qve_identity_issuer_chain) {
    if (p_qve_identity) {
        free(p_qve_identity);
        p_qve_identity = NULL;
    }
    if (p_qve_identity_issuer_chain) {
        free(p_qve_identity_issuer_chain);
        p_qve_identity_issuer_chain = NULL;
    }
}

/**
* This API gets Root CA CRL from PCCS server. The p_root_ca_crl buffer allocated by this API
* must be freed with sgx_qcnl_free_root_ca_crl upon success.
*
* @param root_ca_cdp_url The url of root CA CRL
* @param p_root_ca_crl Output buffer for Root CA CRL 
* @param p_root_ca_cal_size Size of Root CA CRL
*
* @return SGX_QCNL_SUCCESS If the Root CA CRL was retrieved from PCCS server successfully.
*/
sgx_qcnl_error_t sgx_qcnl_get_root_ca_crl(const char *root_ca_cdp_url,
                                          const char* custom_param_b64_string,
                                          uint8_t **p_root_ca_crl,
                                          uint16_t *p_root_ca_crl_size) {
    // Check input parameters
    if (root_ca_cdp_url == NULL || p_root_ca_crl == NULL || p_root_ca_crl_size == NULL) {
        return SGX_QCNL_INVALID_PARAMETER;
    }

    QvCollateralProvider qvCollateralProvider(custom_param_b64_string);
    return qvCollateralProvider.get_root_ca_crl(root_ca_cdp_url, p_root_ca_crl, p_root_ca_crl_size);
}

/**
* This API frees the p_root_ca_crl buffer allocated by sgx_qcnl_get_root_ca_crl
*/
void sgx_qcnl_free_root_ca_crl(uint8_t *p_root_ca_crl) {
    if (p_root_ca_crl) {
        free(p_root_ca_crl);
        p_root_ca_crl = NULL;
    }
}

/**
 * This function returns the collateral version.
 */
bool sgx_qcnl_get_api_version(uint16_t *p_major_ver, uint16_t *p_minor_ver) {
    string version = QcnlConfig::Instance()->getCollateralVersion();
    if (!version.empty()) {
        auto pos = version.find(".");
        if (pos != std::string::npos) {
            auto s_major = version.substr(0, pos);
            auto s_minor = version.substr(pos + 1);
            try {
                string::size_type sz;
                *p_major_ver = (uint16_t)stoi(s_major, &sz);
                *p_minor_ver = (uint16_t)stoi(s_minor, &sz);
                return true;
            } catch (const invalid_argument &e) {
                qcnl_log(SGX_QL_LOG_ERROR, "[QCNL] Failed to parse version string : %s", e.what());
                return false;
            }
        } else {
            return false;
        }
    }

    string url = QcnlConfig::Instance()->getCollateralServiceUrl();
    if (url.find("/v3/") != std::string::npos) {
        *p_major_ver = 3;
        *p_minor_ver = 0;
    } else if (url.find("/v4/") != std::string::npos) {
        *p_major_ver = 4;
        *p_minor_ver = 0;
    } else {
        return false;
    }

    return true;
}

sgx_qcnl_error_t sgx_qcnl_set_logging_callback(sgx_ql_logging_callback_t logger) {
    logger_callback = logger;
    return SGX_QCNL_SUCCESS;
}
