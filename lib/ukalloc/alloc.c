/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Florian Schmidt <florian.schmidt@neclab.eu>
 *          Hugo Lefeuvre <hugo.lefeuvre@neclab.eu>
 *
 * Copyright (c) 2017-2020, NEC Laboratories Europe GmbH, NEC Corporation,
 *                          All rights reserved.
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

#include <errno.h>
#include <string.h>
#include <stddef.h>
#include <uk/alloc_impl.h>
#include <uk/config.h>
#include <uk/essentials.h>
#include <uk/assert.h>
#include <uk/arch/limits.h>
#include <uk/arch/lcpu.h>
#include <uk/arch/paging.h>
#ifdef CONFIG_LIBKASAN
#include <uk/kasan.h>
#endif

#if CONFIG_HAVE_MEMTAG
#include <uk/arch/memtag.h>
#endif

#define size_to_num_pages(size) \
	(PAGE_ALIGN_UP((unsigned long)(size)) / __PAGE_SIZE)
#define page_off(x) ((unsigned long)(x) & (__PAGE_SIZE - 1))

struct uk_alloc *_uk_alloc_head;

int uk_alloc_register(struct uk_alloc *a)
{
	struct uk_alloc *this = _uk_alloc_head;

	if (!_uk_alloc_head) {
		_uk_alloc_head = a;
		a->next = __NULL;
		return 0;
	}

	while (this && this->next)
		this = this->next;
	this->next = a;
	a->next = __NULL;
	return 0;
}

#ifdef CONFIG_HAVE_MEMTAG
#define __align_metadata_ifpages __align(MEMTAG_GRANULE)
#else
#define __align_metadata_ifpages
#endif /* CONFIG_HAVE_MEMTAG */
struct metadata_ifpages {
	__sz		size;      /* user size */
	unsigned long	num_pages; /* alloc pages */
	void		*base;
} __align_metadata_ifpages;

/* METADATA_IFPAGES_SIZE_POW2 is a power of two larger or equal to
 * sizeof(struct metadata_ifpages). The optimal value for this is
 * architecture specific. If the actual sizeof(struct metadata_ifpages) is
 * smaller, we will just waste a few negligible bytes. If it is larger, the
 * compile time assertion will abort the compilation and this value will have
 * to be increased.
 */
#define METADATA_IFPAGES_SIZE_POW2 32
UK_CTASSERT(!(sizeof(struct metadata_ifpages) > METADATA_IFPAGES_SIZE_POW2));

/* The METADATA_IFPAGES_SIZE_POW2 size must not break any alignments */
UK_CTASSERT(
	METADATA_IFPAGES_SIZE_POW2 % __alignof__(max_align_t) == 0
);

static struct metadata_ifpages *uk_get_metadata(const void *ptr)
{
	__uptr metadata;

	/* a ptr less or equal to page size would mean that the actual allocated
	 * object started at 0x0, so it was NULL.
	 * any value between page size and page size + size of metadata would
	 * also imply that the actual allocated object started at 0x0 because
	 * we need space to store metadata.
	 */
	UK_ASSERT((__uptr) ptr >= __PAGE_SIZE +
		  METADATA_IFPAGES_SIZE_POW2);

	metadata = PAGE_ALIGN_DOWN((__uptr)ptr);
	if (metadata == (__uptr) ptr) {
		/* special case: the memory was page-aligned.
		 * In this case the metadata lies at the start of the
		 * previous page, with the rest of that page unused.
		 */
		metadata -= __PAGE_SIZE;
	}

	return (struct metadata_ifpages *) metadata;
}

static __sz uk_getmallocsize(const void *ptr)
{
	struct metadata_ifpages *metadata = uk_get_metadata(ptr);

	return (__sz)metadata->base + (__sz)(metadata->num_pages) *
	       __PAGE_SIZE - (__sz)ptr;
}

/* This is a very simple, naive implementation of malloc.
 * It's not space-efficient, because it always requests full pages from the
 * underlying memory allocator. It's also not very fast, because for the
 * same reason, it will never hand out memory from already-requested pages,
 * and has to go through the underlying memory allocator on every malloc()
 * and free() (and friends. And God have mercy on your soul if you call free()
 * with a pointer that wasn't received from malloc(). But it's simple, and,
 * above all, it is inherently reentrant, because all bookkeeping is
 * decentralized. This is important, because we currently don't have proper
 * locking support yet. Eventually, this should probably be replaced by
 * something better.
 */
