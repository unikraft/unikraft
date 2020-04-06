/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Florian Schmidt <florian.schmidt@neclab.eu>
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
 *
 * THIS HEADER MAY NOT BE EXTRACTED OR MODIFIED IN ANY WAY.
 */

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

#include <errno.h>
#include <string.h>
#include <uk/alloc_impl.h>
#include <uk/config.h>
#include <uk/essentials.h>
#include <uk/assert.h>
#include <uk/arch/limits.h>

#define size_to_num_pages(size) \
	(ALIGN_UP((unsigned long)(size), __PAGE_SIZE) / __PAGE_SIZE)
#define page_off(x) ((unsigned long)(x) & (__PAGE_SIZE - 1))

static struct uk_alloc *uk_alloc_head;

int uk_alloc_register(struct uk_alloc *a)
{
	struct uk_alloc *this = uk_alloc_head;

	if (!uk_alloc_head) {
		uk_alloc_head = a;
		a->next = NULL;
		return 0;
	}

	while (this && this->next)
		this = this->next;
	this->next = a;
	a->next = NULL;
	return 0;
}

struct uk_alloc *uk_alloc_get_default(void)
{
	return uk_alloc_head;
}

int uk_alloc_set_default(struct uk_alloc *a)
{
	struct uk_alloc *head, *this, *prev;

	head = uk_alloc_get_default();

	if (a == head)
		return 0;

	if (!head) {
		uk_alloc_head = a;
		return 0;
	}

	this = head;
	while (this->next) {
		prev = this;
		this = this->next;
		if (a == this) {
			prev->next = this->next;
			this->next = head->next;
			head = this;
			return 0;
		}
	}

	/* a is not registered yet. Add in front of the queue. */
	a->next = head;
	uk_alloc_head = a;
	return 0;
}

struct metadata_ifpages {
	unsigned long	num_pages;
	void		*base;
};

/* METADATA_IFPAGES_SIZE_POW2 is a power of two larger or equal to
 * sizeof(struct metadata_ifpages). The optimal value for this is
 * architecture specific. If the actual sizeof(struct metadata_ifpages) is
 * smaller, we will just waste a few negligible bytes. If it is larger, the
 * compile time assertion will abort the compilation and this value will have
 * to be increased.
 */
#define METADATA_IFPAGES_SIZE_POW2 16
UK_CTASSERT(!(sizeof(struct metadata_ifpages) > METADATA_IFPAGES_SIZE_POW2));

static struct metadata_ifpages *uk_get_metadata(const void *ptr)
{
	uintptr_t metadata;

	/* a ptr less or equal to page size would mean that the actual allocated
	 * object started at 0x0, so it was NULL.
	 * any value between page size and page size + size of metadata would
	 * also imply that the actual allocated object started at 0x0 because
	 * we need space to store metadata.
	 */
	UK_ASSERT((uintptr_t) ptr >= __PAGE_SIZE +
		  sizeof(struct metadata_ifpages));

	metadata = ALIGN_DOWN((uintptr_t) ptr, (uintptr_t) __PAGE_SIZE);
	if (metadata == (uintptr_t) ptr) {
		/* special case: the memory was page-aligned.
		 * In this case the metadata lies at the start of the
		 * previous page, with the rest of that page unused.
		 */
		metadata -= __PAGE_SIZE;
	}

	return (struct metadata_ifpages *) metadata;
}

static size_t uk_getmallocsize(const void *ptr)
{
	struct metadata_ifpages *metadata = uk_get_metadata(ptr);

	return (size_t)metadata->base + (size_t)(metadata->num_pages) *
	       __PAGE_SIZE - (size_t)ptr;
}

void *uk_malloc_ifpages(struct uk_alloc *a, size_t size)
{
	uintptr_t intptr;
	unsigned long num_pages;
	struct metadata_ifpages *metadata;
	size_t realsize = sizeof(*metadata) + size;

	UK_ASSERT(a);
	/* check for invalid size and overflow */
	if (!size || realsize < size)
		return NULL;

	num_pages = size_to_num_pages(realsize);
	intptr = (uintptr_t)uk_palloc(a, num_pages);

	if (!intptr)
		return NULL;

	metadata = (struct metadata_ifpages *) intptr;
	metadata->num_pages = num_pages;
	metadata->base = (void *) intptr;

	return (void *)(intptr + sizeof(*metadata));
}

