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
 * File: sgx_dcap_ql_wrapper.cpp
 *
 * Description: Wrapper around the core
 * SGX Quote Library to expose the ECDSA
 * specific API requred by DCAP SDK.
 */

#include <string.h>
#include <stdio.h>

#include "user_types.h"
#include "sgx_report.h"
#include "sgx_dcap_ql_wrapper.h"
#include "sgx_ql_core_wrapper.h"
#include "sgx_quote_3.h"
#include "se_trace.h"


static bool g_out_of_proc = false;

#ifndef _MSC_VER
#include "sgx_uae_quote_ex.h"
#include "sgx_pce.h"
#include <stdlib.h>
#define SGX_AESM_ADDR "SGX_AESM_ADDR"

typedef sgx_status_t (*func_sgx_init_quote_ex_t)(const sgx_att_key_id_t* p_att_key_id,
                                               sgx_target_info_t *p_qe_target_info,
                                               size_t* p_pub_key_id_size,
                                               uint8_t* p_pub_key_id);
typedef sgx_status_t (*func_sgx_get_quote_size_ex_t)(const sgx_att_key_id_t *p_att_key_id,
                                                   uint32_t* p_quote_size);
typedef sgx_status_t (*func_sgx_get_quote_ex_t)(const sgx_report_t *p_app_report,
                                              const sgx_att_key_id_t *p_att_key_id,
                                              sgx_qe_report_info_t *p_qe_report_info,
                                              uint8_t *p_quote,
                                              uint32_t quote_size);

static void* g_dlopen_handle = NULL;
#define SGX_QUOTE_EX_SO "libsgx_quote_ex.so.1"
#define SGX_GET_QUOTE_EX "sgx_get_quote_ex"
#define SGX_INIT_QUOTE_EX "sgx_init_quote_ex"
#define SGX_GET_QUOTE_SIZE_EX "sgx_get_quote_size_ex"

static sgx_att_key_id_t g_att_keyid = {};
static func_sgx_init_quote_ex_t g_init_quote_ex = NULL;
static func_sgx_get_quote_size_ex_t g_get_quote_size_ex = NULL;
static func_sgx_get_quote_ex_t g_get_quote_ex = NULL;


static quote3_error_t sgx_status_to_quote3_error(sgx_status_t sgx_status)
{
    quote3_error_t ret = SGX_QL_SUCCESS;
    switch (sgx_status)
    {
    case SGX_SUCCESS:
        ret = SGX_QL_SUCCESS;
        break;

        // Common errors
    case SGX_ERROR_UNEXPECTED:
        ret = SGX_QL_ERROR_UNEXPECTED;
        break;
    case SGX_ERROR_INVALID_PARAMETER:
        ret = SGX_QL_ERROR_INVALID_PARAMETER;
        break;
    case SGX_ERROR_OUT_OF_MEMORY:
        ret = SGX_QL_ERROR_OUT_OF_MEMORY;
        break;

        // Device/Driver related errors
    case SGX_ERROR_NO_DEVICE:
        ret = SGX_QL_NO_DEVICE;
        break;

        // RPC related erros
    case SGX_ERROR_SERVICE_UNAVAILABLE:
        ret = SGX_QL_SERVICE_UNAVAILABLE;
        break;
    case SGX_ERROR_NETWORK_FAILURE:
        ret = SGX_QL_NETWORK_FAILURE;
        break;
    case SGX_ERROR_SERVICE_TIMEOUT:
        ret = SGX_QL_SERVICE_TIMEOUT;
        break;
    case SGX_ERROR_BUSY:
        ret = SGX_QL_ERROR_BUSY;
        break;

        // ECDSA related errors
    case SGX_ERROR_UNSUPPORTED_ATT_KEY_ID:
        ret = SGX_QL_UNSUPPORTED_ATT_KEY_ID;
        break;
    case SGX_ERROR_ATT_KEY_CERTIFICATION_FAILURE:
        ret = SGX_QL_ATT_KEY_CERT_DATA_INVALID;
        break;
    case SGX_ERROR_PLATFORM_CERT_UNAVAILABLE:
        ret = SGX_QL_NO_PLATFORM_CERT_DATA;
        break;
    case SGX_ERROR_ATT_KEY_UNINITIALIZED:
        ret = SGX_QL_ATT_KEY_NOT_INITIALIZED;
        break;
    default:
        ret = SGX_QL_ERROR_UNEXPECTED;
        break;
    }
    return(ret);
}