void *uk_malloc_ifpages(struct uk_alloc *a, __sz size)
{
	__uptr intptr;
	unsigned long num_pages;
	struct metadata_ifpages *metadata;
#ifdef CONFIG_HAVE_MEMTAG
	size = MEMTAG_ALIGN(size);
#endif /* CONFIG_HAVE_MEMTAG */

#ifdef CONFIG_LIBKASAN
	__sz realsize = sizeof(*metadata) + size + KASAN_KMALLOC_REDZONE_SIZE;
#else
	__sz realsize = sizeof(*metadata) + size;
#endif

	UK_ASSERT(a);
	/* check for invalid size and overflow */
	if (!size || realsize < size)
		return __NULL;

	num_pages = size_to_num_pages(realsize);
	intptr = (__uptr)uk_palloc(a, num_pages);

	if (!intptr)
		return __NULL;

	metadata = (struct metadata_ifpages *) intptr;
	metadata->size = size;
	metadata->num_pages = num_pages;
	metadata->base = (void *) intptr;

#ifdef CONFIG_LIBKASAN
	kasan_mark((void *)(intptr + sizeof(*metadata)),
		size, metadata->num_pages * __PAGE_SIZE - sizeof(*metadata),
		KASAN_CODE_KMALLOC_OVERFLOW);
#endif

#ifdef CONFIG_HAVE_MEMTAG
	return ukarch_memtag_region(
		(void *)(intptr + METADATA_IFPAGES_SIZE_POW2),
		size
	);
#else
	return (void *)(intptr + METADATA_IFPAGES_SIZE_POW2);
#endif /* CONFIG_HAVE_MEMTAG */
}

void uk_free_ifpages(struct uk_alloc *a, void *ptr)
{
	struct metadata_ifpages *metadata;
#ifdef CONFIG_HAVE_MEMTAG
	__sz size;
#endif /* CONFIG_HAVE_MEMTAG */

	UK_ASSERT(a);
	if (!ptr)
		return;

#ifdef CONFIG_HAVE_MEMTAG
	metadata = uk_get_metadata((void *)((uint64_t)ptr & ~MTE_TAG_MASK));
	size = metadata->size;
#else
	metadata = uk_get_metadata(ptr);
#endif /* CONFIG_HAVE_MEMTAG */

	UK_ASSERT(metadata->base != __NULL);
	UK_ASSERT(metadata->num_pages != 0);

#ifdef CONFIG_LIBKASAN
	kasan_mark_invalid(metadata->base + sizeof(*metadata),
		metadata->num_pages * 4096 - sizeof(*metadata),
		KASAN_CODE_KMALLOC_FREED);
#endif

	uk_pfree(a, metadata->base, metadata->num_pages);

#ifdef CONFIG_HAVE_MEMTAG
	ukarch_memtag_region(ptr, size);
#endif /* CONFIG_HAVE_MEMTAG */
}

void *uk_realloc_ifpages(struct uk_alloc *a, void *ptr, __sz size)
{
	void *retptr;
	__sz mallocsize;

	UK_ASSERT(a);
	if (!ptr)
		return uk_malloc_ifpages(a, size);

	if (!size) {
		uk_free_ifpages(a, ptr);
		return __NULL;
	}

	retptr = uk_malloc_ifpages(a, size);
	if (!retptr)
		return __NULL;

	mallocsize = uk_getmallocsize(ptr);

	if (size < mallocsize)
		memcpy(retptr, ptr, size);
	else
		memcpy(retptr, ptr, mallocsize);

	uk_free_ifpages(a, ptr);
	return retptr;
}

int uk_posix_memalign_ifpages(struct uk_alloc *a,
				void **memptr, __sz align, __sz size)
{
	struct metadata_ifpages *metadata;
	unsigned long num_pages;
	__uptr intptr;
	__sz realsize, padding;

	UK_ASSERT(a);
	if (((align - 1) & align) != 0
	    || (align % sizeof(void *)) != 0)
		return EINVAL;

	/* According to POSIX, calling posix_memalign with a size of zero can
	 * be handled by (1) setting memptr to NULL and returning 0 (success),
	 * OR (2) leaving memptr untouched and returning an error code. We
	 * implement (2).
	 */
	if (!size) {
		return EINVAL;
	}

	/* For page-aligned memory blocks (align is a power of two, this is true
	 * for any align >= __PAGE_SIZE), metadata are not stored immediately
	 * preceding the memory block, but instead at the beginning of the page
	 * preceding the memory returned by this function.
	 *
	 * align < METADATA_IFPAGES_SIZE_POW2 implies that metadata are too
	 * large to be stored preceding the first memory block at given
	 * alignment. In this case, set align to METADATA_IFPAGES_SIZE_POW2,
	 * the next power of two >= sizeof(*metadata). Since it is a power of
	 * two, the returned pointer will still be aligned at the requested
	 * alignment.
	 */
	if (align > __PAGE_SIZE) {
		padding = __PAGE_SIZE;
	} else if (align == __PAGE_SIZE) {
		/* No padding needed: in this case we already know that the next
		 * aligned pointer will be intptr (as handed to by palloc) +
		 * __PAGE_SIZE.
		 */
		padding = 0;
	} else if (align < METADATA_IFPAGES_SIZE_POW2) {
		align = METADATA_IFPAGES_SIZE_POW2;
		padding = 0;
	} else {
		padding = sizeof(*metadata);
	}

#ifdef CONFIG_HAVE_MEMTAG
	size = MEMTAG_ALIGN(size);
#endif /* CONFIG_HAVE_MEMTAG */

	/* In addition to metadata space, allocate `align` more bytes in
	 * order to be sure to find an aligned pointer preceding `size` bytes.
	 */
	realsize = size + padding + align;

	/* check for overflow */
	if (realsize < size)
		return EINVAL;

	num_pages = size_to_num_pages(realsize);
	intptr = (__uptr) uk_palloc(a, num_pages);

	if (!intptr)
		return ENOMEM;

	*memptr = (void *) ALIGN_UP(intptr + sizeof(*metadata),
				    (__uptr) align);

	metadata = uk_get_metadata(*memptr);

	/* check for underflow (should not happen) */
	UK_ASSERT(intptr <= (__uptr) metadata);

	metadata->size = size;
	metadata->num_pages = num_pages;
	metadata->base = (void *) intptr;

#ifdef CONFIG_HAVE_MEMTAG
	ukarch_memtag_region(*memptr, size);
#endif /* CONFIG_HAVE_MEMTAG */
	return 0;
}

