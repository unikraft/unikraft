/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Costin Lupu <costin.lupu@cs.pub.ro>
 *
 * Copyright (c) 2018, NEC Europe Ltd., NEC Corporation. All rights reserved.
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

#include <uk/plat/common/bootinfo.h>
#include <uk/asm/limits.h>
#include <uk/alloc.h>
#include <stddef.h>
#include <stdbool.h>

static struct uk_alloc *plat_allocator;

int ukplat_memallocator_set(struct uk_alloc *a)
{
	UK_ASSERT(a != NULL);

	if (plat_allocator != NULL)
		return -1;

	plat_allocator = a;

	_ukplat_mem_mappings_init();

	return 0;
}

struct uk_alloc *ukplat_memallocator_get(void)
{
	return plat_allocator;
}

void *ukplat_memregion_alloc(__sz size, int type)
{
	struct ukplat_memregion_desc *mrd, alloc_mrd = {0};
	__vaddr_t unmap_start, unmap_end;
	struct ukplat_bootinfo *bi;
	__paddr_t pstart, pend;
	__paddr_t ostart, olen;
	__sz unmap_len;
	int rc;

	unmap_start = ALIGN_DOWN(bpt_unmap_mrd.vbase, __PAGE_SIZE);
	unmap_end = unmap_start + ALIGN_DOWN(bpt_unmap_mrd.len, __PAGE_SIZE);
	unmap_len = unmap_end - unmap_start;

	size = ALIGN_UP(size, __PAGE_SIZE);
	ukplat_memregion_foreach(&mrd, UKPLAT_MEMRT_FREE, 0, 0) {
		UK_ASSERT(mrd->pbase <= __U64_MAX - size);
		pstart = ALIGN_UP(mrd->pbase, __PAGE_SIZE);
		pend   = pstart + size;

		if (unmap_len &&
		    (!RANGE_CONTAIN(unmap_start, unmap_len, pstart, size) ||
		    pend > mrd->pbase + mrd->len))
			continue;

		if ((mrd->flags & UKPLAT_MEMRF_PERMS) !=
			    (UKPLAT_MEMRF_READ | UKPLAT_MEMRF_WRITE))
			return NULL;

		ostart = mrd->pbase;
		olen   = mrd->len;

		/* If fragmenting this memory region leaves it with length 0,
		 * then simply overwrite and return it instead.
		 */
		if (olen - (pstart - ostart) == size) {
			mrd->pbase = pstart;
			mrd->vbase = pstart;
			mrd->len = pend - pstart;
			mrd->type = type;
			mrd->flags = UKPLAT_MEMRF_READ |
				     UKPLAT_MEMRF_WRITE |
				     UKPLAT_MEMRF_MAP;

			return (void *)pstart;
		}

		/* Adjust free region */
		mrd->len  -= pend - mrd->pbase;
		mrd->pbase = pend;

		mrd->vbase = (__vaddr_t)mrd->pbase;

		/* Insert allocated region */
		alloc_mrd.vbase = pstart;
		alloc_mrd.pbase = pstart;
		alloc_mrd.len   = size;
		alloc_mrd.type  = type;
		alloc_mrd.flags = UKPLAT_MEMRF_READ  |
				  UKPLAT_MEMRF_WRITE |
				  UKPLAT_MEMRF_MAP;

		bi = ukplat_bootinfo_get();
		if (unlikely(!bi))
			return NULL;

		rc = ukplat_memregion_list_insert(&bi->mrds, &alloc_mrd);
		if (unlikely(rc < 0)) {
			/* Restore original region */
			mrd->vbase = ostart;
			mrd->len   = olen;

			return NULL;
		}

		return (void *)pstart;
	}

	return NULL;
}
