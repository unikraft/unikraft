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
 * File: app.cpp
 * generate the raw data for PCK Cert retrieval
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <algorithm>
#ifdef _MSC_VER
#include <Windows.h>
#include <tchar.h>
#include <string>

#include <sgx_enclave_common.h>
extern "C"
uint32_t COMM_API sgx_tool_get_launch_token(
    COMM_IN const enclave_init_sgx_t* css,
    COMM_IN const enclave_sgx_attr_t* attr,
    COMM_OUT      enclave_sgx_token_t* token
);

#else
#include <dlfcn.h>
#endif
#include "se_version.h"
#include "sgx_pce.h"
#include "sgx_quote_3.h"
#include "network_wrapper.h"
#include "utility.h"
     
#define MAX_PATH 260
#define VER_FILE_DESCRIPTION_STR    "Intel(R) Software Guard Extensions PCK Cert ID Retrieval Tool"
#define VER_PRODUCTNAME_STR         "PCKIDRetrievalTool"


void PrintHelp() {
    printf("Usage: %s [OPTION] \n", VER_PRODUCTNAME_STR);
    printf("Example: %s -f pck_retrieval_result.csv -url https://localhost:8081 -user_token 123456 -use_secure_cert true -platform_id\n", VER_PRODUCTNAME_STR);
    printf( "\nOptions:\n");
    printf( " -f filename                          - output the retrieval result to the \"filename\"\n");
    printf( " -url cache_server_address            - cache server's address \n");
    printf( " -user_token token_string             - user token to access the cache server \n");
    printf( " -proxy_type proxy_type               - proxy setting when access the cache server \n");
    printf( " -proxy_url  proxy_server_address     - proxy server's address \n");
    printf( " -use_secure_cert [true | false]      - accept secure/insecure https cert, default value is true \n");
    printf( " -platform_id \"platform_id_string\"  - in this mode, enclave is not needed to load, but platform id need to input\n");
    printf( " -?                                   - show command help\n");
    printf( " -h                                   - show command help\n");
    printf( " -help                                - show command help\n");
    printf( "If option is not specified, it will write the retrieved data to file: pckid_retrieval.csv\n\n");
}


// Some utility MACRO to output some of the data structures.
#define PRINT_BYTE_ARRAY(stream,mem, len)                     \
{                                                             \
    if (!(mem) || !(len)) {                                   \
        fprintf(stream,"\n( null )\n");                       \
    } else {                                                  \
        uint8_t *array = (uint8_t *)(mem);                    \
        uint32_t i = 0;                                       \
        for (i = 0; i < (len) - 1; i++) {                     \
            fprintf(stream,"%02x", array[i]);                 \
            if (i % 32 == 31 && stream == stdout)             \
               fprintf(stream,"\n");                          \
        }                                                     \
        fprintf(stream,"%02x", array[i]);                     \
    }                                                         \
}


#define PRINT_ZERO(stream,len)                                \
{                                                             \
    if (!(len)) {                                             \
        fprintf(stream,"\n( null )\n");                       \
    } else {                                                  \
        uint32_t i = 0;                                       \
        for (i = 0; i < (len); i++) {                         \
            fprintf(stream,"%02x", 0);                        \
        }                                                     \
    }                                                         \
}

#define WRITE_COMMA                                           \
    fprintf(pFile,",");                                       \

#ifdef DEBUG
#define PRINT_MESSAGE(message) printf(message);
#else
#define PRINT_MESSAGE(message) ;
#endif

char toUpper(char ch)
{
    return static_cast<char>(toupper(ch));
}

std::string server_url_string = "";
std::string proxy_type_string = "";
std::string proxy_url_string = "";
std::string user_token_string = "";
std::string use_secure_cert_string = "";
std::string output_filename = "";
std::string platform_id_string = "";
bool non_enclave_mode = false;

