/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */
#ifndef __UK_HEAP_H__
#define __UK_HEAP_H__

#include <uk/config.h>
#include <uk/arch/types.h>
#include <uk/essentials.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The heap backs the pages it hands out with frames, and gives the frames of
 * the pages it takes back to the processes (lib/ukboot/boot.c). These calls
 * let the holder of an allocation do the same with part of it: a buffer with
 * room to grow into need not hold the room's frames until it does. Where the
 * heap does not do that (LIBUKBOOT_HEAP_RELEASE off), they allocate and free
 * as any allocation, and frames are neither given nor taken.
 */
#if CONFIG_LIBUKBOOT_HEAP_RELEASE

/**
 * Allocates @num_pages pages of the heap into @obj, with frames behind the
 * first @backed of them, as uk_heap_back_range() puts them there, and behind
 * the pages the allocator will write to; the rest are as the allocator left
 * them. -ENOMEM when the frames for the first @backed are not to be had,
 * -ENOSPC when no run of pages that long is free or the frames behind the
 * rest are not to be had, which a smaller allocation may yet find, else what
 * uk_vma_advise() returns.
 */
int uk_heap_palloc_backed(unsigned long num_pages, unsigned long backed,
			  void **obj);

/** Frees @num_pages pages at @obj from uk_heap_palloc_backed() */
void uk_heap_pfree(void *obj, unsigned long num_pages);

/**
 * Gives the frames behind the pages of [@start, @end) of the allocation of
 * @num_pages pages at @base back, as the heap does for a free block: all but
 * the pages heap_release() keeps, a small chunk whole and, in a larger one,
 * its first page and those a power of two from its start, where the
 * allocator writes the heads of the halves it splits it into. (Freed, an
 * allocation is the chunks of the powers of two of its size, largest first.
 * A head the allocator writes elsewhere, trimming a chunk to a smaller
 * allocation, is a fault the reserve serves, for a free block as for this.)
 * A page given back is taken again on a fault, in the kernel, which under
 * pressure may find no frame and cannot recover: the holder takes the frames
 * back with uk_heap_back_range() before it writes to a page. Returns the
 * lowest address given back, or @end when none was.
 */
__vaddr_t uk_heap_release_range(void *base, unsigned long num_pages,
				__vaddr_t start, __vaddr_t end);

/**
 * Takes frames for the heap's pages [@start, @end) that have none, leaving
 * the frames the kernel keeps for faults that cannot fail, as the heap does
 * when it hands pages out. -ENOMEM when they are not to be had, else what
 * uk_vma_advise() returns; the pages backed by then stay backed either way.
 * Where the frames are short, a page present but not writable (a platform's
 * copy-on-write page, after a restore) is -ENOMEM too: the frame the write
 * will take on its fault cannot be taken here, as the heap does not hand
 * such a block out either.
 */
int uk_heap_back_range(__vaddr_t start, __vaddr_t end);

#elif CONFIG_LIBUKALLOC /* !CONFIG_LIBUKBOOT_HEAP_RELEASE */
#include <uk/alloc.h>

static inline int uk_heap_palloc_backed(unsigned long num_pages,
					unsigned long backed __unused,
					void **obj)
{
	struct uk_alloc *a = uk_alloc_get_default();
	long room = uk_alloc_pmaxalloc(a);

	if (room >= 0 && num_pages > (unsigned long)room)
		return -ENOSPC;
	*obj = uk_palloc(a, num_pages);
	return *obj ? 0 : -ENOSPC;
}

static inline void uk_heap_pfree(void *obj, unsigned long num_pages)
{
	uk_pfree(uk_alloc_get_default(), obj, num_pages);
}

static inline __vaddr_t uk_heap_release_range(void *base __unused,
					      unsigned long num_pages __unused,
					      __vaddr_t start __unused,
					      __vaddr_t end)
{
	return end;
}

static inline int uk_heap_back_range(__vaddr_t start __unused,
				     __vaddr_t end __unused)
{
	return 0;
}

#endif /* CONFIG_LIBUKALLOC */

#ifdef __cplusplus
}
#endif

#endif /* __UK_HEAP_H__ */
