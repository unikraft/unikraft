/*-
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2008 Yahoo!, Inc.
 * All rights reserved.
 * Written by: John Baldwin <jhb@FreeBSD.org>
 *			   Sharan Santhanam <sharan.santhanam@neclab.eu>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD$
 */
/**
 * Taken and modified from FreeBSD
 * Commit id: 4736ccfd9c34
 */

#ifndef UK__SGLIST_H__
#define UK__SGLIST_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <uk/config.h>
#include <stdint.h>
#include <uk/arch/types.h>
#include <uk/refcount.h>
#ifdef CONFIG_LIBUKALLOC
#include <uk/alloc.h>
#endif /* CONFIG_LIBUKALLOC */
#ifdef CONFIG_LIBUKNETDEV
#include <uk/netbuf.h>
#endif /* CONFIG_LIBUKNETDEV */

struct uk_sglist_seg {
	__phys_addr  ss_paddr; /* Physical address */
	size_t      ss_len;   /* Length of the buffer */
};

struct uk_sglist {
	struct uk_sglist_seg *sg_segs; /* Segment management */
	__atomic    sg_refs; /* Reference count for the sg list */
	uint16_t    sg_nseg; /* Number of segment in the sg list */
	uint16_t    sg_maxseg; /* Maximum number of segment in the sg list */
};

/**
 * Initialize the sg list.
 * @param sg
 *	A reference to sg list.
 * @param maxsegs
 *	The max nr of segments.
 * @param segs
 *	An array of segments.
 */
static inline void uk_sglist_init(struct uk_sglist *sg,
				uint16_t maxsegs, struct uk_sglist_seg *segs)
{
	sg->sg_segs = segs;
	sg->sg_nseg = 0;
	sg->sg_maxseg = maxsegs;
	uk_refcount_init(&sg->sg_refs, 1);
}

/**
 * Reset the sg list.
 * @param sg
 *	A reference to the sg list.
 */
static inline void uk_sglist_reset(struct uk_sglist *sg)
{
	sg->sg_nseg = 0;
}

/**
 * Determine the number of scatter/gather list elements needed to describe a
 * kernel virtual address range.
 * @param buf
 *	The virtual address of the scatter gather list.
 * @param len
 *	The size of the scatter gather list.
 *
 * @return
 *      - 0: The length was zero.
 *      - int: The number of segments
 */
int uk_sglist_count(void *buf, size_t len);

/**
 * Append the segments to describe a single kernel virtual address range to a
 * scatter gather list.
 *
 * @param sg
 *	A reference to the scatter gather list.
 * @param buf
 *	A reference to data buffer associated with the scatter gather list.
 * @return
 *	- EINVAL: Invalid sg list.
 *	- EFBIG : Insufficient segments.
 *	- 0:      Buffer was appended to the list.
 */
int uk_sglist_append(struct uk_sglist *sg, void *buf, size_t len);

/**
 * Append the subset of the sg list 'source' to sg list 'sg'.
 *
 * @param sg
 *	A reference to the update sg list.
 * @param source
 *	A reference to the source sg list.
 * @param offset
 *	The offset in the source from which th segment have to be copied.
 * @param length
 *	The length to be copied.
 * @return
 *	EINVAL: Invalid sg list or length parameter.
 *	EFBIG: Insufficient segments.
 *	0 : Successful in appending subset of the list.
 */
int uk_sglist_append_sglist(struct uk_sglist *sg,
				const struct uk_sglist *source,
				size_t offset, size_t length);

/**
 * Calculate the total length of the segments described in a sg list.
 * @param sg
 *	A reference to the sg list.
 * @return
 *	size_t : The total length in the segments.
 */
size_t uk_sglist_length(struct uk_sglist *sg);

/**
 * Append the scatter/gather list elements in 'second' to the
 * scatter/gather list 'first'.
 * @param first
 *	The first sg list
 * @param second
 *	The second sg list.
 * @return
 *	EFBIG: Insufficient space.
 *	0 : Successful joining of the list.
 */
int uk_sglist_join(struct uk_sglist *first, struct uk_sglist *second);