#include "se_thread.h"
#include <dlfcn.h>

static se_mutex_t g_dlopen_mutex;

static void __attribute__((constructor)) _sgx_dcap_ql_init()
{
    char *out_of_proc = getenv(SGX_AESM_ADDR);
    se_mutex_init(&g_dlopen_mutex);
    if(out_of_proc)
    {
        g_out_of_proc = true;
    }
    sgx_ql_get_keyid((sgx_att_key_id_ext_t *)&g_att_keyid);
}

static void close_sofile(void)
{
    se_mutex_lock(&g_dlopen_mutex);
    if (g_dlopen_handle != NULL) {
        dlclose(g_dlopen_handle);
        g_dlopen_handle = NULL;
    }
    se_mutex_unlock(&g_dlopen_mutex);
}

static void __attribute__((destructor)) _sgx_dcap_ql_fini(void)
{
    close_sofile();
    se_mutex_destroy(&g_dlopen_mutex);
}

static func_sgx_init_quote_ex_t init_quote_ex_function(void)
{
    if (g_init_quote_ex == NULL) {
        se_mutex_lock(&g_dlopen_mutex);
        if (g_init_quote_ex != NULL)
        {
            se_mutex_unlock(&g_dlopen_mutex);
            return g_init_quote_ex;
        }

        if (g_dlopen_handle == NULL) {
            g_dlopen_handle = dlopen(SGX_QUOTE_EX_SO, RTLD_LAZY);
            if (g_dlopen_handle == NULL) {
                se_mutex_unlock(&g_dlopen_mutex);
                return NULL;
            }
        }

        g_init_quote_ex = (func_sgx_init_quote_ex_t)dlsym(g_dlopen_handle, SGX_INIT_QUOTE_EX);
        se_mutex_unlock(&g_dlopen_mutex);
    }

    return g_init_quote_ex;
}

static func_sgx_get_quote_size_ex_t get_quote_size_ex_function(void)
{
    if (g_get_quote_size_ex == NULL) {
        se_mutex_lock(&g_dlopen_mutex);
        if (g_get_quote_size_ex != NULL)
        {
            se_mutex_unlock(&g_dlopen_mutex);
            return g_get_quote_size_ex;
        }

        if (g_dlopen_handle == NULL) {
            g_dlopen_handle = dlopen(SGX_QUOTE_EX_SO, RTLD_LAZY);
            if (g_dlopen_handle == NULL) {
                se_mutex_unlock(&g_dlopen_mutex);
                return NULL;
            }
        }

        g_get_quote_size_ex = (func_sgx_get_quote_size_ex_t)dlsym(g_dlopen_handle, SGX_GET_QUOTE_SIZE_EX);
        se_mutex_unlock(&g_dlopen_mutex);
    }

    return g_get_quote_size_ex;
}

static func_sgx_get_quote_ex_t get_quote_ex_function(void)
{
    if (g_get_quote_ex == NULL) {
        se_mutex_lock(&g_dlopen_mutex);
        if (g_get_quote_ex != NULL)
        {
            se_mutex_unlock(&g_dlopen_mutex);
            return g_get_quote_ex;
        }

        if (g_dlopen_handle == NULL) {
            g_dlopen_handle = dlopen(SGX_QUOTE_EX_SO, RTLD_LAZY);
            if (g_dlopen_handle == NULL) {
                se_mutex_unlock(&g_dlopen_mutex);
                return NULL;
            }
        }

        g_get_quote_ex = (func_sgx_get_quote_ex_t)dlsym(g_dlopen_handle, SGX_GET_QUOTE_EX);
        se_mutex_unlock(&g_dlopen_mutex);
    }

    return g_get_quote_ex;
}

#endif


