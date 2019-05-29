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
#define UKPLAT_MEMRF_FREE	(0x1)	/* Region with uninitialized memory */
#define UKPLAT_MEMRF_RESERVED	(0x2)	/* Region is in use by platform */
#define UKPLAT_MEMRF_EXTRADATA	(0x4)	/* Contains additional loaded data
					 * (e.g., initramdisk)
					 */
#define UKPLAT_MEMRF_READABLE	(0x10)	/* Region is readable */
#define UKPLAT_MEMRF_WRITABLE	(0x20)	/* Region is writable */

#define UKPLAT_MEMRF_ALLOCATABLE (UKPLAT_MEMRF_FREE \
				  | UKPLAT_MEMRF_READABLE \
				  | UKPLAT_MEMRF_WRITABLE)

#define UKPLAT_MEMRF_INITRD      (UKPLAT_MEMRF_EXTRADATA \
				  | UKPLAT_MEMRF_READABLE)

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
 * Searches for the next memory region after `i` that has at least `sflags`
 * flags set.
 * @param i Memory region number to start searching
 *        To start searching from the beginning, set `i` to `-1`
 * @param sflags Find only memory regions that have at least `sflags` flags set.
 *        If no flags are given (`0x0`), any found region is returned
 * @Param mrd Pointer to memory region descriptor that will be filled out
 * @return On success the function returns the next region number after `i`
 *         that fulfills `sflags`. `mrd` is filled out with the memory region
 *         details. A value < 0 is returned if no more region could be found
 *         that fulfills the search criteria. `mrd` may be filled out with
 *         undefined values.
 */
static inline int ukplat_memregion_find_next(int i, int sflags,
					     struct ukplat_memregion_desc *mrd)
{
	int rc, count;

	count = ukplat_memregion_count();

	if (i >= count)
		return -1;

	do
		rc = ukplat_memregion_get(++i, mrd);
	while (i < count && (rc < 0 || ((mrd->flags & sflags) != sflags)));

	if (i == count)
		return -1;
	return i;
}

/**
 * Iterates over all memory regions that have at least `sflags` flags set.
 * @param mrd Pointer to memory region descriptor that will be filled out
 *        during iteration
 * @param sflags Iterate only over memory regions that have at least `sflags`
 *        flags set. If no flags are given (`0x0`), every existing region is
 *        iterated.
 */
#define ukplat_memregion_foreach(mrd, sflags)				\
	for (int __ukplat_memregion_foreach_i				\
		     = ukplat_memregion_find_next(-1, (sflags), (mrd));	\
	     __ukplat_memregion_foreach_i >= 0;				\
	     __ukplat_memregion_foreach_i				\
		     = ukplat_memregion_find_next(__ukplat_memregion_foreach_i,\
						  (sflags), (mrd)))

/**
 * Searches for the first initrd module
 * @param mrd Pointer to memory region descriptor that will be filled out
 * @return On success, returns the region number of the first initrd module,
 *         `mrd` is filled out with the memory region details.
 *         A return value < 0 means that there is no initrd module,
 *         `mrd` may be filled out with undefined values.
 */
#define ukplat_memregion_find_initrd0(mrd) \
	ukplat_memregion_find_next(-1, UKPLAT_MEMRF_INITRD, (mrd))

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

/**
 * Sets the current thread address on top of kernel stack
 * @param thread_addr Current thread address
 */
void ukplat_stack_set_current_thread(void *thread_addr);

#ifdef __cplusplus
}
#endif

#endif /* __PLAT_MEMORY_H__ */