void uk_free_ifpages(struct uk_alloc *a, void *ptr)
{
	struct metadata_ifpages *metadata;

	UK_ASSERT(a);
	if (!ptr)
		return;

	metadata = uk_get_metadata(ptr);

	UK_ASSERT(metadata->base != NULL);
	UK_ASSERT(metadata->num_pages != 0);
	uk_pfree(a, metadata->base, metadata->num_pages);
}

void *uk_realloc_ifpages(struct uk_alloc *a, void *ptr, size_t size)
{
	void *retptr;
	size_t mallocsize;

	UK_ASSERT(a);
	if (!ptr)
		return uk_malloc_ifpages(a, size);

	if (ptr && !size) {
		uk_free_ifpages(a, ptr);
		return NULL;
	}

	retptr = uk_malloc_ifpages(a, size);
	if (!retptr)
		return NULL;

	mallocsize = uk_getmallocsize(ptr);

	if (size < mallocsize)
		memcpy(retptr, ptr, size);
	else
		memcpy(retptr, ptr, mallocsize);

	uk_free_ifpages(a, ptr);
	return retptr;
}

int uk_posix_memalign_ifpages(struct uk_alloc *a,
				void **memptr, size_t align, size_t size)
{
	struct metadata_ifpages *metadata;
	unsigned long num_pages;
	uintptr_t intptr;
	size_t realsize, padding;

	UK_ASSERT(a);
	if (((align - 1) & align) != 0
	    || (align % sizeof(void *)) != 0)
		return EINVAL;

	if (!size) {
		*memptr = NULL;
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

	/* In addition to metadata space, allocate `align` more bytes in
	 * order to be sure to find an aligned pointer preceding `size` bytes.
	 */
	realsize = size + padding + align;

	/* check for overflow */
	if (realsize < size)
		return EINVAL;

	num_pages = size_to_num_pages(realsize);
	intptr = (uintptr_t) uk_palloc(a, num_pages);

	if (!intptr)
		return ENOMEM;

	*memptr = (void *) ALIGN_UP(intptr + sizeof(*metadata),
				    (uintptr_t) align);

	metadata = uk_get_metadata(*memptr);

	/* check for underflow (should not happen) */
	UK_ASSERT(intptr <= (uintptr_t) metadata);

	metadata->num_pages = num_pages;
	metadata->base = (void *) intptr;

	return 0;
}

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
	if (num_pages > (~(size_t)0)/__PAGE_SIZE)
		return NULL;

	if (uk_posix_memalign(a, &ptr, __PAGE_SIZE, num_pages * __PAGE_SIZE))
		return NULL;

	return ptr;
}

void *uk_realloc_compat(struct uk_alloc *a, void *ptr, size_t size)
{
	void *retptr;

	UK_ASSERT(a);
	if (!ptr)
		return uk_malloc(a, size);

	if (ptr && !size) {
		uk_free(a, ptr);
		return NULL;
	}

	retptr = uk_malloc(a, size);
	if (!retptr)
		return NULL;

	memcpy(retptr, ptr, size);

	uk_free(a, ptr);
	return retptr;
}

void *uk_calloc_compat(struct uk_alloc *a, size_t nmemb, size_t size)
{
	void *ptr;
	size_t tlen = nmemb * size;

	/* check for overflow */
	if (nmemb > (~(size_t)0)/size)
		return NULL;

	UK_ASSERT(a);
	ptr = uk_malloc(a, tlen);
	if (!ptr)
		return NULL;

	memset(ptr, 0, tlen);
	return ptr;
}

void *uk_memalign_compat(struct uk_alloc *a, size_t align, size_t size)
{
	void *ptr;

	UK_ASSERT(a);
	if (uk_posix_memalign(a, &ptr, align, size) != 0)
		return NULL;

	return ptr;
}
