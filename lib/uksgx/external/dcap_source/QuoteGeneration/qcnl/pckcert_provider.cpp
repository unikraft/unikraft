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
 * File: pckcert_provider.cpp 
 *  
 * Description: PCK certificates provider
 *
 */
#include "pckcert_provider.h"
#include "local_cache.h"
#include "network_wrapper.h"
#include "pccs_response_object.h"

PckCertProvider::PckCertProvider() {
}

PckCertProvider::~PckCertProvider() {
}

sgx_qcnl_error_t PckCertProvider::build_pckcert_url(const string &base_url,
                                                    const sgx_ql_pck_cert_id_t *p_pck_cert_id,
                                                    string &url) {
    sgx_qcnl_error_t ret = SGX_QCNL_UNEXPECTED_ERROR;

    url = base_url;

    // Append QE ID
    url.append("pckcert?qeid=");
    if (!concat_string_with_hex_buf(url, p_pck_cert_id->p_qe3_id, p_pck_cert_id->qe3_id_size)) {
        return ret;
    }

    // Append encrypted PPID
    url.append("&encrypted_ppid=");
    if (p_pck_cert_id->p_encrypted_ppid == NULL) {
        uint8_t enc_ppid_unused[consts::ENC_PPID_SIZE] = {0};
        if (!concat_string_with_hex_buf(url, (const uint8_t *)&enc_ppid_unused, sizeof(enc_ppid_unused))) {
            return ret;
        }
    } else {
        if (!concat_string_with_hex_buf(url, p_pck_cert_id->p_encrypted_ppid, p_pck_cert_id->encrypted_ppid_size)) {
            return ret;
        }
    }

    // Append cpusvn
    url.append("&cpusvn=");
    if (!concat_string_with_hex_buf(url, reinterpret_cast<const uint8_t *>(p_pck_cert_id->p_platform_cpu_svn), sizeof(sgx_cpu_svn_t))) {
        return ret;
    }

    // Append pcesvn
    url.append("&pcesvn=");
    if (!concat_string_with_hex_buf(url, reinterpret_cast<const uint8_t *>(p_pck_cert_id->p_platform_pce_isv_svn), sizeof(sgx_isv_svn_t))) {
        return ret;
    }

    // Append pceid
    url.append("&pceid=");
    if (!concat_string_with_hex_buf(url, reinterpret_cast<const uint8_t *>(&p_pck_cert_id->pce_id), sizeof(p_pck_cert_id->pce_id))) {
        return ret;
    }

    // Custom request parameters
    Document &custom_options = QcnlConfig::Instance()->getCustomRequestOptions();
    if (!custom_options.IsNull() && custom_options.HasMember("get_cert") && custom_options["get_cert"].IsObject()) {
        if (custom_options["get_cert"].HasMember("params")) {
            Value &params = custom_options["get_cert"]["params"];
            if (params.IsObject()) {
                Value::ConstMemberIterator it = params.MemberBegin();
                while (it != params.MemberEnd()) {
                    if (it->value.IsString()) {
                        string key = it->name.GetString();
                        string value = it->value.GetString();
                        url.append("&").append(key).append("=").append(value);
                    }
                    it++;
                }
            }
        }
    }

    qcnl_log(SGX_QL_LOG_INFO, "[QCNL] Request URL %s \n", url.c_str());

    return SGX_QCNL_SUCCESS;
}

