/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <stddef.h>
#include <uk/plat/bootstrap.h>
#include <uk/plat/common/sections.h>
#include <uk/plat/memory.h>
#include <uk/reloc.h>

/* `lt_baddr` contains the link time absolute symbol value of
 * `_base_addr`, while `rt_baddr` will end up, through `get_rt_addr()`,
 * to contain the current, runtime, base address of the loaded image.
 * This works because `lt_baddr` will make the linker generate an
 * absolute 64-bit value relocation, that will be statically resolved
 * anyway  in the final binary.
 */
static unsigned long lt_baddr = __BASE_ADDR;
static unsigned long rt_baddr;

/* Use `get_rt_addr()` to obtain the runtime base address */
#if defined(__X86_64__)

/* For x86, this is resolved to a `%rip` relative access anyway */
static inline unsigned long get_rt_addr(unsigned long sym)
{
	return	sym;
}

#elif defined(__ARM_64__)

static inline unsigned long get_rt_addr(unsigned long sym)
{
	__asm__ __volatile__(
		"adrp	x0, _base_addr\n\t"
		"add	x0, x0, :lo12:_base_addr\n\t"
		"str	x0, %0\n\t"
		: "=m"(rt_baddr)
		:
		: "x0", "memory"
	);

	return (sym - lt_baddr) + rt_baddr;
}
#endif

#define ukreloc_crash(s)				ukplat_halt()

static inline struct ukreloc_hdr *get_ukreloc_hdr()
{
	struct ukreloc_hdr *ur_hdr;

	ur_hdr = (struct ukreloc_hdr *)get_rt_addr(__UKRELOC_START);
	if (unlikely(!ur_hdr) ||
	    unlikely(ur_hdr->signature != UKRELOC_SIGNATURE))
		return NULL;

	return ur_hdr;
}

void __used do_ukreloc(__paddr_t r_paddr, __vaddr_t r_vaddr)
{
	struct ukplat_memregion_desc *mrdp;
	unsigned long bkp_lt_baddr;
	struct ukreloc_hdr *ur_hdr;
	struct ukreloc *ur;
	__u64 val;

	/* Check .ukreloc signature */
	ur_hdr = get_ukreloc_hdr();
	if (unlikely(!ur_hdr))
		ukreloc_crash("Invalid UKRELOC signature");

	rt_baddr = get_rt_addr(__BASE_ADDR);
	if (r_paddr == 0)
		r_paddr = (__paddr_t)rt_baddr;
	if (r_vaddr == 0)
		r_vaddr = (__vaddr_t)rt_baddr;

	/* Since we may have been placed at a random physical address, adjust
	 * the initial memory region descriptors added through mkbootinfo.py
	 * since they contain the link-time addresses, relative to rt_baddr.
	 * Do this before anything else, since `lt_baddr`'s relocation has
	 * no been resolved yet and contains the link time address.
	 */
	ukplat_memregion_foreach(&mrdp, UKPLAT_MEMRT_KERNEL, 0, 0) {
		mrdp->pbase -= (__paddr_t)lt_baddr;
		mrdp->pbase += r_paddr;
		mrdp->vbase -= (__vaddr_t)lt_baddr;
		mrdp->vbase += r_vaddr;
	}

	/* Back up the original link time base address. We are going to lose
	 * it once we apply all relocations. Instead of impacting the runtime
	 * performance of the relocator by doing a check for every relocation
	 * address to be different from &lt_baddr, restore it at the end, when
	 * the relocator has finished its job.
	 */
	bkp_lt_baddr = lt_baddr;

	for (ur = ur_hdr->urs; ur->r_sz; ur++) {
		if (ur->flags & UKRELOC_FLAGS_PHYS_REL)
			val = (__u64)r_paddr + ur->r_addr;
		else
			val = (__u64)r_vaddr + ur->r_addr;

		apply_ukreloc(ur, val, (void *)rt_baddr);
	}

	/* Restore link time base address previously relocated to contain the
	 * runtime base address.
	 */
	lt_baddr = bkp_lt_baddr;
}
