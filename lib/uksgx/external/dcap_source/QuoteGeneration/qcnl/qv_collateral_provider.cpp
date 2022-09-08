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
 * File: qv_collateral_provider.cpp
 *
 * Description: Quote verification collateral provider
 *
 */
#include "qv_collateral_provider.h"
#include "network_wrapper.h"
#include "pccs_response_object.h"
#include "qcnl_config.h"
#include "se_memcpy.h"
#include "qcnl_util.h"
#include <regex>

#define FREE_RESP_BUFFER(resp_msg, resp_header) \
    if (resp_msg) {                             \
        free(resp_msg);                         \
        resp_msg = NULL;                        \
    }                                           \
    if (resp_header) {                          \
        free(resp_header);                      \
        resp_header = NULL;                     \
    }

QvCollateralProvider::QvCollateralProvider() {
}

QvCollateralProvider::QvCollateralProvider(const char *custom_param) {
    if (custom_param)
        custom_param_ = custom_param;
}

QvCollateralProvider::~QvCollateralProvider() {
}

string QvCollateralProvider::get_custom_param_string() {
    if (custom_param_.empty())
        return "";

    // custom_param_ is BASE64 encoded string, so we need to escape '+','/','='
    string s = custom_param_;
    s = regex_replace(s, regex("\\+"), "%2B");
    s = regex_replace(s, regex("\\/"), "%2F");
    s = regex_replace(s, regex("\\="), "%3D");
    return "customParameters=" + s;
}

sgx_qcnl_error_t QvCollateralProvider::build_pck_crl_url(const char *ca, string &url) {
    // initialize https request url
    url = QcnlConfig::Instance()->getCollateralServiceUrl();

    // Append ca and encoding
    url.append("pckcrl?ca=").append(ca);
    if (is_collateral_service_pcs() || QcnlConfig::Instance()->getCollateralVersion() == "3.1") {
        url.append("&encoding=der");
    }
    if (!custom_param_.empty()) {
        url.append("&").append(get_custom_param_string());
    }

    qcnl_log(SGX_QL_LOG_INFO, "[QCNL] Request URL %s \n", url.c_str());

    return SGX_QCNL_SUCCESS;
}

sgx_qcnl_error_t QvCollateralProvider::get_pck_crl_chain(const char *ca,
                                                         uint16_t ca_size,
                                                         uint8_t **p_crl_chain,
                                                         uint16_t *p_crl_chain_size) {
    (void)ca_size; // UNUSED
    // initialize https request url
    string url("");
    sgx_qcnl_error_t ret = build_pck_crl_url(ca, url);
    if (ret != SGX_QCNL_SUCCESS) {
        return ret;
    }

    char *resp_msg = NULL, *resp_header = NULL;
    uint32_t resp_size = 0, header_size = 0;
    map<string, string> header_map;

    ret = qcnl_https_request(url.c_str(), header_map, NULL, 0, NULL, 0, &resp_msg, resp_size, &resp_header, header_size);
    if (ret != SGX_QCNL_SUCCESS) {
        return ret;
    } else if (!resp_msg || resp_size == 0) {
        return SGX_QCNL_UNEXPECTED_ERROR;
    }

    PckCrlResponseObject pckcrl_resp_obj;
    pckcrl_resp_obj.set_raw_header(resp_header, header_size).set_raw_body(resp_msg, resp_size);

    do {
        string certchain = pckcrl_resp_obj.get_pckcrl_issuer_chain();
        string crl = pckcrl_resp_obj.get_pckcrl();
        if (certchain.empty() || crl.empty()) {
            ret = SGX_QCNL_MSG_ERROR;
            break;
        }

        certchain = unescape(certchain);

        // Always append a NULL terminator to CRL and certchain
        *p_crl_chain_size = (uint16_t)(certchain.size() + crl.size() + 2);
        *p_crl_chain = (uint8_t *)malloc(*p_crl_chain_size);
        if (*p_crl_chain == NULL) {
            ret = SGX_QCNL_OUT_OF_MEMORY;
            break;
        }

        // set certchain (crl || ('\0) || intermediateCA || root CA || '\0')
        uint8_t *ptr = *p_crl_chain;
        if (memcpy_s(ptr, crl.size(), crl.data(), crl.size()) != 0) {
            ret = SGX_QCNL_UNEXPECTED_ERROR;
            break;
        }
        ptr += crl.size();
        *ptr++ = '\0'; // add NULL terminator

        if (memcpy_s(ptr, certchain.size(), certchain.data(), certchain.size()) != 0) {
            ret = SGX_QCNL_UNEXPECTED_ERROR;
            break;
        }
        ptr += certchain.size();
        *ptr = '\0'; // add NULL terminator

        ret = SGX_QCNL_SUCCESS;
    } while (0);

    if (ret != SGX_QCNL_SUCCESS) {
        sgx_qcnl_free_pck_crl_chain(*p_crl_chain);
    }

    FREE_RESP_BUFFER(resp_msg, resp_header)

    return ret;
}