/**
 * When the Quoting Library is linked to a process, it needs to know the proper enclave loading policy.  The library
 * may be linked with a long lived process, such as a service, where it can load the enclaves and leave them loaded
 * (persistent).  This better ensures that the enclaves will be available upon quote requests and not subject to EPC
 * limitations if loaded on demand. However, if the Quoting library is linked with an application process, there may be
 * many applications with the Quoting library and a better utilization of EPC is to load and unloaded the quoting
 * enclaves on demand (ephemeral).  The library will be shipped with a default policy of loading enclaves and leaving
 * them loaded until the library is unloaded (PERSISTENT). If the policy is set to EPHEMERAL, then the QE and PCE will
 * be loaded and unloaded on-demand.  If either enclave is already loaded when the policy is change to EPHEMERAL, the
 * enclaves will be unloaded before returning.
 *
 * @param policy Sets the requested enclave loading policy to either SGX_QL_PERSISTENT, SGX_QL_EPHEMERAL or
 *               SGX_QL_DEFAULT.
 *
 * @return SGX_QL_SUCCESS Successfully set the enclave loading policy for the quoting library's enclaves.
 * @return SGX_QL_UNSUPPORTED_LOADING_POLICY The selected policy is not support by the quoting library.
 * @return SGX_QL_UNSUPPORTED_MODE This function is called on Windows or in out-of-process mode.
 * @return SGX_QL_ERROR_UNEXPECTED Unexpected internal error.
 *
 */
extern "C" quote3_error_t sgx_qe_set_enclave_load_policy(sgx_ql_request_policy_t policy)
{
    quote3_error_t quote_ret = SGX_QL_UNSUPPORTED_MODE;

    if(false == g_out_of_proc)
        quote_ret = sgx_ql_set_enclave_load_policy(policy);

    return(quote_ret);
}

/**
 * This API will allow the calling code to retrieve the target info of the QE.  This maps to the Initialize Quote
 * Attestation Key API of the core library.  It will be a simple wrapper around the core library's
 * sgx_ql_init_quote() API. The loading of the QE and the PCE will follow the selected loading policy.
 *
 * It is during this API that the Quoting Library will generate and certify the attestation key.  The key and
 * certification data will be stored in process memory for use by the sgx_qe_get_quote_size() and sgx_get_quote()
 * APIs.  Generating and certifying the keys at this point makes the following APIs more efficient.  If the
 * following APIs return the SGX_QL_ATT_KEY_NOT_INITIALIZED error, this API needs to be called again to
 * regenerate and recertify the key.
 *
 * @param  p_qe_target_info Pointer the buffer that will contain the QE's target information.  This is used by the
 *                          application's enclave to generate a REPORT verifiable by the QE.  Must not be NULL.
 *
 * @return SGX_QL_SUCCESS Successfully retrieved the target information.
 * @return SGX_QL_ERROR_INVALID_PARAMETER Invalid parameter if p_target_info is NULL
 * @return SGX_QL_ERROR_UNEXPECTED Unexpected internal error.
 * @return SGX_QL_OUT_OF_EPC There is not enough EPC memory to load one of the Architecture Enclaves needed to
 *         complete this operation.
 * @return SGX_QL_ERROR_OUT_OF_MEMORY Heap memory allocation error in library or enclave.
 * @return SGX_QL_ENCLAVE_LOAD_ERROR Unable to load the enclaves required to initialize the attestation key.
 *         Could be due to file I/O error, loading infrastructure error.
 * @return SGX_QL_ENCLAVE_LOST Enclave lost after power transition or used in child process created by
 *         linux:fork().
 *
 */
