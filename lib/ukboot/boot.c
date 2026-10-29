/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Unikraft bootstrapping
 *
 * Authors: Simon Kuenzer <simon.kuenzer@neclab.eu>
 *
 *
 * Copyright (c) 2017, NEC Europe Ltd., NEC Corporation. All rights reserved.
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

#include <uk/config.h>
#include <uk/plat/config.h>

#include <stddef.h>
#include <stdio.h>
#include <errno.h>

#include <uk/boot.h>
#if CONFIG_LIBUKBOOT_HEAP_RELEASE
#include <uk/heap.h>
#endif
#ifdef CONFIG_LIBUKPAGING
#include <uk/falloc.h>
#include <uk/paging.h>
#ifdef CONFIG_LIBUKVMEM
#include <uk/vmem.h>
#endif /* CONFIG_LIBUKVMEM */
#endif /* CONFIG_LIBUKPAGING */
/* FIXME: allocators are hard-coded for now */
#if CONFIG_LIBUKBOOT_INITBBUDDY
#include <uk/allocbbuddy.h>
#define uk_alloc_init uk_allocbbuddy_init
#elif CONFIG_LIBUKBOOT_INITREGION
#include <uk/allocregion.h>
#define uk_alloc_init uk_allocregion_init
#elif CONFIG_LIBUKBOOT_INITMIMALLOC
#include <uk/mimalloc.h>
#define uk_alloc_init uk_mimalloc_init
#elif CONFIG_LIBUKBOOT_INITTLSF
#include <uk/tlsf.h>
#define uk_alloc_init uk_tlsf_init
#elif CONFIG_LIBUKBOOT_INITTINYALLOC
#include <uk/tinyalloc.h>
#define uk_alloc_init uk_tinyalloc_init
#endif
#if CONFIG_LIBUKBOOT_ALLOCSTACK
#include <uk/allocstack.h>
#if CONFIG_LIBUKBOOT_ALLOCSTACK_PREMAP_ORDER
#define ALLOCSTACK_INITIAL_SIZE				\
	(UK_PAGING_PAGE_SIZE * (1 << CONFIG_LIBUKBOOT_ALLOCSTACK_PREMAP_ORDER))
#endif /* CONFIG_LIBUKBOOT_ALLOCSTACK_PREMAP_ORDER */
#endif /* CONFIG_LIBUKBOOT_ALLOCSTACK */
#if CONFIG_LIBUKSCHED
#include <uk/sched.h>
#endif /* CONFIG_LIBUKSCHED */
#if CONFIG_LIBUKBOOT_INITSCHEDCOOP
#include <uk/schedcoop.h>
#endif /* CONFIG_LIBUKBOOT_INITSCHEDCOOP */
#include <uk/lcpu.h>
#include <uk/pm.h>
#include <uk/plat/memory.h>
#include <uk/plat/time.h>
#include <uk/essentials.h>
#include <uk/print.h>
#include <uk/ctors.h>
#include <uk/init.h>
#ifdef CONFIG_LIBUKSP
#include <uk/sp.h>
#endif
#include <uk/arch/tls.h>
#if CONFIG_LIBUKBOOT_MAINTHREAD
#include "shutdown_req.h"
#endif /* CONFIG_LIBUKBOOT_MAINTHREAD */
#include <uk/errptr.h>
#include "banner.h"

#if CONFIG_LIBUKINTCTLR
#include <uk/intctlr.h>
#endif /* CONFIG_LIBUKINTCTLR */

#include "init.h"

extern char **boot_argv;
extern int boot_argc;

int main(int argc, char *argv[]) __weak;

#if CONFIG_LIBUKBOOT_MAINTHREAD
static __noreturn void main_thread(void *, void *);
static void main_thread_dtor(struct uk_thread *m);
struct uk_semaphore main_sema;
#endif /* CONFIG_LIBUKBOOT_MAINTHREAD */

#if defined(CONFIG_LIBUKBOOT_HEAP_BASE) && defined(CONFIG_LIBUKVMEM)
static struct uk_vas kernel_vas;

#if CONFIG_LIBUKBOOT_INITBBUDDY
/* The heap is a VMA paged in on demand, and processes' mappings take frames
 * from the same allocator. Paged in on first touch, a heap page the kernel
 * touches once processes hold every frame faults with none to give, in the
 * kernel, which cannot recover. So the heap backs the pages it hands out when
 * it hands them out, and fails the allocation when it cannot. And it gives
 * back the frames of the pages it takes back: the memory of a large file,
 * once deleted, is the processes' again.
 */

/* Free blocks smaller than this keep their frames */
#define HEAP_RELEASE_MIN_PAGES	16

/* How much larger than the memory the heap's address range is */
#define HEAP_RANGE_FACTOR	4

/* Pages of 4 KiB: backing a large aligned block could otherwise take large
 * pages, which a release of part of it has to split, with a frame for the
 * new page table it may not find.
 */
#define HEAP_MAP_SIZE		UK_VMA_MAP_SIZE_4KB