int parse_arg(int argc, const char *argv[])
{
    if(argc == 1) {
        output_filename = "pckid_retrieval.csv";
    }

    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], "-f", 2) == 0) {
            if (i == argc - 1 || argv[i+1][0] == '-') {
                fprintf(stderr, "No file name is provided for -f\n");
                return -1;
            }
            else {
                output_filename = argv[i + 1];
                i++;
                continue;
            }
        }
        else if (strncmp(argv[i], "-url", 4) == 0) {
            if (i == argc - 1 || argv[i+1][0] == '-') {
                fprintf(stderr, "No url provided for -url\n");
                return -1;
            }
            else {
                server_url_string = argv[i + 1];
                i++;
                continue;
            }
        }
        else if (strncmp(argv[i], "-defaulturl", 11) == 0) {
            server_url_string = "https://localhost:8081";
            continue;
        }
        else if (strncmp(argv[i], "-proxy_type",11) == 0) {
            if (i == argc - 1 || argv[i+1][0] == '-') {
                fprintf(stderr, "No proxy type provided for -proxy_type\n");
                return -1;
            }
            else {
                proxy_type_string = argv[i + 1];
                std::transform(proxy_type_string.begin(), proxy_type_string.end(), proxy_type_string.begin(), toUpper);
                if (!is_valid_proxy_type(proxy_type_string)) {
                    fprintf(stderr, "Invalid proxy_type %s\n", proxy_type_string.c_str());
                    return -1;
                }
                i++;
                continue;
            }
        }
        else if (strncmp(argv[i], "-proxy_url", 10) == 0) {
            if (i == argc - 1 || argv[i+1][0] == '-') {
                fprintf(stderr, "No proxy url provided for -proxy_url\n");
                return -1;
            }
            else {
                proxy_url_string = argv[i + 1];
                i++;
                continue;
            }
        }
        else if (strncmp(argv[i], "-user_token", 11) == 0) {
            if (i == argc - 1 || argv[i+1][0] == '-') {
                fprintf(stderr, "No user token is provided for -user_token\n");
                return -1;
            }
            else {
                user_token_string = argv[i + 1];
                i++;
                continue;
            }
        }
        else if (strncmp(argv[i], "-use_secure_cert", 16) == 0) {
            if (i == argc - 1 || argv[i+1][0] == '-') {
                fprintf(stderr, "Option is not provided for -use_secure_cert\n");
                return -1;
            }
            else {
                use_secure_cert_string = argv[i + 1];
                std::transform(use_secure_cert_string.begin(), use_secure_cert_string.end(), use_secure_cert_string.begin(),toUpper);
                if (!is_valid_use_secure_cert(use_secure_cert_string)) {
                    fprintf(stderr, "Invalid user secure cert %s\n", use_secure_cert_string.c_str());
                    return -1;
                }
                i++;
                continue;
            }
        }
        else if (strncmp(argv[i], "-platform_id", 12) == 0) {
            non_enclave_mode = true;
            if (i == argc - 1 || argv[i + 1][0] == '-') {
                fprintf(stdout, "Please input the platform ID, and the platform ID's length should not more than 260 bytes: \n");
                char platform_id[MAX_PATH] = {0};
                if (NULL == fgets(platform_id, MAX_PATH, stdin)) {
                    fprintf(stderr, "No platform id is provided for -platform_id\n");
                    return -1;
                }
                platform_id_string = platform_id;
                if (platform_id_string.length() > MAX_PATH) {
                    fprintf(stderr, "Error: the platform ID's length should not more than 260 bytes.\n");
                    return -1;
                }
                i++;
                continue;
            }
            else {
                platform_id_string = argv[i+1];
                if (platform_id_string.length() > MAX_PATH) {
                    fprintf(stderr, "Error: the platform ID's length should not more than 260 bytes.\n");
                    return -1;
                }
                i++;
                continue;
            }
        }
        else {
            fprintf(stderr, "unknown option %s\n", argv[i]);
            return -1;
        }    
    }
    return 0;
}