sgx_qcnl_error_t QvCollateralProvider::build_tcbinfo_url(sgx_prod_type_t prod_type,
                                                         const char *fmspc,
                                                         uint16_t fmspc_size,
                                                         string &url) {
    // initialize https request url
    url = QcnlConfig::Instance()->getCollateralServiceUrl();

    if (prod_type == SGX_PROD_TYPE_TDX) {
        auto found = url.find("/sgx/");
        if (found != std::string::npos) {
            url = url.replace(found, 5, "/tdx/");
        } else {
            return SGX_QCNL_UNEXPECTED_ERROR;
        }
    } 

    // Append fmspc
    url.append("tcb?fmspc=");
    if (!concat_string_with_hex_buf(url, reinterpret_cast<const uint8_t *>(fmspc), fmspc_size)) {
        return SGX_QCNL_UNEXPECTED_ERROR;
    }
    if (!custom_param_.empty()) {
        url.append("&").append(get_custom_param_string());
    }

    qcnl_log(SGX_QL_LOG_INFO, "[QCNL] Request URL %s \n", url.c_str());

    return SGX_QCNL_SUCCESS;
}

sgx_qcnl_error_t QvCollateralProvider::get_tcbinfo(sgx_prod_type_t prod_type,
                                                   const char *fmspc,
                                                   uint16_t fmspc_size,
                                                   uint8_t **p_tcbinfo,
                                                   uint16_t *p_tcbinfo_size) {
    string url("");
    sgx_qcnl_error_t ret = build_tcbinfo_url(prod_type, fmspc, fmspc_size, url);
    if (ret != SGX_QCNL_SUCCESS) {
        return ret;
    }

    char *resp_msg = NULL, *resp_header = NULL;
    uint32_t resp_size = 0, header_size = 0;
    map<string, string> header_map;

    ret = qcnl_https_request(url.c_str(), header_map, NULL, 0, NULL, 0, &resp_msg, resp_size, &resp_header, header_size);
    if (ret != SGX_QCNL_SUCCESS) {
        return ret;
    }

    TcbInfoResponseObject tcbinfo_resp_obj;
    tcbinfo_resp_obj.set_raw_header(resp_header, header_size).set_raw_body(resp_msg, resp_size);

    do {
        string certchain = tcbinfo_resp_obj.get_tcbinfo_issuer_chain();
        string tcbinfo = tcbinfo_resp_obj.get_tcbinfo();
        if (certchain.empty() || tcbinfo.empty()) {
            ret = SGX_QCNL_MSG_ERROR;
            break;
        }

        certchain = unescape(certchain);

        *p_tcbinfo_size = (uint16_t)(certchain.size() + tcbinfo.size() + 2);
        *p_tcbinfo = (uint8_t *)malloc(*p_tcbinfo_size);
        if (*p_tcbinfo == NULL) {
            ret = SGX_QCNL_OUT_OF_MEMORY;
            break;
        }

        // set certchain (tcbinfo || '\0' || signingCA || root CA || '\0')
        if (memcpy_s(*p_tcbinfo, *p_tcbinfo_size, tcbinfo.data(), tcbinfo.size()) != 0) {
            ret = SGX_QCNL_UNEXPECTED_ERROR;
            break;
        }
        (*p_tcbinfo)[tcbinfo.size()] = '\0'; // add NULL terminator
        if (memcpy_s(*p_tcbinfo + tcbinfo.size() + 1, certchain.size(), certchain.data(), certchain.size()) != 0) {
            ret = SGX_QCNL_UNEXPECTED_ERROR;
            break;
        }
        (*p_tcbinfo)[*p_tcbinfo_size - 1] = '\0'; // add NULL terminator

        ret = SGX_QCNL_SUCCESS;
    } while (0);

    if (ret != SGX_QCNL_SUCCESS) {
        sgx_qcnl_free_tcbinfo(*p_tcbinfo);
    }

    FREE_RESP_BUFFER(resp_msg, resp_header)

    return ret;
}

