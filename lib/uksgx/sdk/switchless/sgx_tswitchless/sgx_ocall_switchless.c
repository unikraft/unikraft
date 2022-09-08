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

#include <sgx_error.h>
#include <sgx_trts.h>
#include <sgx_edger8r.h>
#include <internal/thread_data.h>
#include <internal/util.h>
#include <sl_uswitchless.h>
#include <sl_fcall_mngr_common.h>
#include <sl_init.h>
#include <sl_once.h>
#include <sl_util.h>
#include <sl_debug.h>
#include <sl_atomic.h>
#include <rts.h>


/*=========================================================================
 * Initialization
 *========================================================================*/

static struct sl_call_mngr g_ocall_mngr;

static sl_once_t g_init_ocall_mngr_done = SL_ONCE_INITIALIZER;

static int_type init_tswitchless_ocall_mngr(void* param)
{
	(void)param;

    // check if switchless has been initialized
    if (g_uswitchless_handle == NULL) return (int_type)(-1);

    // g_uswitchless_handle is checked in sl_init_switchless()
    struct sl_call_mngr* mngr_u = &g_uswitchless_handle->us_ocall_mngr;

    // clone data from the untrusted side, ensuring all the relevant data is outside enclave
    uint32_t ret = sl_call_mngr_clone(&g_ocall_mngr, mngr_u);
    if (ret != 0) 
		return (int_type)ret;

    // wrong manager type, attack ?
    PANIC_ON(sl_call_mngr_get_type(&g_ocall_mngr) != SL_TYPE_OCALL);
    return 0;
}

/*=========================================================================
 * The implementation of switchless OCall
 *========================================================================*/
#include <string.h>
#define SET_VALUE_HARDEN(dest, value, value_type)                     \
    do {                                                              \
        value_type tmp = value;                                       \
        memcpy_verw((void*)(dest), &tmp, sizeof(value_type));         \
    } while(0)

sgx_status_t sgx_ocall_switchless(const unsigned int index, void* ms) 
{
    int error = 0;

    if (sgx_is_enclave_crashed())
        return SGX_ERROR_ENCLAVE_CRASHED;

    /* If Switchless SGX is not enabled at enclave creation, then switchless OCalls
     * fallback to the traditional OCalls */
    if (sl_call_once(&g_init_ocall_mngr_done, init_tswitchless_ocall_mngr, NULL)) 
        return sgx_ocall(index, ms);

    // g_uswitchless_handle is checked in sl_init_switchless()

    /* If no untrusted workers are running, then fallback */ 
    if (g_uswitchless_handle->us_uworkers.num_running == 0) 
        goto on_fallback;

    // if there are sleeping workers, wake them up
    if (g_uswitchless_handle->us_uworkers.num_sleeping > 0)
    {
	SET_VALUE_HARDEN(&g_uswitchless_handle->us_wake_workers, 1, uint64_t);
    }

    struct sl_call_task call_task;

    call_task.status = SL_INIT;
    call_task.func_id = index;
    call_task.func_data = ms;
    call_task.ret_code = SGX_ERROR_UNEXPECTED;

    // perform switchless OCALL
    error = sl_call_mngr_call(&g_ocall_mngr, &call_task, g_uswitchless_handle->us_config.retries_before_fallback);
    if (error) 
        goto on_fallback;
    
    lock_inc(&g_uswitchless_handle->us_uworkers.stats.processed);

    return call_task.ret_code;

on_fallback:
    lock_inc(&g_uswitchless_handle->us_uworkers.stats.missed);
    SET_VALUE_HARDEN(&g_uswitchless_handle->us_has_new_ocall_fallback, 1, uint64_t);
    return sgx_ocall(index, ms);
}

