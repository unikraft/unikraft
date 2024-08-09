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

#include <uk/fallocbuddy.h>
#include <uk/config.h>
#include <uk/essentials.h>
#include <uk/arch/limits.h>
#include <uk/arch/paging.h>
#include <uk/assert.h>
#include <uk/bitops.h>
#include <uk/atomic.h>
#include <uk/list.h>
#include <uk/print.h>

#if CONFIG_LIBUKFALLOC_STATS
#include <uk/falloc/store.h>
#include <uk/spinlock.h>
#endif /* CONFIG_LIBUKFALLOC_STATS */

#include <string.h>
#include <errno.h>

/* In debugging mode, we always explicitly allocate the metadata so we can
 * set the memory blocks to a defined value when allocated without having them
 * overwritten by frame data
 */
#ifndef CONFIG_LIBUKFALLOCBUDDY_DEBUG
#define BFA_DIRECT_MAPPED		CONFIG_PAGING_HAVE_DIRECTMAP
#endif /* !CONFIG_LIBUKFALLOCBUDDY_DEBUG */

#define BFA_MAX_ALLOC_SHIFT		CONFIG_LIBUKFALLOCBUDDY_MAX_ALLOC_ORDER
#if BFA_MAX_ALLOC_SHIFT < PAGE_SHIFT
#error The max allocation size must be greater than or equal to the frame size
#elif BFA_MAX_ALLOC_SHIFT >= UK_BITS_PER_LONG
#error The max allocation size exceeds the address space size
#endif

#define BFA_LEVELS			(BFA_MAX_ALLOC_SHIFT - PAGE_SHIFT + 1)
/* Ensure that the levels fit into the free list map */
#if BFA_LEVELS >= 32
#error Too many levels. Reduce max allocation size.
#endif

#define BFA_Lx_SHIFT(lvl)		((lvl) + PAGE_SHIFT)
#define BFA_Lx_SIZE(lvl)		(1UL << BFA_Lx_SHIFT(lvl))
#define BFA_Lx_MASK(lvl)		(~(BFA_Lx_SIZE(lvl) - 1))

#define BFA_Lx_ALIGNED(addr, lvl)	(!((addr) & (BFA_Lx_SIZE(lvl) - 1)))
#define BFA_Lx_ALIGN_UP(addr, lvl)	ALIGN_UP(addr, BFA_Lx_SIZE(lvl))
#define BFA_Lx_ALIGN_DOWN(addr, lvl)	ALIGN_DOWN(addr, BFA_Lx_SIZE(lvl))

static inline unsigned int bfa_order_to_lvl(unsigned int order)
{
	UK_ASSERT(order >= PAGE_SHIFT);
	return order - PAGE_SHIFT;
}

/* Each zone is a contiguous range of physical frames. We use a hierarchical
 * bitmap to keep track of allocated memory within the zone. Each level
 * reserves one bit per 2^x frames, where x is the level with x in
 * [0..(BFA_LEVELS - 1)]. Due to the buddy system it is guaranteed that every
 * operation addresses a range of memory that is a power of two and aligned to
 * its size. We can thus set only the single bit in the corresponding level
 * that represents the entire range of memory in question to perform an
 * allocation or free a chunk of memory. At any point in time, there must be
 * set only one bit over all levels for a certain index. If a free operation
 * wants to release only a part of an allocation, the corresponding bits have
 * to be updated first by splitting the original allocation. That is, in the
 * level targeted by the free, all bits spanning the original allocation need
 * to be set and the bit for the original allocation in the higher level needs
 * to be cleared.
 */
typedef unsigned long bfa_zbit_word_t;
struct bfa_memblock;

struct bfa_zone {
	__paddr_t start;
	__paddr_t end;

	/* Hierarchical allocation bitmap (0 = free, 1 = allocated) */
	bfa_zbit_word_t *bitmap[BFA_LEVELS];

	/* Pointer to start of memory blocks. Points to first mapped frame in
	 * direct-mapped mode and uses a page size stride in this case.
	 */
	unsigned long nr_blocks;
	struct bfa_memblock *blocks;

	struct bfa_zone *next;

#ifdef CONFIG_LIBUKFALLOCBUDDY_STATS
	unsigned long nr_allocs[BFA_LEVELS][BFA_LEVELS]; /* from -> to */
	unsigned long nr_frees[BFA_LEVELS][BFA_LEVELS]; /* from -> to */
	unsigned long nr_zbit_splits[BFA_LEVELS][BFA_LEVELS]; /* from -> to */
	unsigned long nr_zbit_merges[BFA_LEVELS];
#endif /* CONFIG_LIBUKFALLOCBUDDY_STATS */
};

/* Each memblock represents a contiguous range of free memory within a zone
 * with a power of two size and aligned to its size. The level of the free list
 * in which the range is linked implicitly determines its size. However, for
 * faster merging, we also store the level explicitly. If we have direct-mapped
 * physical memory, we just use the free frames themselves for storing the
 * memblocks. In this case, we have enough space for additional information to
 * speed up lookups. We can compute the physical address directly from the
 * block's virtual address. If we do not have direct-mapped physical memory, we
 * allocate space for the memblocks in the metadata region and compute the
 * physical address from the zone's physical start address and the memblock's
 * position in the memblock array.
 *
 * Note: For non-direct-mapped physical memory the memblock size can be reduced
 * by using a 26-bit index into a zone's memblock array with a 3-bit zone index
 * as forward and backwards links, respectively. Then 6 bits are left for the
 * level and the whole memblock can be stored in 64 bits (i.e., 2 MiB per 1 GiB
 * of RAM).
 */
struct bfa_memblock {
	struct uk_list_head link;

#ifdef BFA_DIRECT_MAPPED
	struct bfa_zone *zone;
#endif /* BFA_DIRECT_MAPPED */

	unsigned int level;
};

/* The buddy allocator keeps track of all free memory across all zones in the
 * shared free lists so that a single check is enough to see if an allocation
 * of a certain size can directly be satisfied. If no element in the correct
 * free list is available, a memblock from the next higher level is taken and
 * split into its buddies. One of the buddies satisfies the request, whereas
 * the other's memblock is added to the free list. Blocks are recursively split
 * if needed.
 */
struct buddy_framealloc {
	struct uk_falloc fa;

	/* Circular singly-linked zone list. Head is moved to last used zone. */
	struct bfa_zone *zones;

	struct uk_list_head free_list[BFA_LEVELS];
	unsigned int free_list_map;
};

/* Forward declarations */
static inline int bfa_largest_level(__paddr_t paddr, __sz len);
static int bfa_do_free(struct buddy_framealloc *bfa, __paddr_t paddr, __sz len);

/* Memory block lookup */
#ifdef BFA_DIRECT_MAPPED
#define BFA_MB_LOOKUP(zone, mbidx)				\
	(struct bfa_memblock *)((__uptr)(zone)->blocks +	\
		((__uptr)(mbidx) << PAGE_SHIFT))
#define BFA_MB_IDX(zone, mb)					\
	(((__uptr)(mb) - (__uptr)(zone)->blocks) >> PAGE_SHIFT)