__ssz uk_alloc_maxalloc_ifpages(struct uk_alloc *a)
{
	long num_pages;
	__ssz maxalloc;

	UK_ASSERT(a);

	num_pages = uk_alloc_pmaxalloc(a);
	if (num_pages < 0) {
		/* forward error code */
		return (__ssz) num_pages;
	}

	maxalloc = ((__ssz) num_pages) << __PAGE_SHIFT;

	if (maxalloc <= METADATA_IFPAGES_SIZE_POW2)
		return 0;

	maxalloc -= METADATA_IFPAGES_SIZE_POW2;
	return maxalloc;
}

__ssz uk_alloc_availmem_ifpages(struct uk_alloc *a)
{
	long num_pages;

	UK_ASSERT(a);

	num_pages = uk_alloc_pavailmem(a);
	if (num_pages < 0)
		return (__ssz) num_pages;

	return ((__ssz) num_pages) << __PAGE_SHIFT;
}

#if CONFIG_LIBUKALLOC_IFMALLOC

struct metadata_ifmalloc {
	__sz	size;
	void	*base;
};

#define METADATA_IFMALLOC_SIZE_POW2 16
UK_CTASSERT(!(sizeof(struct metadata_ifmalloc) > METADATA_IFMALLOC_SIZE_POW2));

static struct metadata_ifmalloc *uk_get_metadata_ifmalloc(const void *ptr)
{
	return (struct metadata_ifmalloc *)((__uptr) ptr -
		METADATA_IFMALLOC_SIZE_POW2);
}

static __sz uk_getmallocsize_ifmalloc(const void *ptr)
{
	struct metadata_ifmalloc *metadata = uk_get_metadata_ifmalloc(ptr);

	return (__sz) ((__uptr) metadata->base + metadata->size -
			 (__uptr) ptr);
}

void uk_free_ifmalloc(struct uk_alloc *a, void *ptr)
{
	struct metadata_ifmalloc *metadata;

	UK_ASSERT(a);
	UK_ASSERT(a->free_backend);
	if (!ptr)
		return;

	metadata = uk_get_metadata_ifmalloc(ptr);
	a->free_backend(a, metadata->base);
}

void *uk_malloc_ifmalloc(struct uk_alloc *a, __sz size)
{
	struct metadata_ifmalloc *metadata;
	__sz realsize = size + METADATA_IFMALLOC_SIZE_POW2;
	void *ptr;

	UK_ASSERT(a);
	UK_ASSERT(a->malloc_backend);

	/* check for overflow */
	if (unlikely(realsize < size))
		return __NULL;

	ptr = a->malloc_backend(a, realsize);
	if (!ptr)
		return __NULL;

	metadata = ptr;
	metadata->size = realsize;
	metadata->base = ptr;

	return (void *) ((__uptr) ptr + METADATA_IFMALLOC_SIZE_POW2);
}

void *uk_realloc_ifmalloc(struct uk_alloc *a, void *ptr, __sz size)
{
	void *retptr;
	__sz mallocsize;

	UK_ASSERT(a);
	if (!ptr)
		return uk_malloc_ifmalloc(a, size);

	if (!size) {
		uk_free_ifmalloc(a, ptr);
		return __NULL;
	}

	retptr = uk_malloc_ifmalloc(a, size);
	if (!retptr)
		return __NULL;

	mallocsize = uk_getmallocsize_ifmalloc(ptr);

	memcpy(retptr, ptr, MIN(size, mallocsize));

	uk_free_ifmalloc(a, ptr);
	return retptr;
}

