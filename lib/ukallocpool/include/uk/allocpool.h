/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Simple memory pool using LIFO principle
 *
 * Authors: Simon Kuenzer <simon.kuenzer@neclab.eu>
 *
 *
 * Copyright (c) 2020, NEC Laboratories Europe GmbH, NEC Corporation,
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

#ifndef __LIBUKALLOCPOOL_H__
#define __LIBUKALLOCPOOL_H__

#include <uk/alloc.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*uk_allocpool_obj_init_t)(void *obj, size_t len, void *cookie);

struct uk_allocpool;

/**
 * Initializes a memory pool on a given memory range.
 *
 * @param base
 *  Base address of memory range.
 * @param len
 *  Length of memory range (bytes).
 * @param obj_len
 *  Size of one object (bytes).
 * @param obj_align
 *  Alignment requirement for each pool object.
 * @return
 *  - (NULL): Not enough memory for pool.
 *  - pointer to initializes pool.
 */
struct uk_allocpool *uk_allocpool_init(void *base, size_t len,
				       size_t obj_len, size_t obj_align);

/**
 * Return the number of current available (free) objects.
 *
 * @param p
 *  Pointer to memory pool.
 * @return
 *  Number of free objects in the pool.
 */
unsigned int uk_allocpool_availcount(struct uk_allocpool *p);

/**
 * Return the size of an object.
 *
 * @param p
 *  Pointer to memory pool.
 * @return
 *  Size of an object.
 */
size_t uk_allocpool_objlen(struct uk_allocpool *p);

/**
 * Get one object from a pool.
 *
 * @param p
 *  Pointer to memory pool.
 * @return
 *  - (NULL): No more free objects available.
 *  - Pointer to object.
 */
void *uk_allocpool_take(struct uk_allocpool *p);

/**
 * Return one object back to a pool.
 *
 * @param p
 *  Pointer to memory pool.
 * @param obj
 *  Pointer to object that should be returned.
 */
void uk_allocpool_return(struct uk_allocpool *p, void *obj);

#ifdef __cplusplus
}
#endif

#endif /* __LIBUKALLOCPOOL_H__ */
