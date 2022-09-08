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

#include "global_data.h"
#include "sgx_error.h"
#include "stdint.h"


void *rsrv_mem_base __attribute__((section(RELRO_SECTION_NAME))) = NULL;
size_t rsrv_mem_size __attribute__((section(RELRO_SECTION_NAME))) = 0;
size_t rsrv_mem_min_size __attribute__((section(RELRO_SECTION_NAME))) = 0;

SE_DECLSPEC_EXPORT size_t g_peak_rsrv_mem_committed = 0;


extern "C" int rsrv_mem_init(void *_rsrv_mem_base, size_t _rsrv_mem_size, size_t _rsrv_mem_min_size)
{
    if ((_rsrv_mem_base == NULL) || (((size_t) _rsrv_mem_base) & (SE_PAGE_SIZE - 1)))
        return SGX_ERROR_UNEXPECTED;

    if (_rsrv_mem_size & (SE_PAGE_SIZE - 1))
        return SGX_ERROR_UNEXPECTED;

    if (_rsrv_mem_min_size & (SE_PAGE_SIZE - 1))
        return SGX_ERROR_UNEXPECTED;

    if (_rsrv_mem_size > SIZE_MAX - (size_t)rsrv_mem_base)
        return SGX_ERROR_UNEXPECTED;

    rsrv_mem_base = _rsrv_mem_base;
    rsrv_mem_size = _rsrv_mem_size;
    rsrv_mem_min_size = _rsrv_mem_min_size;

    return SGX_SUCCESS;
}