static struct uk_alloc *heap_alloc;
static uk_alloc_palloc_func_t heap_palloc_backend;
static uk_alloc_getpsize_func_t heap_pmaxalloc_backend;
static uk_alloc_getpsize_func_t heap_pavailmem_backend;

/* The pages the heap can still back, with the page tables mapping them:
 * its range is larger than that
 */
static long heap_pages_left(void)
{
	__sz free = kernel_vas.pt->fa->free_memory >> UK_PAGING_PAGE_SHIFT;
	__sz left;

	if (free <= CONFIG_LIBUKVMEM_RESERVE_PAGES)
		return 0;
	left = free - CONFIG_LIBUKVMEM_RESERVE_PAGES;
	return (long)(left - MIN(left, (__sz)UK_PAGING_PT_PAGES(left)));
}

static long heap_pmaxalloc(struct uk_alloc *a)
{
	return MIN(heap_pmaxalloc_backend(a), heap_pages_left());
}

static long heap_pavailmem(struct uk_alloc *a)
{
	return MIN(heap_pavailmem_backend(a), heap_pages_left());
}

/* An allocation's pages in question, for the chunks it is freed as, which
 * are the allocator's to say (uk_allocbbuddy_foreach_chunk()). The pages of
 * a chunk the allocator keeps frames behind (heap_keeps()), as
 * heap_release() does for a free block: all of a small chunk, which the
 * next small allocations split; of a larger one its first, where its head
 * is written, and those a power of two from its start, where the heads of
 * the halves it is split into are.
 */
struct heap_walk {
	__vaddr_t start, end;	/* the pages in question */
	__vaddr_t lowest;	/* the lowest address given back */
	int rc;			/* the first failure to take frames */
};

/* Whether the allocator keeps a frame behind page @page, from its start, of
 * a chunk of @size pages (see struct heap_walk): all of a small chunk, and
 * of a larger one its first and those a power of two from it
 */
static bool heap_keeps(unsigned long size, unsigned long page)
{
	return size < HEAP_RELEASE_MIN_PAGES || page == 0 || POWER_OF_2(page);
}

/* The first page from @page on where heap_keeps() says otherwise than for
 * @page, or @size: the kept pages of a large chunk are 0, 1, 2 and the
 * powers of two, so a kept run past page 2 is one page, and an unkept run
 * ends at the next power of two
 */
static unsigned long heap_keeps_until(unsigned long size, unsigned long page)
{
	unsigned long next;

	if (size < HEAP_RELEASE_MIN_PAGES)
		return size;
	if (page < 3)
		return 3;
	if (heap_keeps(size, page))
		return page + 1;
	next = 1UL << (sizeof(long) * 8 - __builtin_clzl(page));
	return MIN(next, size);
}

/* The chunk's pages from @lo to @hi, by runs the allocator keeps (@kept) or
 * does not, each run to @fn
 */
static void heap_chunk_runs(__vaddr_t chunk, unsigned long size,
			    __vaddr_t lo, __vaddr_t hi, bool kept,
			    void (*fn)(__vaddr_t lo, __vaddr_t hi, void *arg),
			    void *arg)
{
	unsigned long page, end, run;

	/* A chunk the range does not reach, or reaches past: nothing of it */
	if (lo >= hi)
		return;
	page = (lo - chunk) >> UK_PAGING_PAGE_SHIFT;
	end = (hi - chunk) >> UK_PAGING_PAGE_SHIFT;
	while (page < end) {
		run = MIN(end, heap_keeps_until(size, page));
		if (heap_keeps(size, page) == kept)
			fn(chunk + (page << UK_PAGING_PAGE_SHIFT),
			   chunk + (run << UK_PAGING_PAGE_SHIFT), arg);
		page = run;
	}
}

/* Give the frames behind a run of pages the allocator does not keep back.
 * Failed part-way, some pages are gone: all are taken to be, the safe way
 * round for whoever writes to them next.
 */
static void heap_release_run(__vaddr_t lo, __vaddr_t hi, void *arg)
{
	struct heap_walk *w = arg;
	int rc = uk_vma_advise(&kernel_vas, lo, hi - lo, UK_VMA_ADV_DONTNEED,
			       0);

	if (unlikely(rc))
		uk_pr_warn("Could not give the frames of %#lx-%#lx back: %d\n",
			   lo, hi, rc);
	w->lowest = MIN(w->lowest, lo);
}

/* Give the frames behind the chunk's pages the allocator does not keep,
 * among the pages in question, back
 */
static void heap_release_chunk(void *chunk, unsigned long size, void *arg)
{
	struct heap_walk *w = arg;
	__vaddr_t c = (__vaddr_t)chunk;

	heap_chunk_runs(c, size, MAX(w->start, c),
			MIN(w->end, c + (size << UK_PAGING_PAGE_SHIFT)),
			false, heap_release_run, w);
}

