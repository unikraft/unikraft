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

#include <sl_init.h>
#include <sl_uswitchless.h>
#include <sl_atomic.h>
#include <sl_debug.h>
#include <sgx_trts.h>


// holds the pointer to untrusted structure describing switchless configuration
// pointer is assigned only after all necessary checks
struct sl_uswitchless* g_uswitchless_handle = NULL;

/* Override the weak symbol defined in tRTS */
sgx_status_t sl_init_switchless(void* _handle_u)
{
    struct sl_uswitchless* handle_u = (struct sl_uswitchless*)_handle_u;

    // ensure that the whole structure is outside enclave before coping pointer to global variable on trusted side
    PANIC_ON(!sgx_is_outside_enclave(handle_u, sizeof(*handle_u)));
    sgx_lfence();

    if (lock_cmpxchg_ptr((void*)&g_uswitchless_handle, NULL, (void*)handle_u) != NULL)
    {
        return SGX_ERROR_UNEXPECTED;
    }
    return SGX_SUCCESS;
}