int send_collected_data_to_file(FILE* pFile, uint8_t* p_data_buffer, uint8_t* p_platform_manifest_buffer, uint16_t platform_manifest_buffer_size, std::string& platform_id)
{
     if (p_data_buffer == NULL) {//,PCE_ID,,,PLATFORM_ID[,PLATFORM MANIFEST]
        WRITE_COMMA;

        // write pceid to file
        PRINT_ZERO(pFile, PCE_ID_LENGTH);
        WRITE_COMMA;

        WRITE_COMMA;

        WRITE_COMMA;

        //write platform id to file
        PRINT_BYTE_ARRAY(pFile, platform_id.c_str(), platform_id.length());
    }
    else {
        // Output PCK Cert Retrieval Data
#ifdef _MSC_VER
        sgx_quote3_t* p_quote = (sgx_quote3_t*)(p_data_buffer);
        sgx_ql_ecdsa_sig_data_t* p_sig_data = (sgx_ql_ecdsa_sig_data_t*)p_quote->signature_data;
        sgx_ql_auth_data_t* p_auth_data = (sgx_ql_auth_data_t*)p_sig_data->auth_certification_data;
        sgx_ql_certification_data_t* p_temp_cert_data = (sgx_ql_certification_data_t*)((uint8_t*)p_auth_data + sizeof(*p_auth_data) + p_auth_data->size);
        p_data_buffer = p_temp_cert_data->certification_data;
#endif
        uint64_t data_index = 0;
#ifdef DEBUG
        PRINT_MESSAGE("EncPPID:\n");
        PRINT_BYTE_ARRAY(stdout, p_data_buffer + data_index, ENCRYPTED_PPID_LENGTH);
        PRINT_MESSAGE("\n PCE_ID:\n");
        data_index = data_index + ENCRYPTED_PPID_LENGTH;
        PRINT_BYTE_ARRAY(stdout, p_data_buffer + data_index, PCE_ID_LENGTH);
        PRINT_MESSAGE("\n TCBr - CPUSVN:\n");
        data_index = data_index + PCE_ID_LENGTH;
        PRINT_BYTE_ARRAY(stdout, p_data_buffer + data_index, CPU_SVN_LENGTH);
        PRINT_MESSAGE("\n TCBr - PCE_ISVSVN:\n");
        data_index = data_index + CPU_SVN_LENGTH;
        PRINT_BYTE_ARRAY(stdout, p_data_buffer + data_index, ISV_SVN_LENGTH);
        PRINT_MESSAGE("\n PLATFORM_ID:\n");
#ifdef _MSC_VER
        PRINT_BYTE_ARRAY(stdout, &p_quote->header.user_data[0], DEFAULT_PLATFORM_ID_LENGTH);
#else
        data_index = data_index + ISV_SVN_LENGTH;
        PRINT_BYTE_ARRAY(stdout, p_data_buffer, DEFAULT_PLATFORM_ID_LENGTH);
#endif
        PRINT_MESSAGE("\n\n");
#endif
        data_index = 0;
        //write encrypted PPID to file
        PRINT_BYTE_ARRAY(pFile, p_data_buffer + data_index, ENCRYPTED_PPID_LENGTH);
        WRITE_COMMA;

        data_index = data_index + ENCRYPTED_PPID_LENGTH;
        // write pceid to file
        PRINT_BYTE_ARRAY(pFile, p_data_buffer + data_index, PCE_ID_LENGTH);
        WRITE_COMMA;

        data_index = data_index + PCE_ID_LENGTH;
        //write cpusvn to file
        PRINT_BYTE_ARRAY(pFile, p_data_buffer + data_index, CPU_SVN_LENGTH);
        WRITE_COMMA;

        data_index = data_index + CPU_SVN_LENGTH;
        //write pce isv_svn to file
        PRINT_BYTE_ARRAY(pFile, p_data_buffer + data_index, ISV_SVN_LENGTH);
        WRITE_COMMA;

#ifdef _MSC_VER
        //write qe_id to file
        PRINT_BYTE_ARRAY(pFile, &p_quote->header.user_data[0], DEFAULT_PLATFORM_ID_LENGTH);
#else
        data_index = data_index + ISV_SVN_LENGTH;
        //write qe_id to file
        PRINT_BYTE_ARRAY(pFile, p_data_buffer + data_index, DEFAULT_PLATFORM_ID_LENGTH);
#endif   
    }
    //write platform manifest.
    if (platform_manifest_buffer_size > 0 ) {
        WRITE_COMMA;
        PRINT_BYTE_ARRAY(pFile, p_platform_manifest_buffer, static_cast<uint32_t>(platform_manifest_buffer_size));
        PRINT_MESSAGE("\n");
    }  
    return 0;
}