/* See uk/heap.h */
__vaddr_t uk_heap_release_range(void *base, unsigned long num_pages,
				__vaddr_t start, __vaddr_t end)
{
	struct heap_walk w = { .start = start, .end = end, .lowest = end };

	uk_allocbbuddy_foreach_chunk(base, num_pages, heap_release_chunk, &w);
	return w.lowest;
}

/* The allocator does not use these free pages, the block at @base less one
 * page but its first (see uk_allocbbuddy_release_func_t): their frames are
 * the processes' again, but for the pages heap_release_chunk() keeps.
 */
static void heap_release(struct uk_alloc *a __unused, void *base,
			 unsigned long num_pages)
{
	uk_heap_release_range((char *)base - UK_PAGING_PAGE_SIZE,
			      num_pages + 1, (__vaddr_t)base,
			      (__vaddr_t)base +
			      (num_pages << UK_PAGING_PAGE_SHIFT));
}

/* Whether there are frames for @pages more heap pages: what heap_pages_left()
 * counts, the page tables mapping them and the frames ukvmem keeps for faults
 * that cannot fail set aside
 */
static bool heap_can_back(unsigned long pages)
{
	return pages <= (unsigned long)heap_pages_left();
}

/* Whether half of the frames ukvmem keeps for faults that cannot fail are
 * gone: then the allocator is not asked for a block, as its own writes to
 * the free blocks it splits are such faults
 */
static bool heap_reserve_drained(void)
{
	const __sz reserve = (__sz)CONFIG_LIBUKVMEM_RESERVE_PAGES
			     << UK_PAGING_PAGE_SHIFT;

	return kernel_vas.pt->fa->free_memory < reserve / 2;
}

/* The pages of [@start, @end) without a frame: counted up to one past
 * @limit, where there is no point in going on. A page present but not
 * writable (a platform's copy-on-write page, after a restore) is counted
 * in @cow instead: a write to it takes a frame of its own, on the fault,
 * which nothing here can take beforehand. One past the limit, such a page
 * further on goes unseen, and need not be: the answer is no either way.
 */
static unsigned long heap_unbacked(__vaddr_t start, __vaddr_t end,
				   unsigned long limit, unsigned long *cow)
{
	unsigned long pages = 0;
	unsigned int lvl;
	__pte_t pte;

	*cow = 0;
	for (; start < end && pages <= limit; start += UK_PAGING_PAGE_SIZE) {
		lvl = UK_PAGING_PAGE_LEVEL;
		if (uk_paging_pt_walk(kernel_vas.pt, start, &lvl, __NULL,
				      &pte) ||
		    !UK_PAGING_PT_Lx_PTE_PRESENT(pte, lvl))
			pages++;
		else if (!(uk_pal_attr_from_pte(pte, lvl) &
			   UK_PAGING_PAGE_ATTR_PROT_WRITE))
			(*cow)++;
	}
	return pages;
}

/* See uk/heap.h: the headroom heap_palloc() leaves, reckoned for the whole
 * range first, as heap_palloc() reckons it (a page a write will have to
 * copy takes its frame then, from that headroom, before anything else can:
 * the write follows in the same call, and nothing runs in between), and
 * for the pages without a frame only when that is short. Then a page a
 * write would have to copy is one too many: its frame cannot be taken here.
 */
int uk_heap_back_range(__vaddr_t start, __vaddr_t end)
{
	unsigned long pages = (end - start) >> UK_PAGING_PAGE_SHIFT;
	unsigned long cow;

	if (!heap_can_back(pages)) {
		pages = heap_unbacked(start, end, heap_pages_left(), &cow);
		if (cow)
			return -ENOMEM;
		if (!pages)
			return 0;
		if (!heap_can_back(pages))
			return -ENOMEM;
	}
	return uk_vma_advise(&kernel_vas, start, end - start,
			     UK_VMA_ADV_WILLNEED, 0);
}

/* Frames behind a run of pages the allocator keeps, the first failure kept */
static void heap_back_run(__vaddr_t lo, __vaddr_t hi, void *arg)
{
	struct heap_walk *w = arg;

	if (!w->rc)
		w->rc = uk_heap_back_range(lo, hi);
}

/* Frames behind the chunk's pages the allocator keeps frames behind (see
 * heap_keeps()), among the pages in question, its first excepted
 * (heap_back_head()): splitting the chunk later writes the heads of its
 * halves there. (The heads the allocator writes trimming a chunk to a
 * smaller allocation lie elsewhere, and are faults the reserve serves, as
 * for a free block heap_release() gave back.)
 */
static void heap_back_chunk(void *chunk, unsigned long size, void *arg)
{
	struct heap_walk *w = arg;
	__vaddr_t c = (__vaddr_t)chunk;

	heap_chunk_runs(c, size, MAX(w->start, c + UK_PAGING_PAGE_SIZE),
			MIN(w->end, c + (size << UK_PAGING_PAGE_SHIFT)),
			true, heap_back_run, w);
}

