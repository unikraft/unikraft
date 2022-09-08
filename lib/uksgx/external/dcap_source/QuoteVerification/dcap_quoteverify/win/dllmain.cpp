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
 * File: dllmain.cpp : Defines the entry point for the DLL application.
 *
 * Description: Wrapper functions for the
 * reference implementing the QvE
 * function defined in sgx_qve.h. This
 * would be replaced or used to wrap the
 * PSW defined interfaces to the QvE.
 *
 */

#include <Windows.h>
#include "sgx_dcap_pcs_com.h"
#include "sgx_urts_wrapper.h"
#include "se_trace.h"
#include "se_thread.h"

#define SGX_URTS_LIB_FILE_NAME "sgx_urts.dll"
#define SGX_QL_QUOTE_CONFIG_LIB_FILE_NAME "dcap_quoteprov.dll"

HINSTANCE g_qpl_handle = NULL;
se_mutex_t g_qpl_mutex;

HINSTANCE g_urts_handle = NULL;
se_mutex_t g_urts_mutex;

extern sgx_get_quote_verification_collateral_func_t p_sgx_ql_get_quote_verification_collateral;
extern sgx_free_quote_verification_collateral_func_t p_sgx_ql_free_quote_verification_collateral;

extern sgx_ql_get_qve_identity_func_t p_sgx_ql_get_qve_identity;
extern sgx_ql_free_qve_identity_func_t p_sgx_ql_free_qve_identity;

extern sgx_ql_get_root_ca_crl_func_t p_sgx_ql_get_root_ca_crl;
extern sgx_ql_free_root_ca_crl_func_t p_sgx_ql_free_root_ca_crl;

extern tdx_get_quote_verification_collateral_func_t p_tdx_ql_get_quote_verification_collateral;
extern tdx_free_quote_verification_collateral_func_t p_tdx_ql_free_quote_verification_collateral;

extern sgx_create_enclave_func_t p_sgx_urts_create_enclave;
extern sgx_destroy_enclave_func_t p_sgx_urts_destroy_enclave;
extern sgx_ecall_func_t p_sgx_urts_ecall;
extern sgx_oc_cpuidex_func_t p_sgx_oc_cpuidex;
extern sgx_thread_wait_untrusted_event_ocall_func_t p_sgx_thread_wait_untrusted_event_ocall;
extern sgx_thread_set_untrusted_event_ocall_func_t p_sgx_thread_set_untrusted_event_ocall;
extern sgx_thread_setwait_untrusted_events_ocall_func_t p_sgx_thread_setwait_untrusted_events_ocall;
extern sgx_thread_set_multiple_untrusted_events_ocall_func_t p_sgx_thread_set_multiple_untrusted_events_ocall;



