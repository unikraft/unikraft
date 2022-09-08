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

#include <sgx_trts.h>
#include "sl_fcall_mngr_common.h"

// switchless call managers are allocated on untrusted side, then pointers to data structures are passed to the enclave.
// the following function clones the data, performing all neccessary checks
//
// returns 0 on success
uint32_t sl_call_mngr_clone(struct sl_call_mngr* mngr, const struct sl_call_mngr* untrusted)
{
    // copies fields of untrusted structures ensuring that all the data resided on untrusted side
    BUG_ON(mngr == NULL);
    BUG_ON(untrusted == NULL);

    PANIC_ON(!sgx_is_outside_enclave(untrusted, sizeof(*untrusted)));
    sgx_lfence();

    sl_call_type_t type_u = untrusted->type;

    // garbage data ? probably an attack
    PANIC_ON((type_u != SL_TYPE_ECALL) && (type_u != SL_TYPE_OCALL));
    
    mngr->type = type_u;

    // clone internal pointers to siglines structure
    uint32_t ret = sl_siglines_clone(&mngr->siglns,
                                     &untrusted->siglns,
                                     can_type_process(type_u) ? process_switchless_call : NULL);
    if (ret != 0) return ret;

    // check that the call manager is properly initialized 
    // if not, can indicate an attack 
    PANIC_ON(call_type2direction(type_u) != sl_siglines_get_direction(&mngr->siglns));

    struct sl_call_task* tasks_u = untrusted->tasks;

    if (tasks_u == NULL)
        return EINVAL;

    // check that the array of task structures is outside enclave
    PANIC_ON(!sgx_is_outside_enclave(tasks_u, sizeof(tasks_u[0]) * sl_siglines_size(&mngr->siglns)));
    sgx_lfence();

    mngr->tasks = tasks_u;
    mngr->call_table = NULL;

    return 0;
}