/* A frame behind the chunk's first page, where its head is written when
 * the allocation is freed: every chunk's, whatever else fails, so that the
 * allocation can be freed without a fault; the first failure is kept
 */
static void heap_back_head(void *chunk, unsigned long size __unused,
			   void *arg)
{
	struct heap_walk *w = arg;
	int rc = uk_heap_back_range((__vaddr_t)chunk,
				    (__vaddr_t)chunk + UK_PAGING_PAGE_SIZE);

	if (rc && !w->rc)
		w->rc = rc;
}

/* See uk/heap.h: what heap_palloc() does, but for the pages past the first
 * @backed, which get frames behind the pages the allocator writes to only.
 * The chunks' heads come first, so that the block can be freed should the
 * rest fail (a head that could not be backed is the reserve's to serve on
 * the free, which the headroom checks left whole), then the first @backed
 * pages, then the rest the allocator keeps: -ENOMEM when the frames for the
 * first two are not to be had, -ENOSPC for the last, as for a run of pages,
 * which a smaller room may yet find.
 */
int uk_heap_palloc_backed(unsigned long num_pages, unsigned long backed,
			  void **obj)
{
	struct heap_walk w = { .rc = 0 };
	__vaddr_t o;

	UK_ASSERT(backed <= num_pages);
	if (heap_reserve_drained() || !heap_can_back(backed))
		return -ENOMEM;
	/* Asked for a run it has not got, the allocator warns: a caller
	 * trying for room before it settles for less is not news
	 */
	if (num_pages > (unsigned long)heap_pmaxalloc_backend(heap_alloc))
		return -ENOSPC;
	o = (__vaddr_t)heap_palloc_backend(heap_alloc, num_pages);
	if (unlikely(!o))
		return -ENOSPC;
	w.start = o + (backed << UK_PAGING_PAGE_SHIFT);
	w.end = o + (num_pages << UK_PAGING_PAGE_SHIFT);
	uk_allocbbuddy_foreach_chunk((void *)o, num_pages, heap_back_head, &w);
	if (!w.rc) {
		/* The room's few pages before the many of the contents: an
		 * attempt that fails here has cost little
		 */
		uk_allocbbuddy_foreach_chunk((void *)o, num_pages,
					     heap_back_chunk, &w);
		if (w.rc == -ENOMEM)
			w.rc = -ENOSPC;
	}
	if (!w.rc)
		w.rc = uk_heap_back_range(o, w.start);
	if (w.rc) {
		uk_pfree(heap_alloc, (void *)o, num_pages);
		return w.rc;
	}
	*obj = (void *)o;
	return 0;
}

/* See uk/heap.h */
void uk_heap_pfree(void *obj, unsigned long num_pages)
{
	uk_pfree(heap_alloc, obj, num_pages);
}

static void *heap_palloc(struct uk_alloc *a, unsigned long num_pages)
{
	void *obj;

	/* Leave the frames ukvmem keeps for faults that cannot fail, after
	 * backing the pages and the page tables that map them
	 * (uk_heap_back_range()). A block whose pages are backed still, as a
	 * freed block's may be, takes none: taken while half of them is
	 * left, as the allocator's own writes to the free blocks it splits
	 * are such faults.
	 */
	if (heap_reserve_drained())
		goto err_nomem;
	obj = heap_palloc_backend(a, num_pages);
	if (unlikely(!obj))
		return __NULL;
	if (unlikely(uk_heap_back_range((__vaddr_t)obj, (__vaddr_t)obj +
					(num_pages << UK_PAGING_PAGE_SHIFT)))) {
		uk_pfree(a, obj, num_pages);
		goto err_nomem;
	}
	return obj;

err_nomem:
	errno = ENOMEM;
	return __NULL;
}
#else /* !CONFIG_LIBUKBOOT_INITBBUDDY */
#define HEAP_MAP_SIZE		0
#endif /* !CONFIG_LIBUKBOOT_INITBBUDDY */
#endif /* CONFIG_LIBUKBOOT_HEAP_BASE && CONFIG_LIBUKVMEM */

static struct uk_alloc *heap_init()
{
	struct uk_alloc *a = NULL;
	struct ukplat_memregion_desc *md __maybe_unused;
#ifdef CONFIG_LIBUKBOOT_HEAP_BASE
	struct uk_pagetable *pt = uk_paging_pt_get_active();
	__sz free_pages, alloc_pages;
	__vaddr_t heap_base;
#ifdef CONFIG_LIBUKVMEM
	__vaddr_t vaddr;
#endif /* CONFIG_LIBUKVMEM */
	int rc;
#endif /* CONFIG_LIBUKBOOT_HEAP_BASE */

#ifdef CONFIG_LIBUKBOOT_HEAP_BASE
	UK_ASSERT(pt);

