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
 * File: MPSynchronicSender.cpp
 *   
 * Description: Linux specific implementation for the MPSynchronicSender 
 * class to send network requests.	 
 */
#include <curl/curl.h>
#include <sstream>
#include <cstring>
#include <unistd.h>

#include "network_logger.h"
#include "MPSynchronicSender.h"

#define ERROR_CODE_STR_RESPONSE_HEADER "Error-Code: "
#define NETWORK_RETRY_COUNT 12 //this service will try 1 minute(12*5 =60s) if network is not ready.

using namespace std;

struct Buffer {
  uint8_t *buff;
  uint16_t size;
  uint16_t pos;
};

LogLevel mpNetworkLoglevel;
 
static size_t responseCallback(void *b, size_t size, size_t nmemb, void *useptr) {
    uint16_t chunkSize = (uint16_t)(size*nmemb);
    struct Buffer *mem = (struct Buffer*)useptr;

    if (mem->pos + chunkSize < chunkSize) {
        network_log_message_aux(mpNetworkLoglevel, MP_REG_LOG_LEVEL_ERROR, "Integer overflow happens during processing reponse.\n");
        return 0;
    }

    if (mem->pos + chunkSize > mem->size) {
        network_log_message_aux(mpNetworkLoglevel, MP_REG_LOG_LEVEL_ERROR, "Resposne is too big for internal buffer, required size: %d, available size: %d\n", 
            mem->pos + chunkSize, mem->size);
        return 0;
    }

    memcpy(&(mem->buff[mem->pos]), b, chunkSize);
    mem->pos = (uint16_t)(mem->pos+chunkSize);

    return chunkSize;
}

static size_t responseHeaderCallBack(char* b, size_t size, size_t nitems, void *userdata) {
    size_t numbytes = size * nitems;
    string strline = string(b, numbytes);
    string *errorCodeStr = (string*)userdata;
    if (numbytes > 2) {
        network_log_message_aux(mpNetworkLoglevel, MP_REG_LOG_LEVEL_INFO, "%.*s", numbytes, b);
    }

    size_t found = strline.find(ERROR_CODE_STR_RESPONSE_HEADER);
    if (found != std::string::npos) {
        size_t begin = string(ERROR_CODE_STR_RESPONSE_HEADER).length();
        size_t end = strline.find('\r');
        *errorCodeStr = strline.substr(begin, end - begin);
    }
    return numbytes;
}