extern "C" quote3_error_t sgx_qe_get_target_info(sgx_target_info_t *p_qe_target_info)
{
    quote3_error_t quote_ret = SGX_QL_ERROR_UNEXPECTED;
    bool refresh_att_key;
    size_t pub_key_id_size_out;
    ref_sha256_hash_t pub_key_id_out;

    // Verify inputs
    if(NULL == p_qe_target_info)
    {
        return(SGX_QL_ERROR_INVALID_PARAMETER);
    }

    memset(p_qe_target_info, 0, sizeof(*p_qe_target_info));

    if(false == g_out_of_proc)
    {
        // Call Init Quote
        // Generates a new ECDSA Attestation key of one does not exist.
        // Returns the qe_target_info so that the app enclave can generate a report.
        // Certifies the ECDSA Attestation key if a new key is generated.
        // Stores the ECDSA key in a file for later use
        // 1. Specify attestation key ID as an ECDSA quote using the reference ECDSA QE
        // 2. Specify the certification key type (reference only supports cleartext(PPID)+PCEID + PSVN)
        // 3. Specify whether to create a new attestation key.
        // 4. Returns the QE target info
        // 5. Returns the ECDSA_ID in pub_key_id_out
        refresh_att_key = false;  ///@todo: Consider adding the ability to refresh the key with a new KEY_ID for key wearout.
        SE_TRACE(SE_TRACE_DEBUG,"Call sgx_ql_init_quote - first to get pub_key_id_size.\n");
        quote_ret = sgx_ql_init_quote(NULL,
                                      NULL,
                                      refresh_att_key,
                                      &pub_key_id_size_out,
                                      NULL);
        if (SGX_QL_SUCCESS != quote_ret) {
            SE_TRACE(SE_TRACE_ERROR,"Error in sgx_ql_init_quote. 0x%04x\n", quote_ret);
            goto CLEANUP;
        }
        SE_TRACE(SE_TRACE_DEBUG, "Required pub key id size is: %ld\n", pub_key_id_size_out);
        if (pub_key_id_size_out != sizeof(ref_sha256_hash_t)) {
            quote_ret = SGX_QL_ERROR_UNEXPECTED;
            goto CLEANUP;
        }

        pub_key_id_size_out = sizeof(ref_sha256_hash_t);
        SE_TRACE(SE_TRACE_DEBUG, "Call sgx_ql_init_quote - second with allocated pub_key_id_buffer.\n");
        quote_ret = sgx_ql_init_quote(NULL,
                                      p_qe_target_info,
                                      refresh_att_key,
                                      &pub_key_id_size_out,
                                      (uint8_t*)&pub_key_id_out);
        if (SGX_QL_SUCCESS != quote_ret) {
            SE_TRACE(SE_TRACE_ERROR, "Error in sgx_ql_init_quote. 0x%04x\n", quote_ret);
            goto CLEANUP;
        }
        if (pub_key_id_size_out != 0) {
            SE_TRACE(SE_TRACE_DEBUG, "sgx_ql_init_quote used ECDSA_ID:\n");
            PRINT_BYTE_ARRAY(SE_TRACE_DEBUG, (uint8_t*)&pub_key_id_out, pub_key_id_size_out);
        }
    }
#ifndef _MSC_VER
    else
    {
        func_sgx_init_quote_ex_t func = init_quote_ex_function();
        if (NULL == func) {
            SE_TRACE(SE_TRACE_ERROR,"Error in get symbol %s.\n", SGX_INIT_QUOTE_EX);
            quote_ret = SGX_QL_ERROR_UNEXPECTED;
            goto CLEANUP;
        }
        quote_ret= sgx_status_to_quote3_error(func(&g_att_keyid,
                                                                 p_qe_target_info,
                                                                 &pub_key_id_size_out,
                                                                 NULL));
        if (SGX_QL_SUCCESS != quote_ret) {
            SE_TRACE(SE_TRACE_ERROR,"Error in %s. 0x%04x\n", SGX_INIT_QUOTE_EX, quote_ret);
            goto CLEANUP;
        }
        SE_TRACE(SE_TRACE_DEBUG, "Required pub key id size is: %ld\n", pub_key_id_size_out);
        if (pub_key_id_size_out != sizeof(ref_sha256_hash_t)) {
            quote_ret = SGX_QL_ERROR_UNEXPECTED;
            goto CLEANUP;
        }

        pub_key_id_size_out = sizeof(ref_sha256_hash_t);
        SE_TRACE(SE_TRACE_DEBUG, "Call sgx_init_quote_ex - second with allocated pub_key_id_buffer.\n");
        quote_ret = sgx_status_to_quote3_error(func(&g_att_keyid,
                                               p_qe_target_info,
                                               &pub_key_id_size_out,
                                               (uint8_t*)&pub_key_id_out));
        if (SGX_QL_SUCCESS != quote_ret) {
            SE_TRACE(SE_TRACE_ERROR,"Error in %s. 0x%04x\n", SGX_INIT_QUOTE_EX, quote_ret);
            goto CLEANUP;
        }
        if (pub_key_id_size_out != 0) {
            SE_TRACE(SE_TRACE_DEBUG, "%s used ECDSA_ID:\n", SGX_INIT_QUOTE_EX);
            PRINT_BYTE_ARRAY(SE_TRACE_DEBUG, (uint8_t*)&pub_key_id_out, pub_key_id_size_out);
        }
    }
#endif

    CLEANUP:
    return(quote_ret);
}