bool sgx_dcap_load_qpl()
{
    bool ret = false;

    int rc = se_mutex_lock(&g_qpl_mutex);
    if (rc != TRUE) {
        SE_TRACE(SE_TRACE_ERROR, "Failed to lock qpl mutex\n");
        return false;
    }

    // don't include TDX related check due to user may use old version QPL
    //
    if (g_qpl_handle &&
            p_sgx_ql_get_quote_verification_collateral && p_sgx_ql_free_quote_verification_collateral &&
            p_sgx_ql_get_qve_identity && p_sgx_ql_free_qve_identity &&
            p_sgx_ql_get_root_ca_crl && p_sgx_ql_free_root_ca_crl) {

        rc = se_mutex_unlock(&g_qpl_mutex);
        if (rc != TRUE) {
            SE_TRACE(SE_TRACE_ERROR, "Failed to unlock qpl mutex\n");
            return false;
        }

        return true;
    }

    do {

        //try to dynamically load dcap_quoteprov.dll
        //
		g_qpl_handle = LoadLibrary(TEXT(SGX_QL_QUOTE_CONFIG_LIB_FILE_NAME));
        if (g_qpl_handle == NULL) {
            SE_TRACE(SE_TRACE_ERROR, "Couldn't load the Quote Provider library %s.\n", SGX_QL_QUOTE_CONFIG_LIB_FILE_NAME);
            break;
        }

        //search for sgx_ql_get_quote_verification_collateral symbol in dcap_quoteprov library
        //
        p_sgx_ql_get_quote_verification_collateral = (sgx_get_quote_verification_collateral_func_t)GetProcAddress(g_qpl_handle, QL_API_GET_QUOTE_VERIFICATION_COLLATERAL);
        if (p_sgx_ql_get_quote_verification_collateral == NULL) {
            SE_TRACE(SE_TRACE_ERROR, "Couldn't locate %s in Quote Provider library %s.\n", QL_API_GET_QUOTE_VERIFICATION_COLLATERAL, SGX_QL_QUOTE_CONFIG_LIB_FILE_NAME);
            break;
        }

        //search for sgx_ql_free_quote_verification_collateral symbol in dcap_quoteprov library
        //
        p_sgx_ql_free_quote_verification_collateral = (sgx_free_quote_verification_collateral_func_t)GetProcAddress(g_qpl_handle, QL_API_FREE_QUOTE_VERIFICATION_COLLATERAL);
        if (p_sgx_ql_free_quote_verification_collateral == NULL) {
            SE_TRACE(SE_TRACE_ERROR, "Couldn't locate %s in Quote Provider library %s.\n", QL_API_FREE_QUOTE_VERIFICATION_COLLATERAL, SGX_QL_QUOTE_CONFIG_LIB_FILE_NAME);
            break;
        }

        //search for sgx_ql_get_qve_identity symbol in dcap_quoteprov library
        //
        p_sgx_ql_get_qve_identity = (sgx_ql_get_qve_identity_func_t)GetProcAddress(g_qpl_handle, QL_API_GET_QVE_IDENTITY);
        if (p_sgx_ql_get_qve_identity == NULL) {
            SE_TRACE(SE_TRACE_ERROR, "Couldn't locate %s in Quote Provider library %s.\n", QL_API_GET_QVE_IDENTITY, SGX_QL_QUOTE_CONFIG_LIB_FILE_NAME);
            break;
        }

        //search for sgx_ql_free_qve_identity symbol in dcap_quoteprov library
        //
        p_sgx_ql_free_qve_identity = (sgx_ql_free_qve_identity_func_t)GetProcAddress(g_qpl_handle, QL_API_FREE_QVE_IDENTITY);
        if (p_sgx_ql_free_qve_identity == NULL) {
            SE_TRACE(SE_TRACE_ERROR, "Couldn't locate %s in Quote Provider library %s.\n", QL_API_FREE_QVE_IDENTITY, SGX_QL_QUOTE_CONFIG_LIB_FILE_NAME);
            break;
        }

        //search for sgx_ql_get_root_ca_crl symbol in dcap_quoteprov library
        //
        p_sgx_ql_get_root_ca_crl = (sgx_ql_get_root_ca_crl_func_t)GetProcAddress(g_qpl_handle, QL_API_GET_ROOT_CA_CRL);
        if (p_sgx_ql_get_root_ca_crl == NULL) {
            SE_TRACE(SE_TRACE_ERROR, "Couldn't locate %s in Quote Provider library %s.\n", QL_API_GET_ROOT_CA_CRL, SGX_QL_QUOTE_CONFIG_LIB_FILE_NAME);
            break;
        }

        //search for sgx_ql_free_root_ca_crl symbol in dcap_quoteprov library
        //
        p_sgx_ql_free_root_ca_crl = (sgx_ql_free_root_ca_crl_func_t)GetProcAddress(g_qpl_handle, QL_API_FREE_ROOT_CA_CRL);
        if (p_sgx_ql_free_root_ca_crl == NULL) {
            SE_TRACE(SE_TRACE_ERROR, "Couldn't locate %s in Quote Provider library %s.\n", QL_API_FREE_ROOT_CA_CRL, SGX_QL_QUOTE_CONFIG_LIB_FILE_NAME);
            break;
        }

        //search for tdx_ql_get_quote_verification_collateral symbol in dcap_quoteprov library
        //
        p_tdx_ql_get_quote_verification_collateral = (tdx_get_quote_verification_collateral_func_t)GetProcAddress(g_qpl_handle, TDX_QL_API_GET_QUOTE_VERIFICATION_COLLATERAL);
        if (p_tdx_ql_get_quote_verification_collateral == NULL) {
            // don't return error due to user may use old version QPL
            SE_TRACE(SE_TRACE_DEBUG, "Couldn't locate %s in Quote Provider library %s.\n", TDX_QL_API_GET_QUOTE_VERIFICATION_COLLATERAL, SGX_QL_QUOTE_CONFIG_LIB_FILE_NAME);
        }

        //search for tdx_ql_free_quote_verification_collateral symbol in dcap_quoteprov library
        //
        p_tdx_ql_free_quote_verification_collateral = (tdx_free_quote_verification_collateral_func_t)GetProcAddress(g_qpl_handle, TDX_QL_API_FREE_QUOTE_VERIFICATION_COLLATERAL);
        if (p_tdx_ql_free_quote_verification_collateral == NULL) {
            // don't return error due to user may use old version QPL
            SE_TRACE(SE_TRACE_DEBUG, "Couldn't locate %s in Quote Provider library %s.\n", TDX_QL_API_FREE_QUOTE_VERIFICATION_COLLATERAL, SGX_QL_QUOTE_CONFIG_LIB_FILE_NAME);
        }


        ret = true;

    } while (0);


    rc = se_mutex_unlock(&g_qpl_mutex);
    if (rc != TRUE) {
        SE_TRACE(SE_TRACE_ERROR, "Failed to unlock qpl mutex\n");
        return false;
    }

    return ret;
}