int uk_posix_memalign_ifmalloc(struct uk_alloc *a,
				     void **memptr, __sz align, __sz size)
{
	struct metadata_ifmalloc *metadata;
	__sz realsize, padding;
	__uptr intptr;

	UK_ASSERT(a);
	if (((align - 1) & align) != 0
	    || align < sizeof(void *))
		return EINVAL;

	/* Leave memptr untouched. See comment in uk_posix_memalign_ifpages. */
	if (!size)
		return EINVAL;

	/* Store size information preceding the memory block. Since we return
	 * pointers aligned at `align` we need to reserve at least that much
	 * space for the size information.
	 */
	if (align < METADATA_IFMALLOC_SIZE_POW2) {
		align = METADATA_IFMALLOC_SIZE_POW2;
		padding = 0;
	} else {
		padding = METADATA_IFMALLOC_SIZE_POW2;
	}

	realsize = size + padding + align;

	/* check for overflow */
	if (unlikely(realsize < size))
		return ENOMEM;

	intptr = (__uptr) a->malloc_backend(a, realsize);

	if (!intptr)
		return ENOMEM;

	*memptr = (void *) ALIGN_UP(intptr + METADATA_IFMALLOC_SIZE_POW2,
				    (__uptr) align);

	metadata = uk_get_metadata_ifmalloc(*memptr);

	/* check for underflow */
	UK_ASSERT(intptr <= (__uptr) metadata);

	metadata->size = realsize;
	metadata->base = (void *) intptr;

	return 0;
}

#endif

void uk_pfree_compat(struct uk_alloc *a, void *ptr,
		     unsigned long num_pages __unused)
{
	UK_ASSERT(a);

	/* if the object is not page aligned it was clearly not from us */
	UK_ASSERT(page_off(ptr) == 0);

	uk_free(a, ptr);
}

void *uk_palloc_compat(struct uk_alloc *a, unsigned long num_pages)
{
	void *ptr;

	UK_ASSERT(a);

	/* check for overflow */
	if (num_pages > (~(__sz)0)/__PAGE_SIZE)
		return __NULL;

	if (uk_posix_memalign(a, &ptr, __PAGE_SIZE, num_pages * __PAGE_SIZE))
		return __NULL;

	return ptr;
}

void *uk_realloc_compat(struct uk_alloc *a, void *ptr, __sz size)
{
	void *retptr;

	UK_ASSERT(a);
	if (!ptr)
		return uk_malloc(a, size);

	if (!size) {
		uk_free(a, ptr);
		return __NULL;
	}

	retptr = uk_malloc(a, size);
	if (!retptr)
		return __NULL;

	memcpy(retptr, ptr, size);

	uk_free(a, ptr);
	return retptr;
}

void *uk_calloc_compat(struct uk_alloc *a, __sz nmemb, __sz size)
{
	void *ptr;
	__sz tlen = nmemb * size;

	/* check for overflow */
	if (nmemb > (~(__sz)0)/size)
		return __NULL;

	UK_ASSERT(a);
	ptr = uk_malloc(a, tlen);
	if (!ptr)
		return __NULL;

	memset(ptr, 0, tlen);
	return ptr;
}

void *uk_memalign_compat(struct uk_alloc *a, __sz align, __sz size)
{
	void *ptr;

	UK_ASSERT(a);
	if (uk_posix_memalign(a, &ptr, align, size) != 0)
		return __NULL;

	return ptr;
}

long uk_alloc_pmaxalloc_compat(struct uk_alloc *a)
{
	__ssz mem;

	UK_ASSERT(a);

	mem = uk_alloc_maxalloc(a);
	if (mem < 0)
		return (long) mem;

	return (long) (mem >> __PAGE_SHIFT);
}

long uk_alloc_pavailmem_compat(struct uk_alloc *a)
{
	__ssz mem;

	UK_ASSERT(a);

	mem = uk_alloc_availmem(a);
	if (mem < 0)
		return (long) mem;

	return (long) (mem >> __PAGE_SHIFT);
}

__sz uk_alloc_availmem_total(void)
{
	struct uk_alloc *a;
	__ssz availmem;
	__sz total;

	total = 0;
	uk_alloc_foreach(a) {
		availmem = uk_alloc_availmem(a);
		if (availmem > 0)
			total += availmem;
	}
	return total;
}

unsigned long uk_alloc_pavailmem_total(void)
{
	struct uk_alloc *a;
	long pavailmem;
	unsigned long total;

	total = 0;
	uk_alloc_foreach(a) {
		pavailmem = uk_alloc_pavailmem(a);
		if (pavailmem > 0)
			total += pavailmem;
	}
	return total;
}