/**
 * This API is a thin wrapper around the core sgx_ql_get_quote_size() API.  The size returned in this API will indicate
 * the size of the quote buffer required in the sgx_oe_get_quote() API.
 *
 * @param p_quote_size Pointer to hold the size of the buffer in bytes required to contain the full quote.  This value
 *                     is passed in to the sgx_qe_get_quote() API.  The caller is responsible for allocating a buffer
 *                     large enough to contain the quote.
 *
 * @return SGX_QL_SUCCESS Successfully calculated the required quote size. The required size in bytes is returned in the
 *         memory pointed to by p_quote_size.
 * @return SGX_QL_ERROR_INVALID_PARAMETER Invalid parameter.  p_quote_size must not be NULL.
 * @return SGX_QL_ERROR_UNEXPECTED Unexpected internal error.
 * @return SGX_QL_ATT_KEY_NOT_INITIALIZED The platform quoting infrastructure does not have the attestation key
 *         available to generate quotes.  sgx_qe_get_target_info() must be called again.
 * @return SGX_QL_ATT_KEY_CERT_DATA_INVALID The data returned by the platform library's sgx_ql_get_quote_config() is
 *         invalid.
 * @return SGX_QL_OUT_OF_EPC There is not enough EPC memory to load one of the Architecture Enclaves needed to complete
 *         this operation.
 * @return SGX_QL_ERROR_OUT_OF_MEMORY Heap memory allocation error in library or enclave.
 * @return SGX_QL_ENCLAVE_LOAD_ERROR Unable to load the enclaves required to initialize the attestation key.  Could be
 *         due to file I/O error, loading infrastructure error or insufficient enclave memory.
 * @return SGX_QL_ENCLAVE_LOST Enclave lost after power transition or used in child process created by linux:fork().
 *
 */
extern "C" quote3_error_t sgx_qe_get_quote_size(uint32_t *p_quote_size)
{
    quote3_error_t quote_ret = SGX_QL_ERROR_UNEXPECTED;

    //Verify inputs
    if(NULL == p_quote_size)
    {
        return(SGX_QL_ERROR_INVALID_PARAMETER);
    }

    // Get the Quote size and allocate the memory
    SE_TRACE(SE_TRACE_DEBUG, "Call sgx_ql_get_quote_size.\n");
    if(false == g_out_of_proc)
    {
        quote_ret = sgx_ql_get_quote_size(NULL,
                                          p_quote_size);
        if (SGX_QL_SUCCESS != quote_ret) {
            SE_TRACE(SE_TRACE_ERROR, "Error in sgx_ql_get_quote_size. 0x%04x\n", quote_ret);
            goto CLEANUP;
        }
        SE_TRACE(SE_TRACE_DEBUG, "quote_size = %d\n",*p_quote_size);
    }
#ifndef _MSC_VER
    else
    {
        func_sgx_get_quote_size_ex_t func = get_quote_size_ex_function();
        if (NULL == func) {
            SE_TRACE(SE_TRACE_ERROR,"Error in get symbol %s.\n", SGX_GET_QUOTE_SIZE_EX);
            quote_ret = SGX_QL_ERROR_UNEXPECTED;
            goto CLEANUP;
        }
        quote_ret = sgx_status_to_quote3_error(func(&g_att_keyid, p_quote_size));
        if (SGX_QL_SUCCESS != quote_ret) {
            SE_TRACE(SE_TRACE_ERROR, "Error in %s. 0x%04x\n", SGX_GET_QUOTE_SIZE_EX, quote_ret);
            goto CLEANUP;
        }
        SE_TRACE(SE_TRACE_DEBUG, "quote_size = %d\n",*p_quote_size);
    }
#endif

    CLEANUP:
    ///todo:  For DCAP, consider calling sgx_qe_get_target_info if the
    // error code indicates the attestation key doesn't exist.  For typical usages, you
    // want to separate key generation flows from the quote generation flows but the
    // initial DCAP usage doesn't need this separation

    return(quote_ret);
}