#else /* BFA_DIRECT_MAPPED */
#define BFA_MB_LOOKUP(zone, mbidx)	((zone)->blocks + (mbidx))
#define BFA_MB_IDX(zone, mb)		((mb) - (zone)->blocks)
#endif /* !BFA_DIRECT_MAPPED */

#define BFA_LAST_MB(zone)					\
	BFA_MB_LOOKUP(zone, (zone)->nr_blocks - 1)

/* Zone bitmap */
#define BITS_PER_ZBIT_WORD		(sizeof(bfa_zbit_word_t) << 3)

#define BFA_Lx_ZBIT_BITS(frames, lvl)				\
	(BFA_Lx_ALIGN_UP(frames << PAGE_SHIFT, lvl) >> BFA_Lx_SHIFT(lvl))

#define BFA_Lx_ZBIT_SIZE(frames, lvl)				\
	DIV_ROUND_UP(BFA_Lx_ZBIT_BITS(frames, lvl), 8)

#define BFA_Lx_ZBIT_WORDS(frames, lvl)				\
	DIV_ROUND_UP(BFA_Lx_ZBIT_SIZE(frames, lvl), sizeof(bfa_zbit_word_t))

#define BFA_Lx_ZBIT_ALIGN_DOWN(zone, paddr, lvl)		\
	(BFA_Lx_ALIGN_DOWN(paddr, lvl) - BFA_Lx_ALIGN_DOWN(zone->start, lvl))

#define BFA_Lx_ZBIT_IDX(zone, paddr, lvl)			\
	(BFA_Lx_ZBIT_ALIGN_DOWN(zone, paddr, lvl) >> BFA_Lx_SHIFT(lvl))

#define BFA_ZBIT_MASK(idx)					\
	(1UL << ((idx) & (BITS_PER_ZBIT_WORD - 1)))

#define BFA_Lx_ZBIT_WORD(zone, lvl, idx)			\
	((zone)->bitmap[lvl] + ((idx) / BITS_PER_ZBIT_WORD))

#define SET_BITS(val, mask)					\
	do {							\
		UK_ASSERT(((val) & (mask)) == 0);		\
		val |= mask;					\
	} while (0)

#define CLR_BITS(val, mask)					\
	do {							\
		UK_ASSERT(((val) & (mask)) == (mask));		\
		val &= ~(mask);					\
	} while (0)

static inline int bfa_zbit_scan(struct bfa_zone *zone, __paddr_t paddr,
				unsigned int *level, unsigned int end_level,
				int reset, int dir)
{
	unsigned int idx, lvl = *level;
	bfa_zbit_word_t *word, mask;

	UK_ASSERT(lvl < BFA_LEVELS);
	UK_ASSERT(end_level < BFA_LEVELS);
	UK_ASSERT(BFA_Lx_ALIGNED(paddr, lvl));
	UK_ASSERT(paddr >= zone->start && paddr < zone->end);
	UK_ASSERT(dir == -1 || dir == 1);

	/* First look if there is an allocation in the requested level. If not,
	 * move to the next smaller or larger level according to the search
	 * direction and check again.
	 */
	while (lvl <= end_level) {
		idx  = BFA_Lx_ZBIT_IDX(zone, paddr, lvl);
		mask = BFA_ZBIT_MASK(idx);
		word = BFA_Lx_ZBIT_WORD(zone, lvl, idx);

		if (*word & mask) {
			if (reset)
				CLR_BITS(*word, mask);

			*level = lvl;
			return 1;
		}

		lvl += dir; /* Will wrap for 0 and dir < 0 */
	}

	return 0;
}

static void bfa_zbit_fill(struct bfa_zone *zone, __paddr_t paddr,
			  __sz len, unsigned int level)
{
	unsigned int idx[2];
	bfa_zbit_word_t *word[2], mask[2];
	__paddr_t end;

	if (len == 0)
		return;

	UK_ASSERT(level < BFA_LEVELS);
	UK_ASSERT(BFA_Lx_ALIGNED(paddr, level));
	UK_ASSERT(BFA_Lx_ALIGNED(len, level));
	UK_ASSERT(paddr >= zone->start && paddr < zone->end);
	UK_ASSERT(paddr <= (__PADDR_MAX - len + 1));

	end = BFA_Lx_ALIGN_DOWN(paddr + len - 1, level);
	UK_ASSERT(end >= zone->start && end < zone->end);

	/* Compute the first and last position in the bitmap and then fill
	 * all bits starting from the first one up to (including) the last one
	 */

	/* Set all bits >= idx[0] */
	idx[0]  = BFA_Lx_ZBIT_IDX(zone, paddr, level);
	mask[0] = -BFA_ZBIT_MASK(idx[0]);
	word[0] = BFA_Lx_ZBIT_WORD(zone, level, idx[0]);

	/* Set all bits <= idx[1] */
	idx[1]  = BFA_Lx_ZBIT_IDX(zone, end, level);
	mask[1] = (BFA_ZBIT_MASK(idx[1]) << 1) - 1;
	word[1] = BFA_Lx_ZBIT_WORD(zone, level, idx[1]);

	if (word[0] == word[1]) {
		UK_ASSERT(mask[0] & mask[1]);

		SET_BITS(*word[0], mask[0] & mask[1]);
	} else {
		UK_ASSERT(word[0] < word[1]);
		SET_BITS(*word[0], mask[0]);

		while (++word[0] < word[1])
			SET_BITS(*word[0], ~0UL);

		SET_BITS(*word[0], mask[1]);
	}
}

static inline void bfa_zbit_alloc(struct bfa_zone *zone, __paddr_t paddr,
				  unsigned int level)
{
	unsigned int idx;
	bfa_zbit_word_t *word;

	UK_ASSERT(level < BFA_LEVELS);
	UK_ASSERT(BFA_Lx_ALIGNED(paddr, level));
	UK_ASSERT(paddr >= zone->start);
	UK_ASSERT(paddr <= (zone->end - BFA_Lx_SIZE(level)));

	/* We have to set the respective bit in the level of the allocation. We
	 * can implicitly assume that there are no other bits set for this
	 * address in the other levels because this allocation had an entry in
	 * a free list.
	 */
#ifdef CONFIG_LIBUKFALLOCBUDDY_DEBUG
	unsigned int lvl = 0;
	/* Ensure that all bits in the hierarchical bitmap for this
	 * allocation are cleared (i.e., not allocated)
	 */
	UK_ASSERT(!bfa_zbit_scan(zone, paddr, &lvl, BFA_LEVELS - 1, 0, 1));
#endif /* CONFIG_LIBUKFALLOCBUDDY_DEBUG */

	idx  = BFA_Lx_ZBIT_IDX(zone, paddr, level);
	word = BFA_Lx_ZBIT_WORD(zone, level, idx);
	SET_BITS(*word, BFA_ZBIT_MASK(idx));
}

