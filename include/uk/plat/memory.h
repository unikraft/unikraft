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
 *
 * THIS HEADER MAY NOT BE EXTRACTED OR MODIFIED IN ANY WAY.
 */

#ifndef __UKPLAT_MEMORY_H__
#define __UKPLAT_MEMORY_H__

#include <uk/arch/types.h>
#include <uk/alloc.h>
#include <uk/config.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Memory region flags */
#define UKPLAT_MEMRF_FREE        (0x1)
#define UKPLAT_MEMRF_RESERVED    (0x2)
#define UKPLAT_MEMRF_READABLE    (0x4)
#define UKPLAT_MEMRF_WRITABLE    (0x8)

#define UKPLAT_MEMRF_ALLOCATABLE (UKPLAT_MEMRF_FREE \
				  | UKPLAT_MEMRF_READABLE \
				  | UKPLAT_MEMRF_WRITABLE)

/* Descriptor of a memory region */
struct ukplat_memregion_desc {
	void *base;
	__sz len;
	int flags;
#if CONFIG_UKPLAT_MEMRNAME
	const char *name;
#endif
};

/**
 * Returns the number of available memory regions
 * @return Number of memory regions
 */
int ukplat_memregion_count(void);

/**
 * Reads a memory region to mrd
 * @param i Memory region number
 * @param mrd Pointer to memory region descriptor that will be filled out
 * @return 0 on success, < 0 otherwise
 */
int ukplat_memregion_get(int i, struct ukplat_memregion_desc *mrd);

/**
 * Sets the platform memory allocator and triggers the platform memory mappings
 * for which an allocator is needed.
 * @param a Memory allocator
 * @return 0 on success, < 0 otherwise
 */
int ukplat_memallocator_set(struct uk_alloc *a);

/**
 * Returns the platform memory allocator
 * @return Platform memory allocator address
 */
struct uk_alloc *ukplat_memallocator_get(void);

#ifdef __cplusplus
}
#endif

#endif /* __PLAT_MEMORY_H__ */
