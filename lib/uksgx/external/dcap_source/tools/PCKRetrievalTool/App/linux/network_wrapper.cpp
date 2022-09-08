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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <map>
#include <fstream>
#include <algorithm>
#include "sgx_ql_lib_common.h"
#include "network_wrapper.h"
#include "utility.h"

using namespace std;

typedef struct _network_malloc_info_t{
    char *base;
    size_t size;
}network_malloc_info_t;

#define MAX_URL_LENGTH  2083
#define LOCAL_NETWORK_SETTING "network_setting.conf"

#ifndef MAX_PATH
#define MAX_PATH 260
#endif

extern string server_url_string;
extern string proxy_type_string;
extern string proxy_url_string;
extern string user_token_string;
extern string use_secure_cert_string;

typedef enum _network_proxy_type {
    DIRECT = 0,
    DEFAULT,
    MANUAL
} network_proxy_type;

// Use secure HTTPS certificate or not
static bool g_use_secure_cert = true;

/**
* Method converts byte containing value from 0x00-0x0F into its corresponding ASCII code,
* e.g. converts 0x00 to '0', 0x0A to 'A'.
* Note: This is mainly a helper method for internal use in byte_array_to_hex_string().
*
* @param in byte to be converted (allowed values: 0x00-0x0F)
*
* @return ASCII code representation of the byte or 0 if method failed (e.g input value was not in provided range).
*/
static uint8_t convert_value_to_ascii(uint8_t in)
{
    if(in <= 0x09)
    {
        return (uint8_t)(in + '0');
    }
    else if(in <= 0x0F)
    {
        return (uint8_t)(in - 10 + 'A');
    }

    return 0;
}

//Function to do HEX encoding of array of bytes
//@param in_buf, bytes array whose length is in_size
//       out_buf, output the HEX encoding of in_buf on success.
//@return true on success and false on error
//The out_size must always be 2*in_size since each byte into encoded by 2 characters
static bool byte_array_to_hex_string(const uint8_t *in_buf, uint32_t in_size, uint8_t *out_buf, uint32_t out_size)
{
    if(in_size>UINT32_MAX/2)return false;
    if(in_buf==NULL||out_buf==NULL|| out_size!=in_size*2 )return false;

    for(uint32_t i=0; i< in_size; i++)
    {
        *out_buf++ = convert_value_to_ascii( static_cast<uint8_t>(*in_buf >> 4));
        *out_buf++ = convert_value_to_ascii( static_cast<uint8_t>(*in_buf & 0xf));
        in_buf++;
    }
    return true;
}



/**
* This function appends request parameters of byte array type to the URL in HEX string format
*
* @param url Request URL
* @param request  Request parameter in byte array
* @param request_size Size of byte array
*
* @return true If the byte array was appended to the URL successfully
*/
network_post_error_t append_body_context(string& url, const uint8_t* request, const uint32_t request_size)
{
    if (request_size >= UINT32_MAX / 2)
        return POST_INVALID_PARAMETER_ERROR;

    uint8_t* hex = (uint8_t*)malloc(request_size * 2);
    if (!hex)
        return POST_OUT_OF_MEMORY_ERROR;
    if (!byte_array_to_hex_string(request, request_size, hex, request_size*2)){
        free(hex);
        return POST_UNEXPECTED_ERROR;
    }
    url.append(reinterpret_cast<const char*>(hex), request_size*2);
    free(hex);
    return POST_SUCCESS;
}


static size_t write_callback(void *ptr, size_t size, size_t nmemb, void *stream)
{
    network_malloc_info_t* s=reinterpret_cast<network_malloc_info_t *>(stream);
    size_t start=0;
    if(s->base==NULL){
        s->base = reinterpret_cast<char *>(malloc(size*nmemb));
        s->size = static_cast<uint32_t>(size*nmemb);
        if(s->base==NULL)return 0;
    }else{
        size_t newsize = s->size + size*nmemb;
        char *p=reinterpret_cast<char *>(realloc(s->base, newsize));
        if(p == NULL){
            return 0;
        }
        start = s->size;
        s->base = p;
        s->size = newsize;
    }
    memcpy(s->base +start, ptr, size*nmemb);
    return size*nmemb;
}

