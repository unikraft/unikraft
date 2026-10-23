/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <errno.h>

#include <uk/assert.h>
#include <uk/essentials.h>
#include <uk/fixmap.h>
#include <uk/pt.h>

#define FIXMAP_VADDR(s)							\
	(UK_PAL_FIXMAP_BASE + (__vaddr_t)(s) * UK_FIXMAP_SLOT_SIZE)

/* No locking: the fixmap is used at boot, on a single CPU */
static struct uk_pt *fixmap_pt;
static struct {
	__paddr_t blk;	/* The block the slot maps */
	int used;
} slots[UK_PAL_FIXMAP_NR_SLOTS];

/* Tests if a page table of the hierarchy covers the slot at vaddr */
static int slot_linked(struct uk_pt *pt, __vaddr_t vaddr)
{
	unsigned int lvl = UK_PAL_FIXMAP_LEVEL;

	return !uk_pt_walk(pt, vaddr, &lvl, __NULL, __NULL) &&
	       lvl == UK_PAL_FIXMAP_LEVEL;
}

int uk_fixmap_init(struct uk_pt *pt)
{
	UK_ASSERT(pt);

	/* ukpt does no TLB maintenance on a hierarchy it doesn't know is
	 * active
	 */
	UK_ASSERT(uk_pt_isactive(pt));

	if (unlikely(!slot_linked(pt, FIXMAP_VADDR(0)) ||
		     !slot_linked(pt,
				  FIXMAP_VADDR(UK_PAL_FIXMAP_NR_SLOTS - 1))))
		return -ENOENT;

	fixmap_pt = pt;

	return 0;
}

static int slot_free(unsigned int s)
{
	slots[s].used = 0;

	/* Invalidate and flush the TLB before the slot is used again */
	return uk_pt_unmap(fixmap_pt, FIXMAP_VADDR(s), UK_PAL_FIXMAP_LEVEL,
			   __NULL);
}

__vaddr_t uk_fixmap_map(unsigned int window, __paddr_t paddr, __sz len)
{
	__paddr_t blk = ALIGN_DOWN(paddr, UK_FIXMAP_SLOT_SIZE);
	__sz off = paddr - blk;
	unsigned int nr, end, k, s, i, nfree;
	int rc;

	UK_ASSERT(fixmap_pt);
	UK_ASSERT(window < UK_PAL_FIXMAP_NR_SLOTS);

	nr = uk_pal_fixmap_window_slots(window);
	if (unlikely(!len || !nr))
		return (__vaddr_t)-EINVAL;

	end = window + nr;
	UK_ASSERT(end <= UK_PAL_FIXMAP_NR_SLOTS);

	/* Number of blocks the range spans */
	k = DIV_ROUND_UP(off + len, UK_FIXMAP_SLOT_SIZE);

	/* Mapping a block twice in the same window is a bug of the caller */
	for (s = window; s < end; s++)
		if (slots[s].used && slots[s].blk >= blk &&
		    slots[s].blk < blk + k * UK_FIXMAP_SLOT_SIZE)
			return (__vaddr_t)-EEXIST;

	/* First fit: k consecutive free slots, whose addresses ascend */
	for (s = window, nfree = 0; s < end && nfree < k; s++)
		nfree = slots[s].used ? 0 : nfree + 1;
	if (unlikely(nfree < k))
		return (__vaddr_t)-ENOSPC;

	for (s -= k, i = 0; i < k; i++) {
		rc = uk_pt_map(fixmap_pt, FIXMAP_VADDR(s + i),
			       blk + i * UK_FIXMAP_SLOT_SIZE,
			       UK_PAL_FIXMAP_LEVEL,
			       uk_pal_fixmap_slot_attr(s + i),
			       UK_PT_TMPL_NONE, UK_PT_TMPL_LEVEL_NONE);
		if (unlikely(rc)) {
			while (i--)
				slot_free(s + i);
			return (__vaddr_t)rc;
		}

		slots[s + i].blk = blk + i * UK_FIXMAP_SLOT_SIZE;
		slots[s + i].used = 1;
	}

	return FIXMAP_VADDR(s) + off;
}

int uk_fixmap_unmap(__vaddr_t vaddr, __sz len)
{
	__vaddr_t va = ALIGN_DOWN(vaddr, UK_FIXMAP_SLOT_SIZE);
	unsigned int s, k, i;
	int rc;

	if (unlikely(!len || va < UK_PAL_FIXMAP_BASE))
		return -EINVAL;

	s = (va - UK_PAL_FIXMAP_BASE) / UK_FIXMAP_SLOT_SIZE;
	k = DIV_ROUND_UP(vaddr - va + len, UK_FIXMAP_SLOT_SIZE);
	if (unlikely(s + k > UK_PAL_FIXMAP_NR_SLOTS))
		return -EINVAL;

	for (i = 0; i < k; i++)
		if (unlikely(!slots[s + i].used))
			return -ENOENT;

	for (i = 0; i < k; i++) {
		rc = slot_free(s + i);
		if (unlikely(rc))
			return rc;
	}

	return 0;
}