#ifdef CONFIG_LIBUKALLOC
/**
 * Allocate a scatter/gather list along with 'nsegs' segments.
 *
 * @param a
 *	The allocator to allocate memory for the scatter gather list.
 * @param nsegs
 *	The max number of segments.
 * @return
 *	- NULL:  Allocation failed.
 *	- (struct uk_sglist *): reference to scatter/gather list.
 */
struct uk_sglist *uk_sglist_alloc(struct uk_alloc *a, int nsegs);

/**
 * Free the scatter/gather list.
 * @param sg
 *	A reference to the scatter gather list.
 * @param a
 *	The allocator to allocate memory for the scatter gather list.
 */
void uk_sglist_free(struct uk_sglist *sg, struct uk_alloc *a);

/**
 * Allocate and populate a scatter/gather list to describe a single kernel
 * virtual address range.
 * @param a
 *	The allocator to used allocate the sg list.
 * @param buf
 *	The virtual address of the buffer.
 * @param len
 *	The length of the buffer.
 * @return
 *	- NULL: Failed to create the sg list.
 *	- (struct uk_sglist *) reference to scatter/gather list.
 */
struct uk_sglist *uk_sglist_build(struct uk_alloc *a, void *buf,
					size_t len);

/**
 * Clone a new copy of a scatter/gather list.
 * @param sg
 *	A reference to the sg list to be cloned.
 * @param a
 *	The allocator to use to allocate the sg list.
 * @return
 *	NULL: Failed to clone the list.
 *	(struct uk_sglist *): reference to the sg list.
 */
struct uk_sglist *uk_sglist_clone(struct uk_sglist *sg,
				struct uk_alloc *a);

/**
 * Split a scatter/gather list into two lists.  The scatter/gather
 * entries for the first 'length' bytes of the 'original' list are
 * stored in the '*head' list and are removed from 'original'.
 *
 * @param original
 *	A reference to the sg list.
 * @param head
 *	A reference to the head of the sg list. If NULL a new list will be
 *	allocated.  If '*head' is not NULL, it should point to an empty sglist.
 * @param a
 *	A reference to the allocator for maintaining the list.
 * @param length
 *	The length of the list.
 *
 * @return
 *	EINVAL: Invalid  sg list.
 *	ENOMEM: Allocation fails.
 *	EFBIG: Insufficient space.
 *	0: Successful split of the list.
 */
int uk_sglist_split(struct uk_sglist *original, struct uk_sglist **head,
				struct uk_alloc *a, size_t length);

/**
 * Generate a new scatter/gather list from a range of an existing
 * scatter/gather list.  The 'offset' and 'length' parameters specify
 * the logical range of the 'original' list to extract.
 *
 * @param original
 *	The reference to a sg list to be extracted.
 * @param slice
 *	If '*slice' is NULL, then a new list will be allocated.
 *	If '*slice' is not NULL, it should point to an empty sglist.  If it
 *	does not have enough room for the remaining space
 * @param a
 *	The allocator to be used.
 * @param offset
 *	The offset from which the sglist had to be spliced.
 * @param length
 *	The length of buffer in the original list.
 * @return
 *	EINVAL: Invalid sg list.
 *	ENOMEM: No memory
 *	EFBIG: Insufficient space
 *	0: Successful in splicing the list.
 */
int uk_sglist_slice(struct uk_sglist *original, struct uk_sglist **slice,
			struct uk_alloc *a, size_t offset, size_t length);
#endif /* CONFIG_LIBUKALLOC */

#ifdef CONFIG_LIBUKNETDEV
/**
 * The function create a scatter gather list from the netbuf
 * @param sg
 *	A reference to the scatter gather list.
 * @param m0
 *	A reference to the netbuf
 * @return
 *	0, on successful creation of the scatter gather list
 *	-EINVAL, Invalid sg list.
 */
int uk_sglist_append_netbuf(struct uk_sglist *sg, struct uk_netbuf *netbuf);
#endif /* CONFIG_LIBUKNET */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* UK__SGLIST_H__ */