/**
* This method converts CURL error codes to quote3 error codes
*
* @param curl_error Curl library error codes
*
* @return network post Error Codes
*/
static network_post_error_t curl_error_to_network_post_error(CURLcode curl_error)
{
    switch(curl_error){
        case CURLE_OK:
            return POST_SUCCESS;
        default:
            return POST_NETWORK_ERROR;
    }
}

static bool process_configuration_setting(const char *config_file_name, string& url, string &proxy_type, string &proxy_url, string &user_token)
{
    bool ret = true;
    ifstream ifs(config_file_name);
    string line;
    if (ifs.is_open()) {
        auto f = [](unsigned char const c) { return std::isspace(c); };
        while (getline(ifs, line)) {
            line.erase(std::remove_if(line.begin(), line.end(), f), line.end());
            if (line[0] == '#' || line.empty())
                continue;
            size_t pos = line.find("=");
            string name = line.substr(0, pos);
            std::transform(name.begin(), name.end(), name.begin(), ::toupper);
            string value = line.substr(pos + 1);
            if (name.compare("PCCS_URL") == 0) {
                if (server_url_string.empty() == true) {
                    url = value;
                }
                else {
                    url = server_url_string + "/sgx/certification/v4/platforms";
                }
            }
            else if (name.compare("USE_SECURE_CERT") == 0) {
                if (use_secure_cert_string.empty() == true) {
                    std::transform(value.begin(), value.end(), value.begin(), ::toupper);
                    if (value.compare("FALSE") == 0) {
                        g_use_secure_cert = false;
                    }
                }
                else if (use_secure_cert_string.compare("FALSE") == 0 || use_secure_cert_string.compare("false") == 0) {
                    g_use_secure_cert = false;
                }
            }
            else if (name.compare("PROXY_TYPE") == 0) {
                if(proxy_type_string.empty() == true) {
                    std::transform(value.begin(), value.end(), value.begin(), ::toupper);
                    proxy_type = value;
                } 
            }
            else if (name.compare("PROXY_URL") == 0) {
                if(proxy_url_string.empty() == true) {
                    proxy_url = value;
                }
            }
            else if (name.compare("USER_TOKEN") == 0) {
                if(user_token_string.empty() == true) {
                    user_token = value;
                }
            }
            else {
                continue;
            }
        }
    }
    else {
        if (use_secure_cert_string.compare("FALSE") == 0 || use_secure_cert_string.compare("false") == 0) {
            g_use_secure_cert = false;
        }

        url = server_url_string + "/sgx/certification/v2/platforms";
        ret = false;
    }
    return ret;
}

/**
*  Read configuration data
*/
static void network_configuration(string &url, string &proxy_type, string &proxy_url, string& user_token)
{
    //firstly read local configuration File
    char local_configuration_file_path[MAX_PATH] = "";
    bool ret = get_program_path(local_configuration_file_path, MAX_PATH);
    if (ret) {
        if(strnlen(local_configuration_file_path ,MAX_PATH)+strnlen(LOCAL_NETWORK_SETTING,MAX_PATH)+sizeof(char) > MAX_PATH) {
            ret = false;
        }
        else {
            (void)strncat(local_configuration_file_path,LOCAL_NETWORK_SETTING, strnlen(LOCAL_NETWORK_SETTING,MAX_PATH));
        }
    }
    if (ret){
        process_configuration_setting(local_configuration_file_path,url, proxy_type, proxy_url, user_token);
    }
}


