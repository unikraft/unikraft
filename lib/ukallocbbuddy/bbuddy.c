/* SPDX-License-Identifier: MIT */
/*
 * MIT License
 ****************************************************************************
 * (C) 2003 - Rolf Neugebauer - Intel Research Cambridge
 * (C) 2005 - Grzegorz Milos - Intel Research Cambridge
 * (C) 2017 - Simon Kuenzer - NEC Europe Ltd.
 * (C) 2025 - Unikraft GmbH and The Unikraft Authors
 ****************************************************************************
 *
 *        File: mm.c
 *      Author: Rolf Neugebauer (neugebar@dcs.gla.ac.uk)
 *     Changes: Grzegorz Milos
 *     Changes: Simon Kuenzer <simon.kuenzer@neclab.eu>
 *     Changes: Nour-eddine Taleb <contact@noureddine.xyz>
 *     Changes: Andrei Tatar <ttr@unikraft.io>
 *
 *        Date: Aug 2003, changes Aug 2005, changes Oct 2017, changes Dec 2022,
 *              changes Feb/Jul 2025
 *
 * Environment: Unikraft
 * Description: buddy page allocator from Xen.
 *
 ****************************************************************************
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <errno.h>

#include <uk/allocbbuddy.h>
#include <uk/alloc_impl.h>
#include <uk/arch/limits.h>
#include <uk/atomic.h>
#include <uk/bitops/bitscan.h>
#include <uk/print.h>
#include <uk/assert.h>
#include <uk/list.h>
#include <uk/page.h>

struct chunk_head {
	struct uk_hlist_node link;
	unsigned int page_order;
};

#define CHUNK_END(buf, ord) \
	((void *)(((char *)(buf)) + (1ULL << ((ord) + __PAGE_SHIFT))))

/* Linked lists of free chunks of different powers-of-two in size. */
#define FREELIST_SIZE ((sizeof(void *) << 3) - __PAGE_SHIFT)
#define FREELIST_EMPTY(_l) uk_hlist_empty((_l))
#define FREELIST_ALIGNED(ptr, lvl) \
	!((uintptr_t)(ptr) & ((1ULL << ((lvl) + __PAGE_SHIFT)) - 1))

/* keep a bitmap for each memory region separately */
struct uk_bbpalloc_memr {
	struct uk_bbpalloc_memr *next;
	unsigned long first_page;
	unsigned long nr_pages;
	unsigned long mm_alloc_bitmap_size;
	unsigned long *mm_alloc_bitmap;
};

struct uk_bbpalloc {
	unsigned long nr_free_pages;
	struct uk_hlist_head free_head[FREELIST_SIZE];
	struct uk_bbpalloc_memr *memr_head;
};

#if CONFIG_LIBUKALLOCBBUDDY_FREELIST_SANITY
/* Provide sanity checking of freelists, walking their length and checking
 * for consistency. Useful when suspecting memory corruption.
 */

#include <uk/arch/paging.h>
#define _FREESAN_NONCANON(x) ((x) && (~(uintptr_t)(x)))
#define _FREESAN_BAD_CHUNKPTR(x) \
	(((uintptr_t)x & (sizeof(void *) - 1)) || \
	_FREESAN_NONCANON((uintptr_t)(x) >> PAGE_Lx_SHIFT(PT_LEVELS - 1)))

#define _FREESAN_LOCFMT "\t@ %p (free_head[%zu](%p) + %zu): "

#define _FREESAN_HEAD(head, ord)					\
do {									\
	size_t off = 0;							\
	struct chunk_head *c;						\
									\
	uk_hlist_for_each_entry(c, (head), link) {			\
		if (_FREESAN_BAD_CHUNKPTR(c)) {				\
			uk_pr_err("Invalid chunk pointer" _FREESAN_LOCFMT "\n",\
				  c, (ord), (head), off);		\
			break;						\
		}							\
		if (!FREELIST_ALIGNED(c, (ord)))			\
			uk_pr_err("Unaligned chunk" _FREESAN_LOCFMT	\
				  "%p not aligned to %llx boundary\n",	\
				  c, (ord), (head), off, c, BBUDDY_LEN((ord)));\
		if (c->page_order != (ord))				\
			uk_pr_err("Bad page level" _FREESAN_LOCFMT	\
				  "got %u, expected %zu\n",		\
				  c, (ord), (head), off, c->page_order, (ord));\
		if (c->link.pprev && (*c->link.pprev)->next != &c->link) \
			uk_pr_err("Bad backlink" _FREESAN_LOCFMT	\
				  "got %p, expected %p\n",		\
				  c, (ord), (head), off,		\
				  (*c->link.pprev)->next, &c->link);	\
	}								\
} while (0)

