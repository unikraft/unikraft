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

#include "sl_siglines.h"
#include "sl_debug.h"
#include <sgx_trts.h>
#include <sgx_error.h>
#include <sgx_lfence.h>
#include <errno.h>
#include <stdlib.h>

//
// clones the signal lines structure
// in case of OCALL manager, allocates part of structure used by enclave senders inside the enclave.
//
// returns 0 on success, error code otherwise
uint32_t sl_siglines_clone(struct sl_siglines* sglns,
                           const struct sl_siglines* untrusted,
                           sl_sighandler_t handler)
{
    // copies pointers to untrusted structures ensuring that all the data is outside enclave
    PANIC_ON(!sgx_is_outside_enclave(untrusted, sizeof(*untrusted)));
    sgx_lfence();

    sl_siglines_dir_t direction = untrusted->direction;
    PANIC_ON((direction != SL_SIGLINES_DIR_T2U) && (direction != SL_SIGLINES_DIR_U2T));

    BUG_ON(sglns == NULL);

    sglns->direction = direction;

    BUG_ON(is_direction_sender(direction) && (handler != NULL));

    uint32_t num_lines = untrusted->num_lines;
    if ((num_lines <= 0) || ((num_lines % NBITS_PER_LINE) != 0))
        return EINVAL;

    sglns->num_lines = num_lines;
    uint32_t nlong = num_lines / NBITS_PER_LINE;

    sl_sigline_t* event_lines_u = untrusted->event_lines;
    if (event_lines_u == NULL)
        return EINVAL;
    
    PANIC_ON(!sgx_is_outside_enclave(event_lines_u, sizeof(sl_sigline_t) * nlong));
    sgx_lfence();
    sglns->event_lines = event_lines_u;

    // free lines are only used by senders
    sl_sigline_t* free_lines = NULL;
    if (is_direction_sender(direction)) 
    {
        // OCALL manager, enclave is the sender
        // free_lines is used by enclave threads only, so allocate it inside the enclave
        // never gets freed, no global termination hooks defined
        free_lines = (sl_sigline_t*)malloc(sizeof(sl_sigline_t) * nlong);
        if (free_lines == NULL) 
            return ENOMEM;
		
        for (uint32_t i = 0; i < nlong; i++)
            free_lines[i] = SL_FREE_LINE_INIT;
    }

    sglns->free_lines = free_lines;
    sglns->handler = handler;

    return 0;
}
