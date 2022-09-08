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


#include <stdint.h>
#include <memory.h>
#include <stdlib.h>
#include "sgx_read_rand.h"
#include "se_wrapper.h"
#include "aesm_rand.h"
#include "AEClass.h"

ae_error_t sgx_error_to_ae_error(sgx_status_t status)
{
    if(SGX_ERROR_OUT_OF_MEMORY == status)
        return AE_OUT_OF_MEMORY_ERROR;
    if(SGX_SUCCESS == status)
        return AE_SUCCESS;
    return AE_FAILURE;
}

extern "C" ae_error_t aesm_read_rand(unsigned char *rand_buf, unsigned int length_in_bytes)
{
    // check parameters
    //
    if (!rand_buf || !length_in_bytes)
    {
        return AE_INVALID_PARAMETER;
    }

    sgx_status_t sgx_status = sgx_read_rand(rand_buf, length_in_bytes);

    return sgx_error_to_ae_error(sgx_status);
}