#define freelist_sanitycheck(free_head)					\
for (size_t i = 0; i < FREELIST_SIZE; i++)				\
	_FREESAN_HEAD(&(free_head)[i], i)

#else /* !CONFIG_LIBUKALLOCBBUDDY_FREELIST_SANITY */

#define freelist_sanitycheck(x) do {} while (0)

#endif /* CONFIG_LIBUKALLOCBBUDDY_FREELIST_SANITY */

/*********************
 * ALLOCATION BITMAP
 *  One bit per page of memory. Bit set => page is allocated.
 *
 * Hint regarding bitwise arithmetic in map_{alloc,free}:
 *  -(1<<n)  sets all bits >= n.
 *  (1<<n)-1 sets all bits <  n.
 * Variable names in map_{alloc,free}:
 *  *_idx == Index into `mm_alloc_bitmap' array.
 *  *_off == Bit offset within an element of the `mm_alloc_bitmap' array.
 */
#define BITS_PER_BYTE       8
#define BYTES_PER_MAPWORD   (sizeof(unsigned long))
#define PAGES_PER_MAPWORD   (BYTES_PER_MAPWORD * BITS_PER_BYTE)

static inline struct uk_bbpalloc_memr *map_get_memr(struct uk_bbpalloc *b,
						    unsigned long page_va)
{
	struct uk_bbpalloc_memr *memr = NULL;

	/*
	 * Find bitmap of according memory region
	 * This is a linear search but it is expected that we have only a few
	 * of them. It should be just one region in most cases
	 */
	for (memr = b->memr_head; memr != NULL; memr = memr->next) {
		if ((page_va >= memr->first_page)
		    && (page_va < (memr->first_page +
		    (memr->nr_pages << __PAGE_SHIFT))))
			return memr;
	}

	/*
	 * No region found
	 */
	return NULL;
}

static inline unsigned long allocated_in_map(struct uk_bbpalloc *b,
				   unsigned long page_va)
{
	struct uk_bbpalloc_memr *memr = map_get_memr(b, page_va);
	unsigned long page_idx;
	unsigned long bm_idx, bm_off;

	/* treat pages outside of region as allocated */
	if (!memr)
		return 1;

	page_idx = (page_va - memr->first_page) >> __PAGE_SHIFT;
	bm_idx = page_idx / PAGES_PER_MAPWORD;
	bm_off = page_idx & (PAGES_PER_MAPWORD - 1);

	return ((memr)->mm_alloc_bitmap[bm_idx] & (1UL << bm_off));
}

static void map_alloc(struct uk_bbpalloc *b, uintptr_t first_page,
		      unsigned long nr_pages)
{
	struct uk_bbpalloc_memr *memr;
	unsigned long first_page_idx, end_page_idx;
	unsigned long start_off, end_off, curr_idx, end_idx;

	/*
	 * In case there was no memory region found, the allocator
	 * is in a really bad state. It means that the specified page
	 * region is not covered by our allocator.
	 */
	memr = map_get_memr(b, first_page);
	UK_ASSERT(memr != NULL);
	UK_ASSERT((first_page + (nr_pages << __PAGE_SHIFT))
		  <= (memr->first_page + (memr->nr_pages << __PAGE_SHIFT)));

	first_page -= memr->first_page;
	first_page_idx = first_page >> __PAGE_SHIFT;
	curr_idx = first_page_idx / PAGES_PER_MAPWORD;
	start_off = first_page_idx & (PAGES_PER_MAPWORD - 1);
	end_page_idx = first_page_idx + nr_pages;
	end_idx = end_page_idx / PAGES_PER_MAPWORD;
	end_off = end_page_idx & (PAGES_PER_MAPWORD - 1);

	if (curr_idx == end_idx) {
		memr->mm_alloc_bitmap[curr_idx] |=
		    ((1UL << end_off) - 1) & -(1UL << start_off);
	} else {
		memr->mm_alloc_bitmap[curr_idx] |= -(1UL << start_off);
		while (++curr_idx < end_idx)
			memr->mm_alloc_bitmap[curr_idx] = ~0UL;
		memr->mm_alloc_bitmap[curr_idx] |= (1UL << end_off) - 1;
	}

	b->nr_free_pages -= nr_pages;
}