cache_server_delivery_status_t send_collected_data_to_server(uint8_t* p_data_buffer, uint8_t* p_platform_manifest_buffer, uint16_t platform_manifest_buffer_size, std::string& platform_id)
{
    network_post_error_t ret_status = POST_SUCCESS;
    uint8_t* raw_data = NULL;
    uint16_t platform_id_length = DEFAULT_PLATFORM_ID_LENGTH;
    cache_server_delivery_status_t delivery_status = DELIVERY_ERROR_MAX;
    uint32_t data_length_except_platform_manifest = ENCRYPTED_PPID_LENGTH + PCE_ID_LENGTH + CPU_SVN_LENGTH + ISV_SVN_LENGTH + DEFAULT_PLATFORM_ID_LENGTH;

    // raw_data include: enclave mode(need load enclave) 
    // 1. enc_ppid, pce_svn, pce_id, cpu_svn, the size is sizeof(sgx_ql_ppid_rsa3072_encrypted_cert_info_t)
    // 2. qe_id: its size is 16
    // 3. platform_manifest: its size is platform_manifest_buffer_size if it is multi-package platform, otherwise it is 0

    // raw_data include: non-enclave mode(don't need load enclave) 
    // 1. pce_id, the size is PCE_ID_LENGTH 
    // 2. platform_id: its size depends on user's input, but the size should not more than 260 bytes.
    // 3. platform_manifest: its size is platform_manifest_buffer_size when platform manifest UEFI variable is available,

    uint32_t raw_data_size = 0;
    if (true == non_enclave_mode) {
        platform_id_length = static_cast<uint16_t>(platform_id.length());
        raw_data_size = platform_manifest_buffer_size + platform_id_length + PCE_ID_LENGTH;
        raw_data = new (std::nothrow) uint8_t[raw_data_size];
        if (raw_data == NULL) {
            fprintf(stderr,"Error: Memory has been used up.\n");
            return DELIVERY_FAIL;
        }
        memset(raw_data, 0x00, raw_data_size);//now pce id is zero
        memcpy(raw_data + PCE_ID_LENGTH, platform_id.c_str(), platform_id_length);
        if(platform_manifest_buffer_size) {
            memcpy(raw_data + PCE_ID_LENGTH + platform_id_length, p_platform_manifest_buffer, platform_manifest_buffer_size);
        }
    }
    else {
        // Output PCK Cert Retrieval Data
#ifdef _MSC_VER
        sgx_quote3_t* p_quote = (sgx_quote3_t*)(p_data_buffer);
        sgx_ql_ecdsa_sig_data_t* p_sig_data = (sgx_ql_ecdsa_sig_data_t*)p_quote->signature_data;
        sgx_ql_auth_data_t* p_auth_data = (sgx_ql_auth_data_t*)p_sig_data->auth_certification_data;
        sgx_ql_certification_data_t* p_temp_cert_data = (sgx_ql_certification_data_t*)((uint8_t*)p_auth_data + sizeof(*p_auth_data) + p_auth_data->size);
#endif
        raw_data_size = platform_manifest_buffer_size + data_length_except_platform_manifest;
        raw_data = new (std::nothrow) uint8_t[raw_data_size];
        if (raw_data == NULL) {
            fprintf(stderr,"Error: Memory has been used up.\n");
            return DELIVERY_FAIL;
        }
        memset(raw_data, 0x00, raw_data_size);
#ifdef _MSC_VER
        memcpy(raw_data, p_temp_cert_data->certification_data, sizeof(sgx_ql_ppid_rsa3072_encrypted_cert_info_t) + DEFAULT_PLATFORM_ID_LENGTH);
#else
        memcpy(raw_data, p_data_buffer, data_length_except_platform_manifest);
#endif
        if (platform_manifest_buffer_size > 0) { //for multi-package scenario
            memcpy(raw_data + data_length_except_platform_manifest, p_platform_manifest_buffer, platform_manifest_buffer_size);
        }
    }

    ret_status = network_https_post(raw_data, raw_data_size, platform_id_length, non_enclave_mode);
    if (POST_SUCCESS == ret_status) {
        delivery_status = DELIVERY_SUCCESS;
    }
    else if (POST_AUTHENTICATION_ERROR == ret_status) {
        fprintf(stderr, "Error: the input password is not correct.\n");
        delivery_status = DELIVERY_FAIL;
    }
    else if (POST_NETWORK_ERROR == ret_status) {
        fprintf(stderr, "Error: network error, please check the network setting or whether the cache server is down.\n");
        delivery_status = DELIVERY_FAIL;
    }
    else {
        fprintf(stderr, "Error: unexpected error happend during sending data to cache server.\n");
        delivery_status = DELIVERY_FAIL;
    }

    delete[] raw_data;
    raw_data = NULL;
    return delivery_status;
}

