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

#ifndef __PLAT_CMN_MEMORY_H__
#define __PLAT_CMN_MEMORY_H__

#include <uk/config.h>
#include <uk/assert.h>
#include <uk/essentials.h>
#include <uk/arch/paging.h>
#include <uk/arch/types.h>
#include <uk/plat/memory.h>

#include <errno.h>
#include <string.h>

struct ukplat_memregion_list {
	/** Maximum regions in the list */
	__u32 capacity;
	/** Current number of regions in the list */
	__u32 count;
	/** Array of memory regions */
	struct ukplat_memregion_desc mrds[];
} __packed __align(__SIZEOF_LONG__);

/**
 * Initializes the fields of an empty memory region list.
 *
 * @param list
 *   The list to initialize
 * @param capacity
 *   The number of memory region descriptors that the list can hold. This
 *   number must be set according to the amount of allocated memory.
 */
static inline void
ukplat_memregion_list_init(struct ukplat_memregion_list *list, __u32 capacity)
{
	UK_ASSERT(capacity >= 1);

	list->capacity = capacity;
	list->count    = 0;
}

/**
 * Inserts the region into the memory region list. The list is kept in
 * ascending order of physical base addresses. Order is only preserved among
 * ranges that do not overlap.
 *
 * @param list
 *   The memory region list to insert the range into
 * @param mrd
 *   The memory range to insert
 *
 * @return
 *   The index of the element added on success, an negative errno otherwise
 */
static inline int
ukplat_memregion_list_insert(struct ukplat_memregion_list *list,
			     const struct ukplat_memregion_desc *mrd)
{
	struct ukplat_memregion_desc *p;
	__u32 i;

	if (unlikely(!mrd->len))
		return -EINVAL;

	if (unlikely(list->count == list->capacity))
		return -ENOMEM;

	/* We start from the tail as it is more likely that we add memory
	 * regions in semi-sorted order.
	 */
	p = &list->mrds[list->count];
	for (i = list->count; i > 0; i--) {
		--p;

		if (p->pbase + p->len <= mrd->pbase) {
			++p;
			break;
		}
	}

	memmove(p + 1, p, sizeof(*p) * (list->count - i));

	*p = *mrd;
	list->count++;
	return (int)i;
}

#if defined(__X86_64__)
#define	X86_HI_MEM_START		0xA0000UL
#define X86_HI_MEM_LEN			0x40000UL

#define X86_BIOS_ROM_START		0xE0000UL
#define X86_BIOS_ROM_LEN		0x20000UL

static inline int
ukplat_memregion_list_insert_legacy_hi_mem(struct ukplat_memregion_list *list)
{
	int rc;

	/* Note that we are mapping it as writable as well to cope with the
	 * potential existence of the VGA framebuffer/SMM shadow memory.
	 */
	rc = ukplat_memregion_list_insert(list,
			&(struct ukplat_memregion_desc){
				.vbase = X86_HI_MEM_START,
				.pbase = X86_HI_MEM_START,
				.len   = X86_HI_MEM_LEN,
				.type  = UKPLAT_MEMRT_RESERVED,
				.flags = UKPLAT_MEMRF_READ  |
					 UKPLAT_MEMRF_WRITE |
					 UKPLAT_MEMRF_MAP,
			});
	if (unlikely(rc < 0))
		return rc;

	/* Keep compatibility with other possible reports of reserved memory
	 * regions of this area and mark the BIOS System Memory as read-only.
	 */
	rc = ukplat_memregion_list_insert(list,
			&(struct ukplat_memregion_desc){
				.vbase = X86_BIOS_ROM_START,
				.pbase = X86_BIOS_ROM_START,
				.len   = X86_BIOS_ROM_LEN,
				.type  = UKPLAT_MEMRT_RESERVED,
				.flags = UKPLAT_MEMRF_READ  |
					 UKPLAT_MEMRF_MAP,
			});
	if (unlikely(rc < 0))
		return rc;

	return 0;
}

#if defined(CONFIG_HAVE_SMP)
extern void *x86_start16_begin[];
extern void *x86_start16_end[];
extern __uptr x86_start16_addr; /* target address */

static inline int
ukplat_memregion_alloc_sipi_vect(void)
{
	__sz len;

	len = (__sz)((__uptr)x86_start16_end - (__uptr)x86_start16_begin);
	len = PAGE_ALIGN_UP(len);
	x86_start16_addr = (__uptr)ukplat_memregion_alloc(len,
							  UKPLAT_MEMRT_RESERVED,
							  UKPLAT_MEMRF_READ  |
							  UKPLAT_MEMRF_WRITE |
							  UKPLAT_MEMRF_MAP);
	if (unlikely(!x86_start16_addr || x86_start16_addr >= X86_HI_MEM_START))
		return -ENOMEM;

	return 0;
}
#else
static inline int
ukplat_memregion_alloc_sipi_vect(void)
{
	return 0;
}
#endif
#endif

/**
 * Inserts the region into the memory region list by index. The list is kept in
 * ascending order of physical base addresses. Order is only preserved among
 * ranges that do not overlap.
 *
 * NOTE:This function assumes that the caller knows what they are doing, as
 *	it is expected that the memory region descriptor list is always
 *	ordered. USE WITH CAUTION!
 */
static inline int
ukplat_memregion_list_insert_at_idx(struct ukplat_memregion_list *list,
				    const struct ukplat_memregion_desc *mrd,
				    __u32 idx)
{
	struct ukplat_memregion_desc *p;

	if (unlikely(!mrd->len))
		return -EINVAL;

	if (unlikely(list->count == list->capacity))
		return -ENOMEM;

	p = &list->mrds[idx];
	memmove(p + 1, p, sizeof(*p) * (list->count - idx));

	*p = *mrd;
	list->count++;
	return (int)idx;
}