static void map_free(struct uk_bbpalloc *b, uintptr_t first_page,
		     unsigned long nr_pages)
{
	struct uk_bbpalloc_memr *memr;
	unsigned long first_page_idx, end_page_idx;
	unsigned long start_off, end_off, curr_idx, end_idx;

	/*
	 * In case there was no memory region found, the allocator
	 * is in a really bad state. It means that the specified page
	 * region is not covered by our allocator.
	 */
	memr = map_get_memr(b, first_page);
	UK_ASSERT(memr != NULL);
	UK_ASSERT((first_page + (nr_pages << __PAGE_SHIFT))
		  <= (memr->first_page + (memr->nr_pages << __PAGE_SHIFT)));

	first_page -= memr->first_page;
	first_page_idx = first_page >> __PAGE_SHIFT;
	curr_idx = first_page_idx / PAGES_PER_MAPWORD;
	start_off = first_page_idx & (PAGES_PER_MAPWORD - 1);
	end_page_idx = first_page_idx + nr_pages;
	end_idx = end_page_idx / PAGES_PER_MAPWORD;
	end_off = end_page_idx & (PAGES_PER_MAPWORD - 1);

	if (curr_idx == end_idx) {
		memr->mm_alloc_bitmap[curr_idx] &=
		    -(1UL << end_off) | ((1UL << start_off) - 1);
	} else {
		memr->mm_alloc_bitmap[curr_idx] &= (1UL << start_off) - 1;
		while (++curr_idx != end_idx)
			memr->mm_alloc_bitmap[curr_idx] = 0;
		memr->mm_alloc_bitmap[curr_idx] &= -(1UL << end_off);
	}

	b->nr_free_pages += nr_pages;
}

/* Number of pages covered by page order `ord` */
#define BBUDDY_PAGES(ord) (1ULL << (ord))

/* Bit corresponding to page order `ord`; identical to size of chunk at `ord` */
#define BBUDDY_LEN(ord) (BBUDDY_PAGES((ord)) << __PAGE_SHIFT)

/* Address of buddy page of `ptr` at page order `ord` */
#define BBUDDY_BUDDY_ADDR(ptr, ord) \
	((void *)(((uintptr_t)(ptr)) ^ BBUDDY_LEN(ord)))

/* Non-zero if `ptr` would be the head buddy in page order `ord` */
#define BBUDDY_ISHEAD(ptr, ord) (!(((uintptr_t)(ptr)) & BBUDDY_LEN(ord)))

/* return log of the next power of two of passed number */
static inline unsigned long num_pages_to_order(unsigned long num_pages)
{
	UK_ASSERT(num_pages != 0);

	/* uk_mssbl has undefined behavior when called with zero */
	if (num_pages == 1)
		return 0;

	/* uk_mssbl(num_pages - 1) returns log of the previous power of two
	 * of num_pages. uk_mssbl is called with `num_pages - 1` and not
	 * `num_pages` to handle the case where num_pages is already a power
	 * of two.
	 */
	return uk_mssbl(num_pages - 1) + 1;
}

/* return the highest page order that `ptr` is aligned to, up to maxord */
static inline unsigned long ptr_order(void *ptr, unsigned long maxord)
{
	unsigned long v = (uintptr_t)ptr;

	UK_ASSERT(IS_ALIGNED(v, __PAGE_SIZE));
	v >>= __PAGE_SHIFT;
	if (!v)
		return maxord; /* page 0 is aligned to any order */
	return MIN(uk_lssbl(v), maxord);
}

/* return the highest power of two less than or equal to num_pages */
static inline unsigned long npages_order(unsigned long num_pages)
{
	UK_ASSERT(num_pages);
	return uk_mssbl(num_pages);
}

/*********************
 * BINARY BUDDY PAGE ALLOCATOR
 */

/**
 * INTERNAL. Trim chunk `ch` of page order `ord` down to `num_pages` pages.
 *
 * Page-order-sized chunks are trimmed off the end of `ch` and placed back into
 * their respective freelists as needed.
 */
