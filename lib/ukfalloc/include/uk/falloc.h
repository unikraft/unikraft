/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Marc Rittinghaus <marc.rittinghaus@kit.edu>
 *
 * Copyright (c) 2021, Karlsruhe Institute of Technology (KIT).
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

#ifndef __UK_FALLOC_H__
#define __UK_FALLOC_H__

#include <uk/arch/paging.h>
#include <uk/arch/types.h>
#include <uk/essentials.h>
#include <uk/assert.h>

#ifdef CONFIG_LIBUKFALLOC_STATS
#include <uk/list.h>
#include <uk/store.h>
#endif /* CONFIG_LIBUKFALLOC_STATS */

#ifdef __cplusplus
extern "C" {
#endif

struct uk_falloc {

#define FALLOC_FLAG_ALIGNED		0x01 /* align allocation to its size */

	/**
	 * Allocates physical memory
	 *
	 * @param fa the instance of the frame allocator
	 * @param [in,out] paddr the start address of the physical memory to
	 *    allocate. The memory range has to be included in a prior call to
	 *    `addmem`. Can be __PADDR_ANY to let the allocator choose an
	 *    address. In this case, paddr receives the start address of the
	 *    physical memory area on success, __PADDR_INV otherwise.
	 *    FALLOC_FLAG_ALIGNED is ignored if a physical address is supplied
	 * @param flags allocation flags (FALLOC_FLAG_*)
	 * @param frames the number of frames to allocate (i.e., PAGE_SIZE)
	 *
	 * @return 0 on success, a non-zero error otherwise
	 */
	int (*falloc)(struct uk_falloc *fa, __paddr_t *paddr,
		      unsigned long frames, unsigned long flags);

	/**
	 * Allocates physical memory in a certain address range. The range has
	 * to be included in a prior call to `addmem`.
	 *
	 * @param fa the instance of the frame allocator
	 * @param [out] paddr the start address of the allocated physical
	 *    memory area. Not touched on error
	 * @param flags allocation flags (FALLOC_FLAG_*)
	 * @param frames the number of frames to allocate (i.e., PAGE_SIZE)
	 * @param min the start of the permissable address range. Can be
	 *    __PADDR_MIN to not restrict the permissable range at the low end
	 * @param max the end of the permissable address range. Can be
	 *    __PADDR_MAX to not restrict the permissable range at the hight end
	 *
	 * @return 0 on success, a non-zero error otherwise
	 */
	int (*falloc_from_range)(struct uk_falloc *fa, __paddr_t *paddr,
				 unsigned long frames, unsigned long flags,
				 __paddr_t min, __paddr_t max);

	/**
	 * Frees physical memory
	 *
	 * Note that the address and size do not have to match a previous
	 * allocation. The physical memory area must only fully cover one or
	 * more previous allocations with no holes in between. Attempting to
	 * free unallocated memory fails with an error code
	 *
	 * @param fa the instance of the frame allocator
	 * @param paddr the start address of the physical memory area to free
	 * @param frames the number of frames to free
	 *
	 * @return 0 on success, a non-zero error otherwise. On error, the
	 *    state of the specified memory region is undefined (i.e., some
	 *    parts may have been freed)
	 */
	int (*ffree)(struct uk_falloc *fa, __paddr_t paddr,
		     unsigned long frames);

	/**
	 * Adds a physical memory region to be managed by the allocator
	 *
	 * @param fa the instance of the frame allocator
	 * @param metadata pointer to a buffer that will hold the metadata of
	 *    the allocator for the memory range. See the respective allocator
	 *    for a utility function to compute the required size
	 * @param paddr the start address of the physical memory area which is
	 *    to be added to the allocator
	 * @param frames the number of frames to add
	 * @param dm_off the offset in the virtual address space where a
	 *    direct mapping of the specified physical memory area can be found
	 *    or __VADDR_INV if no such mapping exists
	 *
	 * @return 0 on success, a non-zero error otherwise
	 */
	int (*addmem)(struct uk_falloc *fa, void *metadata, __paddr_t paddr,
		      unsigned long frames, __vaddr_t dm_off);

	/** The amount of free memory managed by the allocator in bytes */
	__sz free_memory;

	/** The total amount of memory managed by the allocator in bytes */
	__sz total_memory;

#ifdef CONFIG_LIBUKFALLOC_STATS
	struct uk_list_head store_obj_list;
	struct uk_store_object *stats;
#endif /* CONFIG_LIBUKFALLOC_STATS */
};

/**
 * Allocates physical memory
 *
 * @param fa the frame allocator from which to allocate
 * @param frames the number of frames to allocate
 *
 * @return the start address of the allocated physical memory area on success,
 *    __PADDR_INV otherwise
 */
static inline __paddr_t uk_falloc(struct uk_falloc *fa, unsigned long frames)
{
	__paddr_t lpaddr = __PADDR_ANY;
	int rc;

	UK_ASSERT(fa);
	UK_ASSERT(fa->falloc);

	rc = fa->falloc(fa, &lpaddr, frames, 0);
	if (unlikely(rc))
		return __PADDR_INV;

	return lpaddr;
}

/**
 * Frees physical memory
 *
 * Note that the address and size do not have to match a previous
 * allocation. The physical memory area must only fully cover one or
 * more previous allocations with no holes in between.
 *
 * @param fa the frame allocator from which the memory has been allocated
 * @param paddr the start address of the allocated physical memory area which
 *    is to be freed
 * @param frames the number of frames to free
 */
static inline void uk_ffree(struct uk_falloc *fa, __paddr_t paddr,
			    unsigned long frames)
{
	int rc __maybe_unused;

	UK_ASSERT(fa);
	UK_ASSERT(fa->ffree);

	rc = fa->ffree(fa, paddr, frames);
	UK_ASSERT(rc == 0);
}

#ifdef __cplusplus
}
#endif

#endif /* __UK_FALLOC_H__ */