static inline void bfa_zbit_alloc_range(struct bfa_zone *zone, __paddr_t paddr,
					__sz len)
{
	unsigned int lvl;
	__sz size;

	UK_ASSERT(len > 0);
	UK_ASSERT(BFA_Lx_ALIGNED(len, 0));
	UK_ASSERT(paddr >= zone->start);
	UK_ASSERT(paddr <= (zone->end - len));

	/* Mark the area as allocated */
	do {
		lvl = bfa_largest_level(paddr, len);
		UK_ASSERT(lvl < BFA_LEVELS);

		size = BFA_Lx_SIZE(lvl);

		bfa_zbit_alloc(zone, paddr, lvl);

		UK_ASSERT(len >= size);
		UK_ASSERT(paddr <= (zone->end - size));

		paddr += size;
		len -= size;
	} while (len > 0);
}

static int bfa_zbit_free(struct bfa_zone *zone, __paddr_t paddr,
			 unsigned int level)
{
	unsigned int lvl = level;
	unsigned int end_lvl;
	__paddr_t start, addr, end;
	int rc;

	UK_ASSERT(lvl < BFA_LEVELS);
	UK_ASSERT(BFA_Lx_ALIGNED(paddr, level));
	UK_ASSERT(paddr >= zone->start && paddr < zone->end);

	/* We can run into four cases for the original allocation(s) that
	 * make(s) up the physical memory range to free:
	 * 1) Same size -> this level set (no other level set)
	 * 2) Larger size -> this level not set, but higher level set
	 * 3) Smaller size -> this level not set, but lower level set
	 * 4) Not allocated -> this level not set, no higher or lower level set
	 */
	rc = bfa_zbit_scan(zone, paddr, &lvl, BFA_LEVELS - 1, 1, 1);
	if (rc) {
		if (lvl == level)
			return 0; /* same size (case 1) */

		/* At this point, we know that we ran into case 2 and we have to
		 * split the allocation into allocations of the requested size.
		 * We thus have to set all bits in the requested level that
		 * make up the memory region of the original allocation. We
		 * leave out the bit representing the freed area.
		 */
		UK_ASSERT(lvl > level);

#ifdef CONFIG_LIBUKFALLOCBUDDY_STATS
		zone->nr_zbit_splits[lvl][level]++;
#endif /* CONFIG_LIBUKFALLOCBUDDY_STATS */

		start = BFA_Lx_ALIGN_DOWN(paddr, lvl);
		addr = paddr + BFA_Lx_SIZE(level);
		end = BFA_Lx_ALIGN_UP(addr, lvl);

		UK_ASSERT((start < addr) && (addr < end));

		bfa_zbit_fill(zone, start, paddr - start, level);
		bfa_zbit_fill(zone, addr, end - addr, level);

		return 0;
	}

	/* We did not find an allocation of same or larger size. If we started
	 * to search from the lowest level, then there is no allocation.
	 */
	if (level == 0)
		return -ENOMEM;

	/* See if the allocation is made up from smaller allocations. This
	 * happens if the continuous range of physical memory has been
	 * allocated in small chunks but should be freed in one call; for
	 * example, because the pages mapping the physical memory have been
	 * merged into a large page and we are freeing the large page. In this
	 * case, all bits must be set that cover the requested physical memory
	 * range. Otherwise, we have a hole in the space (which is not allowed).
	 */
	end = paddr + BFA_Lx_SIZE(level);
	end_lvl = level - 1;

	lvl = end_lvl;
	for (addr = paddr; addr < end; addr += BFA_Lx_SIZE(lvl)) {
		/* Scan current level and lower levels */
		rc = bfa_zbit_scan(zone, addr, &lvl, end_lvl, 1, -1);
		if (rc)
			continue;

		/* We did not find anything. Scan upwards */
		UK_ASSERT(lvl < end_lvl + 1);
		lvl++;

		rc = bfa_zbit_scan(zone, addr, &lvl, end_lvl, 1, 1);
		if (rc)
			continue;

		/* There is a hole here. Undo the changes by allocating all
		 * bits in this level up to but not including the current
		 * address
		 */
		UK_ASSERT(BFA_Lx_ALIGNED(addr - paddr, lvl - 1));
		bfa_zbit_fill(zone, paddr, addr - paddr, lvl - 1);

		return -ENOMEM;
	}

#ifdef CONFIG_LIBUKFALLOCBUDDY_STATS
	zone->nr_zbit_merges[level]++;
#endif /* CONFIG_LIBUKFALLOCBUDDY_STATS */

	return 0;
}

static inline __sz bfa_zone_metadata_size(unsigned long frames)
{
	__sz size = sizeof(struct bfa_zone);
	unsigned int lvl;

#ifndef BFA_DIRECT_MAPPED
	/* Reserve one memblock for every frame. With direct-mapped physical
	 * memory, we don't need this because we store the memory blocks in the
	 * free frames.
	 */
	size += frames * sizeof(struct bfa_memblock);
#endif /* !BFA_DIRECT_MAPPED */

	for (lvl = 0; lvl < BFA_LEVELS; ++lvl)
		size += BFA_Lx_ZBIT_WORDS(frames, lvl) *
				sizeof(bfa_zbit_word_t);

	return size;
}

static struct bfa_zone *bfa_zone_init(void *buffer, __paddr_t start, __sz len,
				      __vaddr_t dm_off __maybe_unused)
{
	struct bfa_zone *zn = (struct bfa_zone *)buffer;
	unsigned long frames;
	__sz bm_words;
	unsigned int lvl = 0;

	UK_ASSERT(len > 0);
	UK_ASSERT(BFA_Lx_ALIGNED(len, 0));
	frames = len >> PAGE_SHIFT;

	UK_ASSERT(BFA_Lx_ALIGNED(start, 0));
	UK_ASSERT(start <= __PADDR_MAX - len);
#ifdef CONFIG_PAGING
	UK_ASSERT(ukarch_paddr_range_isvalid(start, len));
#endif /* CONFIG_PAGING */
	zn->start = start;
	zn->end = start + len;

	zn->next = __NULL;

	zn->nr_blocks = frames;
#ifdef BFA_DIRECT_MAPPED
	UK_ASSERT(dm_off != __VADDR_INV);
	zn->blocks = (struct bfa_memblock *)dm_off;
	zn->bitmap[0] = (bfa_zbit_word_t *)(zn + 1);
#else /* BFA_DIRECT_MAPPED */
	zn->blocks = (struct bfa_memblock *)(zn + 1);

#ifdef CONFIG_LIBUKFALLOCBUDDY_DEBUG
	memset(zn->blocks, 0xCD, sizeof(struct bfa_memblock) * zn->nr_blocks);
#endif /* CONFIG_LIBUKFALLOCBUDDY_DEBUG */

	zn->bitmap[0] = (bfa_zbit_word_t *)(zn->blocks + zn->nr_blocks);
#endif /* !BFA_DIRECT_MAPPED */

	/* Clear the bitmaps */
	do {
		bm_words = BFA_Lx_ZBIT_WORDS(frames, lvl);
		memset(zn->bitmap[lvl], 0, bm_words * sizeof(bfa_zbit_word_t));

		if (++lvl == BFA_LEVELS)
			break;

		zn->bitmap[lvl] = zn->bitmap[lvl - 1] + bm_words;
	} while (1);

	return zn;
}

static inline void bfa_zone_add(struct buddy_framealloc *bfa,
				struct bfa_zone *zone)
{
	struct bfa_zone *tail;

