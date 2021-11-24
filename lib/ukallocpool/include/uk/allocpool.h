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

struct uk_allocpool;

/**
 * Computes the required memory for a pool allocation.
 *
 * @param obj_count
 *  Number of objects that are allocated with the pool.
 * @param obj_len
 *  Size of one object (bytes).
 * @param obj_align
 *  Alignment requirement for each pool object.
 * @return
 *  Number of bytes needed for pool allocation.
 */
__sz uk_allocpool_reqmem(unsigned int obj_count, __sz obj_len,
			 __sz obj_align);

/**
 * Allocates a memory pool on a parent allocator.
 *
 * @param parent
 *  Allocator on which the pool will be allocated.
 * @param obj_count
 *  Number of objects that are allocated with the pool.
 * @param obj_len
 *  Size of one object (bytes).
 * @param obj_align
 *  Alignment requirement for each pool object.
 * @return
 *  - (NULL): If allocation failed (e.g., ENOMEM).
 *  - pointer to allocated pool.
 */
struct uk_allocpool *uk_allocpool_alloc(struct uk_alloc *parent,
					unsigned int obj_count,
					__sz obj_len, __sz obj_align);

/**
 * Frees a memory pool that was allocated with
 * uk_allocpool_alloc(). The memory is returned to
 * the parent allocator.
 * Note: Please make sure that all taken objects
 * are returned to the pool before free'ing the
 * pool.
 *
 * @param p
 *  Pointer to memory pool that will be free'd.
 */
void uk_allocpool_free(struct uk_allocpool *p);

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
struct uk_allocpool *uk_allocpool_init(void *base, __sz len,
				       __sz obj_len, __sz obj_align);

/**
 * Return uk_alloc compatible interface for allocpool.
 * With this interface, uk_malloc(), uk_free(), etc. can
 * be used with the pool.
 *
 * @param p
 *  Pointer to memory pool.
 * @return
 *  Pointer to uk_alloc interface of given pool.
 */
struct uk_alloc *uk_allocpool2ukalloc(struct uk_allocpool *p);

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
__sz uk_allocpool_objlen(struct uk_allocpool *p);

/**
 * Get one object from a pool.
 * HINT: It is recommended to use this call instead of uk_malloc() whenever
 *       feasible. This call is avoiding indirections.
 *
 * @param p
 *  Pointer to memory pool.
 * @return
 *  - (NULL): No more free objects available.
 *  - Pointer to object.
 */
void *uk_allocpool_take(struct uk_allocpool *p);

/**
 * Get multiple objects from a pool.
 *
 * @param p
 *  Pointer to memory pool.
 * @param obj
 *  Pointer to array that will be filled with pointers of
 *  allocated objects from the pool.
 * @param count
 *  Maximum number of objects that should be taken from the pool.
 * @return
 *  Number of successfully allocated objects on the given array.
 */
unsigned int uk_allocpool_take_batch(struct uk_allocpool *p,
				     void *obj[], unsigned int count);

/**
 * Return one object back to a pool.
 * HINT: It is recommended to use this call instead of uk_free() whenever
 *       feasible. This call is avoiding indirections.
 *
 * @param p
 *  Pointer to memory pool.
 * @param obj
 *  Pointer to object that should be returned.
 */
void uk_allocpool_return(struct uk_allocpool *p, void *obj);

/**
 * Return multiple objects to a pool.
 *
 * @param p
 *  Pointer to memory pool.
 * @param obj
 *  Pointer to array that with pointers of objects that
 *  should be returned.
 * @param count
 *  Number of objects that are on the array.
 */
void uk_allocpool_return_batch(struct uk_allocpool *p,
			       void *obj[], unsigned int count);

#ifdef __cplusplus
}
#endif

#endif /* __LIBUKALLOCPOOL_H__ */
