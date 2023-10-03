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
/* For EISA or MCA systems, the EBDA real-mode segmented pointer can be found
 * in the two-byte location 40:0Eh, right after the BIOS IVT.
 */
#define X86_EBDA_PTR			((0x40UL << 4) + 0x0EUL)
#define X86_EBDA_LEN			0x400UL
static inline unsigned long get_ebda_addr(void)
{
	__u16 *ebda_ptr = (__u16 *)X86_EBDA_PTR;

	return (*ebda_ptr << 4);
}

#define X86_HI_MEM_START		0xA0000UL
#define X86_DISPLAY_BUF_START		0xA0000UL
#define X86_DISPLAY_BUF_LEN		0x20000UL

#define X86_BIOS_XROM_START		0xC0000UL
#define X86_BIOS_XROM_LEN		0x20000UL

#define X86_BIOS_SYSROM_START		0xE0000UL
#define X86_BIOS_SYSROM_LEN		0x20000UL

/* WARNING! DO NOT CALL THIS FUNCTION AFTER PAGING INITIALIZATION! THIS RELIES
 * ON THE VERY FIRST PAGE (0 - 4KB) BEING MAPPED! ONLY CALL IF YOU KNOW WHAT YOU
 * YOU ARE DOING!
 *
 * Expected memory map for the first 1MiB w.r.t. PC-AT systems compatiblity
 * 0x00000 – 0x9FFFF 640KB Main memory, DOS compatible. May contain the
 *		     following memory subregions that we will not need after
 *		     boot. If the EBDA is defined here, then 1KB starting
 *		     at that address shall be treated as a reserved region
 *	0x00000 - 0x003FF Legacy BIOS IVT, segmented pointers to software
 *			  interrupt routines defined in the ROM BIOS
 *	0x0040E - 0x0040F Optional EBDA segmented pointer
 *	0x9FC00 - 0xA0000 Default 1KB of EBDA if not defined in previous region
 * 0xA0000 – 0xBFFFF 128KB Display buffer for video adapters and possible SMM
 *		     Shadow Memory
 * 0xC0000 – 0xDFFFF 128KB ROM BIOS for add-on cards (PCI XROMBARs)
 * 0xE0000 – 0xFFFFF 128KB System ROM BIOS
 */
static inline int
ukplat_memregion_list_insert_legacy_hi_mem(struct ukplat_memregion_list *list)
{
	unsigned long ebda_addr;
	int rc;

	/* According to ACPI spec for IA-PC systems, RSDP can be in:
	 * 1. the first 1KiB of EBDA
	 * 2. BIOS read-only memory space between 0xE0000 and 0xFFFFF
	 *
	 * According to MP spec for PC-AT systems, MP Floating Pointer Structure
	 * can be found in:
	 * 1. the first 1KiB of EBDA
	 * 2. if EBDA segment is not defined, the last 1KiB of PC-AT base ram:
	 * e.g., 639K-640K for systems with 640 KB of base memory or 511K-512K
	 * for systems with 512 KB of base memory (WE DO NOT TARGET THE LATTER)
	 * 3. BIOS read-only memory space between 0xE0000 and 0xFFFFF
	 */

	/* Check for EBDA explicit segmented pointer */
	ebda_addr = get_ebda_addr();

	/* If we do not have an EBDA pointer or it is invalid (beyond DOS
	 * compatible RAM), then resort to default: the last 1KB of DOS
	 * compatible RAM.
	 */
	if (!ebda_addr || ebda_addr > X86_HI_MEM_START - X86_EBDA_LEN)
		ebda_addr = X86_HI_MEM_START - X86_EBDA_LEN;

	/* Now insert EBDA memregion */
	rc = ukplat_memregion_list_insert(list,
			&(struct ukplat_memregion_desc){
				.vbase = ebda_addr,
				.pbase = ebda_addr,
				.len   = X86_EBDA_LEN,
				.type  = UKPLAT_MEMRT_LBIOS,
				.flags = UKPLAT_MEMRF_READ  |
					 UKPLAT_MEMRF_MAP,
			});
	if (unlikely(rc < 0))
		return rc;

	/* Note that we are mapping it as writable as well to cope with the
	 * potential existence of the VGA framebuffer/SMM shadow memory.
	 */
	rc = ukplat_memregion_list_insert(list,
			&(struct ukplat_memregion_desc){
				.vbase = X86_DISPLAY_BUF_START,
				.pbase = X86_DISPLAY_BUF_START,
				.len   = X86_DISPLAY_BUF_LEN,
				.type  = UKPLAT_MEMRT_RESERVED,
				.flags = UKPLAT_MEMRF_READ  |
					 UKPLAT_MEMRF_WRITE |
					 UKPLAT_MEMRF_MAP,
			});
	if (unlikely(rc < 0))
		return rc;

	/* Note that we are assigning UKPLAT_MEMRT_RESERVED to BIOS PCI ROM,
	 * instead of UKPLAT_MEMRT_LBIOS, as this is not needed at boot time,
	 * and it's rather more related to the XROM of add-on cards than BIOS
	 * itself. We usually have here the routines used by real-mode
	 * bootloaders invoked through the BIOS IVT. Although this may not be
	 * necessary anymore, we cannot assign UKPLAT_MEMRT_FREE either since
	 * some BIOSes do set this as a RO segment in the corresponding chipset
	 * registers, leaving this potentially unusable. Thus, just treat it
	 * as a memory hole.
	 */
	rc = ukplat_memregion_list_insert(list,
			&(struct ukplat_memregion_desc){
				.vbase = X86_BIOS_XROM_START,
				.pbase = X86_BIOS_XROM_START,
				.len   = X86_BIOS_XROM_LEN,
				.type  = UKPLAT_MEMRT_RESERVED,
				.flags = UKPLAT_MEMRF_READ  |
					 UKPLAT_MEMRF_MAP,
			});
	if (unlikely(rc < 0))
		return rc;

	/* Keep compatibility with other possible reports of reserved memory
	 * regions of this area and mark the BIOS System Memory as read-only.
	 */
	rc = ukplat_memregion_list_insert(list,
			&(struct ukplat_memregion_desc){
				.vbase = X86_BIOS_SYSROM_START,
				.pbase = X86_BIOS_SYSROM_START,
				.len   = X86_BIOS_SYSROM_LEN,
				.type  = UKPLAT_MEMRT_LBIOS,
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

/**
 * Coalesces the memory regions of a given memory region descriptor list.
 *
 * @param list
 *   The list whose memory region descriptors to coalesce.
 *
 * @return
 *   0 on success, < 0 otherwise.
 */
int ukplat_memregion_list_coalesce(struct ukplat_memregion_list *list);

/**
 * Initializes the platform memory mappings which require an allocator. This
 * function must always be called after initializing a memory allocator and
 * before initializing the subsystems that require memory allocation. It is an
 * internal function common to all platforms.
 * @return 0 on success, < 0 otherwise
 */
int _ukplat_mem_mappings_init(void);

#endif /* __PLAT_CMN_MEMORY_H__ */