/**
 * This API is a thin wrapper around the core sgx_ql_get_quote() API and is used to return the quote for an
 * application's enclave.
 *
 * The function will take the application enclave's REPORT that will be converted into a quote after the QE verifies
 * the REPORT.  Once verified it will sign it with platform's attestation key matching the selected attestation key
 * ID.  If the key is not available, this API may return an error (SGX_QL_ATT_KEY_NOT_INITIALIZED) depending on the
 * algorithm.  In this case, the caller must call sgx_qe_get_target_info() to get the library to
 * re-generate/re-certify the attestation key.
 *
 * For DCAP, the Quote.Header.UserData[0..15] will contain the 128bit platform identifier (QE_ID) based on the
 * QE's Seal Key at TCB 0.  This will allow the DCAP infrastructure to link a quote generated on the platform
 * with the platform's PCK Cert.  Also, for DCAP, the Quote Library will always generate an attestation key
 * that is consistent for a given platform at a given TCB without requiring persistent storage.  The key will also be
 * consistent for the given platform for all VMs.  The key will change when the raw TCB of the platform changes.
 *
 * @param p_app_report Pointer to the application enclave's REPORT that needs the quote.  The report needs to be
 *                     generated using the QE's target info returned by the sgx_qe_get_target_info() API.  Must not be
 *                     NULL.
 * @param quote_size Size of the buffer pointed to by p_quote (in bytes).
 * @param p_quote Pointer to the buffer that will contain the generated quote.  Must not be NULL.
 *
 * @return SGX_QL_SUCCESS Successfully calculated the required quote size. The required size is returned in the memory
 *         pointed to by p_quote_size.
 * @return SGX_QL_ERROR_UNEXPECTED An unexpected internal error occurred.
 * @return SGX_QL_ERROR_INVALID_PARAMETER If either p_app_report or p_quote is null. Or, if quote_size isn't large
 *         enough.
 * @return SGX_QL_ATT_KEY_NOT_INITIALIZED The platform quoting infrastructure does not have the attestation key
 *         available to generate quotes.  sgx_qe_get_target_info() must be called again.
 * @return SGX_QL_ATT_KEY_CERT_DATA_INVALID The data returned by the platform library's sgx_ql_get_quote_config() is
 *         invalid.
 * @return SGX_QL_OUT_OF_EPC There is not enough EPC memory to load one of the Architecture Enclaves needed to complete
 *         this operation.
 * @return SGX_QL_ERROR_OUT_OF_MEMORY Heap memory allocation error in library or enclave.
 * @return SGX_QL_ENCLAVE_LOST Enclave lost after power transition or used in child process created by linux:fork().
 * @return SGX_QL_ENCLAVE_LOAD_ERROR Unable to load the enclaves required to initialize the attestation key.  Could be
 *         due to file I/O error, loading infrastructure error or insufficient enclave memory.
 *
 */
extern "C" quote3_error_t sgx_qe_get_quote(const sgx_report_t *p_app_report,
                                           uint32_t quote_size,
                                           uint8_t *p_quote)
{
    quote3_error_t quote_ret = SGX_QL_ERROR_UNEXPECTED;

    // Verify Inputs
    if((NULL == p_app_report) ||
       (NULL == p_quote))
    {
        return(SGX_QL_ERROR_INVALID_PARAMETER);
    }
    // Get the Quote
    // 1. Input the app enclave's report
    // 2. Input Size of this quote and a pointer to the buffer of that size to contain the quote.
    // 3. Returns the quote
    SE_TRACE(SE_TRACE_DEBUG, "sgx_ql_get_quote\n.");

    if(false == g_out_of_proc)
    {
        quote_ret = sgx_ql_get_quote(p_app_report,
                                     NULL,
                                     NULL,
                                     p_quote,
                                     quote_size);
        if (SGX_QL_SUCCESS != quote_ret) {
            SE_TRACE(SE_TRACE_ERROR, "Error in sgx_ql_get_quote. 0x%04x\n", quote_ret);
            goto CLEANUP;
        }
    }
#ifndef _MSC_VER
    else
    {
        func_sgx_get_quote_ex_t func = get_quote_ex_function();
        if (NULL == func) {
            SE_TRACE(SE_TRACE_ERROR,"Error in get symbol %s.\n", SGX_GET_QUOTE_EX);
            quote_ret = SGX_QL_ERROR_UNEXPECTED;
            goto CLEANUP;
        }
        quote_ret = sgx_status_to_quote3_error(func(p_app_report, &g_att_keyid, NULL, p_quote, quote_size));
        if (SGX_QL_SUCCESS != quote_ret) {
            SE_TRACE(SE_TRACE_ERROR, "Error in %s. 0x%04x\n", SGX_GET_QUOTE_EX, quote_ret);
            goto CLEANUP;
        }
    }
#endif

    CLEANUP:
    return(quote_ret);
}