// returned error code:
//     0: success, maybe there are some warning message
//    -1: when error happens
//    when this tool was used in user's script, maybe we need give more 
//    returned error code to help user identify what kinds of warning message.

int main(int argc, const char* argv[])
{
    int ret = -1;
    uint8_t* p_data_buffer = NULL;
    FILE* pFile = NULL;
    uint8_t *p_platform_manifest_buffer = NULL;
    uint16_t platform_manifest_out_buffer_size = UINT16_MAX;
    bool is_server_url_provided = false;
    cache_server_delivery_status_t delivery_status = DELIVERY_ERROR_MAX;
    uefi_status_t ret_mpa = UEFI_OPERATION_UNEXPECTED_ERROR;

    printf("\n%s Version ", VER_FILE_DESCRIPTION_STR);
    printf("%s\n\n", STRPRODUCTVER);

    // parse the command options
    if (parse_arg(argc, argv) != 0) {
        PrintHelp();
        return ret;
    }

#ifdef _MSC_VER
    // Check SGX_TOOL_GET_LAUNCH_TOKEN env var.
    // If set, then ask the sgx_enclave_common to use our custom function to obtain launch tokens.
    // GetEnvironmentVariableA: 
    //   If lpBuffer is not large enough to hold the data, the return value is the buffer size, in characters, required to hold the string 
    if (GetEnvironmentVariableA("SGX_TOOL_GET_LAUNCH_TOKEN", NULL, 0))
    {
        uint32_t set_err = 0;
        bool set_ret = enclave_set_information(NULL, 2, &sgx_tool_get_launch_token, sizeof(void*), &set_err);
        if (!set_ret)
        {
            fprintf(stderr, "\nError calling enclave_set_information( sgx_tool_get_launch_token ). Error code = %d\n", set_err);
        }
    }
#endif

    //check whether it is needed to save the collected data to a file
    if (output_filename.empty() == false) {
#ifdef _MSC_VER
        if (0 != fopen_s(&pFile, output_filename.c_str(), "w")) {
#else
        if (NULL == (pFile = fopen(output_filename.c_str(), "w"))) {
#endif
            fprintf(stderr, "\nError opening %s output file.\n", output_filename.c_str());
            return ret;
        }
    }

    //check whether it is needed to upload the collected data to the cache server
    if (server_url_string.empty() == false) {
        is_server_url_provided = true;
    }
    else {
        is_server_url_provided = is_server_url_available();
    }

    // for multi-package platform: get platform manifest  
    ret_mpa = get_platform_manifest(&p_platform_manifest_buffer, platform_manifest_out_buffer_size);
    if (non_enclave_mode) {
        if (ret_mpa == UEFI_OPERATION_VARIABLE_NOT_AVAILABLE) {
            // in this mode, it is possible platform manifest is not avaialbe,
            // if it happens, since there is no valid data, 
            // will not write data to file or send data to cache server, just return
            if (NULL != p_platform_manifest_buffer) {
                free(p_platform_manifest_buffer);
                p_platform_manifest_buffer = NULL;
            }
            if (pFile) {
               fclose(pFile);
            }
            ret = 0;
            return ret;
        }
        else if(ret_mpa != UEFI_OPERATION_SUCCESS) {
            if (NULL != p_platform_manifest_buffer) {
                free(p_platform_manifest_buffer);
                p_platform_manifest_buffer = NULL;
            }
            fprintf(stderr,"Error: Can NOT retrieve the platform manifest.\n");
            if (pFile) {
               fclose(pFile);
            }
            return ret;
        }
    }
    else {
        // in enclave mode: collecting the platform manifest is not MUST, so just give a warning 
        if (ret_mpa != UEFI_OPERATION_SUCCESS) {
            // in this mode, if platform manifest is not availabe, just ignore it. 
            platform_manifest_out_buffer_size = 0;
        }
#ifdef _MSC_VER
        uint32_t quote_size = 0;
        ret = generate_quote(&p_data_buffer, quote_size);
#else
	ret = collect_data(&p_data_buffer);
#endif
        if (ret != 0) {
            if (NULL != p_data_buffer) {
                free(p_data_buffer);
                p_data_buffer = NULL;
            }
            if (NULL != p_platform_manifest_buffer) {
                free(p_platform_manifest_buffer);
                p_platform_manifest_buffer = NULL;
            }
            if (pFile) {
               fclose(pFile);
            }
            return ret;
        }
    }

    ret = 0;
    if (pFile != NULL) {
        send_collected_data_to_file(pFile, p_data_buffer, p_platform_manifest_buffer, platform_manifest_out_buffer_size, platform_id_string);
    }

    if (is_server_url_provided) {
        delivery_status = send_collected_data_to_server(p_data_buffer, p_platform_manifest_buffer, platform_manifest_out_buffer_size, platform_id_string);
    }

    if (ret_mpa == UEFI_OPERATION_SUCCESS && (pFile != NULL || delivery_status == DELIVERY_SUCCESS)) {
        if (UEFI_OPERATION_SUCCESS == set_registration_status()) {
            fprintf(stdout,"Registration status has been set to completed status.\n");
        }
        else {
            fprintf(stdout, "Warning: could NOT set the Registration Status to completed status.\n");
        }
    }

    if (NULL != p_data_buffer) {
        free(p_data_buffer);
        p_data_buffer = NULL;
    }
    if(NULL != p_platform_manifest_buffer) {
        free(p_platform_manifest_buffer);
        p_platform_manifest_buffer = NULL;
    }
    
    if (pFile) {
        fclose(pFile);
    }

    if(delivery_status == DELIVERY_SUCCESS) {
        if(pFile != NULL) {
            fprintf(stdout, "the data has been sent to cache server successfully and %s has been generated successfully!\n",output_filename.c_str());
        }
        else {
            fprintf(stdout, "the data has been sent to cache server successfully!\n");
        }	
    }
    else if(delivery_status == DELIVERY_FAIL) {
        if(pFile != NULL) {
            fprintf(stdout, "%s has been generated successfully, however the data couldn't be sent to cache server!\n",output_filename.c_str());
        }
        else {
            fprintf(stderr, "Error: the data couldn't be sent to cache server!\n");
        }	
    } 
    else {
        if(pFile != NULL) {
            fprintf(stdout, "%s has been generated successfully!\n",output_filename.c_str());
        }
        else {
            fprintf(stderr, "Error: the retrieved data doesn't save to file, and it doesn't upload to cache server.\n");
        }	
    }
    return ret;
}
