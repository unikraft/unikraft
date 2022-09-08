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
 * File: network_wrapper.cpp
 *
 * Description: Network access logic
 *
 */

#include "network_wrapper.h"
#include "qcnl_config.h"
#include "se_memcpy.h"
#include "sgx_default_qcnl_wrapper.h"
#include <curl/curl.h>
#include <unistd.h>

typedef struct _network_malloc_info_t {
    char *base;
    uint32_t size;
} network_malloc_info_t;

static size_t write_callback(void *ptr, size_t size, size_t nmemb, void *stream) {
    network_malloc_info_t *s = reinterpret_cast<network_malloc_info_t *>(stream);
    uint32_t start = 0;
    if (s->base == NULL) {
        s->base = reinterpret_cast<char *>(malloc(size * nmemb));
        s->size = static_cast<uint32_t>(size * nmemb);
        if (s->base == NULL)
            return 0;
    } else {
        uint32_t newsize = s->size + static_cast<uint32_t>(size * nmemb);
        char *p = reinterpret_cast<char *>(realloc(s->base, newsize));
        if (p == NULL) {
            return 0;
        }
        start = s->size;
        s->base = p;
        s->size = newsize;
    }
    if (memcpy_s(s->base + start, s->size - start, ptr, size * nmemb) != 0) {
        return 0;
    }
    return size * nmemb;
}

/**
 * This method converts CURL error codes to QCNL error codes
 *
 * @param curl_error Curl library error codes
 *
 * @return Collateral Network Library Error Codes
 */
static sgx_qcnl_error_t curl_error_to_qcnl_error(CURLcode curl_error) {
    switch (curl_error) {
    case CURLE_OK:
        return SGX_QCNL_SUCCESS;
    case CURLE_COULDNT_RESOLVE_PROXY:
        return SGX_QCNL_NETWORK_PROXY_FAIL;
    case CURLE_COULDNT_RESOLVE_HOST:
        return SGX_QCNL_NETWORK_HOST_FAIL;
    case CURLE_COULDNT_CONNECT:
        return SGX_QCNL_NETWORK_COULDNT_CONNECT;
    case CURLE_WRITE_ERROR:
        return SGX_QCNL_NETWORK_WRITE_ERROR;
    case CURLE_OPERATION_TIMEDOUT:
        return SGX_QCNL_NETWORK_OPERATION_TIMEDOUT;
    case CURLE_SSL_CONNECT_ERROR:
        return SGX_QCNL_NETWORK_HTTPS_ERROR;
    case CURLE_UNKNOWN_OPTION:
        return SGX_QCNL_NETWORK_UNKNOWN_OPTION;
    case CURLE_PEER_FAILED_VERIFICATION:
        return SGX_QCNL_NETWORK_HTTPS_ERROR;
    default:
        return SGX_QCNL_NETWORK_ERROR;
    }
}

/**
 * This method converts PCCS HTTP status codes to QCNL error codes
 *
 * @param pccs_status_code PCCS HTTP status codes
 *
 * @return Collateral Network Library Error Codes
 */
static sgx_qcnl_error_t pccs_status_to_qcnl_error(long pccs_status_code) {
    switch (pccs_status_code) {
    case 200: // PCCS_STATUS_SUCCESS
        return SGX_QCNL_SUCCESS;
    case 403:
        return SGX_QCNL_NETWORK_ERROR;
    case 404: // PCCS_STATUS_NO_CACHE_DATA
        return SGX_QCNL_ERROR_STATUS_NO_CACHE_DATA;
    case 461: // PCCS_STATUS_PLATFORM_UNKNOWN
        return SGX_QCNL_ERROR_STATUS_PLATFORM_UNKNOWN;
    case 462: // PCCS_STATUS_CERTS_UNAVAILABLE
        return SGX_QCNL_ERROR_STATUS_CERTS_UNAVAILABLE;
    case 503: // PCCS_STATUS_SERVICE_UNAVAILABLE
        return SGX_QCNL_ERROR_STATUS_SERVICE_UNAVAILABLE;
    default:
        return SGX_QCNL_ERROR_STATUS_UNEXPECTED;
    }
}

/**
 * This method calls curl library to perform https POST request with raw body in JSON format and returns response body and header
 *
 * @param url HTTPS GET/POST URL
 * @param req_body Request body in raw JSON format. For GET request it should be NULL.
 * @param req_body_size Size of request body. For GET request it should be 0.
 * @param user_token user token to access PCCS v3/platforms API. For GET request it should be NULL.
 * @param user_token_size Size of user token. For GET request it should be 0.
 * @param resp_msg Output buffer of response body
 * @param resp_size Size of response body
 * @param resp_header Output buffer of response header
 * @param header_size Size of response header
 *
 * @return SGX_QCNL_SUCCESS Call https post successfully. Other return codes indicate an error occured.
 */
