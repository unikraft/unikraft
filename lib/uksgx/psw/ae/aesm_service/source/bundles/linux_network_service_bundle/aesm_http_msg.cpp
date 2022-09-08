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


#include <string>
#include "aesm_encode.h"
#include "oal.h"
#include "se_wrapper.h"
#include "prof_fun.h"
#include "aesm_proxy_type.h"
#include "aesm_network.h"
#include "aesm_config.h"
#include "util.h"

#include <curl/curl.h>
#ifndef INTERNET_DEFAULT_HTTP_PORT
#define INTERNET_DEFAULT_HTTP_PORT 80
#endif

#define AESM_DEFAULT_CONN_TIME_OUT 1000
#define AESM_DEFAULT_TIME_OUT 10000

#define REQUEST_ID ("Request-ID")


typedef struct _network_malloc_info_t{
    char *base;
    uint32_t size;
}network_malloc_info_t;

static size_t write_callback(void *ptr, size_t size, size_t nmemb, void *stream)
{
    network_malloc_info_t* s=reinterpret_cast<network_malloc_info_t *>(stream);
    uint32_t start=0;
    if(s->base==NULL){
        if(UINT32_MAX/size<nmemb){
              return 0;//buffer overflow
        }
        s->base = reinterpret_cast<char *>(malloc(size*nmemb));
        s->size = static_cast<uint32_t>(size*nmemb);
        if(s->base==NULL){
            AESM_DBG_ERROR("malloc error in write callback fun");
            return 0;
        }
    }else{
        uint32_t newsize = s->size+static_cast<uint32_t>(size*nmemb);
        if((UINT32_MAX-s->size)/size<nmemb){
            free(s->base);
            s->base = NULL;
            return 0;//buffer overflow
        }
        char *p=reinterpret_cast<char *>(malloc(newsize));
        if(p == NULL){
            free(s->base);
            s->base = NULL;
            AESM_DBG_ERROR("malloc error in write callback fun");
            return 0;
        }
        memcpy_s(p, newsize, s->base, s->size);
        free(s->base);
        start = s->size;
        s->base = p;
        s->size = newsize;
    }
    memcpy_s(s->base +start, s->size-start, ptr, size*nmemb);
    return nmemb;
}

static ae_error_t http_network_init(CURL **curl, const char *url, bool is_ocsp)
{
    CURLcode cc = CURLE_OK;
    UNUSED(is_ocsp);
    AESM_DBG_TRACE("http init for url %s",url);

    if(NULL == url){
        AESM_DBG_ERROR("NULL url");
        return AE_FAILURE;
    }
    std::string url_path = url;
    aesm_config_infos_t urls = {0};
    if(false == read_aesm_config(urls)){
        return AE_FAILURE;
    }

    *curl = curl_easy_init();
    if(!*curl){
         AESM_DBG_ERROR("fail to init curl handle");
         return AE_FAILURE;
    }
    if((cc=curl_easy_setopt(*curl, CURLOPT_URL, url_path.c_str()))!=CURLE_OK){
       AESM_DBG_ERROR("fail error code %d in set url %s",(int)cc, url_path.c_str());
       curl_easy_cleanup(*curl);
       return AE_FAILURE;
    }
    (void)curl_easy_setopt(*curl, CURLOPT_REDIR_PROTOCOLS, CURLPROTO_HTTP | CURLPROTO_HTTPS);
    //setting proxy now
    if(urls.proxy_type == AESM_PROXY_TYPE_DIRECT_ACCESS){
        AESM_DBG_TRACE("use no proxy");
        (void)curl_easy_setopt(*curl, CURLOPT_NOPROXY , "*");
    }else if(urls.proxy_type == AESM_PROXY_TYPE_MANUAL_PROXY){
        AESM_DBG_TRACE("use manual proxy %s",urls.aesm_proxy);
        (void)curl_easy_setopt(*curl, CURLOPT_PROXY, urls.aesm_proxy);
    }
    return AE_SUCCESS;
}

