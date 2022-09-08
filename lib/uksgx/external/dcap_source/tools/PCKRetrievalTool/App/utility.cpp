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
 * File: utility.cpp
 *
 * Description: utility functions
 *
 */
#include <stdio.h>
#include <string>
#ifdef _MSC_VER
#include <Windows.h>
#include <tchar.h>
#include "sgx_dcap_ql_wrapper.h"
#include "Enclave_u.h"
#else
#include <dlfcn.h>
#include <unistd.h>
#include "id_enclave_u.h"
#include "pce_u.h"
#endif
#include "sgx_urts.h"     
#include "utility.h"

#ifndef MAX_PATH
#define MAX_PATH 260
#endif


#ifdef DEBUG
#define PRINT_MESSAGE(message) printf(message);
#define PRINT_MESSAGE2(message1,message2) printf(message1, message2);
#else
#define PRINT_MESSAGE(message) ;
#define PRINT_MESSAGE2(message1,message2) ;
#endif

#ifdef  _MSC_VER                
#define TOOL_ENCLAVE_NAME _T("pck_id_retrieval_tool_enclave.signed.dll")
#define SGX_URTS_LIBRARY _T("sgx_urts.dll")
#define SGX_DCAP_QUOTE_GENERATION_LIBRARY _T("sgx_dcap_ql.dll")
#define SGX_QL_QUOTE_CONFIG_LIB_FILE_NAME _T("dcap_quoteprov.dll")
#define SGX_MULTI_PACKAGE_AGENT_UEFI_LIBRARY _T("mp_uefi.dll")
#define FINDFUNCTIONSYM   GetProcAddress
#define CLOSELIBRARYHANDLE  FreeLibrary
#define EFIVARS_FILE_SYSTEM_IN_OS ""//for Windows OS, don't need this path

HINSTANCE sgx_urts_handle = NULL;
#ifdef UNICODE
typedef sgx_status_t (SGXAPI *sgx_create_enclave_func_t)(const LPCWSTR file_name, const int debug, sgx_launch_token_t* launch_token, int* launch_token_updated, sgx_enclave_id_t* enclave_id, sgx_misc_attribute_t* misc_attr);
#else 
typedef sgx_status_t (SGXAPI *sgx_create_enclave_func_t)(const LPCSTR file_name, const int debug, sgx_launch_token_t* launch_token, int* launch_token_updated, sgx_enclave_id_t* enclave_id, sgx_misc_attribute_t* misc_attr);
#endif

#else
#define PCE_ENCLAVE_NAME  "libsgx_pce.signed.so.1"
#define ID_ENCLAVE_NAME   "libsgx_id_enclave.signed.so.1"
#define SGX_URTS_LIBRARY "libsgx_urts.so.1"             
#define SGX_MULTI_PACKAGE_AGENT_UEFI_LIBRARY "libmpa_uefi.so.1"
#define FINDFUNCTIONSYM   dlsym
#define CLOSELIBRARYHANDLE  dlclose
#define EFIVARS_FILE_SYSTEM_IN_OS "/sys/firmware/efi/efivars/"
typedef sgx_status_t (SGXAPI *sgx_create_enclave_func_t)(const char* file_name,
    const int debug,
    sgx_launch_token_t* launch_token,
    int* launch_token_updated,
    sgx_enclave_id_t* enclave_id,
    sgx_misc_attribute_t* misc_attr);


void* sgx_urts_handle = NULL;
#endif 


typedef sgx_status_t(SGXAPI* sgx_ecall_func_t)(const sgx_enclave_id_t eid,
    const int index,
    const void* ocall_table,
    void* ms);

typedef sgx_status_t (SGXAPI* sgx_destroy_enclave_func_t)(const sgx_enclave_id_t enclave_id);
typedef sgx_status_t (SGXAPI* sgx_get_target_info_func_t)(const sgx_enclave_id_t enclave_id, sgx_target_info_t* target_info);

#ifdef _MSC_VER
#pragma warning(disable: 4201)    // used to eliminate `unused variable' warning
#define UNUSED(val) (void)(val)

typedef quote3_error_t (*sgx_qe_get_target_info_func_t)(sgx_target_info_t* p_qe_target_info);
typedef quote3_error_t (*sgx_qe_get_quote_size_func_t)(uint32_t* p_quote_size);
typedef quote3_error_t (*sgx_qe_get_quote_func_t)(const sgx_report_t* p_app_report,uint32_t quote_size, uint8_t* p_quote);