/**
 * Insert a new region into the memory region list. This extends
 * ukplat_memregion_list_insert to carve out the area of any pre-existing
 * overlapping regions. The virtual addresses will leave out any carved out
 * region.
 * If there is not enough space for all resulting regions, the function will
 * insert as many as possible and then return an error.
 * @param list
 *   The memory region list to insert the range into
 * @param mrd
 *   The memory range to insert
 * @param min_size
 *   The minimum size an inserted region has to have. Regions smaller than this
 *   size won't be added. Setting this parameter to zero will disable this
 *   behavior.
 * @returns
 *   Zero if the operation was successful, a negative errno otherwise
 */
static inline int
ukplat_memregion_list_insert_split_phys(struct ukplat_memregion_list *list,
					const struct ukplat_memregion_desc *mrd,
					const __sz min_size)
{
	struct ukplat_memregion_desc *mrdp;
	struct ukplat_memregion_desc mrdc;
	__paddr_t pstart, pend;
	__vaddr_t voffset;
	int i;
	int rc;

	if (unlikely(!mrd->len))
		return -EINVAL;

	voffset = mrd->vbase - mrd->pbase;
	pstart = mrd->pbase;
	pend = mrd->pbase + mrd->len;

	mrdc = *mrd;

	for (i = 0; i < (int)list->count; i++) {
		mrdp = &list->mrds[i];
		if (!ukplat_memregion_desc_overlap(mrdp, pstart, pend))
			continue;
		if (pend <= mrdp->pbase)
			break;

		if (!mrdp->type)
			continue;

		if (pstart < mrdp->pbase) {
			/* Some part of the inserted region is before the
			 * overlapping region. Try to insert that part if it's
			 * large enough.
			 */
			mrdc.pbase = pstart;
			mrdc.vbase = pstart + voffset;
			mrdc.len   = mrdp->pbase - pstart;

			if (mrdc.len >= min_size) {
				rc = ukplat_memregion_list_insert_at_idx(list,
									 &mrdc,
									 i - 1);
				if (unlikely(rc < 0))
					return rc;
			}
		}

		pstart = mrdp->pbase + mrdp->len;
	}

	if (pend - pstart < min_size)
		return 0;

	mrdc.pbase = pstart;
	mrdc.vbase = pstart + voffset;
	mrdc.len = pend - pstart;

	/* Add the remaining region */
	rc = ukplat_memregion_list_insert(list, &mrdc);
	return rc < 0 ? rc : 0;
}

/**
 * Delete the specified region from the memory list.
 *
 * @param list
 *   The memory region list from which to delete the region
 * @param idx
 *   The index of the region to delete
 */
static inline void
ukplat_memregion_list_delete(struct ukplat_memregion_list *list, __u32 idx)
{
	struct ukplat_memregion_desc *p;

	UK_ASSERT(idx < list->count);

	if (idx != list->count - 1) {
		p = &list->mrds[idx];
		memmove(p, p + 1, sizeof(*p) * (list->count - idx));
	}

	list->count--;
}

#if CONFIG_LIBUKDEBUG_PRINTD
static inline void
ukplat_memregion_print_desc(struct ukplat_memregion_desc *mrd)
{
	const char *type;

	switch (mrd->type) {
	case UKPLAT_MEMRT_RESERVED:
		type = "rsvd";
		break;
	case UKPLAT_MEMRT_KERNEL:
		type = "krnl";
		break;
	case UKPLAT_MEMRT_INITRD:
		type = "ramd";
		break;
	case UKPLAT_MEMRT_CMDLINE:
		type = "cmdl";
		break;
	case UKPLAT_MEMRT_DEVICETREE:
		type = "dtb ";
		break;
	case UKPLAT_MEMRT_STACK:
		type = "stck";
		break;
	default:
		type = "";
		break;
	}

	uk_pr_debug(" %012lx-%012lx %012lx %c%c%c %016lx %s %s\n",
		   mrd->pbase, mrd->pbase + mrd->len, mrd->len,
		   (mrd->flags & UKPLAT_MEMRF_READ) ? 'r' : '-',
		   (mrd->flags & UKPLAT_MEMRF_WRITE) ? 'w' : '-',
		   (mrd->flags & UKPLAT_MEMRF_EXECUTE) ? 'x' : '-',
		   mrd->vbase,
		   type,
#if CONFIG_UKPLAT_MEMRNAME
		   mrd->name
#else /* !CONFIG_UKPLAT_MEMRNAME */
		   ""
#endif /* !CONFIG_UKPLAT_MEMRNAME */
		   );
}
#else  /* !CONFIG_LIBUKDEBUG_PRINTD */
static inline void
ukplat_memregion_print_desc(struct ukplat_memregion_desc *mrd __unused) { }
#endif /* !CONFIG_LIBUKDEBUG_PRINTD */

/**
 * Coalesces the memory regions of a given memory region descriptor list.
 *
 * @param list
 *   The list whose memory region descriptors to coalesce.
 */
void ukplat_memregion_list_coalesce(struct ukplat_memregion_list *list);

/**
 * Initializes the platform memory mappings which require an allocator. This
 * function must always be called after initializing a memory allocator and
 * before initializing the subsystems that require memory allocation. It is an
 * internal function common to all platforms.
 * @return 0 on success, < 0 otherwise
 */
int _ukplat_mem_mappings_init(void);

#endif /* __PLAT_CMN_MEMORY_H__ */