/**
 * This API is primarily a hint for SGX lib that it can release a QE it had cached for efficiency: In the mainline case,
 * sgx_get_qe_targetinfo, sgx_get_quote_size and sgx_get_quote would be called in succession. If SGX lib keeps QE
 * between sgx_get_qe_targetinfo and sgx_get_quote_size, then it would not be cleaned up if DCAP failed prior to
 * sgx_get_quote. sgx_cleanup_qe_by_policy allows SGX lib to be informed that it should clean up the QE as sgx_get_quote
 * would not be called. If SGX_QE_PERSISTENT is the default policy, it can choose to no-op.
 *
 * @return SGX_QL_SUCCESS
 * @return SGX_QL_UNSUPPORTED_MODE This function is called on Windows or in out-of-process mode.
 */
quote3_error_t sgx_qe_cleanup_by_policy()
{
    if(g_out_of_proc)
        return(SGX_QL_UNSUPPORTED_MODE);

    // This is just a NO-OP since the default policy of the QE Library is PERISTENT.

    return(SGX_QL_SUCCESS);
}

#ifndef _MSC_VER
#include <sys/types.h>
#include <sys/stat.h>
/**
 * This API can be used to set the full path of QE3, PCE and QPL library.
 *
 * The function takes the enum and the corresponding full path. Sor far, this function works on
 * Linux in-proc mode only(the envrionment variable "SGX_AESM_ADDR" is not set), otherwise,
 * SGX_QL_UNSUPPORTED_MODE is returned.
 *
 * @param path_type Specify the full path of which binary is going to be changed.
 * @param p_path It should be a valid path.
 *
 * @return SGX_QL_SUCCESS  Successfully set the full path.
 * @return SGX_QL_UNSUPPORTED_MODE This function is called on Windows or in out-of-process mode.
 * @return SGX_QL_ERROR_INVALID_PARAMETER p_path is not a valid full path or the path is too long.
 * @return SGX_QL_ERROR_UNEXPECTED An unexpected internal error occurred.
 *
 */
quote3_error_t sgx_ql_set_path(sgx_ql_path_type_t path_type, const char *p_path)
{
    quote3_error_t ret = SGX_QL_SUCCESS;
    sgx_pce_error_t pce_ret = SGX_PCE_SUCCESS;
    struct stat info;

    if(g_out_of_proc)
        return(SGX_QL_UNSUPPORTED_MODE);

    if (!p_path)
        return(SGX_QL_ERROR_INVALID_PARAMETER);

    if(stat(p_path, &info) != 0)
        return(SGX_QL_ERROR_INVALID_PARAMETER);
    else if((info.st_mode & S_IFREG) == 0 && (info.st_mode & S_IFLNK) == 0)
        return(SGX_QL_ERROR_INVALID_PARAMETER);

    switch(path_type)
    {
        case SGX_QL_QE3_PATH:
            ret = sgx_set_qe3_path(p_path);
            break;
        case SGX_QL_PCE_PATH:
            pce_ret = sgx_set_pce_path(p_path);
            if(SGX_PCE_INVALID_PARAMETER == pce_ret)
                ret =  SGX_QL_ERROR_INVALID_PARAMETER;
            if(SGX_PCE_UNEXPECTED == pce_ret)
                ret =  SGX_QL_ERROR_UNEXPECTED;
            break;
        case SGX_QL_QPL_PATH:
            ret = sgx_set_qpl_path(p_path);
            break;
        case SGX_QL_IDE_PATH:
            ret = sgx_set_ide_path(p_path);
            break;
    default:
        ret = SGX_QL_ERROR_INVALID_PARAMETER;
        break;
    }
    return(ret);
}
#endif