#endif 
#include "MPUefi.h"
typedef MpResult(*mp_uefi_init_func_t)(const char* path, const LogLevel logLevel);
typedef MpResult(*mp_uefi_get_request_type_func_t)(MpRequestType* type);
typedef MpResult(*mp_uefi_get_request_func_t)(uint8_t *request, uint16_t *request_size);
typedef MpResult(*mp_uefi_get_registration_status_func_t)(MpRegistrationStatus* status);
typedef MpResult(*mp_uefi_set_registration_status_func_t)(const MpRegistrationStatus* status);
typedef MpResult(*mp_uefi_terminate_func_t)();



//redefine this function to avoid sgx_urts library compile dependency
sgx_status_t  SGXAPI sgx_ecall(const sgx_enclave_id_t eid,
                              const int index,
                              const void* ocall_table,
                              void* ms)
{
    // sgx_urts library has been tried to loaded before this function call, and this function is be called during sgx_create_enclave call
#if defined(_MSC_VER)
    if (sgx_urts_handle == NULL) {
        printf("ERROR: didn't find the sgx_urts.dll library, please make sure you have installed PSW installer package. \n");
        return SGX_ERROR_UNEXPECTED;
    }
    sgx_ecall_func_t p_sgx_ecall = (sgx_ecall_func_t)FINDFUNCTIONSYM(sgx_urts_handle, "sgx_ecall");
    if(p_sgx_ecall != NULL) {
        return p_sgx_ecall(eid, index, ocall_table, ms);
    }
    else {
        printf("ERROR: didn't find function sgx_ecall in the sgx_urts.dll library. \n");
        return SGX_ERROR_UNEXPECTED;
    }
#else
    if (sgx_urts_handle == NULL) {
        printf("ERROR: didn't find the sgx_urts.so library, please make sure you have installed sgx_urts installer package. \n");
        return SGX_ERROR_UNEXPECTED;
    }
    sgx_ecall_func_t p_sgx_ecall = (sgx_ecall_func_t)FINDFUNCTIONSYM(sgx_urts_handle, "sgx_ecall");
    if(p_sgx_ecall != NULL ) {
        return p_sgx_ecall(eid, index, ocall_table, ms);
    }
    else {
        printf("ERROR: didn't find function sgx_ecall in the sgx_urts.dll library. \n");
        return SGX_ERROR_UNEXPECTED;
    }
#endif

}


#ifdef _MSC_VER
bool get_program_path(TCHAR *p_file_path, size_t buf_size)
{
    UNUSED(p_file_path);
    UNUSED(buf_size);
    return true;
}
#else
bool get_program_path(char *p_file_path, size_t buf_size)
{
    if(NULL == p_file_path || 0 == buf_size){
        return false;
    }

    ssize_t i = readlink( "/proc/self/exe", p_file_path, buf_size );
    if (i == -1)
        return false;
    p_file_path[i] = '\0';

    char* p_last_slash = strrchr(p_file_path, '/' );
    if ( p_last_slash != NULL ) {
        p_last_slash++;   //increment beyond the last slash
        *p_last_slash = '\0';  //null terminate the string
    }
    else {
        p_file_path[0] = '\0';
    }
    return true;
}
#endif


bool get_urts_library_handle()
{
    // try to sgx_urts library to create enclave.
#if defined(_MSC_VER)
    sgx_urts_handle = LoadLibrary(SGX_URTS_LIBRARY);
    if (sgx_urts_handle == NULL) {
        printf("ERROR: didn't find the sgx_urts.dll library, please make sure you have installed PSW installer package. \n");
        return false;
    }
#else
    sgx_urts_handle = dlopen(SGX_URTS_LIBRARY, RTLD_LAZY);
    if (sgx_urts_handle == NULL) {
        printf("ERROR: didn't find the sgx_urts.so library, please make sure you have installed sgx_urts installer package. \n");
        return false;
    }
#endif
    return true;
}

void close_urts_library_handle()
{
    CLOSELIBRARYHANDLE(sgx_urts_handle);
}