sgx_qcnl_error_t QvCollateralProvider::build_qeidentity_url(sgx_qe_type_t qe_type, string &url) {
    url = QcnlConfig::Instance()->getCollateralServiceUrl();

    if (qe_type == SGX_QE_TYPE_TD) {
        auto found = url.find("/sgx/");
        if (found != std::string::npos) {
            url = url.replace(found, 5, "/tdx/");
        } else {
            return SGX_QCNL_UNEXPECTED_ERROR;
        }
    } 

    url.append("qe/identity");
    if (!custom_param_.empty()) {
        url.append("?").append(get_custom_param_string());
    }

    qcnl_log(SGX_QL_LOG_INFO, "[QCNL] Request URL %s \n", url.c_str());

    return SGX_QCNL_SUCCESS;
}

sgx_qcnl_error_t QvCollateralProvider::get_qe_identity(sgx_qe_type_t qe_type,
                                                       uint8_t **p_qe_identity,
                                                       uint16_t *p_qe_identity_size) {
    (void)qe_type;
    // initialize https request url
    string url("");
    sgx_qcnl_error_t ret = build_qeidentity_url(qe_type, url);
    if (ret != SGX_QCNL_SUCCESS) {
        return ret;
    }

    char *resp_msg = NULL, *resp_header = NULL;
    uint32_t resp_size = 0, header_size = 0;
    map<string, string> header_map;

    ret = qcnl_https_request(url.c_str(), header_map, NULL, 0, NULL, 0, &resp_msg, resp_size, &resp_header, header_size);
    if (ret != SGX_QCNL_SUCCESS) {
        return ret;
    }

    QeIdentityResponseObject qe_identity_resp_obj;
    qe_identity_resp_obj.set_raw_header(resp_header, header_size).set_raw_body(resp_msg, resp_size);

    do {
        string certchain = qe_identity_resp_obj.get_enclave_id_issuer_chain();
        string qeidentity = qe_identity_resp_obj.get_qeidentity();
        if (certchain.empty() || qeidentity.empty()) {
            ret = SGX_QCNL_MSG_ERROR;
            break;
        }

        certchain = unescape(certchain);

        *p_qe_identity_size = (uint16_t)(certchain.size() + qeidentity.size() + 2);
        *p_qe_identity = (uint8_t *)malloc(*p_qe_identity_size);
        if (*p_qe_identity == NULL) {
            ret = SGX_QCNL_OUT_OF_MEMORY;
            break;
        }

        // set certchain (QE identity || '\0' || signingCA || root CA || '\0')
        if (memcpy_s(*p_qe_identity, *p_qe_identity_size, qeidentity.data(), qeidentity.size()) != 0) {
            ret = SGX_QCNL_UNEXPECTED_ERROR;
            break;
        }
        (*p_qe_identity)[qeidentity.size()] = '\0'; // add NULL terminator
        if (memcpy_s(*p_qe_identity + qeidentity.size() + 1, certchain.size(), certchain.data(), certchain.size()) != 0) {
            ret = SGX_QCNL_UNEXPECTED_ERROR;
            break;
        }
        (*p_qe_identity)[*p_qe_identity_size - 1] = '\0'; // add NULL terminator

        ret = SGX_QCNL_SUCCESS;
    } while (0);

    if (ret != SGX_QCNL_SUCCESS) {
        sgx_qcnl_free_qe_identity(*p_qe_identity);
    }

    FREE_RESP_BUFFER(resp_msg, resp_header)

    return ret;
}