bool sgx_dcap_load_urts()
{
    bool ret = false;

    int rc = se_mutex_lock(&g_urts_mutex);
    if (rc != TRUE) {
        SE_TRACE(SE_TRACE_ERROR, "Failed to lock urts mutex\n");
        return false;
    }

   if (g_urts_handle &&
            p_sgx_urts_create_enclave &&
            p_sgx_urts_destroy_enclave &&
            p_sgx_urts_ecall ) {

        rc = se_mutex_unlock(&g_urts_mutex);
        if (rc != TRUE) {
            SE_TRACE(SE_TRACE_ERROR, "Failed to unlock urts mutex\n");
            return false;
        }

        return true;
    }

    do {

        //try to dynamically load sgx_urts.dll
        //
		g_urts_handle = LoadLibrary(TEXT(SGX_URTS_LIB_FILE_NAME));
        if (g_urts_handle == NULL) {
            SE_TRACE(SE_TRACE_DEBUG, "Couldn't load urts library %s.\n", SGX_URTS_LIB_FILE_NAME);
            break;
        }

        //search for sgx_create_enclave symbol in urts library
        //
        p_sgx_urts_create_enclave = (sgx_create_enclave_func_t)GetProcAddress(g_urts_handle, SGX_URTS_API_CREATE_ENCLAVE);
        if (p_sgx_urts_create_enclave == NULL) {
            SE_TRACE(SE_TRACE_ERROR, "Couldn't locate %s in urts library %s.\n", SGX_URTS_API_CREATE_ENCLAVE, SGX_URTS_LIB_FILE_NAME);
            break;
        }

        //search for sgx_destroy_enclave symbol in urts library
        //
        p_sgx_urts_destroy_enclave = (sgx_destroy_enclave_func_t)GetProcAddress(g_urts_handle, SGX_URTS_API_DESTROY_ENCLAVE);
        if (p_sgx_urts_destroy_enclave == NULL) {
            SE_TRACE(SE_TRACE_ERROR, "Couldn't locate %s in urts library %s.\n", SGX_URTS_API_DESTROY_ENCLAVE, SGX_URTS_LIB_FILE_NAME);
            break;
        }

        //search for sgx_ecall symbol in urts library
        //
        p_sgx_urts_ecall = (sgx_ecall_func_t)GetProcAddress(g_urts_handle, SGX_URTS_API_ECALL);
        if (p_sgx_urts_ecall == NULL) {
            SE_TRACE(SE_TRACE_ERROR, "Couldn't locate %s in urts library %s.\n", SGX_URTS_API_ECALL, SGX_URTS_LIB_FILE_NAME);
            break;
        }


        //search for sgx_oc_cpuidex symbol in urts library
        //
        p_sgx_oc_cpuidex = (sgx_oc_cpuidex_func_t)GetProcAddress(g_urts_handle, SGX_URTS_API_OCALL_CPUID);
        if (p_sgx_oc_cpuidex == NULL) {
            SE_TRACE(SE_TRACE_ERROR, "Couldn't locate %s in urts library %s.\n", SGX_URTS_API_OCALL_CPUID, SGX_URTS_LIB_FILE_NAME);
            break;
        }

        //search for sgx_thread_wait_untrusted_event_ocall symbol in urts library
        //
        p_sgx_thread_wait_untrusted_event_ocall = (sgx_thread_wait_untrusted_event_ocall_func_t)GetProcAddress(g_urts_handle, SGX_URTS_API_OCALL_WAIT_EVENT);
        if (p_sgx_thread_wait_untrusted_event_ocall == NULL) {
            SE_TRACE(SE_TRACE_ERROR, "Couldn't locate %s in urts library %s.\n", SGX_URTS_API_OCALL_WAIT_EVENT, SGX_URTS_LIB_FILE_NAME);
            break;
        }

        //search for sgx_thread_set_untrusted_event_ocall symbol in urts library
        //
        p_sgx_thread_set_untrusted_event_ocall = (sgx_thread_set_untrusted_event_ocall_func_t)GetProcAddress(g_urts_handle, SGX_URTS_API_OCALL_SET_EVENT);
        if (p_sgx_thread_set_untrusted_event_ocall == NULL) {
            SE_TRACE(SE_TRACE_ERROR, "Couldn't locate %s in urts library %s.\n", SGX_URTS_API_OCALL_SET_EVENT, SGX_URTS_LIB_FILE_NAME);
            break;
        }

        //search for sgx_thread_setwait_untrusted_events_ocall symbol in urts library
        //
        p_sgx_thread_setwait_untrusted_events_ocall = (sgx_thread_setwait_untrusted_events_ocall_func_t)GetProcAddress(g_urts_handle, SGX_URTS_API_OCALL_WAITSET_EVENT);
        if (p_sgx_thread_setwait_untrusted_events_ocall == NULL) {
            SE_TRACE(SE_TRACE_ERROR, "Couldn't locate %s in urts library %s.\n", SGX_URTS_API_OCALL_WAITSET_EVENT, SGX_URTS_LIB_FILE_NAME);
            break;
        }

        //search for sgx_thread_set_multiple_untrusted_events_ocall symbol in urts library
        //
        p_sgx_thread_set_multiple_untrusted_events_ocall = (sgx_thread_set_multiple_untrusted_events_ocall_func_t)GetProcAddress(g_urts_handle, SGX_URTS_API_OCALL_SET_MULTIPLE_EVENT);
        if (p_sgx_thread_set_multiple_untrusted_events_ocall == NULL) {
            SE_TRACE(SE_TRACE_ERROR, "Couldn't locate %s in urts library %s.\n", SGX_URTS_API_OCALL_SET_MULTIPLE_EVENT, SGX_URTS_LIB_FILE_NAME);
            break;
        }


        ret = true;

    } while (0);


    rc = se_mutex_unlock(&g_urts_mutex);
    if (rc != TRUE) {
        SE_TRACE(SE_TRACE_ERROR, "Failed to unlock urts mutex\n");
        return false;
    }

    return ret;
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    (void)(hModule);
    (void)(lpReserved);
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        {
            se_mutex_init(&g_qpl_mutex);
            se_mutex_init(&g_urts_mutex);
            break;
        }
    case DLL_PROCESS_DETACH:
        // try to unload QPL if exist
        {
            int rc = se_mutex_lock(&g_qpl_mutex);
            if (rc != 1) {
                SE_TRACE(SE_TRACE_ERROR, "Failed to lock qpl mutex\n");
                //destroy the mutex before lib is unloaded, even there are some errs here
                se_mutex_destroy(&g_qpl_mutex);
                break;
            }

            if (p_sgx_ql_get_quote_verification_collateral)
                p_sgx_ql_get_quote_verification_collateral = NULL;
            if (p_sgx_ql_free_quote_verification_collateral)
                p_sgx_ql_free_quote_verification_collateral = NULL;

            if (p_sgx_ql_get_qve_identity)
                p_sgx_ql_get_qve_identity = NULL;
			if (p_sgx_ql_free_qve_identity)
				p_sgx_ql_free_qve_identity = NULL;

            if (p_sgx_ql_get_root_ca_crl)
                p_sgx_ql_get_root_ca_crl = NULL;
            if (p_sgx_ql_free_root_ca_crl)
                p_sgx_ql_free_root_ca_crl = NULL;

            if (p_tdx_ql_get_quote_verification_collateral)
                p_tdx_ql_get_quote_verification_collateral = NULL;
            if (p_tdx_ql_free_quote_verification_collateral)
                p_tdx_ql_free_quote_verification_collateral = NULL;

            if (g_qpl_handle) {
                FreeLibrary(g_qpl_handle);
                g_qpl_handle = NULL;
            }

            rc = se_mutex_unlock(&g_qpl_mutex);
            if (rc != 1) {
                SE_TRACE(SE_TRACE_ERROR, "Failed to unlock qpl mutex\n");
            }

           se_mutex_destroy(&g_qpl_mutex);


           // Try to unload urts library
           rc = se_mutex_lock(&g_urts_mutex);
           if (rc != 1) {
               SE_TRACE(SE_TRACE_ERROR, "Failed to lock urts mutex\n");
               //destroy the mutex before lib is unloaded, even there are some errs here
               se_mutex_destroy(&g_urts_mutex);
               break;
           }

           if (p_sgx_urts_create_enclave)
               p_sgx_urts_create_enclave = NULL;

           if (p_sgx_urts_destroy_enclave)
               p_sgx_urts_destroy_enclave = NULL;

           if (p_sgx_urts_ecall)
               p_sgx_urts_ecall = NULL;

           if (p_sgx_oc_cpuidex)
               p_sgx_oc_cpuidex = NULL;

           if (p_sgx_thread_wait_untrusted_event_ocall)
               p_sgx_thread_wait_untrusted_event_ocall = NULL;

           if (p_sgx_thread_set_untrusted_event_ocall)
               p_sgx_thread_set_untrusted_event_ocall = NULL;

           if (p_sgx_thread_setwait_untrusted_events_ocall)
               p_sgx_thread_setwait_untrusted_events_ocall = NULL;

           if (p_sgx_thread_set_multiple_untrusted_events_ocall)
               p_sgx_thread_set_multiple_untrusted_events_ocall = NULL;

           if (g_urts_handle) {
               FreeLibrary(g_urts_handle);
               g_urts_handle = NULL;
           }

           rc = se_mutex_unlock(&g_urts_mutex);
           if (rc != 1) {
               SE_TRACE(SE_TRACE_ERROR, "Failed to unlock urts mutex\n");
           }

           se_mutex_destroy(&g_urts_mutex);

           break;
        }
    case DLL_THREAD_ATTACH:
	    break;
    case DLL_THREAD_DETACH:
	    break;
    }
    return TRUE;
}
