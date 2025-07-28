/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Hugo Lefeuvre <hugo.lefeuvre@neclab.eu>
 *          Sergiu Moga <sergiu@unikraft.io>
 *
 * Copyright (c) 2020, NEC Laboratories Europe GmbH, NEC Corporation,
 *                     All rights reserved.
 * Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 *                     All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __LIBUKALLOCREGION_H__
#define __LIBUKALLOCREGION_H__

#include <uk/alloc.h>

#ifdef __cplusplus
extern "C" {
#endif

struct uk_allocregion;

/**
 * Allocate memory from an allocation region.
 *
 * @param ar
 *   Pointer to the allocator region instance
 * @param size
 *   Number of bytes to allocate
 * @return
 *  - (NULL): If allocation failed (e.g., ENOMEM)
 *  - pointer to allocated region
 */
void *uk_allocregion_bump(struct uk_allocregion *a, size_t size);

/**
 * Get remaining space in an allocation region.
 *
 * @param a
 *   Pointer to the allocation region instance
 * @return
 *   Remaining memory in the allocation region
 */
size_t uk_allocregion_availmem(const struct uk_allocregion *ar);

/**
 * Return uk_alloc compatible interface for allocregion.
 * With this interface, uk_malloc(), uk_free(), etc. can
 * be used with the allocation region.
 *
 * @param ar
 *  Pointer to the allocation region instance
 * @return
 *  Pointer to uk_alloc interface of given allocation region
 */
struct uk_alloc *uk_allocregion2ukalloc(const struct uk_allocregion *ar);

/**
 * Initializes a memory allocation region on a given memory range.
 *
 * @param base
 *  Base address of memory range.
 * @param len
 *  Length of memory range (bytes).
 * @return
 *  - (NULL): Not enough memory for allocation region
 *  - pointer to the allocation region
 */
struct uk_allocregion *uk_allocregion_init(void *base, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* __LIBUKALLOCREGION_H__ */
