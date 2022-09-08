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
#include <string.h>
#include "sgx_secure_align_api.h"
#include "sgx_secure_align.h"

#define GET_ALIGNED_PTR(raw, align, offset)   (void*)((((size_t)(raw) + (align) - 1) & ~((align) - 1)) + (offset));
#define CHECK_OVERFLOW(buf, len) ((size_t)(buf) + (len) < (len) ||  (size_t)(buf) + (len) < (size_t)(buf))

static bool check_align_req(size_t size, const align_req_t *data, size_t count)
{
    if (!data || !count)
    {
        return false;
    }
    size_t len = (size + 7) / 8;
    uint8_t *bmp = (uint8_t*)malloc(len);
    if (!bmp)
    {
        return false;
    }
    memset(bmp, 0, len);
    for (size_t i = 0; i < count; ++i) {
        if (CHECK_OVERFLOW(data[i].offset, data[i].len) || data[i].offset + data[i].len > size) {
            free(bmp);
            return false;
        }
        for (size_t j = 0; j < data[i].len; j++)
        {
            size_t offset = data[i].offset + j;
            if (bmp[offset / 8] & (1 << (offset % 8))) {
                // overlap in req data
                free(bmp);
                return false;
            }
            uint8_t tmp = (uint8_t)(1 << (offset % 8));
            bmp[offset / 8] |= tmp;
        }
    }
    free(bmp);
    return true;
}
static int64_t make_bitmap(const align_req_t *data, size_t count)
{
    int64_t bmp = 0;
    for (size_t i = 0; i < count; ++i) {
        if (data[i].len > 63) return -1;
        bmp |= sgx::rol((1ll << data[i].len) - 1, data[i].offset);
    }
	return bmp;
}
void *sgx_aligned_malloc(size_t size, size_t alignment, align_req_t *data, size_t count)
{
    // check parameters
    if (!size || !alignment || (alignment & (alignment -1)))
        return NULL;
    align_req_t tmp_req = { 0, size };
    align_req_t *req = data;
    size_t req_cnt = count;
    if (!data && !count)
    {
        req = &tmp_req;
        req_cnt = 1;
    }
    else if (!check_align_req(size, req, req_cnt))
        return NULL;

    // get aligned pointer
    int64_t bmp = make_bitmap(req, req_cnt);
    int offset = sgx::__custom_alignment_internal::calc_lspc(alignment, bmp);
    if (offset < 0) return NULL;
    size_t align = sgx::__custom_alignment_internal::calc_algn(alignment, size + offset);
    void *raw = malloc(size + align + offset + sizeof(void*));
    if (!raw) return NULL;
    void *ptr = GET_ALIGNED_PTR((void*)((size_t)raw + sizeof(void*)), align, offset);
    ((void**)ptr)[-1] = raw;

    return ptr;
}
void sgx_aligned_free(void *ptr)
{
    void *raw = ((void**)ptr)[-1];
    free(raw);
}
void *sgx_get_aligned_ptr(void *raw, size_t raw_size, size_t allocate_size, size_t alignment, align_req_t *data, size_t count)
{
    // check parameters
    if (!raw || !raw_size || !allocate_size || allocate_size > raw_size || !alignment || (alignment & (alignment - 1)))
        return NULL;
    if (CHECK_OVERFLOW(raw, raw_size))
        return NULL;
    align_req_t tmp_req = {0, allocate_size};
    align_req_t *req = data;
    size_t req_cnt = count;
    if (!data && !count)
    {
        req = &tmp_req;
        req_cnt = 1;
    }
    else if (!check_align_req(allocate_size, req, req_cnt))
        return NULL;

    // get aligned pointer
    int64_t bmp = make_bitmap(req, req_cnt);
    int offset = sgx::__custom_alignment_internal::calc_lspc(alignment, bmp);
    if (offset < 0) return NULL;
    size_t align = sgx::__custom_alignment_internal::calc_algn(alignment, allocate_size + offset);
    void *ptr = GET_ALIGNED_PTR(raw, align, offset);

    // check if raw is big enough to allocate the buffer
    if (CHECK_OVERFLOW(ptr, allocate_size))
        return NULL;
    if ((size_t)ptr + allocate_size > (size_t)raw + raw_size)
        return NULL;

    return ptr;
}
