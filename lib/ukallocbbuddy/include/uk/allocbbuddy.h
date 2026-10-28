/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Simon Kuenzer <simon.kuenzer@neclab.eu>
 *
 *
 * Copyright (c) 2017, NEC Europe Ltd., NEC Corporation. All rights reserved.
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

#ifndef __UKALLOCBBUDDY_H__
#define __UKALLOCBBUDDY_H__

#include <uk/alloc.h>

#ifdef __cplusplus
extern "C" {
#endif

struct uk_alloc *uk_allocbbuddy_init(void *base, size_t len);

/**
 * Called, once a free is done, with each free block the free made or grew
 * (merged with its free buddies) but for its first page, which holds its free
 * list entry: pages the allocator does not read or write until it splits the
 * block. Over memory paged in on demand, their frames can be given back. Some
 * may have been given back already, before the block grew.
 */
typedef void (*uk_allocbbuddy_release_func_t)(struct uk_alloc *a,
					       void *base,
					       unsigned long num_pages);

/**
 * Set the function called with the pages the allocator stops using
 * (see uk_allocbbuddy_release_func_t), or none.
 */
void uk_allocbbuddy_set_release(struct uk_alloc *a,
				uk_allocbbuddy_release_func_t release);

#ifdef __cplusplus
}
#endif

#endif /* __UKALLOCBBUDDY_H__ */