extern "C"
#if defined(_MSC_VER)
bool load_enclave(const TCHAR* enclave_name, sgx_enclave_id_t* p_eid)
#else
bool load_enclave(const char* enclave_name, sgx_enclave_id_t* p_eid)
#endif
{
    bool ret = true;
    sgx_status_t sgx_status = SGX_SUCCESS;
    int launch_token_updated = 0;
    sgx_launch_token_t launch_token = { 0 };
    memset(&launch_token, 0, sizeof(sgx_launch_token_t));

#if defined(_MSC_VER)
    TCHAR enclave_path[MAX_PATH] = _T("");
#else
    char enclave_path[MAX_PATH] = "";
#endif

    if (!get_program_path(enclave_path, MAX_PATH))
        return false;
#if defined(_MSC_VER)    
    if (_tcsnlen(enclave_path, MAX_PATH) + _tcsnlen(enclave_name, MAX_PATH) + sizeof(char) > MAX_PATH)
        return false;
    (void)_tcscat_s(enclave_path, MAX_PATH, enclave_name);

#ifdef UNICODE
    sgx_create_enclave_func_t p_sgx_create_enclave = (sgx_create_enclave_func_t)FINDFUNCTIONSYM(sgx_urts_handle, "sgx_create_enclavew");
#else
    sgx_create_enclave_func_t p_sgx_create_enclave = (sgx_create_enclave_func_t)FINDFUNCTIONSYM(sgx_urts_handle, "sgx_create_enclavea");
#endif
#else
    if (strnlen(enclave_path, MAX_PATH) + strnlen(enclave_name, MAX_PATH) + sizeof(char) > MAX_PATH)
        return false;
    (void)strncat(enclave_path, enclave_name, strnlen(enclave_name, MAX_PATH));

    sgx_create_enclave_func_t p_sgx_create_enclave = (sgx_create_enclave_func_t)FINDFUNCTIONSYM(sgx_urts_handle, "sgx_create_enclave");
#endif


    if (p_sgx_create_enclave == NULL ) {
        printf("ERROR: Can't find the function sgx_create_enclave in sgx_urts library.\n");
        return false;
    }
    
    sgx_status = p_sgx_create_enclave(enclave_path,
        0,
        &launch_token,
        &launch_token_updated,
        p_eid,
        NULL);
    if (SGX_SUCCESS != sgx_status) {
        printf("Error, call sgx_create_enclave: fail [%s], SGXError:%04x.\n",__FUNCTION__, sgx_status);
        ret = false;
    }

    return ret;
}

void unload_enclave(sgx_enclave_id_t* p_eid)
{
    sgx_destroy_enclave_func_t p_sgx_destroy_enclave = (sgx_destroy_enclave_func_t)FINDFUNCTIONSYM(sgx_urts_handle, "sgx_destroy_enclave");
    if (p_sgx_destroy_enclave == NULL) {
        printf("ERROR: Can't find the function sgx_destory_enclave in sgx_urts library.\n");
        return;
    }
    p_sgx_destroy_enclave(*p_eid);
}


#if defined(_MSC_VER)
bool create_app_enclave_report(sgx_target_info_t& qe_target_info, sgx_report_t *app_report)
{
    bool ret = true;
    uint32_t retval = 0;
    sgx_status_t sgx_status = SGX_SUCCESS;
    sgx_enclave_id_t eid = 0;

    // try to sgx_urts library to create enclave.
    sgx_urts_handle = LoadLibrary(SGX_URTS_LIBRARY);
    if (sgx_urts_handle == NULL) {
        printf("ERROR: didn't find the sgx_urts.dll library, please make sure you have installed PSW installer package. \n");
        return false;
    }

    ret = load_enclave(TOOL_ENCLAVE_NAME, &eid);
    if (ret == false) {
        goto CLEANUP;
    }

    // Get the app enclave report targeting the QE3
    sgx_status = enclave_create_report(eid,
        &retval,
        &qe_target_info,
        app_report);
    if ((SGX_SUCCESS != sgx_status) || (0 != retval)) {
        printf("\nCall to get_app_enclave_report() failed\n");
        ret = false;
        goto CLEANUP;
    }

CLEANUP:
    if (eid != 0) {
        unload_enclave(&eid);
    }

    if(sgx_urts_handle) {
        CLOSELIBRARYHANDLE(sgx_urts_handle);
    }
    return ret;
}

#endif