static network_post_error_t generate_json_message_body(const uint8_t *raw_data, 
                                                       const uint32_t raw_data_size,
                                                       const uint16_t platform_id_length,
                                                       const bool non_enclave_mode, 
                                                       string &jsonString)
{
    network_post_error_t ret = POST_SUCCESS;
    const uint8_t *position = raw_data;

    jsonString = "{";

    if(true == non_enclave_mode){
        jsonString += "\"pce_id\": \"";
        if ((ret = append_body_context(jsonString, position, PCE_ID_LENGTH)) != POST_SUCCESS) {
            return ret;
        }

        jsonString += "\" ,\"qe_id\": \"";
        position = position + PCE_ID_LENGTH;
        if ((ret = append_body_context(jsonString, position, platform_id_length)) != POST_SUCCESS) {
            return ret;
        }

        jsonString += "\" ,\"platform_manifest\": \"";
        position = position + platform_id_length;
        if ((ret = append_body_context(jsonString, position, raw_data_size - PCE_ID_LENGTH - platform_id_length)) != POST_SUCCESS) {
            return ret;
        }
    }
    else {
        uint32_t left_size = raw_data_size - platform_id_length - CPU_SVN_LENGTH - ISV_SVN_LENGTH - PCE_ID_LENGTH - ENCRYPTED_PPID_LENGTH;
        jsonString += "\"enc_ppid\": \"";
        if ((ret = append_body_context(jsonString, position, ENCRYPTED_PPID_LENGTH)) != POST_SUCCESS) {
            return ret;
        }

        jsonString += "\" ,\"pce_id\": \"";
        position = position + ENCRYPTED_PPID_LENGTH;
        if ((ret = append_body_context(jsonString, position, PCE_ID_LENGTH)) != POST_SUCCESS) {
            return ret;
        }
        jsonString += "\" ,\"cpu_svn\": \"";
        position = position + PCE_ID_LENGTH;
        if ((ret = append_body_context(jsonString, position, CPU_SVN_LENGTH)) != POST_SUCCESS) {
            return ret;
        }

        jsonString += "\" ,\"pce_svn\": \"";
        position = position + CPU_SVN_LENGTH;
        if ((ret = append_body_context(jsonString, position, ISV_SVN_LENGTH)) != POST_SUCCESS) {
            return ret;
        }

        jsonString += "\" ,\"qe_id\": \"";
        position = position + ISV_SVN_LENGTH;
        if ((ret = append_body_context(jsonString, position, platform_id_length)) != POST_SUCCESS) {
            return ret;
        }

        jsonString += "\" ,\"platform_manifest\": \"";
        if (left_size != 0) {
            position = position + platform_id_length;
            if ((ret = append_body_context(jsonString, position, left_size)) != POST_SUCCESS) {
                return ret;
            }
        }
    }
    jsonString += "\" }";
    return ret;
}
   

/**
* This method calls curl library to perform https post requet:
* it will combine the buffer, and post to server.
*
* @param buffer: input buffer, that include qeid, cpusvn, pcesvn, pceid, encrypted ppid, platform_manifest(if platform is multi-package)
* @param buffer_size: size of the buffer 
*
* @return SGX_QCNL_SUCCESS Call https get successfully. Other return codes indicate an error occured.
*/