	UK_ASSERT(!zone->next);

	if (bfa->zones) {
		tail = bfa->zones;
		while (tail->next != bfa->zones)
			tail = tail->next;

		zone->next = bfa->zones;
		tail->next = zone;
	} else {
		zone->next = zone;
		bfa->zones = zone;
	}
}

#ifdef BFA_DIRECT_MAPPED
#define bfa_mb_to_zone(bfa, mb)		((mb)->zone)
#else /* BFA_DIRECT_MAPPED */
static inline struct bfa_zone *bfa_mb_to_zone(struct buddy_framealloc *bfa,
					      struct bfa_memblock *mb)
{
	struct bfa_zone *zone, *start = bfa->zones;

	/* We assume to have only few zones and can thus do a linear search.
	 * As the memory blocks are internal, we can safely assume that we
	 * will find the zone to which the block belongs in any case.
	 * Furthermore, we know we always have at least one zone as we
	 * obviously have a memory block.
	 */
	UK_ASSERT(start);

	zone = start;
	do {
		if ((mb >= zone->blocks) && (mb <= BFA_LAST_MB(zone)))
			break;

		zone = zone->next;
		UK_ASSERT(zone != start);
	} while (1);

	bfa->zones = zone;
	return zone;
}
#endif /* !BFA_DIRECT_MAPPED */

static inline __paddr_t bfa_mb_to_paddr(struct bfa_zone *zone,
					struct bfa_memblock *mb)
{
	__paddr_t paddr;

	UK_ASSERT(mb >= zone->blocks && mb <= BFA_LAST_MB(zone));

	paddr = (BFA_MB_IDX(zone, mb) << PAGE_SHIFT) + zone->start;
	UK_ASSERT(paddr < zone->end);

	return paddr;
}

static inline struct bfa_memblock *bfa_paddr_to_mb(struct bfa_zone *zone,
						   __paddr_t paddr)
{
	struct bfa_memblock *mb;

	UK_ASSERT(paddr >= zone->start && paddr < zone->end);

	mb = BFA_MB_LOOKUP(zone, ((paddr - zone->start) >> PAGE_SHIFT));
	UK_ASSERT(mb <= BFA_LAST_MB(zone));

	return mb;
}

static inline struct bfa_zone *bfa_paddr_to_zone(struct buddy_framealloc *bfa,
						 __paddr_t paddr)
{
	struct bfa_zone *zone, *start = bfa->zones;

	/* We assume to have only few zones and can thus do a linear search
	 * starting at the zone that we used the last time. However, we might
	 * not have any zones yet or the physical address is in no zone, so we
	 * must be careful.
	 */
	if (!start)
		return __NULL;

	zone = start;
	do {
		if ((paddr >= zone->start) && (paddr < zone->end)) {
			bfa->zones = zone;
			return zone;
		}

		zone = zone->next;
	} while (zone != start);

	return __NULL;
}

static inline void bfa_fl_add_tail(struct buddy_framealloc *bfa,
				   struct bfa_memblock *mb)
{
	uk_list_add_tail(&mb->link, &bfa->free_list[mb->level]);
	bfa->free_list_map |= (1 << mb->level);

	UK_ASSERT(mb->level < BFA_LEVELS);
	bfa->fa.free_memory += BFA_Lx_SIZE(mb->level);
#ifdef CONFIG_LIBUKFALLOC_STATS
	uk_spin_lock(&uk_falloc_stats_global_lock);
	uk_falloc_stats_global.free_memory += BFA_Lx_SIZE(mb->level);
	uk_spin_unlock(&uk_falloc_stats_global_lock);
#endif /* CONFIG_LIBUKFALLOC_STATS */
}

static inline void bfa_fl_add(struct buddy_framealloc *bfa,
			      struct bfa_memblock *mb)
{
	uk_list_add(&mb->link, &bfa->free_list[mb->level]);
	bfa->free_list_map |= (1 << mb->level);

	UK_ASSERT(mb->level < BFA_LEVELS);
	bfa->fa.free_memory += BFA_Lx_SIZE(mb->level);
#ifdef CONFIG_LIBUKFALLOC_STATS
	uk_spin_lock(&uk_falloc_stats_global_lock);
	uk_falloc_stats_global.free_memory += BFA_Lx_SIZE(mb->level);
	uk_spin_unlock(&uk_falloc_stats_global_lock);
#endif /* CONFIG_LIBUKFALLOC_STATS */
}

static inline void bfa_fl_del(struct buddy_framealloc *bfa,
			      struct bfa_memblock *mb)
{
	uk_list_del(&mb->link);
	if (uk_list_empty(&bfa->free_list[mb->level]))
		bfa->free_list_map ^= (1 << mb->level);

	UK_ASSERT(mb->level < BFA_LEVELS);
	UK_ASSERT(bfa->fa.free_memory >= BFA_Lx_SIZE(mb->level));
	bfa->fa.free_memory -= BFA_Lx_SIZE(mb->level);
#ifdef CONFIG_LIBUKFALLOC_STATS
	uk_spin_lock(&uk_falloc_stats_global_lock);
	uk_falloc_stats_global.free_memory -= BFA_Lx_SIZE(mb->level);
	uk_spin_unlock(&uk_falloc_stats_global_lock);
#endif /* CONFIG_LIBUKFALLOC_STATS */

#ifdef CONFIG_LIBUKFALLOCBUDDY_DEBUG
	memset(mb, 0xCD, sizeof(struct bfa_memblock));
#endif /* CONFIG_LIBUKFALLOCBUDDY_DEBUG */
}

static inline struct bfa_memblock *bfa_fl_pop_mb(struct buddy_framealloc *bfa,
						 unsigned int *level,
						 struct bfa_zone **zone)
{
	unsigned int map, lvl = *level;
	struct bfa_memblock *mb;

	UK_ASSERT(lvl < BFA_LEVELS);

	/* Mask out all free lists that are too small */
	map = bfa->free_list_map & -(1 << lvl);
	if (map == 0)
		return __NULL;

	// TODO: This should call a generic
	/* Find smallest free list that is not empty */
	lvl = uk_ffs(map);
	UK_ASSERT(lvl < BFA_LEVELS);

	mb = uk_list_first_entry(&bfa->free_list[lvl],
				 struct bfa_memblock, link);

	UK_ASSERT(mb);
	UK_ASSERT(mb->level == lvl);

	*level = lvl;
	*zone = bfa_mb_to_zone(bfa, mb);

	bfa_fl_del(bfa, mb);

	return mb;
}