	/* Paging is enabled. The available physical memory has already been
	 * added to the frame allocator. No heap area has been mapped. We will
	 * do so at the configured heap base.
	 */
	heap_base = CONFIG_LIBUKBOOT_HEAP_BASE;

#ifdef CONFIG_LIBUKVMEM
#define HEAP_INITIAL_PAGES	16
#define HEAP_INITIAL_LEN	(HEAP_INITIAL_PAGES << UK_PAGING_PAGE_SHIFT)
	/* In addition to paging, we have virtual address space management. We
	 * will thus also represent the heap as a dedicated VMA to enable
	 * on-demand paging for the heap. However, we have a chicken-egg
	 * problem here. This is because we need an allocator for allocating
	 * VMA objects but to do so, we need to create the heap VMA first.
	 * Theoretically, we could use a simple region allocator to bootstrap.
	 * But then, we would have to cope with VMA objects from different
	 * allocators. So instead, we just initialize a small heap using fixed
	 * mappings and then create the VMA on top. Afterwards, we add the
	 * remainder of the VMA to the allocator.
	 */
	rc = uk_paging_page_map(pt, heap_base, UK_PAGING_PADDR_ANY,
				HEAP_INITIAL_PAGES, UK_PAGING_PAGE_ATTR_PROT_RW,
				0);
	if (unlikely(rc))
		return NULL;

	a = uk_alloc_init((void *)heap_base,
			  HEAP_INITIAL_PAGES << UK_PAGING_PAGE_SHIFT);
	if (unlikely(!a))
		return NULL;

	rc = uk_vas_init(&kernel_vas, pt, a);
	if (unlikely(rc))
		return NULL;

	rc = uk_vas_set_active(&kernel_vas);
	if (unlikely(rc))
		return NULL;

	free_pages  = pt->fa->free_memory >> UK_PAGING_PAGE_SHIFT;
	alloc_pages = free_pages - UK_PAGING_PT_PAGES(free_pages);

#if CONFIG_LIBUKBOOT_INITBBUDDY
	/* Its pages backed only as they are handed out (see heap_palloc()),
	 * the heap can span more than the memory, and has to: the buddy
	 * allocator serves a block from a free range aligned to the next
	 * power of two, and a block that grows by copy (a file in ramfs)
	 * needs the old and the new at once.  Up to the base of other VMAs.
	 */
	alloc_pages *= HEAP_RANGE_FACTOR;
#if CONFIG_LIBUKVMEM_DEFAULT_BASE > CONFIG_LIBUKBOOT_HEAP_BASE
	alloc_pages = MIN(alloc_pages,
			  ((CONFIG_LIBUKVMEM_DEFAULT_BASE - heap_base)
			   >> UK_PAGING_PAGE_SHIFT) - HEAP_INITIAL_PAGES);
#endif /* CONFIG_LIBUKVMEM_DEFAULT_BASE > CONFIG_LIBUKBOOT_HEAP_BASE */
#endif /* CONFIG_LIBUKBOOT_INITBBUDDY */

	vaddr = heap_base;
	rc = uk_vma_map_anon(&kernel_vas, &vaddr,
			     (alloc_pages + HEAP_INITIAL_PAGES) << UK_PAGING_PAGE_SHIFT,
			     UK_PAGING_PAGE_ATTR_PROT_RW,
			     UK_VMA_MAP_UNINITIALIZED | HEAP_MAP_SIZE,
			     "heap");
	if (unlikely(rc))
		return NULL;

	rc = uk_alloc_addmem(a, (void *)(heap_base + HEAP_INITIAL_LEN),
			     (alloc_pages - HEAP_INITIAL_PAGES) << UK_PAGING_PAGE_SHIFT);
	if (unlikely(rc))
		return NULL;

#if CONFIG_LIBUKBOOT_INITBBUDDY
	heap_alloc = a;
	heap_palloc_backend = a->palloc;
	heap_pmaxalloc_backend = a->pmaxalloc;
	heap_pavailmem_backend = a->pavailmem;
	a->palloc = heap_palloc;
	a->pmaxalloc = heap_pmaxalloc;
	a->pavailmem = heap_pavailmem;
	uk_allocbbuddy_set_release(a, heap_release);
#endif /* CONFIG_LIBUKBOOT_INITBBUDDY */

#if CONFIG_LIBVFSCORE_AUTOMOUNT_EXTRACT_BORROW
	/* Files extracted from the initrd (initrd0) borrow its memory, so it
	 * stays mapped for good.  Represent that mapping, so that an mmap() hint
	 * cannot be placed over it and swap its pages out from under those
	 * files.  A DMA VMA maps the physical range it is given and never
	 * hands those frames, which the platform owns, to the frame allocator.
	 */
	if (ukplat_memregion_find_initrd0(&md) >= 0 && md->pg_count) {
		UK_ASSERT_VALID_MRD(md);
		vaddr = md->vbase;
		rc = uk_vma_map_dma(&kernel_vas, &vaddr,
				    md->pg_count << UK_PAGING_PAGE_SHIFT,
				    UK_PAGING_PAGE_ATTR_PROT_READ, 0, "initrd",
				    md->pbase);
		if (unlikely(rc))
			uk_pr_warn("Could not reserve the initrd at %p: %d\n",
				   (void *)md->vbase, rc);
	}
#endif /* CONFIG_LIBVFSCORE_AUTOMOUNT_EXTRACT_BORROW */
#else /* CONFIG_LIBUKVMEM */
	free_pages  = pt->fa->free_memory >> UK_PAGING_PAGE_SHIFT;
	alloc_pages = free_pages - UK_PAGING_PT_PAGES(free_pages);