network_post_error_t network_https_post(const uint8_t* raw_data, const uint32_t raw_data_size, const uint16_t platform_id_length, const bool non_enclave_mode)
{
    if (raw_data_size < platform_id_length + static_cast<uint32_t>(PCE_ID_LENGTH)) {
        return POST_INVALID_PARAMETER_ERROR;
    }

    network_post_error_t ret = POST_UNEXPECTED_ERROR;  
    string strJson("");
    ret = generate_json_message_body(raw_data, raw_data_size, platform_id_length, non_enclave_mode, strJson);
    if (ret != POST_SUCCESS) {
        printf("Error: unexpected error happens during generate json message body.\n");
        return ret;
    }
    CURL *curl = NULL;
    CURLcode curl_ret = CURLE_OK;
    network_malloc_info_t res_body = {0,0};

    // initialize https request url
    string url(server_url_string);
    string proxy_type(proxy_type_string);
    string proxy_url(proxy_url_string);
    string user_token(user_token_string);
    // initialize network configuration
    network_configuration(url, proxy_type, proxy_url, user_token);                   

    ret = POST_UNEXPECTED_ERROR;
    do {
        curl = curl_easy_init();
        if (!curl)
            break;

        if (curl_easy_setopt(curl, CURLOPT_URL, url.c_str()) != CURLE_OK)
            break;

        struct curl_slist *slist = NULL;
        slist = curl_slist_append(slist, "Content-Type: application/json");

        if (user_token.empty()) {
            printf("\n Please input the pccs password, and use \"Enter key\" to end\n");
            int usless_ret = system("stty -echo");
            user_token = "user-token: ";
            char ch;
            while ((ch = static_cast<char>(getchar())) != '\n') {
                user_token = user_token + ch;
            }
            usless_ret = system("stty echo");
            (void)(usless_ret);
        } else {
            user_token = "user-token: " + user_token;
        }

        slist = curl_slist_append(slist, user_token.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, slist);
        if (!g_use_secure_cert) {
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
        //	curl_easy_setopt(curl, CURLOPT_VERBOSE,1L);
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");

        // size of the POST data 
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strJson.size());
        // pass in a pointer to the data - libcurl will not copy 
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, strJson.c_str());

        // proxy setting	
        if (proxy_type.compare("DIRECT") == 0 || proxy_type.compare("direct") == 0) {
            curl_easy_setopt(curl, CURLOPT_NOPROXY, "*");
        }
        else if (proxy_type.compare("MANUAL") == 0 || proxy_type.compare("manual") == 0) {
            curl_easy_setopt(curl, CURLOPT_PROXY, proxy_url.c_str());
        }


        // Perform request
        if ((curl_ret = curl_easy_perform(curl)) != CURLE_OK) {
            ret = curl_error_to_network_post_error(curl_ret);
            break;
        }
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        if (http_code == 200) {
            ret = POST_SUCCESS;
            break;
        }
        else if (http_code == 401) {
            ret = POST_AUTHENTICATION_ERROR;
            break;
        }
        else {
            ret = POST_UNEXPECTED_ERROR;
            break;
        }

    } while (0);

    if (curl) {
        curl_easy_cleanup(curl);
    }
    if (res_body.base) {
        free(res_body.base);
    }
    return ret;
}

bool is_server_url_available() {
    char local_configuration_file_path[MAX_PATH] = "";
    bool ret = get_program_path(local_configuration_file_path, MAX_PATH);
    if (ret) {
        if(strnlen(local_configuration_file_path ,MAX_PATH)+strnlen(LOCAL_NETWORK_SETTING,MAX_PATH)+sizeof(char) > MAX_PATH) {
            return false;
        }
        else {
            (void)strncat(local_configuration_file_path,LOCAL_NETWORK_SETTING, strnlen(LOCAL_NETWORK_SETTING,MAX_PATH));
        }
    }
    ifstream ifs_local(local_configuration_file_path);
    string line;
    if (ifs_local.is_open()) {
        auto f = [](unsigned char const c) { return std::isspace(c); };
        while (getline(ifs_local, line)) {
            line.erase(std::remove_if(line.begin(), line.end(), f), line.end());
            if (line[0] == '#' || line.empty())
                continue;
            size_t pos = line.find("=");
            string name = line.substr(0, pos);
            std::transform(name.begin(), name.end(), name.begin(), ::toupper);
            string value = line.substr(pos + 1);
            if (name.compare("PCCS_URL") == 0) {
                if (value.empty() == false) {
                    return true;
                }
            }
        }
    }

   return false;
}