static struct bfa_memblock *
bfa_fl_find_mb_in_zone(struct buddy_framealloc *bfa, struct bfa_zone *zone,
		       __paddr_t paddr, unsigned int *level)
{
	unsigned int map, lvl = *level;
	struct bfa_memblock *mb;
	__paddr_t mb_paddr;
	__sz size;

	UK_ASSERT(lvl < BFA_LEVELS);
	UK_ASSERT((paddr >= zone->start) && (paddr < zone->end));

	/* This function searches for the memory block that contains a specific
	 * physical address in the given zone. We search from large to small
	 * free lists to increase the chance of finding the address fast.
	 */

	/* Mask out all free lists that are too small */
	map = bfa->free_list_map & -(1 << lvl);

	while (map) {
		/* Find largest free list that is not empty */
		lvl = uk_fls(map);
		UK_ASSERT(lvl < BFA_LEVELS);

		size = BFA_Lx_SIZE(lvl);

		/* Do a linear search in the free list */
		uk_list_for_each_entry(mb, &bfa->free_list[lvl], link) {
			UK_ASSERT(mb->level == lvl);

			if (bfa_mb_to_zone(bfa, mb) != zone)
				continue;

			mb_paddr = bfa_mb_to_paddr(zone, mb);

			if (paddr >= mb_paddr &&
			    paddr < mb_paddr + size) {
				/* Found ! */

				*level = lvl;

				bfa_fl_del(bfa, mb);
				return mb;
			}
		}

		/* Unset bit in map to go to next free list */
		map ^= (1 << lvl);
	}

	return __NULL;
}

static inline struct bfa_memblock *
bfa_fl_find_mb_in_range(struct buddy_framealloc *bfa, unsigned int *level,
			struct bfa_zone **zone, __paddr_t *paddr, __paddr_t min,
			__paddr_t max)
{
	unsigned int map, to_lvl = *level;
	unsigned int lvl = to_lvl;
	struct bfa_memblock *mb;
	struct bfa_zone *zn;
	__paddr_t mb_paddr, mb_end_paddr;
	__sz to_size __maybe_unused = BFA_Lx_SIZE(to_lvl);
	__sz size;

	UK_ASSERT(to_lvl < BFA_LEVELS);

	/* This function searches for a memory block that can hold at least
	 * to_size bytes, where the to_size bytes (aligned to itself) are in
	 * the permissable address range. We can have the following cases:
	 *
	 * A.1+2) mb completely outside (FAIL)
	 *                    mb              mb_end
	 *      ^          ^   |    |    |    |       ^       ^
	 *      ^          ^        ^                 ^       ^
	 *      min      max        mb + 1 * to_size  min   max
	 *
	 * B.1+2) Overlap, but not large enough (FAIL)
	 *      ^              | ^  |    |  ^ |               ^
	 *      ^                ^          ^                 ^
	 *      min            max          min             max
	 * B.3)
	 *                     |  ^ |   ^|    |
	 *                        ^     ^
	 *                        min max
	 *
	 * C.1+2) Overlap, large enough (SUCCESS)
	 *      ^              |    | ^ ^|    |               ^
	 *      ^                     ^ ^                     ^
	 *      min                 max min                 max
	 * C.3)
	 *                     | ^  |    |  ^ |
	 *                       ^          ^
	 *                       min      max
	 * C.4)
	 *      ^              |    |    |    |               ^
	 *      ^                                             ^
	 *      min                                         max
	 */

	/* Let min and max fall to the next valid multiples of requested size */
	min = BFA_Lx_ALIGN_UP(min, to_lvl);
	max = BFA_Lx_ALIGN_DOWN(max, to_lvl);

	UK_ASSERT(min <= max);
	if (unlikely(max == min)) /* Case B.3, irrespective of overlap */
		return __NULL;

	UK_ASSERT(max - min >= to_size);
	UK_ASSERT(BFA_Lx_ALIGNED(max - min, to_lvl));

	/* Mask out all free lists that are too small */
	map = bfa->free_list_map & -(1 << to_lvl);

	while (map) {
		/* Find smallest free list that is not empty */
		lvl = uk_ffs(map);
		UK_ASSERT(lvl < BFA_LEVELS);

		size = BFA_Lx_SIZE(lvl);

		/* Do a linear search in the free list */
		uk_list_for_each_entry(mb, &bfa->free_list[lvl], link) {
			UK_ASSERT(mb->level == lvl);

			zn = bfa_mb_to_zone(bfa, mb);

			mb_paddr = bfa_mb_to_paddr(zn, mb);
			mb_end_paddr = mb_paddr + size;

			/* Since lvl is at least to_lvl, we know that the
			 * memory block is large enough. But we must ensure
			 * that we are in one of a Case C.X.
			 */
			if ((max <= mb_paddr) || (min >= mb_end_paddr))
				continue; /* Cases A.1+2 and B.1+2 */

			UK_ASSERT(max - mb_paddr >= to_size);
			UK_ASSERT(BFA_Lx_ALIGNED(max - mb_paddr, to_lvl));
			UK_ASSERT(mb_end_paddr - min >= to_size);
			UK_ASSERT(BFA_Lx_ALIGNED(mb_end_paddr - min, to_lvl));

			*level = lvl;
			*zone = zn;
			*paddr = MAX(mb_paddr, min);

			UK_ASSERT(BFA_Lx_ALIGNED(*paddr, to_lvl));

			bfa_fl_del(bfa, mb);

			return mb;
		}

		/* Unset bit in map to go to next free list */
		map ^= (1 << lvl);
	}

	return __NULL;
}

static inline int bfa_largest_level(__paddr_t paddr, __sz len)
{
	unsigned int lvl;

	/* This function computes the maximum level for which the address is
	 * aligned and the length is greater
	 */

	UK_ASSERT(BFA_Lx_ALIGNED(paddr, 0));
	UK_ASSERT((len > 0) && BFA_Lx_ALIGNED(len, 0));

	/* Limit bit scan to BFA_LEVELS - 1 at most */
	paddr |= BFA_Lx_SIZE(BFA_LEVELS - 1);

	/* We compute the maximum level according to the alignment */
	lvl = bfa_order_to_lvl(uk_ffsl(paddr));

	/* Now reduce the level until the length fits */
	while (BFA_Lx_SIZE(lvl) > len) {
		UK_ASSERT(lvl > 0);
		lvl--;
	}

	return lvl;
}

static inline int bfa_largest_level_strong(__paddr_t paddr, __sz len)
{
	__paddr_t value = paddr | len;

	/* This function computes the maximum level for which the address
	 * AND the length are aligned
	 */

	UK_ASSERT(BFA_Lx_ALIGNED(paddr, 0));
	UK_ASSERT((len > 0) && BFA_Lx_ALIGNED(len, 0));

	/* Limit bit scan to BFA_LEVELS - 1 at most */
	value |= BFA_Lx_SIZE(BFA_LEVELS - 1);

	return bfa_order_to_lvl(uk_ffsl(value));
}

static void bfa_fl_addmem(struct buddy_framealloc *bfa, struct bfa_zone *zone,
			  __paddr_t paddr, __sz len, int is_new)
{
	struct bfa_memblock *mb;
	unsigned int lvl;
	__sz size;

	UK_ASSERT(paddr >= zone->start && paddr <= zone->end);
	UK_ASSERT(paddr <= zone->end - len);

	while (len > 0) {
		lvl = bfa_largest_level(paddr, len);
		UK_ASSERT(lvl < BFA_LEVELS);

		size = BFA_Lx_SIZE(lvl);

		mb = bfa_paddr_to_mb(zone, paddr);
		mb->level = lvl;
#ifdef BFA_DIRECT_MAPPED
		mb->zone = zone;
#endif /* BFA_DIRECT_MAPPED */

		if (unlikely(is_new)) {
			uk_pr_debug("%"__PRIuptr": Adding physical memory "
				    "%"__PRIpaddr" - %"__PRIpaddr
				    " (fl %u, %"__PRIsz")\n",
				    (__uptr)bfa, paddr, paddr + size,
				    lvl, size);

			/* Add completely new memory at the end so that we are
			 * more likely to reuse areas we have already touched
			 */
			bfa_fl_add_tail(bfa, mb);
		} else
			bfa_fl_add(bfa, mb);

		UK_ASSERT(len >= size);
		UK_ASSERT(paddr <= (zone->end - size));

		len -= size;
		paddr += size;
	}
}