MpResult MPSynchronicSender::sendBinaryRequest(const string& serverURL, const string& subscriptionKey, 
    const uint8_t *request, const uint16_t requestSize, 
    uint8_t *response, uint16_t &responseSize, 
    HttpStatusCode& http_response_code, string& errorCodeStr) {
    MpResult res = MP_NETWORK_ERROR;
    CURL *curl = NULL;
    struct curl_slist* header_list = NULL;
    CURLcode cret = CURLE_OK;
    char errbuf[CURL_ERROR_SIZE] = {0};
    long response_code = 0;
    uint8_t internalBuff[MAX_RESPONSE_SIZE];
    struct Buffer responseBuff;
    uint8_t retry = NETWORK_RETRY_COUNT;

    responseBuff.buff = internalBuff;
    responseBuff.size = MAX_RESPONSE_SIZE;
    responseBuff.pos = 0;
    
    mpNetworkLoglevel = m_logLevel;

    cret = curl_global_init(CURL_GLOBAL_ALL);
    if (CURLE_OK != cret) {
        network_log_message(MP_REG_LOG_LEVEL_ERROR, "curl_global_init failed with the error: %d\n", cret);
        res = MP_UNEXPECTED_ERROR;
        goto out;
    }

    curl = curl_easy_init();
    if(!curl) {
        network_log_message(MP_REG_LOG_LEVEL_ERROR, "curl_easy_init failed.\n");
        res = MP_UNEXPECTED_ERROR;
        goto out;
    }

    cret = curl_easy_setopt(curl, CURLOPT_URL, serverURL.c_str());
    if (CURLE_OK != cret) {
        network_log_message(MP_REG_LOG_LEVEL_ERROR, "curl_easy_setopt for url failed with the error: %d\n", cret);
        res = MP_UNEXPECTED_ERROR;
        goto out;
    }
    
    /* provide a buffer to store errors */
    cret = curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errbuf);
    if (CURLE_OK != cret) {
        network_log_message(MP_REG_LOG_LEVEL_ERROR, "curl_easy_setopt for errbuf failed with the error: %d\n", cret);
        res = MP_UNEXPECTED_ERROR;
        goto out;
    }

    /* set the error buffer as empty buffer */
    errbuf[0] = 0;

    /* Default value is strict certificate check (1L).  Disable check by using 0L. */
    cret = curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    if (CURLE_OK != cret) {
        network_log_message(MP_REG_LOG_LEVEL_ERROR, "curl_easy_setopt for certificate check failed with the error: %d\n", cret);
        res = MP_UNEXPECTED_ERROR;
        goto out;
    }

    
    /* Default value is strict hostname check (2L).  Disable check by using 0L. */
    cret = curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    if (CURLE_OK != cret) {
        network_log_message(MP_REG_LOG_LEVEL_ERROR, "curl_easy_setopt for hostname check failed with the error: %d\n", cret);
        res = MP_UNEXPECTED_ERROR;
        goto out;
    }
    /* Verbose output sent to sdterr */
    cret = curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);
    if (CURLE_OK != cret) {
        network_log_message(MP_REG_LOG_LEVEL_ERROR, "curl_easy_setopt for verbose logging failed with the error: %d\n", cret);
        res = MP_UNEXPECTED_ERROR;
        goto out;
    }
    
    cret = curl_easy_setopt(curl, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_2);
    if (CURLE_OK != cret) {
        network_log_message(MP_REG_LOG_LEVEL_ERROR, "curl_easy_setopt for TLS 1.2 failed with the error: %d\n", cret);
        res = MP_UNEXPECTED_ERROR;
        goto out;
    }

    cret = curl_easy_setopt(curl, CURLOPT_REDIR_PROTOCOLS, CURLPROTO_HTTP | CURLPROTO_HTTPS);
    if (CURLE_OK != cret) {
        network_log_message(MP_REG_LOG_LEVEL_ERROR, "curl_easy_setopt for setting protocols, error: %d\n", cret);
        res = MP_UNEXPECTED_ERROR;
        goto out;
    }

    /* configure proxy settings */
    switch (m_proxy.proxy_type) {
        case MP_REG_PROXY_TYPE_DEFAULT_PROXY:
            //curl auto detects https_proxy environment variable
            network_log_message(MP_REG_LOG_LEVEL_INFO, "Using default/environment proxy settings.\n");
            //network_log_message(MP_REG_LOG_LEVEL_INFO, "https_proxy = %s. http_proxy = %s\n", getenv("https_proxy"), getenv("http_proxy"));
        break;
        case MP_REG_PROXY_TYPE_MANUAL_PROXY:
            cret = curl_easy_setopt(curl, CURLOPT_PROXY, m_proxy.proxy_url);
            if (CURLE_OK != cret) {
                network_log_message(MP_REG_LOG_LEVEL_ERROR, "curl_easy_setopt for proxy settings failed with the error: %d\n", cret);
                res = MP_UNEXPECTED_ERROR;
                goto out;
            }
            network_log_message(MP_REG_LOG_LEVEL_INFO, "Using manual proxy settings, proxy url: %s\n", m_proxy.proxy_url);
        break;
        default:
            if (MP_REG_PROXY_TYPE_DIRECT_ACCESS != m_proxy.proxy_type) {
                network_log_message(MP_REG_LOG_LEVEL_ERROR, "Unrecognized proxy type, using no proxy. %d\n", m_proxy.proxy_type);
            }
            cret = curl_easy_setopt(curl, CURLOPT_NOPROXY, "*");
            if (CURLE_OK != cret) {
                network_log_message(MP_REG_LOG_LEVEL_ERROR, "curl_easy_setopt for proxy settings failed with the error: %d\n", cret);
                res = MP_UNEXPECTED_ERROR;
                goto out;
            }
        break;
    }
 
    /* Add the custom HTTP header */
    header_list = curl_slist_append(header_list, "Content-Type: application/octet-stream");
    if (!header_list) {
        network_log_message(MP_REG_LOG_LEVEL_ERROR, "curl_slist_append for setting Content-Type header failed with the error: %d\n", cret);
        res = MP_UNEXPECTED_ERROR;
        goto out;
    }

    if (subscriptionKey.length() == SUBSCRIPTION_KEY_SIZE) {
        header_list = curl_slist_append(header_list, (string("Ocp-Apim-Subscription-Key: ") + subscriptionKey).c_str());
        if (!header_list) {
            network_log_message(MP_REG_LOG_LEVEL_ERROR, "curl_slist_append for setting subscription key header failed with the error: %d\n", cret);
            res = MP_UNEXPECTED_ERROR;
            goto out;
        }
    }

    /* Set the header */
    cret = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header_list);
    if (CURLE_OK != cret) {
        network_log_message(MP_REG_LOG_LEVEL_ERROR, "curl_easy_setopt for setting the header failed with the error: %d\n", cret);
        res = MP_UNEXPECTED_ERROR;
        goto out;
    }
 
    /* Write response header to responseHeaderCallBack */
    cret = curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, responseHeaderCallBack);
    if (CURLE_OK != cret) {
        network_log_message(MP_REG_LOG_LEVEL_ERROR, "curl_easy_setopt for setting response header callback failed with the error: %d\n", cret);
        res = MP_UNEXPECTED_ERROR;
        goto out;
    }

    /* Pass string pointer for the response error code to the header parsing callback*/
    cret = curl_easy_setopt(curl, CURLOPT_HEADERDATA, &errorCodeStr);
    if (CURLE_OK != cret) {
        network_log_message(MP_REG_LOG_LEVEL_ERROR, "curl_easy_setopt for setting string pointer to the response header callback failed with the error: %d\n", cret);
        res = MP_UNEXPECTED_ERROR;
        goto out;
    }

    /* Send all response data to responseHeaderCallBack  */ 
    cret = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, responseCallback);
    if (CURLE_OK != cret) {
        network_log_message(MP_REG_LOG_LEVEL_ERROR, "curl_easy_setopt for setting write callback failed with the error: %d\n", cret);
        res = MP_UNEXPECTED_ERROR;
        goto out;
    }

    /* Write the response to this stream instead of stdout */ 
    cret = curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBuff);
    if (CURLE_OK != cret) {
        network_log_message(MP_REG_LOG_LEVEL_ERROR, "curl_easy_setopt for setting write data callback failed with the error: %d\n", cret);
        res = MP_UNEXPECTED_ERROR;
        goto out;
    }

    network_log_message(MP_REG_LOG_LEVEL_INFO, "Request size: %d\n", requestSize);
    /* Set the request size */
    cret = curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, requestSize);
    if (CURLE_OK != cret) {
        network_log_message(MP_REG_LOG_LEVEL_ERROR, "curl_easy_setopt for setting the request size failed with the error: %d\n", cret);
        goto out;
    }

    /* Set the request */
    cret = curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request);
    if (CURLE_OK != cret) {
        network_log_message(MP_REG_LOG_LEVEL_ERROR, "curl_easy_setopt for setting the request failed with the error: %d\n", cret);
        goto out;
    }
    
    /* this service will be started ASAS when OS power on, it is possible that network is not ready, so we will retry network */
    do {    
        /* Send request and get response */
        cret = curl_easy_perform(curl);
	if (CURLE_OK == cret) {
            break;
        } else if (CURLE_COULDNT_RESOLVE_HOST == cret) {
            retry--;
            if (0 == retry) {
                network_log_message(MP_REG_LOG_LEVEL_ERROR, "curl_easy_perform failed with the error: CURLE_COULDNT_RESOLVE_HOST.");
                goto out;
            } else {
                network_log_message(MP_REG_LOG_LEVEL_INFO, "curl_easy_perform failed with CURLE_COULDNT_RESOLVE_HOST... retrying in 5 seconds\n");
                sleep(5);
                continue;
            }
	} else {
            if (CURLE_WRITE_ERROR == cret) {
                res = MP_MEM_ERROR;
            }
            network_log_message(MP_REG_LOG_LEVEL_ERROR, "curl_easy_perform failed with the error: %d\n", cret);
            goto out;
        }
    }while (retry > 0);

    /* Get the HTTP response code */
    cret = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
    if (CURLE_OK != cret) {
        network_log_message(MP_REG_LOG_LEVEL_ERROR, "curl_easy_getinfo failed with the error: %d\n", cret);
        goto out;
    }

    if ((0 < responseBuff.pos) && (response)) {
        if (responseBuff.pos > responseSize) {
            network_log_message(MP_REG_LOG_LEVEL_ERROR, "Reponse buffer too small for pending response, required size: %d.", responseBuff.pos);
            res = MP_USER_INSUFFICIENT_MEM;
            goto out;
        } else {
            memcpy(response, responseBuff.buff, responseBuff.pos);
        }
    }

    http_response_code = (HttpStatusCode)response_code;
    res = MP_SUCCESS;
out:
    responseSize = responseBuff.pos;

    if (CURLE_OK != cret) {
        size_t len = strnlen(errbuf, CURL_ERROR_SIZE);
        if (len) {
            network_log_message(MP_REG_LOG_LEVEL_ERROR, "Detailed curl error: %s%s", errbuf,
                ((errbuf[len - 1] != '\n') ? "\n" : ""));
        }
        else {
            network_log_message(MP_REG_LOG_LEVEL_ERROR, "Readable curl error: %s\n", curl_easy_strerror(cret));
        }
        network_log_message(MP_REG_LOG_LEVEL_INFO, "libcurl version: %s\n", curl_version());
    }
    if (header_list) {
        curl_slist_free_all(header_list);
    }
    if (curl) {
        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();
    return res;
}