sgx_qcnl_error_t PckCertProvider::get_pck_cert_chain_from_url(const string &pck_url,
                                                              const sgx_ql_pck_cert_id_t *p_pck_cert_id,
                                                              sgx_ql_config_t **pp_quote_config) {
    string url("");
    char *resp_msg = NULL;
    uint32_t resp_size = 0;
    char *resp_header = NULL;
    uint32_t header_size = 0;
    sgx_qcnl_error_t ret = SGX_QCNL_UNEXPECTED_ERROR;
    map<string, string> header_map;

    // initialize https request url
    ret = build_pckcert_url(pck_url, p_pck_cert_id, url);
    if (ret != SGX_QCNL_SUCCESS) {
        return ret;
    }

    // Custom request headers
    Document &custom_options = QcnlConfig::Instance()->getCustomRequestOptions();
    if (!custom_options.IsNull() && custom_options.HasMember("get_cert") && custom_options["get_cert"].IsObject()) {
        if (custom_options["get_cert"].HasMember("headers")) {
            Value &headers = custom_options["get_cert"]["headers"];
            if (headers.IsObject()) {
                Value::ConstMemberIterator it = headers.MemberBegin();
                while (it != headers.MemberEnd()) {
                    if (it->value.IsString()) {
                        string key = it->name.GetString();
                        string value = it->value.GetString();
                        header_map.insert(pair<string, string>(key, value));
                    }
                    it++;
                }
            }
        }
    }

    ret = qcnl_https_request(url.c_str(), header_map, NULL, 0, NULL, 0, &resp_msg, resp_size, &resp_header, header_size);
    if (ret != SGX_QCNL_SUCCESS) {
        return ret;
    }

    PckCertResponseObject pckcert_resp_obj;
    pckcert_resp_obj.set_raw_header(resp_header, header_size).set_raw_body(resp_msg, resp_size);

    do {
        // Get TCBm , Issuer Chain, PCK certificate from response
        string tcbm = pckcert_resp_obj.get_tcbm();
        string certchain = pckcert_resp_obj.get_pckcert_issuer_chain();
        string pck_cert = pckcert_resp_obj.get_pckcert();
        if (tcbm.size() != (consts::CPUSVN_SIZE + consts::PCESVN_SIZE) * 2 || certchain.empty() || pck_cert.empty()) {
            ret = SGX_QCNL_MSG_ERROR;
            break;
        }

        certchain = unescape(certchain);

        // allocate output buffer
        *pp_quote_config = (sgx_ql_config_t *)malloc(sizeof(sgx_ql_config_t));
        if (*pp_quote_config == NULL) {
            ret = SGX_QCNL_OUT_OF_MEMORY;
            break;
        }
        memset(*pp_quote_config, 0, sizeof(sgx_ql_config_t));

        // set version
        (*pp_quote_config)->version = SGX_QL_CONFIG_VERSION_1;

        // set tcbm
        if (!hex_string_to_byte_array(reinterpret_cast<const uint8_t *>(tcbm.data()),
                                      consts::CPUSVN_SIZE * 2,
                                      reinterpret_cast<uint8_t *>(&(*pp_quote_config)->cert_cpu_svn),
                                      sizeof(sgx_cpu_svn_t))) {
            ret = SGX_QCNL_MSG_ERROR;
            break;
        }
        if (!hex_string_to_byte_array(reinterpret_cast<const uint8_t *>(tcbm.data() + consts::CPUSVN_SIZE * 2),
                                      consts::PCESVN_SIZE * 2,
                                      reinterpret_cast<uint8_t *>(&(*pp_quote_config)->cert_pce_isv_svn),
                                      sizeof(sgx_isv_svn_t))) {
            ret = SGX_QCNL_MSG_ERROR;
            break;
        }

        // set certchain (leaf cert || intermediateCA || root CA)
        (*pp_quote_config)->cert_data_size = (uint32_t)(certchain.size() + pck_cert.size());
        (*pp_quote_config)->p_cert_data = (uint8_t *)malloc((*pp_quote_config)->cert_data_size);
        if (!(*pp_quote_config)->p_cert_data) {
            ret = SGX_QCNL_OUT_OF_MEMORY;
            break;
        }
        if (memcpy_s((*pp_quote_config)->p_cert_data, (*pp_quote_config)->cert_data_size, pck_cert.data(), pck_cert.size()) != 0) {
            ret = SGX_QCNL_UNEXPECTED_ERROR;
            break;
        }
        if (memcpy_s((*pp_quote_config)->p_cert_data + pck_cert.size(), certchain.size(), certchain.data(), certchain.size()) != 0) {
            ret = SGX_QCNL_UNEXPECTED_ERROR;
            break;
        }

        ret = SGX_QCNL_SUCCESS;
    } while (0);

    if (ret != SGX_QCNL_SUCCESS) {
        sgx_qcnl_free_pck_cert_chain(*pp_quote_config);
    }
    if (resp_msg) {
        free(resp_msg);
        resp_msg = NULL;
    }
    if (resp_header) {
        free(resp_header);
        resp_header = NULL;
    }

    return ret;
}

sgx_qcnl_error_t PckCertProvider::get_pck_cert_chain(const sgx_ql_pck_cert_id_t *p_pck_cert_id,
                                                     sgx_ql_config_t **pp_quote_config) {
    sgx_qcnl_error_t ret = SGX_QCNL_ERROR_STATUS_UNEXPECTED;

    if (!QcnlConfig::Instance()->getLocalPckUrl().empty()) {
        qcnl_log(SGX_QL_LOG_INFO, "[QCNL] Try local service... \n");
        // Will get PCK certificates from LOCAL_PCK_URL
        ret = this->get_pck_cert_chain_from_url(QcnlConfig::Instance()->getLocalPckUrl(),
                                                 p_pck_cert_id, pp_quote_config);
        if (ret == SGX_QCNL_SUCCESS)
            return ret;

        qcnl_log(SGX_QL_LOG_INFO, "[QCNL] Failed to retrieve PCK certchain from local PCK service. \n");
    }

    // try memory cache next
    if (QcnlConfig::Instance()->getCacheExpireHour() > 0) {
        qcnl_log(SGX_QL_LOG_INFO, "[QCNL] Try memory cache... \n");
        if (LocalMemCache::Instance().get_pck_cert_chain(p_pck_cert_id, pp_quote_config)) {
            qcnl_log(SGX_QL_LOG_INFO, "[QCNL] Retrieved PCK certchain from memory cache successfully. \n");
            return SGX_QCNL_SUCCESS;
        }
    }

    // Failover to remote service
    if (!QcnlConfig::Instance()->getServerUrl().empty()) {
        qcnl_log(SGX_QL_LOG_INFO, "[QCNL] Try remote service... \n");
        // Try to fetch PCK certificates from PCCS
        ret = this->get_pck_cert_chain_from_url(QcnlConfig::Instance()->getServerUrl(),
                                                p_pck_cert_id, pp_quote_config);
        if (ret == SGX_QCNL_SUCCESS && QcnlConfig::Instance()->getCacheExpireHour() > 0) {
            // Update local cache
            LocalMemCache::Instance().set_pck_cert_chain(p_pck_cert_id, pp_quote_config);
        }
    }

    return ret;
}