static inline void bbuddy_trim(struct uk_bbpalloc *b, struct chunk_head *ch,
			       size_t ord, unsigned long num_pages)
{
	size_t spare_pages;
	size_t spare_ord;
	size_t spare_len;
	char *spare_end;
	struct chunk_head *spare_ch;

	UK_ASSERT(ch->page_order == ord);

	spare_pages = BBUDDY_PAGES(ord) - num_pages;
	spare_end = CHUNK_END(ch, ord);
	while (spare_pages) {
		spare_ord = npages_order(spare_pages);
		spare_len = BBUDDY_LEN(spare_ord);
		spare_ch = (struct chunk_head *)(spare_end - spare_len);

		UK_ASSERT(spare_ord < FREELIST_SIZE);
		UK_ASSERT(BBUDDY_PAGES(spare_ord) <= spare_pages);

		/* Populate chunk & link in */
		spare_ch->page_order = spare_ord;
		uk_hlist_add_head(&spare_ch->link, &b->free_head[spare_ord]);

		spare_pages -= BBUDDY_PAGES(spare_ord);
		spare_end -= BBUDDY_LEN(spare_ord);
	}
}

static void *bbuddy_palloc(struct uk_alloc *a, unsigned long num_pages)
{
	struct uk_bbpalloc *const b = (struct uk_bbpalloc *)&a->priv;
	struct chunk_head *alloc_ch;
	size_t ord;

	UK_ASSERT(a);
	UK_ASSERT(num_pages);
	freelist_sanitycheck(b->free_head);

	ord = num_pages_to_order(num_pages);
	/* Find the smallest order of free memory that satisfies the request */
	while (ord < FREELIST_SIZE && FREELIST_EMPTY(&b->free_head[ord]))
		ord++;
	/* We use >= as ord may have been set arbitrarily high by num_pages */
	if (ord >= FREELIST_SIZE)
		goto err_nomem;

	/* Grab & unlink a chunk */
	alloc_ch = uk_hlist_entry(b->free_head[ord].first, struct chunk_head,
				  link);
	uk_hlist_del(&alloc_ch->link);
	UK_ASSERT(FREELIST_ALIGNED(alloc_ch, ord));

	/* Trim off any extra pages off the end */
	bbuddy_trim(b, alloc_ch, ord, num_pages);

	/* Mark as in use and return */
	map_alloc(b, (uintptr_t)alloc_ch, num_pages);

	uk_alloc_stats_count_palloc(a, (void *) alloc_ch, num_pages);
	freelist_sanitycheck(b->free_head);

	return (void *)alloc_ch;

err_nomem:
	uk_pr_warn("%p: Cannot handle palloc request of %lu: Out of memory\n",
		   a, num_pages);

	uk_alloc_stats_count_penomem(a, num_pages);
	errno = ENOMEM;
	return NULL;
}

/**
 * INTERNAL. Maximally merge chunk `*chp` of order `ord` with any free buddies,
 * updating its value along the way.
 *
 * `*chp` must be marked as "allocated" in bbuddy's bookkeeping.
 *
 * The merge process for a given to-be-freed chunk is:
 * 1. calculate ch's buddy's starting page address using BBUDDY_BUDDY_ADDR
 * 2. check in the bitmap whether that page is allocated or free
 *   - if allocated, we know buddy cannot be free, so we stop
 *   - if free, we know it has to be a chunk head (see below), and thus has
 *     valid metadata
 * 3. read chunk_head from starting page to determine buddy's actual size/order
 *   - if it matches our own, we can merge and move onto a higher order
 *   - if it differs, we stop
 *
 * Generally speaking, a page marked free in the bitmap is a chunk head of order
 * ord if-and-only-if:
 * (a) the page is aligned to that particular order (all bits < ord are 0), AND
 * (b) the page is not part of a chunk of larger order
 *
 * In the particular care of bbuddy_merge(ch):
 * (a) is guaranteed because ch is aligned to ord, and thus its candidate buddy
 * must necessarily be aligned too.
 * (b) is guaranteed to never happen, as buddy would need ch to already be free
 * in order to form a larger order chunk
 *
 * @return Order of `*ch` after merging
 */
static inline size_t bbuddy_merge(struct uk_bbpalloc *b,
				  struct chunk_head **chp, size_t ord)
{
	struct chunk_head *ch = *chp;
	struct chunk_head *buddy;