static int bfa_do_alloc(struct buddy_framealloc *bfa, __paddr_t paddr,
			__sz len)
{
	struct bfa_memblock *mb;
	struct bfa_zone *zone;
	__paddr_t mb_paddr, mb_end_paddr;
	__paddr_t end, start = paddr;
	unsigned int lvl, fill_lvl;
	__sz fill_len;
	int rc;

	UK_ASSERT(len > 0);
	UK_ASSERT(BFA_Lx_ALIGNED(len, 0));
	UK_ASSERT(BFA_Lx_ALIGNED(paddr, 0));
	UK_ASSERT(paddr <= (__PADDR_MAX - len));

	end = paddr + len;

	/* Find the zone that contains the physical address */
	zone = bfa_paddr_to_zone(bfa, paddr);
	if (unlikely(!zone))
		return -EFAULT;

	UK_ASSERT(paddr < zone->end);

	lvl = 0;

	/* Check if this address is allocated on any level */
	rc = bfa_zbit_scan(zone, paddr, &lvl, BFA_LEVELS - 1, 0, 1);
	if (unlikely(rc))
		return -ENOMEM;

	/* At this point we know that the physical address is free. However,
	 * we don't know how much memory is free, so we have to get the
	 * corresponding memory block that represents this free region and we
	 * can then look at its level. Also we need the memory block, because
	 * we have to remove it from the free list (adding smaller blocks after
	 * splitting). We could find the right memory block by iterating over
	 * the levels starting from the lowest, checking for each level if some
	 * area within the blocks's buddy is allocated. Then at this level we
	 * must have a memory block in the free list for the (lvl-aligned)
	 * current address. However, for checking if there is an allocation
	 * somewhere in the area covered by a buddy, we have to scan all bits
	 * on all levels for the buddy. This makes sense, when there are a lot
	 * of allocations. But it is rather slow if there are only a few
	 * allocations because then we are likely to reach a high level until
	 * we stop. In this case, it is faster to just do a search in the free
	 * lists. Since we expect exact address allocations to happen mostly
	 * early during startup to satisfy MMIO requirements, there probably
	 * won't be many allocations, which makes the search a better
	 * choice.
	 */
	UK_ASSERT(lvl == 0);

	mb = bfa_fl_find_mb_in_zone(bfa, zone, paddr, &lvl);
	UK_ASSERT(mb);

	UK_ASSERT(lvl < BFA_LEVELS);

	mb_paddr = BFA_Lx_ALIGN_DOWN(paddr, lvl);

	/* Re-add the remaining free space in front of the requested address */
	bfa_fl_addmem(bfa, zone, mb_paddr, paddr - mb_paddr, 0);

#ifdef CONFIG_LIBUKFALLOCBUDDY_DEBUG
	/* We claimed memory somewhere in a free block and not as usually at
	 * the beginning of the free block. So clear the block as it would be
	 * the case after bfa_fl_del()
	 */
	mb = bfa_paddr_to_mb(zone, paddr);
	memset(mb, 0xCD, sizeof(struct bfa_memblock));
#endif /* CONFIG_LIBUKFALLOCBUDDY_DEBUG */

	do {
		mb_end_paddr = mb_paddr + BFA_Lx_SIZE(lvl);

		UK_ASSERT(MIN(end, mb_end_paddr) > paddr);
		fill_len = MIN(end, mb_end_paddr) - paddr;
		fill_lvl = bfa_largest_level_strong(paddr, fill_len);

		/* Mark the region as allocated */
		bfa_zbit_fill(zone, paddr, fill_len, fill_lvl);

#ifdef CONFIG_LIBUKFALLOCBUDDY_STATS
		zone->nr_allocs[lvl][fill_lvl]++;
#endif /* CONFIG_LIBUKFALLOCBUDDY_STATS */

		if (end <= mb_end_paddr) {
			/* The remaining range fits into the allocated memory
			 * block. In this case, we also have to release
			 * any memory of the block that comes after the
			 * requested range (if any)
			 */
			bfa_fl_addmem(bfa, zone, end, mb_end_paddr - end, 0);

			break;
		}

		/* The requested memory range does not fit into the current
		 * memory block. So we know that it will also occupy at least
		 * some of the next block. Since we know that the current block
		 * ends here, we know that the next block starts right after it.
		 * In contrast to the beginning, we thus don't have to search
		 * for the memory block of the next address, but can be sure
		 * that IF the address is free, the corresponding block is the
		 * one following the current one. However, we might cross two
		 * consecutive zones.
		 */
		paddr = mb_end_paddr;

		zone = bfa_paddr_to_zone(bfa, paddr);
		if (unlikely(!zone)) {
			rc = -EFAULT;
			goto EXIT_FREE;
		}

		UK_ASSERT(paddr < zone->end);

		lvl = 0;

		/* Check if this address is allocated on any level */
		rc = bfa_zbit_scan(zone, paddr, &lvl, BFA_LEVELS - 1, 0, 1);
		if (unlikely(rc)) {
			rc = -ENOMEM;
			goto EXIT_FREE;
		}

		mb = bfa_paddr_to_mb(zone, paddr);
		UK_ASSERT(mb->level < BFA_LEVELS);
#ifdef BFA_DIRECT_MAPPED
		UK_ASSERT(mb->zone == zone);
#endif /* BFA_DIRECT_MAPPED */

		lvl = mb->level;

		bfa_fl_del(bfa, mb);

		mb_paddr = paddr;
	} while (1);

	return 0;

EXIT_FREE:
	/* Free the allocated memory again */
	UK_ASSERT(paddr > start);
	bfa_do_free(bfa, paddr, paddr - start);

	return rc;
}