	rc = uk_paging_page_map(pt, heap_base, UK_PAGING_PADDR_ANY,
				alloc_pages, UK_PAGING_PAGE_ATTR_PROT_RW, 0);
	if (unlikely(rc))
		return NULL;

	a = uk_alloc_init((void *)heap_base,
			  alloc_pages << UK_PAGING_PAGE_SHIFT);
#endif /* !CONFIG_LIBUKVMEM */
#else /* CONFIG_LIBUKBOOT_HEAP_BASE */
	/* Paging is disabled so we still have the static boot page table set
	 * that maps (some of) the physical memory to virtual memory. The
	 * UKPLAT_MEMRT_FREE memory regions reflect this. Try to use these
	 * memory regions to initialize the allocator. If it fails, we will try
	 * again with the next region. As soon we have an allocator, we simply
	 * add every subsequent region to it.
	 */
	ukplat_memregion_foreach(&md, UKPLAT_MEMRT_FREE, 0, 0) {
		UK_ASSERT_VALID_FREE_MRD(md);

		uk_pr_debug("Trying %p-%p 0x%02x %s\n",
			    (void *)md->vbase, (void *)(md->vbase + md->len),
			    md->flags,
#if CONFIG_UKPLAT_MEMRNAME
			    md->name
#else /* CONFIG_UKPLAT_MEMRNAME */
			    ""
#endif /* !CONFIG_UKPLAT_MEMRNAME */
			    );

		if (!a)
			a = uk_alloc_init((void *)md->vbase, md->len);
		else
			uk_alloc_addmem(a, (void *)md->vbase, md->len);
	}
#endif /* !CONFIG_LIBUKBOOT_HEAP_BASE */

	return a;
}

#if CONFIG_LIBPOSIX_ENVIRON
extern char **environ;
#endif /* CONFIG_LIBPOSIX_ENVIRON */