	/* After successfully merging with our first buddy, we may be able to
	 * merge again with a higher-order buddy, and so on, up to max order.
	 */
	while (ord < FREELIST_SIZE - 1) {
		/* It is safe to deref this only if marked free in bitmap */
		buddy = BBUDDY_BUDDY_ADDR(ch, ord);

		/* Stop if buddy is not free or of wrong order */
		if (allocated_in_map(b, (uintptr_t)buddy) ||
		    buddy->page_order != ord)
			break;

		if (BBUDDY_ISHEAD(buddy, ord))
			/* buddy is predecessor; merge downwards */
			ch = buddy;
		/* else: ch is already correct base, increase ord to merge up */

		/* Unlink buddy from freelist & continue to next order */
		uk_hlist_del(&buddy->link);
		ord++;
	}
	*chp = ch;
	return ord;
}

static void bbuddy_pfree(struct uk_alloc *a, void *obj, unsigned long num_pages)
{
	struct uk_bbpalloc *const b = (struct uk_bbpalloc *)&a->priv;
	char *base = obj;
	size_t freed_pages;
	size_t ord;
	struct chunk_head *ch;

	UK_ASSERT(a);
	UK_ASSERT(obj);
	UK_ASSERT(num_pages);
	UK_ASSERT(IS_ALIGNED((uintptr_t)obj, __PAGE_SIZE));

	uk_alloc_stats_count_pfree(a, obj, num_pages);
	freelist_sanitycheck(b->free_head);

	/* Since obj can span an arbitrary page range, we may need to return it
	 * in multiple page-order-sized chunks. At each iteration we pick the
	 * largest chunk size permitted by alignment, attempt to merge it with
	 * any free buddies, then link it into the appropriate freelist.
	 */
	do {
		ord = ptr_order(base, npages_order(num_pages));
		freed_pages = BBUDDY_PAGES(ord);
		ch = (struct chunk_head *)base;

		UK_ASSERT(ord < FREELIST_SIZE);
		UK_ASSERT(freed_pages <= num_pages);

		ord = bbuddy_merge(b, &ch, ord);

		/* Populate chunk (must be done before marking free) */
		ch->page_order = ord;
		/* Mark pages as free (must be done before linking in) */
		map_free(b, (uintptr_t)base, freed_pages);
		/* Link into freelist */
		uk_hlist_add_head(&ch->link, &b->free_head[ord]);

		num_pages -= freed_pages;
		base += freed_pages * __PAGE_SIZE;
	} while (num_pages);

	freelist_sanitycheck(b->free_head);
}

static long bbuddy_pmaxalloc(struct uk_alloc *a)
{
	struct uk_bbpalloc *b;
	size_t i, order;

	UK_ASSERT(a != NULL);
	b = (struct uk_bbpalloc *)&a->priv;

	/* Find biggest order that has still elements available */
	order = FREELIST_SIZE;
	for (i = 0; i < FREELIST_SIZE; i++) {
		if (!FREELIST_EMPTY(&b->free_head[i]))
			order = i;
	}
	if (order == FREELIST_SIZE)
		return 0; /* no memory left */

	return (long) (1 << order);
}

static long bbuddy_pavailmem(struct uk_alloc *a)
{
	struct uk_bbpalloc *b;

	UK_ASSERT(a != NULL);
	b = (struct uk_bbpalloc *)&a->priv;

	return (long) b->nr_free_pages;
}

