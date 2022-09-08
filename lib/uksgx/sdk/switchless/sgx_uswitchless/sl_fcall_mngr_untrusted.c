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

#include "sl_fcall_mngr_common.h"

uint32_t sl_call_mngr_init(struct sl_call_mngr* mngr,
                       sl_call_type_t type,
                       uint32_t max_pending_calls)
{
    uint32_t i;
    mngr->type = type;

    struct sl_call_task* tasks = (struct sl_call_task*)calloc(max_pending_calls, sizeof(tasks[0]));
    if (tasks == NULL) 
        return ENOMEM;

    // because zero is a valid function id, initialize struct field to a special value
    for (i = 0; i < max_pending_calls; i++)
    {
        tasks[i].func_id = SL_INVALID_FUNC_ID;
    }
    
    mngr->tasks = tasks;

    uint32_t ret = sl_siglines_init(&mngr->siglns,
                                    call_type2direction(type),
                                    max_pending_calls,
                                    can_type_process(type) ? process_switchless_call : NULL);
    if (ret != 0) 
    { 
        free(tasks); 
        return ret; 
    }

    mngr->call_table = NULL;
    return 0;
}

void sl_call_mngr_destroy(struct sl_call_mngr* mngr) 
{
    sl_siglines_destroy(&mngr->siglns);
    free(mngr->tasks);
}