// for multi-package platform, get the platform manifet
// return value:
//  UEFI_OPERATION_SUCCESS: successfully get the platform manifest.
//  UEFI_OPERATION_VARIABLE_NOT_AVAILABLE: it means platform manifest is not avaible: it is not multi-package platform or platform manifest has been consumed.
//  UEFI_OPERATION_LIB_NOT_AVAILABLE: it means that the uefi shared library doesn't exist
//  UEFI_OPERATION_FAIL:  it is one add package request, now we don't support it. 
//  UEFI_OPERATION_UNEXPECTED_ERROR: error happens.
uefi_status_t get_platform_manifest(uint8_t ** buffer, uint16_t &out_buffer_size)
{
    uefi_status_t ret = UEFI_OPERATION_UNEXPECTED_ERROR;
#ifdef _MSC_VER
    HINSTANCE uefi_lib_handle = LoadLibrary(SGX_MULTI_PACKAGE_AGENT_UEFI_LIBRARY);
    if (uefi_lib_handle != NULL) {
        PRINT_MESSAGE("Found the UEFI library. \n");
    }
    else {
        out_buffer_size = 0;
        buffer = NULL;
        printf("Warning: If this is a multi-package platform, please install registration agent package.\n");
        printf("         otherwise, the platform manifest information will NOT be retrieved.\n");
        return UEFI_OPERATION_LIB_NOT_AVAILABLE;
    }
#else
    void *uefi_lib_handle = dlopen(SGX_MULTI_PACKAGE_AGENT_UEFI_LIBRARY, RTLD_LAZY);
    if (uefi_lib_handle != NULL) {
        PRINT_MESSAGE("Found the UEFI library. \n");
    }
    else {
        out_buffer_size = 0;
        buffer = NULL;
        printf("Warning: If this is a multi-package platform, please install registration agent package.\n");
        printf("         otherwise, the platform manifest information will NOT be retrieved.\n");
        return UEFI_OPERATION_LIB_NOT_AVAILABLE;
    }
#endif
    mp_uefi_init_func_t p_mp_uefi_init = (mp_uefi_init_func_t)FINDFUNCTIONSYM(uefi_lib_handle, "mp_uefi_init");
    mp_uefi_get_request_type_func_t p_mp_uefi_get_request_type = (mp_uefi_get_request_type_func_t)FINDFUNCTIONSYM(uefi_lib_handle, "mp_uefi_get_request_type");
    mp_uefi_get_request_func_t p_mp_uefi_get_request = (mp_uefi_get_request_func_t)FINDFUNCTIONSYM(uefi_lib_handle, "mp_uefi_get_request");
    mp_uefi_get_registration_status_func_t p_mp_uefi_get_registration_status = (mp_uefi_get_registration_status_func_t)FINDFUNCTIONSYM(uefi_lib_handle, "mp_uefi_get_registration_status");
    mp_uefi_terminate_func_t p_mp_uefi_terminate = (mp_uefi_terminate_func_t)FINDFUNCTIONSYM(uefi_lib_handle, "mp_uefi_terminate");
    if (p_mp_uefi_init == NULL ||
        p_mp_uefi_get_request_type == NULL ||
        p_mp_uefi_get_request == NULL ||
        p_mp_uefi_get_registration_status == NULL ||
        p_mp_uefi_terminate == NULL) {
        printf("Error: cound't find uefi function interface(s) in the UEFI shared library.\n");
        CLOSELIBRARYHANDLE(uefi_lib_handle);
        return ret;
    }

    MpResult mpResult = MP_SUCCESS;
    MpRequestType type = MP_REQ_NONE;
    mpResult = p_mp_uefi_init(EFIVARS_FILE_SYSTEM_IN_OS, MP_REG_LOG_LEVEL_NONE);
    if (mpResult != MP_SUCCESS) {
        printf("Error: couldn't init UEFI shared library.\n");
        return ret;
    }
    do {
        mpResult = p_mp_uefi_get_request_type(&type);
        if (mpResult == MP_SUCCESS) {
            if (type == MP_REQ_REGISTRATION) {
                *buffer = new (std::nothrow) unsigned char[UINT16_MAX];
                mpResult = p_mp_uefi_get_request(*buffer, &out_buffer_size);
                if (mpResult != MP_SUCCESS) {
                    printf("Error: Couldn't get the platform manifest information.\n");
                    break;
                }
            }
            else if (type == MP_REQ_ADD_PACKAGE) {
                printf("Error: Add Package type is not supported.\n");
                ret = UEFI_OPERATION_FAIL;
                break;
            }
            else {
                printf("Warning: platform manifest is not available or current platform is not multi-package platform.\n");
                ret = UEFI_OPERATION_VARIABLE_NOT_AVAILABLE;
                break;
            }
        }
        else {
            MpRegistrationStatus status;
            MpResult mpResult_registration_status = p_mp_uefi_get_registration_status(&status);
            if (mpResult != MP_SUCCESS) {
                printf("Warning: error happens when get registration status, the error code is: %d \n", mpResult_registration_status);
                break;
            }
            if(status.registrationStatus == MP_TASK_COMPLETED){
                printf("Warning: registration has completed, so platform manifest has been removed. \n");
                ret = UEFI_OPERATION_VARIABLE_NOT_AVAILABLE;
                break;
            }
            else {
                printf("Error: get UEFI request type error, and the error code is: %d.\n", mpResult);
                break;
            }
        }
        ret = UEFI_OPERATION_SUCCESS;
    } while (0);
    p_mp_uefi_terminate();

    if (uefi_lib_handle != NULL) {
        CLOSELIBRARYHANDLE(uefi_lib_handle);
    }
    return ret;
}