static int bbuddy_addmem(struct uk_alloc *a, void *base, size_t len)
{
	struct uk_bbpalloc *b;
	struct uk_bbpalloc_memr *memr;
	size_t memr_size;
	unsigned long i;
	struct chunk_head *ch;
	uintptr_t min, max, range;

	UK_ASSERT(a != NULL);
	UK_ASSERT(base != NULL);
	b = (struct uk_bbpalloc *)&a->priv;

	freelist_sanitycheck(b->free_head);

	min = round_pgup((uintptr_t)base);
	max = round_pgdown((uintptr_t)base + (uintptr_t)len);
	if (max < min) {
		uk_pr_err("%"__PRIuptr": Failed to add memory region %"__PRIuptr"-%"__PRIuptr": Invalid range after applying page alignments\n",
			  (uintptr_t) a, (uintptr_t) base,
			  (uintptr_t) base + (uintptr_t) len);
		return -EINVAL;
	}

	range = max - min;

	/* We should have at least one page for bitmap tracking
	 * and one page for data.
	 */
	if (range < round_pgup(sizeof(*memr) + BYTES_PER_MAPWORD) +
			__PAGE_SIZE) {
		uk_pr_err("%"__PRIuptr": Failed to add memory region %"__PRIuptr"-%"__PRIuptr": Not enough space after applying page alignments\n",
			  (uintptr_t) a, (uintptr_t) base,
			  (uintptr_t) base + (uintptr_t) len);
		return -EINVAL;
	}

	memr = (struct uk_bbpalloc_memr *)min;

	/*
	 * The number of pages is found by solving the inequality:
	 *
	 * sizeof(*memr) + bitmap_size + page_num * page_size <= range
	 *
	 * where: bitmap_size = page_num / BITS_PER_BYTE
	 *
	 */
	memr->nr_pages = range >> __PAGE_SHIFT;
	memr->mm_alloc_bitmap = (unsigned long *) (min + sizeof(*memr));
	memr_size = round_pgup(sizeof(*memr) +
		DIV_ROUND_UP(memr->nr_pages, BITS_PER_BYTE));
	memr->mm_alloc_bitmap_size = memr_size - sizeof(*memr);

	min += memr_size;
	range -= memr_size;
	memr->nr_pages -= memr_size >> __PAGE_SHIFT;

	/*
	 * Initialize region's bitmap
	 */
	memr->first_page = min;
	/* add to list */
	memr->next = b->memr_head;
	b->memr_head = memr;

	/* All allocated by default. */
	memset(memr->mm_alloc_bitmap, (unsigned char) ~0,
			memr->mm_alloc_bitmap_size);

	/* free up the memory we've been given to play with */
	map_free(b, min, memr->nr_pages);

	while (range != 0) {
		/*
		 * Next chunk is limited by alignment of min, but also
		 * must not be bigger than remaining range.
		 */
		for (i = __PAGE_SHIFT; (1UL << (i + 1)) <= range; i++)
			if (min & (1UL << i))
				break;

		uk_pr_debug("%"__PRIuptr": Add allocate unit %"__PRIuptr" - %"__PRIuptr" (order %lu)\n",
			    (uintptr_t)a, min, (uintptr_t)(min + (1UL << i)),
			    (i - __PAGE_SHIFT));

		ch = (struct chunk_head *)min;
		min += 1UL << i;
		range -= 1UL << i;
		i -= __PAGE_SHIFT;
		ch->page_order = i;
		uk_hlist_add_head(&ch->link, &b->free_head[i]);
	}

	freelist_sanitycheck(b->free_head);

	return 0;
}

struct uk_alloc *uk_allocbbuddy_init(void *base, size_t len)
{
	struct uk_alloc *a;
	struct uk_bbpalloc *b;
	size_t metalen;
	uintptr_t min, max;
	unsigned long i;

	min = round_pgup((uintptr_t)base);
	max = round_pgdown((uintptr_t)base + (uintptr_t)len);
	UK_ASSERT(max > min);

	/* Allocate space for allocator descriptor */
	metalen = round_pgup(sizeof(*a) + sizeof(*b));

	/* enough space for allocator available? */
	if (min + metalen > max) {
		uk_pr_err("Not enough space for allocator: %"__PRIsz" B required but only %"__PRIuptr" B usable\n",
			  metalen, (max - min));
		return NULL;
	}

	a = (struct uk_alloc *)min;
	uk_pr_info("Initialize binary buddy allocator %"__PRIuptr"\n",
		   (uintptr_t)a);
	min += metalen;
	memset(a, 0, metalen);
	b = (struct uk_bbpalloc *)&a->priv;

	for (i = 0; i < FREELIST_SIZE; i++)
		UK_INIT_HLIST_HEAD(&b->free_head[i]);
	b->memr_head = NULL;

	/* initialize and register allocator interface */
	uk_alloc_init_palloc(a, bbuddy_palloc, bbuddy_pfree,
			     bbuddy_pmaxalloc, bbuddy_pavailmem,
			     bbuddy_addmem);

	if (max > min)
		/* add left memory - ignore return value */
		bbuddy_addmem(a, (void *)(min),
				 (size_t)(max - min));

	return a;
}