static ae_error_t http_network_send_data(CURL *curl, const char *req_msg, uint32_t msg_size, char **resp_msg, uint32_t& resp_size, http_methods_t method, bool is_ocsp)
{
    AESM_DBG_TRACE("send data method=%d",method);
    struct curl_slist *headers=NULL;
    struct curl_slist *tmp=NULL;
    ae_error_t ae_ret = AE_SUCCESS;
    CURLcode cc=CURLE_OK;
    int num_bytes = 0;
    long resp_code = 0;
    network_malloc_info_t malloc_info_resp = {0, 0};
    network_malloc_info_t malloc_info_header = {0, 0};
    uint32_t header_length = 0;
    uint32_t start = 0, end = 0;
    char* p_header = NULL;
    if(is_ocsp){
        tmp = curl_slist_append(headers, "Accept: application/ocsp-response");
        if(tmp==NULL){
            AESM_DBG_ERROR("fail in add accept ocsp-response header");
            ae_ret = AE_FAILURE;
            goto fini;
        }
        headers = tmp;
        tmp = curl_slist_append(headers, "Content-Type: application/ocsp-request");
        if(tmp == NULL){
           AESM_DBG_ERROR("fail in add content type ocsp-request");
           ae_ret = AE_FAILURE;
           goto fini;
        }
        headers=tmp;
        AESM_DBG_TRACE("ocsp request");
    }
    else{
        tmp = curl_slist_append(headers, "Content-Type: text/plain");
        if(tmp == NULL){
           AESM_DBG_ERROR("fail in add content type text/plain");
           ae_ret = AE_FAILURE;
           goto fini;
        }
        headers=tmp;
    }
    char buf[50];
    num_bytes = snprintf(buf,sizeof(buf), "Content-Length: %u", (unsigned int)msg_size);
    if(num_bytes<0 || num_bytes>=(int)sizeof(buf)){
         AESM_DBG_ERROR("fail to prepare string Content-Length");
         ae_ret = AE_FAILURE;
         goto fini;
    }
    tmp = curl_slist_append(headers, buf);
    if(tmp == NULL){
         AESM_DBG_ERROR("fail to add content-length header");
         ae_ret = AE_FAILURE;
         goto fini;
    }
    headers=tmp;
    if((cc=curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers))!=CURLE_OK){
        AESM_DBG_ERROR("fail to set http header:%d",(int)cc);
        ae_ret = AE_FAILURE;
        goto fini;
    }
    if(method == POST){
        if((cc=curl_easy_setopt(curl, CURLOPT_POSTFIELDS, req_msg))!=CURLE_OK){
            AESM_DBG_ERROR("fail to set POST fields:%d",(int)cc);
            ae_ret = AE_FAILURE;
            goto fini;
        }
        if((cc=curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, msg_size))!=CURLE_OK){
            AESM_DBG_ERROR("fail to set POST fields size:%d",(int)cc);
            ae_ret = AE_FAILURE;
            goto fini;
        }
    }

    if((cc=curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback))!=CURLE_OK){
        AESM_DBG_ERROR("Fail to set callback function:%d",(int)cc);
        ae_ret = AE_FAILURE;
        goto fini;
    }
    if((cc=curl_easy_setopt(curl, CURLOPT_WRITEDATA, reinterpret_cast<void *>(&malloc_info_resp)))!=CURLE_OK){
       AESM_DBG_ERROR("fail to set write back function parameter:%d",(int)cc);
       ae_ret = AE_FAILURE;
       goto fini;
    }

    if((cc=curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, write_callback))!=CURLE_OK){
        AESM_DBG_ERROR("Fail to set callback function:%d",(int)cc);
        ae_ret = AE_FAILURE;
        goto fini;
    }
    if((cc=curl_easy_setopt(curl, CURLOPT_HEADERDATA, reinterpret_cast<void *>(&malloc_info_header)))!=CURLE_OK){
       AESM_DBG_ERROR("fail to set write back function parameter:%d",(int)cc);
       ae_ret = AE_FAILURE;
       goto fini;
    }
    if((cc=curl_easy_perform(curl))!=CURLE_OK){
        AESM_DBG_ERROR("fail in connect:%d",(int)cc);
        ae_ret = OAL_NETWORK_UNAVAILABLE_ERROR;
        goto fini;
    }
    // Check HTTP response code
    // For example, if the remote file does not exist, curl may return CURLE_OK but the http response code 
    // indicates an error has occured
    if((cc=curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &resp_code))!=CURLE_OK || resp_code>=400){
        AESM_DBG_ERROR("Response code error:%d", resp_code);
        ae_ret = AE_FAILURE;
        goto fini;
    }

    // return pointer to response buffer
    *resp_msg = malloc_info_resp.base;
    resp_size = malloc_info_resp.size;

    AESM_DBG_TRACE("get response size=%d",resp_size);

    // Log request-id from HTTP response header
    // which follows the generic HTTP header format
    // Request-ID: XXXXXXXXXXXXXXX 
    header_length = malloc_info_header.size;
    p_header = malloc_info_header.base;
    while(start < header_length) {
        while (end < header_length && p_header[end] != '\r' && p_header[end] != '\n') {
            end++;
        }
        if (end == start) {
            start++;
            end++;
        }
        else {
            // parse one line
            std::string str((unsigned char*)p_header+start, (unsigned char*)p_header+end);
            size_t pos = str.find(": ");
            if (pos != std::string::npos) {
                std::string key = str.substr(0, pos);
                std::string value = str.substr(pos+2);
                if (key.compare(REQUEST_ID) == 0) {
                    AESM_LOG_INFO("The Request ID is %s", value.c_str());
                    break;
                }
            }
            start = end;
        }
    }
    // End of log

    ae_ret = AE_SUCCESS;

fini:
    if(headers!=NULL){
        curl_slist_free_all(headers);
    }
    if (ae_ret != AE_SUCCESS) {
        if(malloc_info_resp.base){
            free(malloc_info_resp.base);
        }
    }
    if(malloc_info_header.base){
        free(malloc_info_header.base);
    }
    return ae_ret;
}

static void http_network_fini(CURL *curl)
{
    if(curl!=NULL)
        curl_easy_cleanup(curl);
}


ae_error_t aesm_network_send_receive(const char *server_url, const uint8_t *req, uint32_t req_size,
                                       uint8_t **p_resp, uint32_t *p_resp_size, http_methods_t method, bool is_ocsp)
{
    AESM_PROFILE_FUN;
    ae_error_t ret= AE_SUCCESS;
    CURL *curl = NULL;
    ret = http_network_init(&curl, server_url, is_ocsp);
    if(ret != AE_SUCCESS){
        goto ret_point;
    }
    ret = http_network_send_data(curl, reinterpret_cast<const char *>(req), req_size,
        reinterpret_cast<char **>(p_resp), *p_resp_size, method, is_ocsp);
ret_point:
    http_network_fini(curl);
    return ret;
}

void aesm_free_network_response_buffer(uint8_t *resp)
{
    if(resp!=NULL)free(resp);
}