// for multi-package platform, set registration status 
// return value:
//  UEFI_OPERATION_SUCCESS: successfully set the platform's registration status.
//  UEFI_OPERATION_LIB_NOT_AVAILABLE: it means that the uefi shared library doesn't exist, maybe the registration agent package is not installed
//  UEFI_OPERATION_UNEXPECTED_ERROR: error happens.
uefi_status_t set_registration_status()
{
    uefi_status_t ret = UEFI_OPERATION_UNEXPECTED_ERROR;
#ifdef _MSC_VER
    HINSTANCE uefi_lib_handle = LoadLibrary(SGX_MULTI_PACKAGE_AGENT_UEFI_LIBRARY);
    if (uefi_lib_handle != NULL) {
        PRINT_MESSAGE("Found the UEFI library. \n");
    }
    else {
        printf("Warning: If this is a multi-package platform, please install registration agent package.\n");
        printf("         otherwise, the platform manifest information will NOT be retrieved.\n");
        return UEFI_OPERATION_LIB_NOT_AVAILABLE;
    }
#else
    void *uefi_lib_handle = dlopen(SGX_MULTI_PACKAGE_AGENT_UEFI_LIBRARY, RTLD_LAZY);
    if (uefi_lib_handle != NULL) {
        PRINT_MESSAGE("Found the UEFI library. \n");
    }
    else {
        printf("Warning: If this is a multi-package platform, please install registration agent package.\n");
        printf("         otherwise, the platform manifest information will NOT be retrieved.\n");
        return UEFI_OPERATION_LIB_NOT_AVAILABLE;
    }
#endif
    mp_uefi_init_func_t p_mp_uefi_init = (mp_uefi_init_func_t)FINDFUNCTIONSYM(uefi_lib_handle, "mp_uefi_init");
    mp_uefi_set_registration_status_func_t p_mp_uefi_set_registration_status = (mp_uefi_set_registration_status_func_t)FINDFUNCTIONSYM(uefi_lib_handle, "mp_uefi_set_registration_status");
    mp_uefi_terminate_func_t p_mp_uefi_terminate = (mp_uefi_terminate_func_t)FINDFUNCTIONSYM(uefi_lib_handle, "mp_uefi_terminate");
    if (p_mp_uefi_init == NULL ||
        p_mp_uefi_set_registration_status == NULL ||
        p_mp_uefi_terminate == NULL) {
        printf("Error: cound't find uefi function interface(s) in the multi-package agent shared library.\n");
        CLOSELIBRARYHANDLE(uefi_lib_handle);
        return ret;
    }

    MpResult mpResult = MP_SUCCESS;
    MpRegistrationStatus status;
    mpResult = p_mp_uefi_init(EFIVARS_FILE_SYSTEM_IN_OS, MP_REG_LOG_LEVEL_NONE);
    if (mpResult != MP_SUCCESS) {
        printf("Error: couldn't init uefi shared library.\n");
        CLOSELIBRARYHANDLE(uefi_lib_handle);
        return ret;
    }

    status.registrationStatus = MP_TASK_COMPLETED;
    status.errorCode = MPA_SUCCESS;
    mpResult = p_mp_uefi_set_registration_status(&status);
    if (mpResult != MP_SUCCESS) {
        printf("Warning: error happens when set registration status, the error code is: %d \n", mpResult);
    }
    else {
        ret = UEFI_OPERATION_SUCCESS;
    }
    
    p_mp_uefi_terminate();

    if (uefi_lib_handle != NULL) {
        CLOSELIBRARYHANDLE(uefi_lib_handle);
    }
    return ret;
}



