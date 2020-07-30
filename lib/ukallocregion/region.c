/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Hugo Lefeuvre <hugo.lefeuvre@neclab.eu>
 *
 * Copyright (c) 2020, NEC Laboratories Europe GmbH, NEC Corporation,
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

/* ukallocregion is a minimalist region implementation.
 *
 * Note that deallocation is not supported. This makes sense because regions
 * only allow for deallocation at region-granularity. In our case, this would
 * imply the freeing of the entire heap, which is generally not possible.
 *
 * Obviously, the lack of deallocation support makes ukallocregion a fairly bad
 * general-purpose allocator. This allocator is interesting in that it offers
 * maximum speed allocation and deallocation (no bookkeeping). It can be used as
 * a baseline for measurements (e.g., boot time) or as a first-level allocator
 * in a nested context.
 *
 * Refer to Gay & Aiken, `Memory management with explicit regions' (PLDI'98) for
 * an introduction to region-based memory management.
 */

#include <uk/allocregion.h>
#include <uk/alloc_impl.h>
#include <uk/page.h>	/* round_pgup() */

struct uk_allocregion {
	void *heap_top;
	void *heap_base;
};

void *uk_allocregion_malloc(struct uk_alloc *a, size_t size)
{
	struct uk_allocregion *b;
	uintptr_t intptr, newbase;

	UK_ASSERT(a != NULL);

	b = (struct uk_allocregion *)&a->priv;

	UK_ASSERT(b != NULL);

	/* return aligned pointers: this is a requirement for some
	 * embedded systems archs, and more generally good for performance
	 */
	intptr = ALIGN_UP(((uintptr_t) b->heap_base),
			     (uintptr_t) sizeof(void *));

	newbase  = intptr + size;
	if (newbase > (uintptr_t) b->heap_top)
		return NULL; /* OOM */

	/* Check for overflow, handle malloc(0) */
	if (newbase <= (uintptr_t) b->heap_base)
		return NULL;

	b->heap_base = (void *)(newbase);

	return (void *) intptr;
}

int uk_allocregion_posix_memalign(struct uk_alloc *a, void **memptr,
					size_t align, size_t size)
{
	struct uk_allocregion *b;
	uintptr_t intptr, newbase;

	UK_ASSERT(a != NULL);

	b = (struct uk_allocregion *)&a->priv;

	UK_ASSERT(b != NULL);

	/* align must be a power of two */
	UK_ASSERT(((align - 1) & align) == 0);

	/* align must be larger than pointer size */
	UK_ASSERT((align % sizeof(void *)) == 0);

	if (!size) {
		*memptr = NULL;
		return EINVAL;
	}

	intptr = ALIGN_UP((uintptr_t) b->heap_base, (uintptr_t) align);

	newbase  = intptr + size;
	if (newbase > (uintptr_t) b->heap_top)
		return ENOMEM; /* out-of-memory */

	/* Check for overflow */
	if (newbase <= (uintptr_t) b->heap_base)
		return EINVAL;

	*memptr = (void *)intptr;
	b->heap_base = (void *)(newbase);

	return 0;
}

void uk_allocregion_free(struct uk_alloc *a __unused, void *ptr __unused)
{
	uk_pr_debug("%p: Releasing of memory is not supported by "
			"ukallocregion\n", a);
}

int uk_allocregion_addmem(struct uk_alloc *a __unused, void *base __unused,
				size_t size __unused)
{
	/* TODO: support multiple regions */
	uk_pr_debug("%p: ukallocregion does not support multiple memory "
			"regions\n", a);
	return 0;
}

struct uk_alloc *uk_allocregion_init(void *base, size_t len)
{
	struct uk_alloc *a;
	struct uk_allocregion *b;
	size_t metalen = sizeof(*a) + sizeof(*b);

	/* TODO: ukallocregion does not support multiple memory regions yet.
	 * Because of the multiboot layout, the first region might be a single
	 * page, so we simply ignore it.
	 */
	if (len <= __PAGE_SIZE)
		return NULL;

	/* enough space for allocator available? */
	if (metalen > len) {
		uk_pr_err("Not enough space for allocator: %"__PRIsz
			  " B required but only %"__PRIuptr" B usable\n",
			  metalen, len);
		return NULL;
	}

	/* store allocator metadata on the heap, just before the memory pool */
	a = (struct uk_alloc *)base;
	b = (struct uk_allocregion *)&a->priv;

	uk_pr_info("Initialize allocregion allocator @ 0x%"
		   __PRIuptr ", len %"__PRIsz"\n", (uintptr_t)a, len);

	b->heap_top  = (void *)((uintptr_t) base + len);
	b->heap_base = (void *)((uintptr_t) base + metalen);

	/* use exclusively "compat" wrappers for calloc, realloc, memalign,
	 * palloc and pfree as those do not add additional metadata.
	 */
	uk_alloc_init_malloc(a, uk_allocregion_malloc, uk_calloc_compat,
				uk_realloc_compat, uk_allocregion_free,
				uk_allocregion_posix_memalign,
				uk_memalign_compat, NULL);

	return a;
}
