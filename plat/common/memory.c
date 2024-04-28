/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Costin Lupu <costin.lupu@cs.pub.ro>
 *          Sergiu Moga <sergiu.moga@protonmail.com>
 *
 * Copyright (c) 2018, NEC Europe Ltd., NEC Corporation. All rights reserved.
 * Copyright (c) 2023, University Politehnica of Bucharest. All rights reserved.
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
#include <stdbool.h>
#include <stddef.h>
#include <uk/plat/common/sections.h>
#include <uk/plat/common/bootinfo.h>
#include <uk/asm/limits.h>
#include <uk/alloc.h>

#if CONFIG_HAVE_PAGING
#include <uk/plat/paging.h>
#endif /* CONFIG_HAVE_PAGING */

extern struct ukplat_memregion_desc bpt_unmap_mrd;

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

void *ukplat_memregion_alloc(__sz size, int type, __u16 flags)
{
	struct ukplat_memregion_desc *mrd, alloc_mrd = {0};
	struct ukplat_bootinfo *bi;
	__paddr_t pstart, pend;
	__paddr_t ostart, olen;
	__sz desired_sz;
	int rc;

	/* Preserve desired size */
	desired_sz = size;
	size = ALIGN_UP(size, __PAGE_SIZE);
	ukplat_memregion_foreach(&mrd, UKPLAT_MEMRT_FREE, 0, 0) {
		UK_ASSERT_VALID_FREE_MRD(mrd);
		UK_ASSERT(mrd->pbase <= __U64_MAX - size);

		pstart = ALIGN_UP(mrd->pbase, __PAGE_SIZE);
		pend = pstart + size;

		if ((mrd->flags & UKPLAT_MEMRF_PERMS) !=
			    (UKPLAT_MEMRF_READ | UKPLAT_MEMRF_WRITE))
			return NULL;

		ostart = mrd->pbase;
		olen = mrd->len;

		/* If fragmenting this memory region leaves it with length 0,
		 * then simply overwrite and return it instead.
		 */
		if (olen - (pstart - ostart) == size) {
			mrd->pbase = pstart;
			mrd->vbase = pstart;
			mrd->pg_off = 0;
			mrd->len = desired_sz;
			mrd->pg_count = PAGE_COUNT(desired_sz);
			mrd->type = type;
			mrd->flags = flags;

			return (void *)pstart;
		}

		/* Adjust free region */
		mrd->len -= pend - mrd->pbase;
		mrd->pg_count = PAGE_COUNT(mrd->len);
		mrd->pbase = pend;

		mrd->vbase = (__vaddr_t)mrd->pbase;

		/* Insert allocated region */
		alloc_mrd.pbase = pstart;
		alloc_mrd.vbase = pstart;
		alloc_mrd.pg_off = 0;
		alloc_mrd.len = desired_sz;
		alloc_mrd.pg_count = PAGE_COUNT(desired_sz);
		alloc_mrd.type = type;
		alloc_mrd.flags = flags;

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

/* We want a criteria based on which we decide which memory region to keep,
 * split or discard when coalescing.
 * - UKPLAT_MEMRT_RESERVED is of highest priority since we should not touch it
 * - UKPLAT_MEMRT_FREE is of lowest priority since it is supposedly free
 * - the others are all allocated for Unikraft so they will have the same
 * priority
 */
#define MRD_PRIO_FREE			0
#define MRD_PRIO_KRNL_RSRC		1
#define MRD_PRIO_RSVD			2
static inline int get_mrd_prio(struct ukplat_memregion_desc *const m)
{
	switch (m->type) {
	case UKPLAT_MEMRT_FREE:
		return MRD_PRIO_FREE;
	case UKPLAT_MEMRT_INITRD:
	case UKPLAT_MEMRT_CMDLINE:
	case UKPLAT_MEMRT_STACK:
	case UKPLAT_MEMRT_DEVICETREE:
	case UKPLAT_MEMRT_KERNEL:
		return MRD_PRIO_KRNL_RSRC;
	case UKPLAT_MEMRT_RESERVED:
		return MRD_PRIO_RSVD;
	default:
		return -1;
	}
}

/* Memory region with lower priority must be adjusted in favor of the one
 * with higher priority, e.g. if left memory region is of lower priority but
 * contains the right memory region of higher priority, then split the left one
 * in two, by adjusting the current left one and inserting a new memory region
 * descriptor.
 * The start of the left memory region in the list is always less
 * or equal to the start of the right memory region in the list.
 */
static inline void overlapping_mrd_fixup(struct ukplat_memregion_list *list,
					 struct ukplat_memregion_desc *const ml,
					 struct ukplat_memregion_desc *const mr,
					 int ml_prio, int mr_prio,
					 __u32 lidx __unused, __u32 ridx)
{
	/* If left memory region is of higher priority */
	if (ml_prio > mr_prio) {
		/* If the right region is contained within the left region,
		 * drop it entirely
		 */
		/* This can only happen if mr is a free mrd or if it has
		 * the same type as ml
		 */
		UK_ASSERT(mr->type == UKPLAT_MEMRT_FREE ||
			  mr->type == ml->type);

		if (RANGE_CONTAIN(ml->pbase, ml->pg_count * PAGE_SIZE,
				  mr->pbase, mr->pg_count * PAGE_SIZE)) {
			mr->len = 0;
			mr->pg_count = 0;

		/* If the right region has a part of itself in the left region,
		 * drop that part of the right region only
		 */
		} else {
			mr->len -= ml->pbase + ml->len - mr->pbase;
			mr->pg_count = PAGE_COUNT(mr->len);
			mr->pbase = ml->pbase + ml->pg_count * PAGE_SIZE;
			mr->vbase = mr->pbase;
		}

	/* If left memory region is of lower priority */
	} else {
		/* This can only happen if ml is a free mrd or if it has the
		 * same type as mr
		 */
		UK_ASSERT(ml->type == UKPLAT_MEMRT_FREE ||
			  ml->type == mr->type);

		/* If the left memory region is contained within the right
		 * region, drop it entirely
		 */
		if (RANGE_CONTAIN(mr->pbase, mr->pg_count * PAGE_SIZE,
				  ml->pbase, ml->pg_count * PAGE_SIZE)) {
			ml->len = 0;
			ml->pg_count = 0;

		/* If the left region has a part of itself in the right region,
		 * drop that part of the left region only and split by creating
		 * a new one if the left region is larger than the right region.
		 */
		} else {
			__sz len = ml->pbase + ml->pg_count * PAGE_SIZE -
				   mr->pbase - mr->pg_count * PAGE_SIZE;
			__uptr base = PAGE_ALIGN_UP(mr->pbase + mr->len);

			if (RANGE_CONTAIN(ml->pbase, ml->pg_count * PAGE_SIZE,
					  mr->pbase, mr->pg_count * PAGE_SIZE))
				/* len here is basically ml_end - mr_end. Thus,
				 * len == 0 can happen only if mr is at the end
				 * of the ml and we therefore ignore the rest.
				 * If we ended up here then mr is actually
				 * somewhere in the middle of ml and we are
				 * inserting the mrd between mr_end and ml_end
				 */
				ukplat_memregion_list_insert_at_idx(list,
					&(struct ukplat_memregion_desc){
						.pbase = base,
						.vbase = base,
						.pg_off = 0,
						.len = len,
						.pg_count = PAGE_COUNT(len),
						.type = UKPLAT_MEMRT_FREE,
						.flags = ml->flags
					}, ridx + 1);

			/* Drop the fraction of ml that overlaps with mr */
			ml->len = (mr->pbase + mr->pg_off) -
				  (ml->pbase + ml->pg_off);
			ml->pg_count = PAGE_COUNT(ml->pg_off + ml->len);
		}
	}
}

/* Quick function to do potentially necessary swapping of two adjacent memory
 * region descriptors. Here just to modularize code because
 * ukplat_memregion_list_coalesce was quite large already.
 * Guarantees the start of the left memory region in the list is always less
 * or equal to the start of the right memory region in the list.
 */
static void ukplat_memregion_swap_if_unordered(struct ukplat_memregion_list *l,
					       __u32 l_idx, __u32 r_idx)
{
	struct ukplat_memregion_desc tmp, *m;

	m = l->mrds;
	if (m[l_idx].pbase > m[r_idx].pbase ||
	    (m[l_idx].pbase == m[r_idx].pbase &&
	     m[l_idx].pbase + m[l_idx].len > m[r_idx].pbase + m[r_idx].len)) {
		tmp = m[l_idx];
		m[l_idx] = m[r_idx];
		m[r_idx] = tmp;
	}
}

void ukplat_memregion_list_coalesce(struct ukplat_memregion_list *list)
{
	struct ukplat_memregion_desc *m, *ml, *mr;
	int ml_prio, mr_prio;
	__u32 i;

	UK_ASSERT(list);

	uk_pr_debug("Coalescing memory region list.\n");

	i = 0;
	m = list->mrds;
	while (i + 1 < list->count) {
		/* Make sure first that they are ordered. If not, swap them */
		ukplat_memregion_swap_if_unordered(list, i, i + 1);

		ml = &m[i];
		mr = &m[i + 1];

		uk_pr_debug("Currently coalescing the following memory "
			    "regions:\n");
		ukplat_memregion_print_desc(ml);
		ukplat_memregion_print_desc(mr);

		UK_ASSERT_VALID_MRD(ml);
		UK_ASSERT_VALID_MRD(mr);

		ml_prio = get_mrd_prio(ml);
		uk_pr_debug("Priority of left memory region: %d\n", ml_prio);
		UK_ASSERT(ml_prio >= 0);

		mr_prio = get_mrd_prio(mr);
		uk_pr_debug("Priority of right memory region: %d\n", mr_prio);
		UK_ASSERT(mr_prio >= 0);

		if (RANGE_OVERLAP(ml->pbase, ml->pg_count * PAGE_SIZE,
				  mr->pbase, mr->pg_count * PAGE_SIZE)) {
			/* If they are not of the same priority */
			if (ml_prio != mr_prio) {
				uk_pr_debug("mrd's of different priority "
					    "overlap!\n");

				/* At least one of the overlapping regions must
				 * be a free one. Otherwise, something wrong
				 * happened.
				 */
				UK_ASSERT(mr_prio == MRD_PRIO_FREE ||
					  ml_prio == MRD_PRIO_FREE);

				overlapping_mrd_fixup(list, ml, mr, ml_prio,
						      mr_prio, i, i + 1);

				/* Remove dropped regions */
				if (ml->len == 0) {
					uk_pr_debug("Deleting left mrd!\n");
					ukplat_memregion_list_delete(list, i);

				} else if (mr->len == 0) {
					uk_pr_debug("Deleting right mrd!\n");
					ukplat_memregion_list_delete(list,
								     i + 1);
				} else {
					i++;
				}

			/* If they have the same priority, merge them. If they
			 * are contained within each other, drop the contained
			 * one. Do not allow merging of kernel resources, as
			 * their resource page offset into the region is
			 * important!
			 */
			} else {
				/* Kernel regions must never overlap! */
				UK_ASSERT(ml_prio != MRD_PRIO_KRNL_RSRC);
				UK_ASSERT(mr_prio != MRD_PRIO_KRNL_RSRC);

				/* We do not allow overlaps of same priority
				 * and of different flags.
				 */
				UK_ASSERT(ml->flags == mr->flags);

				/* We do not allow overlaps of memory regions
				 * whose resource page offset into their region
				 * is not equal to 0. Regions don't that meet
				 * this condition are hand-inserted by us and
				 * should not overlap.
				 */
				UK_ASSERT(!ml->pg_off);
				UK_ASSERT(!mr->pg_off);
				UK_ASSERT(PAGE_ALIGNED(ml->pbase));
				UK_ASSERT(PAGE_ALIGNED(mr->pbase));

				/* If the left region is contained within the
				 * right region, drop it
				 */
				if (RANGE_CONTAIN(mr->pbase,
						  mr->pg_count * PAGE_SIZE,
						  ml->pbase,
						  ml->pg_count * PAGE_SIZE)) {
					uk_pr_debug("Deleting left mrd!\n");
					ukplat_memregion_list_delete(list, i);
					continue;

				/* If the right region is contained within the
				 * left region, drop it
				 */
				} else if (RANGE_CONTAIN(ml->pbase,
							 ml->pg_count *
							 PAGE_SIZE,
							 mr->pbase,
							 mr->pg_count *
							 PAGE_SIZE)) {
					uk_pr_debug("Deleting right mrd!\n");
					ukplat_memregion_list_delete(list,
								     i + 1);
					continue;
				}

				uk_pr_debug("Merging two overlapping mrds.\n");

				/* If they are not contained within each other,
				 * merge them.
				 */
				ml->len += mr->len;

				/* In case they overlap, delete duplicate
				 * overlapping region
				 */
				ml->len -= ml->pbase + ml->len - mr->pbase;
				ml->pg_count = PAGE_COUNT(ml->pg_off + ml->len);

				/* Delete the memory region we just merged into
				 * the previous region.
				 */
				ukplat_memregion_list_delete(list, i + 1);
			}

		/* If they do not overlap but they are contiguous and have the
		 * same flags and priority. Do not merge Kernel type memregions,
		 * as we have to preserve pg_off's and len's.
		 */
		} else if (ml->pbase + ml->len == mr->pbase &&
			   ml_prio == mr_prio && ml->flags == mr->flags &&
			   ml_prio != MRD_PRIO_KRNL_RSRC) {
			/* We do not allow overlaps of memory regions
			 * whose resource page offset into their region
			 * is not equal to 0. Regions don't that meet
			 * this condition are hand-inserted by us and
			 * should not overlap.
			 */
			UK_ASSERT(!ml->pg_off);
			UK_ASSERT(!mr->pg_off);
			UK_ASSERT(PAGE_ALIGNED(ml->pbase));
			UK_ASSERT(PAGE_ALIGNED(mr->pbase));

			uk_pr_debug("Merging two contiguous mrd's.\n");
			ml->len += mr->len;
			ml->pg_count = PAGE_COUNT(ml->len);
			ukplat_memregion_list_delete(list, i + 1);
		} else {
			uk_pr_debug("No adjustment for these mrd's.\n");
			i++;
		}
	}
}

int ukplat_memregion_count(void)
{
	struct ukplat_bootinfo *bi = ukplat_bootinfo_get();

	UK_ASSERT(bi);

	return (int)bi->mrds.count;
}

int ukplat_memregion_get(int i, struct ukplat_memregion_desc **mrd)
{
	struct ukplat_bootinfo *bi = ukplat_bootinfo_get();

	UK_ASSERT(bi);
	UK_ASSERT(i >= 0);

	if (unlikely((__u32)i >= bi->mrds.count))
		return -1;

	*mrd = &bi->mrds.mrds[i];
	return 0;
}

#ifdef CONFIG_HAVE_PAGING
int ukplat_mem_init(void)
{
	return ukplat_paging_init();
}
#else /* CONFIG_HAVE_PAGING */
int ukplat_mem_init(void)
{
	struct ukplat_bootinfo *bi = ukplat_bootinfo_get();
	struct ukplat_memregion_desc *mrdp;
	__vaddr_t unmap_end;
	int i;

	UK_ASSERT(bi);

	unmap_end = ALIGN_DOWN(bpt_unmap_mrd.vbase + bpt_unmap_mrd.len,
			       __PAGE_SIZE);
	for (i = (int)bi->mrds.count - 1; i >= 0; i--) {
		ukplat_memregion_get(i, &mrdp);
		UK_ASSERT_VALID_MRD(mrdp);

		if (mrdp->vbase >= unmap_end) {
			/* Region is outside the mapped area */
			uk_pr_info("Memory %012lx-%012lx outside mapped area\n",
				   mrdp->vbase, mrdp->vbase + mrdp->len);

			if (mrdp->type == UKPLAT_MEMRT_FREE)
				ukplat_memregion_list_delete(&bi->mrds, i);
		} else if (mrdp->vbase + mrdp->len > unmap_end) {
			/* Region overlaps with unmapped area */
			uk_pr_info("Memory %012lx-%012lx outside mapped area\n",
				   unmap_end,
				   mrdp->vbase + mrdp->pg_count * PAGE_SIZE);

			if (mrdp->type == UKPLAT_MEMRT_FREE) {
				mrdp->len -= (mrdp->vbase + mrdp->len) -
					     unmap_end;
				mrdp->pg_count = PAGE_COUNT(mrdp->len);
			}

			/* Since regions are non-overlapping and ordered, we
			 * can stop here, as the next region would be fully
			 * mapped anyways
			 */
			break;
		} else {
			/* Region is fully mapped */
			break;
		}
	}

	return 0;
}
#endif /* !CONFIG_HAVE_PAGING */
