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

#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <sl_uswitchless.h>
#include <sl_debug.h>
#include <sl_atomic.h>
#include <sl_fcall_mngr_common.h>
#include <string.h>

/*=========================================================================
 * Initialization
 *========================================================================*/

static inline bool check_switchless_params(const sgx_uswitchless_config_t* config)
{
    if (((config->num_uworkers == 0) && (config->num_tworkers == 0)) ||
        (config->switchless_calls_pool_size_qwords > SL_MAX_TASKS_MAX_QWORDS))
    {
        return false;
    }

    return true;
}

sgx_status_t sl_uswitchless_new(const sgx_uswitchless_config_t* config, 
                                const sgx_enclave_id_t enclave_id, 
                                struct sl_uswitchless** uswitchless_pp)
{
    BUG_ON(config == NULL);
    uint32_t ret = 0;

    if (!check_switchless_params(config))
    {
        return SGX_ERROR_INVALID_PARAMETER;
    }

    struct sl_uswitchless* handle = (struct sl_uswitchless*)malloc(sizeof(*handle));
    if (handle == NULL)
    {
        return SGX_ERROR_OUT_OF_MEMORY;
    }
    memset(handle, 0, sizeof(*handle));

    handle->us_enclave_id = enclave_id;
    handle->us_ocall_table = NULL;
    handle->us_config = config ? *config : (sl_config_t) SL_CONFIG_INITIALIZER;
    handle->us_init_tswitchless.ret = 0;
    handle->us_init_tswitchless.state = STATE_NO_CALL;
	
    handle->us_config.retries_before_fallback = (handle->us_config.retries_before_fallback == 0) ? 
                                                SL_DEFAULT_FALLBACK_RETRIES : handle->us_config.retries_before_fallback;

    handle->us_config.retries_before_sleep    = (handle->us_config.retries_before_sleep    == 0) ? 
                                                SL_DEFAULT_SLEEP_RETRIES    : handle->us_config.retries_before_sleep;

    uint32_t max_tasks = handle->us_config.switchless_calls_pool_size_qwords == 0 ? 
                         SL_DEFUALT_MAX_TASKS_QWORDS : handle->us_config.switchless_calls_pool_size_qwords;

    max_tasks *= BITS_IN_QWORD;

    ret = sl_call_mngr_init(&handle->us_ocall_mngr,
                            SL_TYPE_OCALL,
		                    max_tasks);

    if (ret) goto on_error_0;

    ret = sl_call_mngr_init(&handle->us_ecall_mngr,
                            SL_TYPE_ECALL,
		                    max_tasks);

    if (ret) goto on_error_1;

    ret = sl_workers_init(&handle->us_uworkers, SL_WORKER_TYPE_UNTRUSTED, handle);
    if (ret) goto on_error_2;

    ret = sl_workers_init(&handle->us_tworkers, SL_WORKER_TYPE_TRUSTED, handle);
    if (ret) goto on_error_3;

    *uswitchless_pp = handle;
    return SGX_SUCCESS;
on_error_3:
    sl_workers_destroy(&handle->us_uworkers);
on_error_2:
    sl_call_mngr_destroy(&handle->us_ecall_mngr);
on_error_1:
    sl_call_mngr_destroy(&handle->us_ocall_mngr);
on_error_0:
    free(handle);
    return SGX_ERROR_OUT_OF_MEMORY;
}

void sl_uswitchless_free(struct sl_uswitchless* handle) 
{
    sl_workers_destroy(&handle->us_tworkers);
    sl_workers_destroy(&handle->us_uworkers);
    sl_call_mngr_destroy(&handle->us_ocall_mngr);
    sl_call_mngr_destroy(&handle->us_ecall_mngr);
    free(handle);
}


sgx_status_t sl_init_uswitchless(sgx_enclave_id_t enclave_id, const void* config, void** switchless_pp)
{
    struct sl_uswitchless* uswitchless_p = NULL;
    sgx_status_t status = SGX_ERROR_UNEXPECTED;

    //Create the per-enclave data object for Switchless SGX
    status = sl_uswitchless_new((const sgx_uswitchless_config_t*)config, enclave_id, &uswitchless_p);
    if (status == SGX_SUCCESS)
    {
        if (sl_uswitchless_init_workers(uswitchless_p))
        {
            status = SGX_ERROR_UNEXPECTED;
            sl_uswitchless_free(uswitchless_p);
        }
    }

    *switchless_pp = uswitchless_p;
    
    return status;
}


sgx_status_t sl_destroy_uswitchless(void* _switchless)
{
    BUG_ON(_switchless == NULL);

    struct sl_uswitchless* uswitchless_p = (struct sl_uswitchless*)_switchless;

    sl_uswitchless_stop_workers(uswitchless_p);
    sl_uswitchless_free(uswitchless_p);

    return SGX_SUCCESS;
}


/*=========================================================================
 * Worker-related APIs
 *========================================================================*/

uint32_t sl_uswitchless_init_workers(struct sl_uswitchless* handle)
{
    uint32_t ret;
    ret = sl_workers_init_threads(&handle->us_uworkers);
    if (ret) return ret;

    ret = sl_workers_init_threads(&handle->us_tworkers);
    if (ret) 
    {
        sl_workers_kill_threads(&handle->us_uworkers);
        return ret;
    }

    return 0;
}

uint32_t sl_uswitchless_start_workers(struct sl_uswitchless* handle,
                                 const sgx_ocall_table_t* ocall_table)
{
    BUG_ON(ocall_table == NULL);
    /* Silently ignore harmless duplication of staring workers */
    
    if (lock_cmpxchg_ptr((void*)&handle->us_ocall_table, NULL, (void*)ocall_table) != NULL)
        return 0;

    /* switchless ocall_table to the sl_fcall_mngr for switchless Calls */
    sl_call_mngr_register_calls(&handle->us_ocall_mngr,
                                 (const sl_call_table_t*)ocall_table);

    handle->us_init_finished = 1;

    sl_workers_run_threads(&handle->us_uworkers);
    sl_workers_run_threads(&handle->us_tworkers);

    return 0;
}

void sl_uswitchless_stop_workers(struct sl_uswitchless* handle) 
{
    handle->us_should_stop = 1;
    sl_workers_kill_threads(&handle->us_uworkers);
    sl_workers_kill_threads(&handle->us_tworkers);
}

/*=========================================================================
 * OCall-specific APIs
 *========================================================================*/

void sl_uswitchless_check_switchless_ocall_fallback(void* _switchless)
{
    BUG_ON(_switchless == NULL);
    struct sl_uswitchless* handle = (struct sl_uswitchless*)_switchless;

    if (xchg(&handle->us_has_new_ocall_fallback, 0) == 1) 
    {
        sl_workers_notify_event(&handle->us_uworkers, SL_WORKER_EVENT_MISS);
    }
}

/*=========================================================================
 * ECall-specific APIs
 *========================================================================*/

sgx_status_t sl_wake_workers(void* ms)
{
    wake_all_threads((struct sl_workers*)ms);
    return SGX_SUCCESS;
}

/* see sgx_ecall_switchless_untrusted.c */
