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

#if CONFIG_LIBUKALLOC_IFPAGES
static void *uk_get_real_start(const void *ptr)
{
	void *intptr;

	/* a ptr less or equal to page size
	 * would mean that the actual allocated
	 * object started at 0x0, so it was NULL
	 */
	UK_ASSERT((uintptr_t) ptr > __PAGE_SIZE);

	intptr = (void *) ALIGN_DOWN((uintptr_t) ptr,
				     (uintptr_t) __PAGE_SIZE);
	if (intptr == ptr) {
		/* special case: the memory was page-aligned.
		 * In this case the size information lies at the start of the
		 * previous page, with the rest of that page unused.
		 */
		intptr -= __PAGE_SIZE;
	}
	return intptr;
}

static size_t uk_getmallocsize(const void *ptr)
{
	size_t *intptr = uk_get_real_start(ptr);
	size_t mallocsize = __PAGE_SIZE << (*intptr);

	if (((uintptr_t) ptr & (~__PAGE_MASK)) == 0) {
		/*
		 * special case: the memory was page-aligned
		 * In this case the allocated size should not account for the
		 * previous page which was used for storing the order
		 */
		mallocsize -= __PAGE_SIZE;
	} else {
		/*
		 * If pointer is not page aligned it means the header is
		 * on the same page. This will break if metadata size increases
		 */
		mallocsize -= sizeof(*intptr);
	}

	return mallocsize;
}

/* return the smallest order (1<<order pages) that can fit size bytes */
static inline size_t uk_alloc_size_to_order(size_t size)
{
	size_t order = 0;

	while ((__PAGE_SIZE << order) < size)
		order++;
	return order;
}

void *uk_malloc_ifpages(struct uk_alloc *a, size_t size)
{
	uintptr_t intptr;
	size_t order;
	size_t realsize = sizeof(order) + size;

	UK_ASSERT(a);
	if (!size)
		return NULL;

	order = uk_alloc_size_to_order(realsize);
	intptr = (uintptr_t)uk_palloc(a, order);

	if (!intptr)
		return NULL;

	*(size_t *)intptr = order;
	return (void *)(intptr + sizeof(order));
}

void uk_free_ifpages(struct uk_alloc *a, void *ptr)
{
	size_t *intptr;

	UK_ASSERT(a);
	if (!ptr)
		return;

	intptr = uk_get_real_start(ptr);
	uk_pfree(a, intptr, *intptr);
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
	uintptr_t *intptr;
	size_t realsize;
	size_t order;

	UK_ASSERT(a);
	if (((align - 1) & align) != 0
	    || (align % sizeof(void *)) != 0
	    || (align > __PAGE_SIZE))
		return EINVAL;

	if (!size) {
		*memptr = NULL;
		return EINVAL;
	}

	/* For page-aligned memory blocks, the size information is not stored
	 * immediately preceding the memory block, but instead at the
	 * beginning of the page preceeding the memory handed out via malloc.
	 */
	if (align == __PAGE_SIZE)
		realsize = ALIGN_UP(size + __PAGE_SIZE, align);
	else
		realsize = ALIGN_UP(size + sizeof(order), align);

	order = uk_alloc_size_to_order(realsize);
	intptr = uk_palloc(a, order);

	if (!intptr)
		return ENOMEM;

	*(size_t *)intptr = order;
	*memptr = (void *) ALIGN_UP((uintptr_t)intptr + sizeof(order), align);
	return 0;
}

#endif

void *uk_calloc_compat(struct uk_alloc *a, size_t nmemb, size_t size)
{
	void *ptr;
	size_t tlen = nmemb * size;

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