sgx_qcnl_error_t QvCollateralProvider::build_qveidentity_url(string &url) {
    url = QcnlConfig::Instance()->getCollateralServiceUrl();
    url.append("qve/identity");
    if (!custom_param_.empty()) {
        url.append("?").append(get_custom_param_string());
    }

    qcnl_log(SGX_QL_LOG_INFO, "[QCNL] Request URL %s \n", url.c_str());

    return SGX_QCNL_SUCCESS;
}

sgx_qcnl_error_t QvCollateralProvider::get_qve_identity(char **pp_qve_identity,
                                                        uint32_t *p_qve_identity_size,
                                                        char **pp_qve_identity_issuer_chain,
                                                        uint32_t *p_qve_identity_issuer_chain_size) {
    string url("");
    sgx_qcnl_error_t ret = build_qveidentity_url(url);
    if (ret != SGX_QCNL_SUCCESS) {
        return ret;
    }

    *pp_qve_identity = NULL;
    *pp_qve_identity_issuer_chain = NULL;

    char *resp_msg = NULL, *resp_header = NULL;
    uint32_t resp_size = 0, header_size = 0;
    map<string, string> header_map;

    ret = qcnl_https_request(url.c_str(), header_map, NULL, 0, NULL, 0, &resp_msg, resp_size, &resp_header, header_size);
    if (ret != SGX_QCNL_SUCCESS) {
        return ret;
    }

    QveIdentityResponseObject qve_identity_resp_obj;
    qve_identity_resp_obj.set_raw_header(resp_header, header_size).set_raw_body(resp_msg, resp_size);

    do {
        string certchain = qve_identity_resp_obj.get_enclave_id_issuer_chain();
        string qveidentity = qve_identity_resp_obj.get_qveidentity();
        if (certchain.empty() || qveidentity.empty()) {
            ret = SGX_QCNL_MSG_ERROR;
            break;
        }

        certchain = unescape(certchain);

        // allocate buffers
        *p_qve_identity_size = (uint32_t)qveidentity.size() + 1;
        *pp_qve_identity = (char *)malloc(*p_qve_identity_size);
        if (*pp_qve_identity == NULL) {
            ret = SGX_QCNL_OUT_OF_MEMORY;
            break;
        }
        *p_qve_identity_issuer_chain_size = (uint32_t)(certchain.size() + 1);
        *pp_qve_identity_issuer_chain = (char *)malloc(*p_qve_identity_issuer_chain_size);
        if (*pp_qve_identity_issuer_chain == NULL) {
            ret = SGX_QCNL_OUT_OF_MEMORY;
            break;
        }

        // set QvE identity
        if (memcpy_s(*pp_qve_identity, *p_qve_identity_size, qveidentity.data(), qveidentity.size()) != 0) {
            ret = SGX_QCNL_UNEXPECTED_ERROR;
            break;
        }
        (*pp_qve_identity)[*p_qve_identity_size - 1] = '\0'; // add NULL terminator

        // set certchain (signingCA || root CA)
        if (memcpy_s(*pp_qve_identity_issuer_chain, *p_qve_identity_issuer_chain_size, certchain.data(), certchain.size()) != 0) {
            ret = SGX_QCNL_UNEXPECTED_ERROR;
            break;
        }
        (*pp_qve_identity_issuer_chain)[*p_qve_identity_issuer_chain_size - 1] = '\0'; // add NULL terminator

        ret = SGX_QCNL_SUCCESS;
    } while (0);

    if (ret != SGX_QCNL_SUCCESS) {
        sgx_qcnl_free_qve_identity(*pp_qve_identity, *pp_qve_identity_issuer_chain);
    }

    FREE_RESP_BUFFER(resp_msg, resp_header)

    return ret;
}

