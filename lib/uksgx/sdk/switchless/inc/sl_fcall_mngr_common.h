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

#ifndef __SL_FCALL_MNGR_COMMON_H__
#define __SL_FCALL_MNGR_COMMON_H__

#include "sgx_trts.h"
#include <sl_fcall_mngr.h>
#include <sl_debug.h>
#include <sl_util.h>
#include <sl_atomic.h>
#include <stdlib.h>
#include <errno.h>
#include <sl_siglines.h>


#ifdef __cplusplus
extern "C" {
#endif

static inline sl_siglines_dir_t call_type2direction(sl_call_type_t type)
{
    /* Use C99's designated initializers to make this conversion more readable */
    static sl_siglines_dir_t table[2] = {
        [SL_TYPE_OCALL] = SL_SIGLINES_DIR_T2U,
        [SL_TYPE_ECALL] = SL_SIGLINES_DIR_U2T
    };
    return table[type];
}

//returns true if calling thread can process this type of calls
static inline int can_type_process(sl_call_type_t type)
{
#ifdef SL_INSIDE_ENCLAVE /* Trusted */
    return type == SL_TYPE_ECALL;
#else /* Untrusted */
    return type == SL_TYPE_OCALL;
#endif
}

//returns true if calling thread can initiate this type of calls
static inline int can_type_call(sl_call_type_t type)
{
#ifdef SL_INSIDE_ENCLAVE /* Trusted */
    return type == SL_TYPE_OCALL;
#else /* Untrusted */
    return type == SL_TYPE_ECALL;
#endif
}


static inline void sl_call_mngr_register_calls(struct sl_call_mngr* mngr,
                                  const sl_call_table_t* call_table)
{
    BUG_ON(call_table == NULL);
    mngr->call_table = call_table;
}

static inline uint32_t sl_call_mngr_process(struct sl_call_mngr* mngr)
{
    BUG_ON(!can_type_process(mngr->type));
    return sl_siglines_process_signals(&mngr->siglns);
}

#ifdef SL_INSIDE_ENCLAVE /* Trusted */
#include <string.h>
#define SET_VALUE_HARDEN_FOR_PROCESS_TASK(dest, value, value_type)    \
    do {                                                              \
        value_type tmp = value;                                       \
        memcpy_verw((void*)(dest), &tmp, sizeof(value_type));         \
    } while(0)
#else /* Untrusted */
#define SET_VALUE_HARDEN_FOR_PROCESS_TASK(dest, value, value_type)    \
    do {                                                              \
        *(dest) = value;                                              \
    } while(0)
#endif

static inline void process_switchless_call(struct sl_siglines* siglns, uint32_t line)
{
    /*

      Main processing function which is called by a worker thread(trusted or untrusted)
      from sl_siglines_process_signals() function when there is a pending request for a switchless ECALL/OCALL.

      siglns points to the untrusted data structure with pending requests bitmap,
      siglns is checked by sl_siglines_clone() function before use by the enclave

      line indicates which bit is set

    */

#ifdef SL_INSIDE_ENCLAVE /* trusted */
    if (sgx_is_enclave_crashed())
        return;
#endif

    // siglns is a memeber of sl_call_mngr structure
    // get the proper call manager (ECALL manager for trusted, OCALL manager for untrusted)
    struct sl_call_mngr* mngr = CONTAINER_OF(siglns, struct sl_call_mngr, siglns);

    // mngr structure and its content are checked by sl_call_mngr_clone() function before use.
    // see init_tswitchless_ecall_mngr(void)
    const sl_call_table_t* call_table = mngr->call_table;
    BUG_ON(call_table == NULL);

    // get the pointer to the structure with the call request (function id and input params for call)
    // tasks array resides outside enclave. checked by sl_call_mngr_clone() function before use.
    // see init_tswitchless_ecall_mngr(void)
    struct sl_call_task *call_task_u = &mngr->tasks[line];

    BUG_ON(call_task_u->status != SL_SUBMITTED);
    SET_VALUE_HARDEN_FOR_PROCESS_TASK(&call_task_u->status, SL_ACCEPTED, sl_call_status_t);

    uint32_t func_id = call_task_u->func_id;

    /* Get the function pointer */
    sl_call_func_t call_func_ptr = NULL;
    if (unlikely(func_id >= call_table->size))
    {
        SET_VALUE_HARDEN_FOR_PROCESS_TASK(&call_task_u->ret_code, SGX_ERROR_INVALID_FUNCTION, sgx_status_t);
        goto on_done;
    }

    sgx_lfence();

    call_func_ptr = call_table->funcs[func_id];
    if (unlikely(call_func_ptr == NULL))
    {
        SET_VALUE_HARDEN_FOR_PROCESS_TASK(&call_task_u->ret_code,
                        mngr->type == SL_TYPE_ECALL ? SGX_ERROR_ECALL_NOT_ALLOWED : SGX_ERROR_OCALL_NOT_ALLOWED,
                        sgx_status_t);
        goto on_done;
    }

    // Do the call.
    // func_data should point to untrusted buffer and should be checked by invoked function
    // in our case, edre8r generated code is performing the check
    {
        sgx_status_t ret = call_func_ptr(call_task_u->func_data);
        SET_VALUE_HARDEN_FOR_PROCESS_TASK(&call_task_u->ret_code, ret, sgx_status_t);
    }

on_done:
    /* Notify the caller that the switchless is done by updating the status.
    * The memory barrier ensures that switchless results are visible to the
    * caller when it finds out that the status becomes SL_DONE. */
    SET_VALUE_HARDEN_FOR_PROCESS_TASK(&call_task_u->status, SL_DONE, sl_call_status_t);
    sgx_mfence();
}

#ifdef SL_INSIDE_ENCLAVE
#define SET_VALUE_HARDEN_FOR_SEND_TASK(dest, value, value_type)        \
    do {                                                               \
        value_type tmp = value;                                        \
        memcpy_verw((void*)(dest), &tmp, sizeof(value_type));          \
    } while(0)
#else
#define SET_VALUE_HARDEN_FOR_SEND_TASK(dest, value, value_type)        \
    do {                                                               \
        *(dest) = value;                                               \
    } while(0)
#endif

static inline int sl_call_mngr_call(struct sl_call_mngr* mngr, struct sl_call_task* call_task, uint32_t max_tries)
{
    /*
        Used to make actual switchless call by both enclave & untrusted code

        mngr:      points to ECALL or OCALL manager. For enclave, OCALL mngr and all its content are checkeds in sl_mngr_clone() function
                   see init_tswitchless_ocall_mngr()

        call_task: contains all the information of function to be called,
                   when called by enclave to make OCALL, call_task resides on enclaves stack
    */

    BUG_ON(!can_type_call(mngr->type));

    int ret = 0;

    /* Allocate a free signal line to send signal */
    struct sl_siglines* siglns = &mngr->siglns;
    uint32_t line = sl_siglines_alloc_line(siglns);
    if (line == SL_INVALID_SIGLINE)
        return -EAGAIN;

    BUG_ON(call_task->status != SL_INIT);
    SET_VALUE_HARDEN_FOR_SEND_TASK(&call_task->status, SL_SUBMITTED, sl_call_status_t);

    // copy task data to internal array accessable by both sides (trusted & untrusted)
    SET_VALUE_HARDEN_FOR_SEND_TASK(&mngr->tasks[line], *call_task, struct sl_call_task);

    /* Send a signal so that workers will access the buffer for switchless call
     * requests. Here, a memory barrier is used to make sure the buffer is
     * visible when the signal is received on other CPUs. */
    sgx_mfence();

    sl_siglines_trigger_signal(siglns, line);

    // wait till the other side has picked the task for processing
    while ((mngr->tasks[line].status == SL_SUBMITTED) && (--max_tries > 0))
    {
#ifdef SL_INSIDE_ENCLAVE /* trusted */
        if (sgx_is_enclave_crashed())
            return SGX_ERROR_ENCLAVE_CRASHED;
#endif
        asm_pause();
    }

    if (unlikely(max_tries == 0))
    {
        if (sl_siglines_revoke_signal(siglns, line) == 0)
        {
            ret = -EAGAIN;
            goto on_exit;
        }
        /* Otherwise, the signal is not revoked succesfully, meaning this
        * call is being or has been processed by workers. So we continue. */
    }

    /* The request must has been accepted. Now wait for its completion */
    while (mngr->tasks[line].status != SL_DONE)
    {
#ifdef SL_INSIDE_ENCLAVE /* trusted */
        if (sgx_is_enclave_crashed())
            return SGX_ERROR_ENCLAVE_CRASHED;
#endif
        asm_pause();
    }

    // copy the return code
    call_task->ret_code = mngr->tasks[line].ret_code;

on_exit:
    SET_VALUE_HARDEN_FOR_SEND_TASK(&mngr->tasks[line].func_id, SL_INVALID_FUNC_ID, uint32_t);
    sl_siglines_free_line(siglns, line);
    return ret;
}


#ifdef __cplusplus
}
#endif

#endif	// __SL_FCALL_MNGR_COMMON_H__