void uk_boot_entry(void)
{
	struct uk_init_ctx ictx = { 0 };
	struct uk_term_ctx tctx = {
		.exit_code = 0,
		.target = UK_PM_SHUTDOWN_OP_SYSCRASH,
	};
	int rc = 0;
#if CONFIG_LIBUKALLOC
	struct uk_alloc *a = NULL, *sa = NULL, *auxsa = NULL;
#endif
#if CONFIG_LIBUKBOOT_INITALLOC
	struct ukarch_auxspcb *auxspcb;
	__uptr auxsp, uktlsp;
	void *tls = NULL;
	void *auxstack;
#endif
#if CONFIG_LIBUKSCHED
	struct uk_sched *s = NULL;
#endif
#if CONFIG_LIBUKBOOT_MAINTHREAD
	struct uk_thread *m;
#endif /* CONFIG_LIBUKBOOT_MAINTHREAD */
	uk_ctor_func_t *ctorfn;
	struct uk_inittab_entry *init_entry;

	/* Make sure uk_early_init has been called to process
	 * the cmdline. We should expect at minimum one arg,
	 * CONFIG_UK_NAME.
	 */
	UK_ASSERT(boot_argc);

#if CONFIG_LIBUKBOOT_MAINTHREAD
	/* Initialize main thread semaphore */
	uk_semaphore_init(&main_sema, 0);
	/* Initialize shutdown control structure */
	uk_boot_shutdown_ctl_init();
#endif /* CONFIG_LIBUKBOOT_MAINTHREAD */

	uk_pr_info("Unikraft constructor table at %p - %p\n",
		   &uk_ctortab_start[0], &uk_ctortab_end);
	uk_ctortab_foreach(ctorfn, uk_ctortab_start, uk_ctortab_end) {
		UK_ASSERT(*ctorfn);
		uk_pr_debug("Call constructor: %p())...\n", *ctorfn);
		(*ctorfn)();
	}

#if CONFIG_LIBUKBOOT_INITALLOC
	uk_pr_info("Initialize memory allocator...\n");

	a = heap_init();
	if (unlikely(!a))
		UK_CRASH("Failed to initialize memory allocator\n");
	else {
		rc = ukplat_memallocator_set(a);
		if (unlikely(rc != 0))
			UK_CRASH("Could not set the platform memory allocator\n");
	}

	sa = uk_allocstack_init(a
#if CONFIG_LIBUKVMEM
				, &kernel_vas, ALLOCSTACK_INITIAL_SIZE
#endif /* CONFIG_LIBUKVMEM */
			       );
	if (unlikely(!sa))
		UK_CRASH("Could not initialize stack allocator\n");

	auxsa = uk_allocstack_init(a
#if CONFIG_LIBUKVMEM
				    /* The auxiliary stack must be always
				     * fully mapped.
				     */
				    , &kernel_vas, AUXSTACK_SIZE
#endif /* CONFIG_LIBUKVMEM */
				   );
	if (unlikely(!auxsa))
		UK_CRASH("Could not initialize auxiliary stack allocator\n");

	/* Allocate a TLS for this execution context */
	tls = uk_memalign(a,
			  ukarch_tls_area_align(),
			  ukarch_tls_area_size());
	if (!tls)
		UK_CRASH("Failed to allocate and initialize TLS\n");

	/* Copy from TLS master template */
	ukarch_tls_area_init(tls);
	/* Activate TLS */
	uktlsp = ukarch_tls_tlsp(tls);
	uk_lcpu_tlsp_set(uktlsp);

	/* Allocate auxiliary stack for this execution context */
	auxstack = uk_memalign(auxsa,
			       UKARCH_AUXSP_ALIGN, AUXSTACK_SIZE);
	if (unlikely(!auxstack))
		UK_CRASH("Failed to allocate the auxiliary stack\n");

	/* Activate auxiliary stack */
	auxsp = ukarch_gen_sp(auxstack, AUXSTACK_SIZE);
	ukarch_auxsp_init(auxsp);
	auxspcb = ukarch_auxsp_get_cb(auxsp);
	UK_ASSERT(auxspcb);
	ukarch_auxspcb_set_uktlsp(auxspcb, uktlsp);
	uk_lcpu_set_auxsp(auxsp);
#endif /* CONFIG_LIBUKBOOT_INITALLOC */

#if CONFIG_LIBUKINTCTLR
	uk_pr_info("Initialize the IRQ subsystem...\n");
	rc = uk_intctlr_init(a);
	if (unlikely(rc))
		UK_CRASH("Could not initialize the IRQ subsystem\n");
#endif /* CONFIG_LIBUKINTCTLR */

	/* On most platforms the timer depend on an initialized IRQ subsystem */
	uk_pr_info("Initialize platform time...\n");
	ukplat_time_init();

#if CONFIG_LIBUKBOOT_INITSCHED
	uk_pr_info("Initialize scheduling...\n");
#if CONFIG_LIBUKBOOT_INITSCHEDCOOP
	s = uk_schedcoop_create(a, sa, auxsa, a);
#endif
	if (unlikely(!s))
		UK_CRASH("Failed to initialize scheduling\n");
	uk_sched_start(s);
#endif /* CONFIG_LIBUKBOOT_INITSCHED */

	ictx.cmdline.argc = boot_argc;
	ictx.cmdline.argv = boot_argv;

#if CONFIG_LIBUKBOOT_MAINTHREAD
	/* Start main thread (will block on semaphore) */
	m = uk_sched_thread_create_fn2(s, main_thread,
				       &ictx, &tctx,
				       0x0 /* default stack size */,
				       0x0 /* default auxiliary stack size */,
				       false, false,
				       "main", NULL,
				       main_thread_dtor);
	if (unlikely(!m || PTRISERR(m)))
		UK_CRASH("Failed to launch application's main()\n");

	ictx.tmain = m;
#endif /* CONFIG_LIBUKBOOT_MAINTHREAD */

	/* Enable interrupts before starting the application */
	uk_lcpu_enable_irq();

	/**
	 * Run init table
	 */
	uk_pr_info("Init Table @ %p - %p\n",
		   &uk_inittab_start[0], &uk_inittab_end);
	uk_inittab_foreach(init_entry, uk_inittab_start, uk_inittab_end) {
		UK_ASSERT(init_entry);

		if (!init_entry->init)
			continue;

		uk_pr_debug("Call init function: %p(%p)...\n",
			    init_entry->init, &ictx);
		rc = (*init_entry->init)(&ictx);
		if (rc < 0) {
			uk_pr_err("Init function at %p returned error %d\n",
				  init_entry->init, rc);
			goto exit;
		}
	}

#ifdef CONFIG_LIBUKSP
	uk_stack_chk_guard_setup();
#endif

	print_banner(stdout);
	fflush(stdout);

#if !CONFIG_LIBUKBOOT_MAINTHREAD
	tctx.exit_code = do_main(ictx.cmdline.argc, ictx.cmdline.argv);
	tctx.target = UK_PM_SHUTDOWN_OP_SYSHALT;
#else /* CONFIG_LIBUKBOOT_MAINTHREAD */
	/* Unblock main thread (will execute main()) */
	uk_semaphore_up(&main_sema);

	/* Block execution of "init" until we receive the first request.
	 * The exit code will be set by the main thread.
	 */
	tctx.target = uk_boot_shutdown_barrier();
#endif /* CONFIG_LIBUKBOOT_MAINTHREAD */

exit:
	uk_pr_info("Shutting down system (%d)\n", tctx.target);

	/**
	 * Call termination functions from init table in reverse order
	 */
	/* NOTE: The init loop left `init_entry` at the position that is one
	 *       step further after the last successfully initialized  entry
	 */
	init_entry--;
	uk_inittab_foreach_reverse2(init_entry, uk_inittab_start) {
		UK_ASSERT(init_entry);

		if (!init_entry->term)
			continue;

		uk_pr_debug("Call term function: %p(%p)...\n",
			    init_entry->term, &tctx);
		(*init_entry->term)(&tctx);
	}

	uk_pr_debug("Unikraft terminates with exit status %d (target: %d)\n",
		    tctx.exit_code, tctx.target);
	uk_pm_shutdown(tctx.target); /* does not return */
}