sgx_qcnl_error_t qcnl_https_request(const char *url,
                                    http_header_map &header_map,
                                    const char *req_body,
                                    uint32_t req_body_size,
                                    const uint8_t *user_token,
                                    uint16_t user_token_size,
                                    char **resp_msg,
                                    uint32_t &resp_size,
                                    char **resp_header,
                                    uint32_t &header_size) {
    CURL *curl = NULL;
    CURLcode curl_ret = CURLE_OK;
    sgx_qcnl_error_t ret = SGX_QCNL_NETWORK_ERROR;
    network_malloc_info_t res_header = {0, 0};
    network_malloc_info_t res_body = {0, 0};
    struct curl_slist *headers = NULL;

    do {
        curl = curl_easy_init();
        if (!curl)
            break;

        if (curl_easy_setopt(curl, CURLOPT_URL, url) != CURLE_OK)
            break;

        // append user token
        if (user_token && user_token_size > 0) {
            if ((headers = curl_slist_append(headers, "Content-Type: application/json")) == NULL)
                break;

            std::string user_token_header("user-token: ");
            user_token_header.append(reinterpret_cast<const char *>(user_token), user_token_size);
            if ((headers = curl_slist_append(headers, user_token_header.c_str())) == NULL)
                break;
        }

        // add custom headers
        http_header_map::iterator it = header_map.begin();
        while (it != header_map.end()) {
            string key = it->first;
            string value = it->second;
            string headerline = key + ": " + value;
            headers = curl_slist_append(headers, headerline.c_str());
            it++;
        }

        // set header
        if (curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers) != CURLE_OK)
            break;

        if (req_body && req_body_size > 0) {
            // using CURLOPT_POSTFIELDS implies setting CURLOPT_POST to 1.
            if (curl_easy_setopt(curl, CURLOPT_POST, 1) != CURLE_OK)
                break;
            // size of the POST data
            if (curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)req_body_size) != CURLE_OK)
                break;
            // pass in a pointer to the data - libcurl will not copy
            if (curl_easy_setopt(curl, CURLOPT_POSTFIELDS, req_body) != CURLE_OK)
                break;
        }

        if (!QcnlConfig::Instance()->is_server_secure()) {
            // if not set this option, the below error code will be returned for self signed cert
            // CURLE_SSL_CACERT (60) Peer certificate cannot be authenticated with known CA certificates.
            if (curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L) != CURLE_OK)
                break;
            // if not set this option, the below error code will be returned for self signed cert
            // // CURLE_PEER_FAILED_VERIFICATION (51) The remote server's SSL certificate or SSH md5 fingerprint was deemed not OK.
            if (curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L) != CURLE_OK)
                break;
        }

        // Set write callback functions
        if (curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback) != CURLE_OK)
            break;
        if (curl_easy_setopt(curl, CURLOPT_WRITEDATA, reinterpret_cast<void *>(&res_body)) != CURLE_OK)
            break;

        // Set header callback functions
        if (curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, write_callback) != CURLE_OK)
            break;
        if (curl_easy_setopt(curl, CURLOPT_HEADERDATA, reinterpret_cast<void *>(&res_header)) != CURLE_OK)
            break;

        uint32_t retry_times = QcnlConfig::Instance()->getRetryTimes() + 1;
        uint32_t retry_delay = QcnlConfig::Instance()->getRetryDelay();
        uint32_t current_delay_time = 1; // wait 1 second before first retry
        do {
            // Perform request
            bool need_retry = false;
            long http_code = 0;
            curl_ret = curl_easy_perform(curl);

            if (curl_ret == CURLE_OK) {
                curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
                qcnl_log(SGX_QL_LOG_INFO, "[QCNL] HTTP status code: %ld \n", http_code);
                if (http_code == 200) {
                    break;
                } else if (http_code == 503) { // SERVICE_UNAVAILABLE
                    need_retry = true;
                }
            } else if (curl_ret == CURLE_OPERATION_TIMEDOUT || curl_ret == CURLE_COULDNT_RESOLVE_HOST || curl_ret == CURLE_COULDNT_RESOLVE_PROXY || curl_ret == CURLE_COULDNT_CONNECT || curl_ret == CURLE_HTTP_RETURNED_ERROR) {
                need_retry = true;
            }

            retry_times--;
            if (need_retry && retry_times > 0) {
                if (retry_delay != 0)
                    sleep(retry_delay);
                else {
                    sleep(current_delay_time);
                    current_delay_time *= 2;
                }
                continue;
            } else {
                if (curl_ret != CURLE_OK)
                    ret = curl_error_to_qcnl_error(curl_ret);
                else
                    ret = pccs_status_to_qcnl_error(http_code);
                goto cleanup;
            }
        } while (true);

        *resp_msg = res_body.base;
        resp_size = res_body.size;
        *resp_header = res_header.base;
        header_size = res_header.size;

        ret = SGX_QCNL_SUCCESS;

    } while (0);

cleanup:
    if (curl) {
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
    }
    if (ret != SGX_QCNL_SUCCESS) {
        if (res_body.base) {
            free(res_body.base);
        }
        if (res_header.base) {
            free(res_header.base);
        }
    }

    return ret;
}