// generate ecdsa quote
// return value:
//  0: successfully generate the ecdsa quote
// -1: error happens.

#ifdef _MSC_VER
int generate_quote(uint8_t **quote_buffer, uint32_t& quote_size)
{
    int ret = -1;
    quote3_error_t qe3_ret = SGX_QL_SUCCESS;
    sgx_target_info_t qe_target_info;
    sgx_report_t app_report;

    // try to load quote provide library.
    HINSTANCE quote_provider_library_handle = LoadLibrary(SGX_QL_QUOTE_CONFIG_LIB_FILE_NAME);
    if (quote_provider_library_handle != NULL) {
        PRINT_MESSAGE("Found the Quote provider library. \n");
    }
    else {
        printf("Warning: didn't find the quote provider library. \n");
    }  
    
    // try to sgx dcap quote generation library to generate quote.
    HINSTANCE sgx_dcap_ql_handle = LoadLibrary(SGX_DCAP_QUOTE_GENERATION_LIBRARY);
    if (sgx_dcap_ql_handle == NULL) {
        printf("ERROR: didn't find the sgx_dcap_ql.dll library, please make sure you have installed DCAP INF installer package. \n");
        CLOSELIBRARYHANDLE(quote_provider_library_handle);
        return ret;
    }
    
    sgx_qe_get_target_info_func_t p_sgx_qe_get_target_info = (sgx_qe_get_target_info_func_t)FINDFUNCTIONSYM(sgx_dcap_ql_handle, "sgx_qe_get_target_info");
    sgx_qe_get_quote_size_func_t p_sgx_qe_get_quote_size = (sgx_qe_get_quote_size_func_t)FINDFUNCTIONSYM(sgx_dcap_ql_handle, "sgx_qe_get_quote_size");
    sgx_qe_get_quote_func_t p_sgx_qe_get_quote = (sgx_qe_get_quote_func_t)FINDFUNCTIONSYM(sgx_dcap_ql_handle, "sgx_qe_get_quote");
    if (p_sgx_qe_get_target_info == NULL || p_sgx_qe_get_quote_size == NULL || p_sgx_qe_get_quote == NULL) {
        printf("ERROR: Can't find the quote generation functions in sgx dcap quote generation shared library.\n");
        if (quote_provider_library_handle != NULL) {
            CLOSELIBRARYHANDLE(quote_provider_library_handle);
        }
        if (sgx_dcap_ql_handle != NULL) {
            CLOSELIBRARYHANDLE(sgx_dcap_ql_handle);
        }
        return ret;
    }
    do {
        PRINT_MESSAGE("\nStep1: Call sgx_qe_get_target_info:");
        qe3_ret = p_sgx_qe_get_target_info(&qe_target_info);
        if (SGX_QL_SUCCESS != qe3_ret) {
            printf("Error in sgx_qe_get_target_info. 0x%04x\n", qe3_ret);
            break;
        }

        PRINT_MESSAGE("succeed! \nStep2: Call create_app_report:");
        if (true != create_app_enclave_report(qe_target_info, &app_report)) {
            printf("\nCall to create_app_report() failed\n");
            break;
        }

        PRINT_MESSAGE("succeed! \nStep3: Call sgx_qe_get_quote_size:");
        qe3_ret = p_sgx_qe_get_quote_size(&quote_size);
        if (SGX_QL_SUCCESS != qe3_ret) {
            printf("Error in sgx_qe_get_quote_size. 0x%04x\n", qe3_ret);
            break;
        }

        PRINT_MESSAGE("succeed!");
        *quote_buffer = (uint8_t*)malloc(quote_size);
        if (NULL == *quote_buffer) {
            printf("Couldn't allocate quote_buffer\n");
            break;
        }
        memset(*quote_buffer, 0, quote_size);

        // Get the Quote
        PRINT_MESSAGE("\nStep4: Call sgx_qe_get_quote:");
        qe3_ret = p_sgx_qe_get_quote(&app_report, quote_size, *quote_buffer);
        if (SGX_QL_SUCCESS != qe3_ret) {
            printf("Error in sgx_qe_get_quote. 0x%04x\n", qe3_ret);
            break;
        }
        PRINT_MESSAGE("succeed!\n");
        ret = 0;
    } while (0);

    if (quote_provider_library_handle != NULL) {
        CLOSELIBRARYHANDLE(quote_provider_library_handle);
    }
    if (sgx_dcap_ql_handle != NULL) {
        CLOSELIBRARYHANDLE(sgx_dcap_ql_handle);
    }
    return ret;
}
#else
int collect_data(uint8_t **pp_data_buffer)
{
    sgx_status_t sgx_status = SGX_SUCCESS;
    sgx_status_t ecall_ret = SGX_SUCCESS;
    sgx_key_128bit_t platform_id = { 0 };
    int ret = 0;
    uint32_t buffer_size = 0;
    uint8_t * p_temp = NULL;

    sgx_enclave_id_t pce_enclave_eid = 0;
    sgx_enclave_id_t id_enclave_eid = 0;
    
    sgx_report_t id_enclave_report;
    uint32_t enc_key_size = REF_RSA_OAEP_3072_MOD_SIZE + REF_RSA_OAEP_3072_EXP_SIZE;
    uint8_t enc_public_key[REF_RSA_OAEP_3072_MOD_SIZE + REF_RSA_OAEP_3072_EXP_SIZE];
    uint8_t encrypted_ppid[REF_RSA_OAEP_3072_MOD_SIZE];
    uint32_t encrypted_ppid_ret_size;
    pce_info_t pce_info;
    uint8_t signature_scheme;
    sgx_target_info_t pce_target_info;

    sgx_get_target_info_func_t p_sgx_get_target_info = NULL;

    bool load_flag = get_urts_library_handle();
    if(false == load_flag) {// can't find urts shared library to load enclave
        ret = -1;
        goto CLEANUP;
    }

    load_flag = load_enclave(ID_ENCLAVE_NAME, &id_enclave_eid);
    if(false == load_flag) { // can't load id_enclave.
        ret = -1;
        goto CLEANUP;
    }

    sgx_status = ide_get_id(id_enclave_eid, &ecall_ret, &platform_id);
    if (SGX_SUCCESS != sgx_status) {
        fprintf(stderr, "Failed to call into the ID_ENCLAVE:get_qe_id. 0x%04x.\n", sgx_status);
        ret = -1;
        goto CLEANUP;
    }

    if (SGX_SUCCESS != ecall_ret) {
        fprintf(stderr, "Failed to get QE_ID. 0x%04x.\n", ecall_ret);
        ret = -1;
        goto CLEANUP;
    }


    load_flag = load_enclave(PCE_ENCLAVE_NAME, &pce_enclave_eid);
    if(false == load_flag) { // can't load pce enclave.
        ret = -1;
        goto CLEANUP;
    }

    p_sgx_get_target_info = (sgx_get_target_info_func_t)FINDFUNCTIONSYM(sgx_urts_handle, "sgx_get_target_info");
    if (p_sgx_get_target_info == NULL) {
        printf("ERROR: Can't find the function sgx_get_target_info in sgx_urts library.\n");
        ret = -1;
        goto CLEANUP;
    }

    sgx_status = p_sgx_get_target_info(pce_enclave_eid, &pce_target_info);
    if (SGX_SUCCESS != sgx_status) {
        fprintf(stderr, "Failed to get pce target info. The error code is:  0x%04x.\n", sgx_status);
        ret = -1;
        goto CLEANUP;
    }

    sgx_status = ide_get_pce_encrypt_key(id_enclave_eid,
                                         &ecall_ret,
                                         &pce_target_info,
                                         &id_enclave_report,
                                         PCE_ALG_RSA_OAEP_3072,
                                         PPID_RSA3072_ENCRYPTED,
                                         enc_key_size,
                                         enc_public_key);
    if (SGX_SUCCESS != sgx_status) {
        fprintf(stderr, "Failed to call into the ID_ENCLAVE: get_report_and_pce_encrypt_key. The error code is: 0x%04x.\n", sgx_status);
        ret = -1;
        goto CLEANUP;
    }

    if (SGX_SUCCESS != ecall_ret) {
        fprintf(stderr, "Failed to generate PCE encryption key. The error code is: 0x%04x.\n", ecall_ret);
        ret = -1;
        goto CLEANUP;
    }

    sgx_status = get_pc_info(pce_enclave_eid,
                              (uint32_t*) &ecall_ret,
                              &id_enclave_report,
                              enc_public_key,
                              enc_key_size,
                              PCE_ALG_RSA_OAEP_3072,
                              encrypted_ppid,
                              REF_RSA_OAEP_3072_MOD_SIZE,
                              &encrypted_ppid_ret_size,
                              &pce_info,
                              &signature_scheme);
    if (SGX_SUCCESS != sgx_status) {
        fprintf(stderr, "Failed to call into PCE enclave: get_pc_info. The error code is: 0x%04x.\n", sgx_status);
        ret = -1;
        goto CLEANUP;
    }
    if (SGX_SUCCESS != ecall_ret) {
        fprintf(stderr, "Failed to get PCE info. The error code is: 0x%04x.\n", ecall_ret);
        ret = -1;
        goto CLEANUP;
    }

    if (signature_scheme != PCE_NIST_P256_ECDSA_SHA256) {
        fprintf(stderr, "PCE returned incorrect signature scheme.\n");
        ret = -1;
        goto CLEANUP;
    }

    if (encrypted_ppid_ret_size != ENCRYPTED_PPID_LENGTH) {
        fprintf(stderr, "PCE returned unexpected returned encrypted PPID size.\n");
        ret = -1;
        goto CLEANUP;
    }

    buffer_size = ENCRYPTED_PPID_LENGTH + CPU_SVN_LENGTH + ISV_SVN_LENGTH + PCE_ID_LENGTH + DEFAULT_PLATFORM_ID_LENGTH;
    *pp_data_buffer = (uint8_t *) malloc(buffer_size);

    if (NULL == *pp_data_buffer) {
        fprintf(stderr,"Couldn't allocate data buffer\n");
        ret = -1;
        goto CLEANUP;
    }
    memset(*pp_data_buffer, 0, buffer_size);
    p_temp = *pp_data_buffer;
    //encrypted ppid
    memcpy(p_temp, encrypted_ppid, ENCRYPTED_PPID_LENGTH);
    
    //pce id
    p_temp = p_temp + ENCRYPTED_PPID_LENGTH;
    memcpy(p_temp , &(pce_info.pce_id), PCE_ID_LENGTH);

    //cpu svn
    p_temp = p_temp + PCE_ID_LENGTH;
    memcpy(p_temp , id_enclave_report.body.cpu_svn.svn, CPU_SVN_LENGTH);
    
    //pce isv svn
    p_temp = p_temp + CPU_SVN_LENGTH;
    memcpy(p_temp , &(pce_info.pce_isvn), ISV_SVN_LENGTH);
    
    //platform id
    p_temp = p_temp + ISV_SVN_LENGTH;
    memcpy(p_temp , platform_id, DEFAULT_PLATFORM_ID_LENGTH);

    
CLEANUP:
    if(pce_enclave_eid != 0) {
        unload_enclave(&pce_enclave_eid);
    }
    if(id_enclave_eid != 0) {
        unload_enclave(&id_enclave_eid);
    }
    close_urts_library_handle();
    return ret;

}
#endif

bool is_valid_proxy_type(std::string& proxy_type) {
    if (proxy_type.compare("DEFAULT") == 0 ||
        proxy_type.compare("MANUAL")  == 0 ||
        proxy_type.compare("AUTO")    == 0 ||
        proxy_type.compare("DIRECT")  == 0 ) { 
        return true;
    }
    else {
        return false;
    }                
}

bool is_valid_use_secure_cert(std::string& use_secure_cert) {
    if (use_secure_cert.compare("TRUE") == 0 ||
        use_secure_cert.compare("FALSE") == 0 ) {
        return true;
    }
    else {
        return false;
    }  
}