static int bfa_do_alloc_any(struct buddy_framealloc *bfa, __paddr_t *paddr,
			    __sz len)
{
	struct bfa_memblock *mb;
	struct bfa_zone *zone;
	__paddr_t mb_paddr;
	__paddr_t mb_end_paddr;
	__paddr_t end;
	unsigned int lvl, to_lvl;

	UK_ASSERT(len > 0);
	UK_ASSERT(BFA_Lx_ALIGNED(len, 0));

	/* We can allocate power of two chunks only. So round up to next higher
	 * power of two. We use `len - 1` and not `len` to handle the case
	 * where len is already a power of two. If lvl is greater than
	 * BFA_LEVELS - 1 the request exceeds the maximum allocation size.
	 */
	to_lvl = bfa_order_to_lvl(uk_flsl(len - 1) + 1);
	if (unlikely(to_lvl >= BFA_LEVELS))
		return -ENOMEM;

	/* Find a memory block that can satisfy the request. If we cannot find
	 * a suitable block, there is not enough free contiguous memory.
	 */
	lvl = to_lvl;
	mb = bfa_fl_pop_mb(bfa, &lvl, &zone);
	if (unlikely(!mb)) {
		uk_pr_debug("%"__PRIuptr": Out of continuous physical memory"
			    " (req: %"__PRIsz" free: %"__PRIsz")\n",
			    (__uptr)bfa, len, bfa->fa.free_memory);

		return -ENOMEM;
	}

	UK_ASSERT(lvl < BFA_LEVELS);
	UK_ASSERT(zone);

	/* Get the address of the free block. */
	mb_paddr = bfa_mb_to_paddr(zone, mb);
	UK_ASSERT(BFA_Lx_ALIGNED(mb_paddr, lvl));

	UK_ASSERT(lvl >= to_lvl);

#ifdef CONFIG_LIBUKFALLOCBUDDY_STATS
	zone->nr_allocs[lvl][to_lvl]++;
#endif /* CONFIG_LIBUKFALLOCBUDDY_STATS */

	*paddr = mb_paddr;

	/* If the requested length is a power of two, we can easily split the
	 * area in halves as necessary and also mark the area in the bitmap
	 * at the respective level. Otherwise, we have work our way through
	 * the buddy zones.
	 */
	if (len == BFA_Lx_SIZE(to_lvl)) {
		/* If the block is too large we split it as often as necessary
		 * and add the unused parts to the free lists again.
		 */
		while (lvl > to_lvl) {
			lvl--; /* Go down one level */

			mb = bfa_paddr_to_mb(zone, mb_paddr + BFA_Lx_SIZE(lvl));
			mb->level = lvl;
#ifdef BFA_DIRECT_MAPPED
			mb->zone = zone;
#endif /* BFA_DIRECT_MAPPED */

			bfa_fl_add(bfa, mb);
		}

		UK_ASSERT(lvl == to_lvl);

		/* Mark the selected area as allocated in the bitmap */
		bfa_zbit_alloc(zone, mb_paddr, to_lvl);
	} else {
		mb_end_paddr = mb_paddr + BFA_Lx_SIZE(lvl);
		end = mb_paddr + len;

		/* Free the memory after the requested length. Note: a regular
		 * buddy system would waste this memory
		 */
		UK_ASSERT(mb_end_paddr > end);
		bfa_fl_addmem(bfa, zone, end, mb_end_paddr - end, 0);

		/* Mark the region as allocated */
		bfa_zbit_alloc_range(zone, mb_paddr, len);
	}

	return 0;
}

static int bfa_alloc(struct uk_falloc *fa, __paddr_t *paddr,
		     unsigned long frames, unsigned long flags __unused)
{
	struct buddy_framealloc *bfa = (struct buddy_framealloc *)fa;
	__sz len;

	UK_ASSERT(frames > 0);
	UK_ASSERT(frames <= (__SZ_MAX / PAGE_SIZE));

	len = frames * PAGE_SIZE;

	/* There is only FALLOC_FLAG_ALIGNED which we implicitly fulfill */
	UK_ASSERT((flags == 0) || (flags == FALLOC_FLAG_ALIGNED));

	/* If a physical address is given, the caller wants to allocate this
	 * exact memory range. Otherwise, just take a free one from the list.
	 */
	if (*paddr == __PADDR_ANY)
		return bfa_do_alloc_any(bfa, paddr, len);
	else
		return bfa_do_alloc(bfa, *paddr, len);
}

static int bfa_do_alloc_any_in_range(struct buddy_framealloc *bfa,
				     __paddr_t *paddr, __sz len, __paddr_t min,
				     __paddr_t max)
{
	struct bfa_memblock *mb;
	struct bfa_zone *zone;
	__paddr_t mb_paddr, mb_end_paddr;
	__paddr_t end;
	unsigned int lvl, to_lvl;

	UK_ASSERT(len > 0);
	UK_ASSERT(BFA_Lx_ALIGNED(len, 0));

	/* We can allocate power of two chunks only. So round up to next higher
	 * power of two. We use `len - 1` and not `len` to handle the case
	 * where len is already a power of two. If lvl is greater than
	 * BFA_LEVELS - 1 the request exceeds the maximum allocation size.
	 */
	to_lvl = bfa_order_to_lvl(uk_flsl(len - 1) + 1);
	if (unlikely(to_lvl >= BFA_LEVELS))
		return -ENOMEM;

	/* Find a memory block that can satisfy the request. If we cannot find
	 * a suitable block, there is not enough free contiguous memory in the
	 * permissable address range.
	 */
	lvl = to_lvl;
	mb = bfa_fl_find_mb_in_range(bfa, &lvl, &zone, paddr, min, max);
	if (unlikely(!mb))
		return -ENOMEM;

	UK_ASSERT(lvl < BFA_LEVELS);
	UK_ASSERT(zone);

	mb_paddr = bfa_mb_to_paddr(zone, mb);
	mb_end_paddr = mb_paddr + BFA_Lx_SIZE(lvl);

	end = *paddr + len;

	/* Re-add all unused memory of the block before the selected region */
	UK_ASSERT(*paddr >= mb_paddr);
	bfa_fl_addmem(bfa, zone, *paddr, *paddr - mb_paddr, 0);

	/* Re-add all unused memory of the block after the selected region */
	UK_ASSERT(end <= mb_end_paddr);
	bfa_fl_addmem(bfa, zone, end, mb_end_paddr - end, 0);

	bfa_zbit_alloc_range(zone, *paddr, len);

	return 0;
}

static int bfa_alloc_from_range(struct uk_falloc *fa, __paddr_t *paddr,
				unsigned long frames,
				unsigned long flags __maybe_unused,
				__paddr_t min, __paddr_t max)
{
	struct buddy_framealloc *bfa = (struct buddy_framealloc *)fa;
	__sz len;

	UK_ASSERT(frames > 0);
	UK_ASSERT(frames <= (__SZ_MAX / PAGE_SIZE));

	len = frames * PAGE_SIZE;

	/* There is only FALLOC_FLAG_ALIGNED which we implicitly fulfill */
	UK_ASSERT((flags == 0) || (flags == FALLOC_FLAG_ALIGNED));

	UK_ASSERT(min <= max);

	return bfa_do_alloc_any_in_range(bfa, paddr, len, min, max);
}