int do_main(int argc, char *argv[])
{
	char **envp __maybe_unused;
	uk_ctor_func_t *ctorfn;
	int ret;

	/*
	 * Application
	 *
	 * We are calling the application constructors right before calling
	 * the application's main(). All of our Unikraft systems, VFS,
	 * networking stack are initialized at this point. This way we closely
	 * mimic what a regular user application (e.g., BSD, Linux) would expect
	 * from its OS being initialized.
	 */
	uk_pr_info("Pre-init table at %p - %p\n",
		   &__preinit_array_start[0], &__preinit_array_end);
	uk_ctortab_foreach(ctorfn,
			   __preinit_array_start, __preinit_array_end) {
		if (!*ctorfn)
			continue;

		uk_pr_debug("Call pre-init constructor: %p()...\n", *ctorfn);
		(*ctorfn)();
	}

	uk_pr_info("Constructor table at %p - %p\n",
		   &__init_array_start[0], &__init_array_end);
	uk_ctortab_foreach(ctorfn, __init_array_start, __init_array_end) {
		if (!*ctorfn)
			continue;

		uk_pr_debug("Call constructor: %p(%d, %p)...\n", *ctorfn,
			    argc, argv);
		(*ctorfn)(argc, argv);
	}

#if CONFIG_LIBUKDEBUG_PRINTK_INFO
#if CONFIG_LIBPOSIX_ENVIRON
	envp = environ;
	if (envp) {
		uk_pr_info("Environment variables:\n");
		while (*envp) {
			uk_pr_info("\t%s\n", *envp);
			envp++;
		}
	}
#endif /* CONFIG_LIBPOSIX_ENVIRON */

	uk_pr_info("Calling main(%d, [", argc);
	for (int i = 0; i < argc; ++i) {
		uk_pr_info("'%s'", argv[i]);
		if ((i + 1) < argc)
			uk_pr_info(", ");
	}
	uk_pr_info("])\n");
#endif /* CONFIG_LIBUKDEBUG_PRINTK_INFO */

	ret = main(argc, argv);
	uk_pr_info("main returned %d\n", ret);
	return ret;
}

#if CONFIG_LIBUKBOOT_MAINTHREAD
/* Pass ictx so that inittab handlers can update args */
static __noreturn void main_thread(void *a0, void *a1)
{
	struct uk_init_ctx *ictx = (struct uk_init_ctx *)a0;
	struct uk_term_ctx *tctx = (struct uk_term_ctx *)a1;

	UK_ASSERT(ictx);
	UK_ASSERT(tctx);

	/* block until we are allowed to execute main() */
	uk_semaphore_down(&main_sema);
#if CONFIG_LIBUKBOOT_INIT
	tctx->exit_code = do_init(ictx->cmdline.argc, ictx->cmdline.argv);
#else /* !CONFIG_LIBUKBOOT_INIT */
	tctx->exit_code = do_main(ictx->cmdline.argc, ictx->cmdline.argv);
#endif /* !CONFIG_LIBUKBOOT_INIT */

#if !CONFIG_LIBUKBOOT_MAINTHREAD_NOHALT
	/* NOTE: The scheduler's garbage collector would also initiate a
	 *       shutdown request via `main_thread_dtor()`.
	 *       But because of an unknown delay before the GC is called
	 *       we already request a shutdown here in order to go down
	 *       as earlier as possible.
	 */
	uk_boot_shutdown_req(UK_PM_SHUTDOWN_OP_SYSHALT);
#endif /* !LIBUKBOOT_MAINTHREAD_NOHALT */
	/* Terminate "main" thread */
	uk_thread_exit();
}

static void main_thread_dtor(struct uk_thread *m __unused)
{
#if !CONFIG_LIBUKBOOT_MAINTHREAD_NOHALT
	/* NOTE: Here, we request a shutdown as soon as "main" exits.
	 *       It covers also the cases where the "main" thread just exits
	 *       without returning to the caller of `main()` (for example
	 *       via `uk_thread_exit()`).
	 */
	uk_boot_shutdown_req(UK_PM_SHUTDOWN_OP_SYSHALT);
#endif /* !LIBUKBOOT_MAINTHREAD_NOHALT */
}
#endif /* CONFIG_LIBUKBOOT_MAINTHREAD */
