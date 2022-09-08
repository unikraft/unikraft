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

#include "sl_uswitchless.h"
#include <sl_atomic.h>
#include <sl_workers.h>
#include <sgx_error.h>
#include <sl_debug.h>
#include "sl_fcall_mngr_common.h"
#include "sl_once.h"
#include "uswitchless.h"
#include "sgx_tswitchless_u.h"

struct init_tswitchless_param_t
{
    struct sl_uswitchless* handle;
    sgx_enclave_id_t  enclave_id;
    sgx_status_t ret_value;
    const void* ocall_table;
};

uint64_t init_tswitchless(void* _param)
{
    struct init_tswitchless_param_t* param = (struct init_tswitchless_param_t*)_param;
   
    sl_init_switchless(param->enclave_id, &param->ret_value, param->handle);

    if (param->ret_value == SGX_SUCCESS)
    {
        sl_uswitchless_start_workers(param->handle, (const sgx_ocall_table_t*)param->ocall_table);
    }

    return param->ret_value == SGX_SUCCESS ? 0 : param->ret_value;
}

sgx_status_t sl_uswitchless_on_first_ecall(void* _switchless, sgx_enclave_id_t enclave_id, const void* ocall_table)
{
    struct init_tswitchless_param_t _param;

    BUG_ON(ocall_table == NULL);
    BUG_ON(_switchless == NULL);

    struct sl_uswitchless* handle = (struct sl_uswitchless*)_switchless;

    //Start the workers of Switchless SGX on the first user-provided ECall
    _param.enclave_id = enclave_id;
    _param.handle = handle;
    _param.ocall_table = ocall_table;
    _param.ret_value = SGX_ERROR_UNEXPECTED;

    if (sl_call_once(&handle->us_init_tswitchless, init_tswitchless, &_param))
        return _param.ret_value;

    return SGX_SUCCESS;
}


/* sgx_ecall_switchless() from uRTS calls this function to do the real job */
sgx_status_t sl_uswitchless_do_switchless_ecall(void* _switchless,
                                                const unsigned int ecall_id,
                                                void* ecall_ms,
                                                int* need_fallback)
{
    BUG_ON(_switchless == NULL);
    struct sl_uswitchless* handle = (struct sl_uswitchless*)_switchless;

    int error = 0;

    /* initialization in progress or no trusted workers are running, then fallback */
    if ((handle->us_init_finished == 0) || (handle->us_tworkers.num_running == 0))
        goto on_fallback;
    
    if (handle->us_tworkers.num_sleeping > 0)
    {
        wake_all_threads(&handle->us_tworkers);
    }

    struct sl_call_task call_task;
    call_task.status = SL_INIT;
    call_task.func_id = ecall_id;
    call_task.func_data = ecall_ms;
    call_task.ret_code = SGX_ERROR_UNEXPECTED;

    error = sl_call_mngr_call(&handle->us_ecall_mngr, &call_task, handle->us_config.retries_before_fallback);
    if (error) 
        goto on_fallback;

    *need_fallback = 0;
    lock_inc(&handle->us_tworkers.stats.processed);
    return call_task.ret_code;
on_fallback:
    *need_fallback = 1;
    lock_inc(&handle->us_tworkers.stats.missed);
    sl_workers_notify_event(&handle->us_tworkers, SL_WORKER_EVENT_MISS);
    return SGX_ERROR_BUSY;
}