static struct bfa_memblock *bfa_try_merge(struct buddy_framealloc *bfa,
					  struct bfa_zone *zone,
					  __paddr_t paddr, unsigned int *level)
{
	struct bfa_memblock *mb, *bmb;
	unsigned int lvl = *level, tmp_lvl;
	__paddr_t buddy;

	mb = bfa_paddr_to_mb(zone, paddr);

#ifdef CONFIG_LIBUKFALLOCBUDDY_DEBUG
	UK_ASSERT(mb->level == 0xCDCDCDCD);
#endif /* CONFIG_LIBUKFALLOCBUDDY_DEBUG */

	if ((BFA_LEVELS < 2) || (lvl >= BFA_LEVELS - 2))
		return mb;

	/* Check if the memory block representing the buddy is allocated. If it
	 * is free, we can probe the block for the level at which the buddy is
	 * free and merge both buddies if they are on the same level. We repeat
	 * this operation until we have found the maximum mergeable level.
	 */
	do {
		/* We can use XOR to get the buddy's address */
		buddy = paddr ^ BFA_Lx_SIZE(lvl);

		UK_ASSERT(BFA_Lx_ALIGNED(paddr, lvl));
		UK_ASSERT(BFA_Lx_ALIGNED(buddy, lvl));

		if ((buddy < zone->start) || (buddy >= zone->end))
			break;

		tmp_lvl = lvl;
		if (bfa_zbit_scan(zone, buddy, &tmp_lvl, lvl, 0, -1))
			break;

		UK_ASSERT(tmp_lvl == lvl);
		bmb = bfa_paddr_to_mb(zone, buddy);

		UK_ASSERT(bmb->level <= lvl);
#ifdef BFA_DIRECT_MAPPED
		UK_ASSERT(bmb->zone == zone);
#endif /* BFA_DIRECT_MAPPED */

		if (bmb->level < lvl)
			break;

		/* We can merge. Remove the memory block from the free list and
		 * go up one level. We set the output memory block to the one
		 * representing the area at the next higher level (i.e., the
		 * merged buddies).
		 */
		bfa_fl_del(bfa, bmb);

		++lvl;

		paddr = BFA_Lx_ALIGN_DOWN(paddr, lvl);

		mb = bfa_paddr_to_mb(zone, paddr);
	} while (1);

	*level = lvl;
	return mb;
}

static int bfa_do_free(struct buddy_framealloc *bfa, __paddr_t paddr, __sz len)
{
	struct bfa_memblock *mb;
	struct bfa_zone *zone;
	unsigned int lvl;
	unsigned int saved_lvl __maybe_unused;
	__sz size;
	int rc;

	UK_ASSERT(len > 0);
	UK_ASSERT(BFA_Lx_ALIGNED(len, 0));
	UK_ASSERT(paddr <= (__PADDR_MAX - len));
#ifdef CONFIG_PAGING
	UK_ASSERT(ukarch_paddr_range_isvalid(paddr, len));
#endif /* CONFIG_PAGING */

	/* The memory area might cross multiple buddies and zones */
	do {
		/* Find the zone that contains the physical address */
		zone = bfa_paddr_to_zone(bfa, paddr);
		if (unlikely(!zone))
			return -EFAULT;

		/* Find the maximum level that we can use to free an area */
		lvl = bfa_largest_level(paddr, MIN(len, paddr - zone->end));
		UK_ASSERT(lvl < BFA_LEVELS);

		size = BFA_Lx_SIZE(lvl);

		UK_ASSERT(len >= size);
		UK_ASSERT(BFA_Lx_ALIGNED(paddr, lvl));
		UK_ASSERT(paddr <= (zone->end - size));

		/* Check if the range is actually allocated and free it */
		rc = bfa_zbit_free(zone, paddr, lvl);
		if (unlikely(rc))
			return rc;

		saved_lvl = lvl;

		/* Merge the block if possible and add it to the free list */
		mb = bfa_try_merge(bfa, zone, paddr, &lvl);
		mb->level = lvl;
#ifdef BFA_DIRECT_MAPPED
		mb->zone = zone;
#endif /* BFA_DIRECT_MAPPED */

		UK_ASSERT(lvl >= saved_lvl);

		bfa_fl_add(bfa, mb);

#ifdef CONFIG_LIBUKFALLOCBUDDY_STATS
		zone->nr_frees[saved_lvl][lvl]++;
#endif /* CONFIG_LIBUKFALLOCBUDDY_STATS */

		len -= size;
		paddr += size;
	} while (len > 0);

	return 0;
}

static int bfa_free(struct uk_falloc *fa, __paddr_t paddr,
		    unsigned long frames)
{
	struct buddy_framealloc *bfa = (struct buddy_framealloc *)fa;
	__sz len;

	if (unlikely(frames == 0))
		return 0;

	UK_ASSERT(frames <= (__SZ_MAX / PAGE_SIZE));

	len = frames * PAGE_SIZE;

	return bfa_do_free(bfa, paddr, len);
}

static int bfa_do_addmem(struct buddy_framealloc *bfa, void *metadata,
			 __paddr_t paddr, __sz len, __vaddr_t dm_off)
{
	struct bfa_zone *zone;

	UK_ASSERT(len > 0);
	UK_ASSERT(BFA_Lx_ALIGNED(len, 0));
	UK_ASSERT(paddr <= (__PADDR_MAX - len));
#ifdef CONFIG_PAGING
	UK_ASSERT(ukarch_paddr_range_isvalid(paddr, len));
#endif /* CONFIG_PAGING */

	zone = bfa_zone_init(metadata, paddr, len, dm_off);

	/* Add zone to zone list */
	bfa_zone_add(bfa, zone);
	bfa->fa.total_memory += len;

#ifdef CONFIG_LIBUKFALLOC_STATS
	uk_spin_lock(&uk_falloc_stats_global_lock);
	uk_falloc_stats_global.total_memory += len;
	uk_spin_unlock(&uk_falloc_stats_global_lock);
#endif /* CONFIG_LIBUKFALLOC_STATS */

	/* Add memory as new free memory */
	bfa_fl_addmem(bfa, zone, paddr, len, 1);

	return 0;
}

static int bfa_addmem(struct uk_falloc *fa, void *metadata, __paddr_t paddr,
		      unsigned long frames, __vaddr_t dm_off)
{
	struct buddy_framealloc *bfa = (struct buddy_framealloc *)fa;
	__sz len;

	if (unlikely(frames == 0))
		return 0;

	UK_ASSERT(frames <= (__SZ_MAX / PAGE_SIZE));

	len = frames * PAGE_SIZE;

	return bfa_do_addmem(bfa, metadata, paddr, len, dm_off);
}

int uk_fallocbuddy_init(struct uk_falloc *fa)
{
	struct buddy_framealloc *bfa = (struct buddy_framealloc *)fa;
	unsigned int i;
	int rc = 0;

	bfa->fa.falloc = bfa_alloc;
	bfa->fa.falloc_from_range = bfa_alloc_from_range;
	bfa->fa.ffree = bfa_free;
	bfa->fa.addmem = bfa_addmem;

	bfa->fa.free_memory = 0;
	bfa->fa.total_memory = 0;

	for (i = 0; i < BFA_LEVELS; i++)
		UK_INIT_LIST_HEAD(&bfa->free_list[i]);

	bfa->free_list_map = 0;

	bfa->zones = __NULL;

#ifdef CONFIG_LIBUKFALLOC_STATS
	rc = uk_falloc_init_stats(fa);
#endif /* CONFIG_LIBUKFALLOC_STATS */

	return rc;
}

__sz uk_fallocbuddy_size(void)
{
	return sizeof(struct buddy_framealloc);
}

__sz uk_fallocbuddy_metadata_size(unsigned long frames)
{
	return bfa_zone_metadata_size(frames);
}