sgx_qcnl_error_t QvCollateralProvider::build_root_ca_crl_url(const char *root_ca_cdp_url, string &url) {
    url = QcnlConfig::Instance()->getCollateralServiceUrl();
    // Append url
    if (!is_collateral_service_pcs()) {
        if (QcnlConfig::Instance()->getCollateralVersion() == "3.0") {
            // For PCCS API version 3.0, will call API /rootcacrl, and it will return HEX encoded CRL
            url.append("rootcacrl");
            if (!custom_param_.empty()) {
                url.append("?").append(get_custom_param_string());
            }
        } else if (QcnlConfig::Instance()->getCollateralVersion() == "3.1") {
            // For PCCS API version 3.0, will call API /crl, and it will return raw DER buffer
            url.append("crl?uri=").append(root_ca_cdp_url);
            if (!custom_param_.empty()) {
                url.append("&").append(get_custom_param_string());
            }
        } else {
            return SGX_QCNL_INVALID_CONFIG;
        }
    }

    return SGX_QCNL_SUCCESS;
}

sgx_qcnl_error_t QvCollateralProvider::get_root_ca_crl(const char *root_ca_cdp_url,
                                                       uint8_t **p_root_ca_crl,
                                                       uint16_t *p_root_ca_crl_size) {
    // initialize https request url
    string url("");
    sgx_qcnl_error_t ret = build_root_ca_crl_url(root_ca_cdp_url, url);
    if (ret != SGX_QCNL_SUCCESS) {
        return ret;
    }

    char *resp_msg = NULL, *resp_header = NULL;
    uint32_t resp_size = 0, header_size = 0;
    map<string, string> header_map;

    if (is_collateral_service_pcs()) {
        qcnl_log(SGX_QL_LOG_INFO, "[QCNL] Request URL %s \n", root_ca_cdp_url);
        ret = qcnl_https_request(root_ca_cdp_url, header_map, NULL, 0, NULL, 0, &resp_msg, resp_size, &resp_header, header_size);
    } else {
        qcnl_log(SGX_QL_LOG_INFO, "[QCNL] Request URL %s \n", url.c_str());
        ret = qcnl_https_request(url.c_str(), header_map, NULL, 0, NULL, 0, &resp_msg, resp_size, &resp_header, header_size);
    }

    if (ret != SGX_QCNL_SUCCESS) {
        return ret;
    }

    PccsResponseObject pccs_resp_obj;
    pccs_resp_obj.set_raw_body(resp_msg, resp_size);

    do {
        string root_ca_crl = pccs_resp_obj.get_raw_body();

        *p_root_ca_crl_size = (uint16_t)root_ca_crl.size();
        *p_root_ca_crl = (uint8_t *)malloc(*p_root_ca_crl_size);
        if (*p_root_ca_crl == NULL) {
            ret = SGX_QCNL_OUT_OF_MEMORY;
            break;
        }

        // set Root CA CRL
        if (memcpy_s(*p_root_ca_crl, *p_root_ca_crl_size, root_ca_crl.data(), root_ca_crl.size()) != 0) {
            ret = SGX_QCNL_UNEXPECTED_ERROR;
            break;
        }

        ret = SGX_QCNL_SUCCESS;
    } while (0);

    if (ret != SGX_QCNL_SUCCESS) {
        sgx_qcnl_free_root_ca_crl(*p_root_ca_crl);
    }

    FREE_RESP_BUFFER(resp_msg, resp_header)

    return ret;
}
