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

#ifndef _SGX_SECURE_ALIGN_API_H_
#define _SGX_SECURE_ALIGN_API_H_

#include <stdint.h>

typedef struct
{
    size_t offset;
    size_t len;
} align_req_t;

#ifdef __cplusplus
extern "C" {
#endif
    /**
     * sgx_aligned_malloc
     *
     *     Allocates memory for a structure on a specified alignment boundary
     *
     * Parameters:
     *     size - the size of the requested memory allocation in bytes.
     *     alignment - the alignment value, which must be an integer power of 2.
     *     data - (offset, length) pairs to define the fields in the structure for secrets 
     *             If data is NULL and count is 0, the whole structure will be aligned.
     *     count - number of align_req_t structure in data
     *             If data is NULL and count is 0, the whole structure will be aligned.
     *
     * Return Value:
     *     A pointer to the memory block that was allocated or NULL if the operation failed.
    */
    void *sgx_aligned_malloc(size_t size, size_t alignment, align_req_t *data, size_t count);
    /**
     * sgx_aligned_free
     *
     *     Frees a block of memory that was allocated with sgx_aligned_malloc
     *
     * Parameters:
     *     ptr - a pointer to the memory block that was returned to the sgx_aligned_malloc
     *
    */
    void sgx_aligned_free(void *ptr);

    /*
     * sgx_get_aligned_ptr
     *
     *     Return a pointer from the pre-allocated memory on a specified alignment boundary
     *
     * Parameters:
     *     raw - the memory allocated by user
     *     raw_size - the size of raw memory in bytes
     *     allocate_size - the size of the requested memory allocation in bytes.
     *     alignment - the alignment value, which must be an integer power of 2.
     *     data - (offset, length) pairs to define the fields in the structure for secrets
     *             If data is NULL and count is 0, the whole structure will be aligned.
     *     count - number of align_req_t structure in data
     *             If data is NULL and count is 0, the whole structure will be aligned.
     * Return Value:
     *     A pointer to the memory block or NULL if the operation failed.
     * Note:
     *     The raw memory should be allocated by user, and it should be big enough to get aligned pointer:
     *          (size + 72)(bytes), if alignment <= 8
     *          (size + 64 + alignment)(bytes), if alignment > 8
     *
     */
    void *sgx_get_aligned_ptr(void *raw, size_t raw_size, size_t allocate_size, size_t alignment, align_req_t *data, size_t count);

#ifdef __cplusplus
}
#endif

#endif
