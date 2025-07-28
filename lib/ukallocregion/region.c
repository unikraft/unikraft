/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Hugo Lefeuvre <hugo.lefeuvre@neclab.eu>
 *          Sergiu Moga <sergiu@unikraft.io>
 *
 * Copyright (c) 2020, NEC Laboratories Europe GmbH, NEC Corporation,
 *                     All rights reserved.
 * Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
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

#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>
#include <uk/allocregion.h>
#include <uk/alloc_impl.h>
#include <uk/page.h>	/* round_pgup() */

struct uk_allocregion {
	struct uk_alloc a;
	void *heap_pos;
	void *heap_base;
};

static inline
struct uk_allocregion *ukalloc2allocregion(struct uk_alloc *a)
{
	return __containerof(a, struct uk_allocregion, a);
}

static inline
void allocregion_do_bump(struct uk_allocregion *ar, size_t size, size_t align)
{
	/* return aligned pointers: this is a requirement for some
	 * embedded systems archs, and more generally good for performance
	 */
	ar->heap_pos = (void *)ALIGN_DOWN((uintptr_t)ar->heap_pos - size,
					  (uintptr_t)align);
}

void *uk_allocregion_bump(struct uk_allocregion *ar, size_t size)
{
	UK_ASSERT(ar);

	if (unlikely(!size))
		return NULL;

	if (unlikely(uk_allocregion_availmem(ar) < size)) {
		uk_alloc_stats_count_enomem(&ar->a, size);
		return NULL;
	}

	allocregion_do_bump(ar, size, sizeof(void *));
	uk_alloc_stats_count_alloc(&ar->a, ar->heap_pos, size);
	return ar->heap_pos;
}

static void *allocregion_malloc(struct uk_alloc *a, size_t size)
{
	UK_ASSERT(a);
	return uk_allocregion_bump((struct uk_allocregion *)&a->priv, size);
}

static int allocregion_posix_memalign(struct uk_alloc *a, void **memptr,
				      size_t align, size_t size)
{
	struct uk_allocregion *ar = ukalloc2allocregion(a);

	UK_ASSERT(a);

	/* align must be a power of two */
	UK_ASSERT(((align - 1) & align) == 0);

	/* align must be larger than pointer size */
	UK_ASSERT((align % sizeof(void *)) == 0);

	if (unlikely(!size)) {
		*memptr = NULL;
		return EINVAL;
	}

	if (unlikely(uk_allocregion_availmem(ar) < size)) {
		uk_alloc_stats_count_enomem(&ar->a, size);
		*memptr = NULL;
		return ENOMEM;
	}

	allocregion_do_bump(ar, size, align);
	*memptr = ar->heap_pos;

	uk_alloc_stats_count_alloc(&ar->a, b->heap_pos, size);
	return 0;
}

static void allocregion_free(struct uk_alloc *a __maybe_unused,
			     void *ptr __maybe_unused)
{
	uk_pr_debug("%p: Releasing of memory is not supported by ukallocregion\n",
		    a);

	/* Count a free operation but do not release memory from stats */
	uk_alloc_stats_count_free(a, ptr, 0);
}

/* NOTE: We use `uk_allocregion_availmem()` for `maxalloc` and `availmem`
 *       because it is the same for this region allocator
 */
size_t uk_allocregion_availmem(const struct uk_allocregion *ar)
{
	UK_ASSERT(ar);
	return (uintptr_t)ar->heap_pos - (uintptr_t)ar->heap_base;
}

static ssize_t allocregion_availmem(struct uk_alloc *a)
{
	UK_ASSERT(a);
	return (ssize_t)uk_allocregion_availmem(ukalloc2allocregion(a));
}

static int allocregion_addmem(struct uk_alloc *a __unused,
			      void *base __unused, size_t size __unused)
{
	/* TODO: support multiple regions */
	uk_pr_debug("%p: ukallocregion does not support multiple memory regions\n",
		    a);
	return 0;
}

struct uk_allocregion *uk_allocregion_init(void *base, size_t len)
{
	struct uk_allocregion *ar;

	/* enough space for allocator available? */
	if (unlikely(sizeof(*ar) > len)) {
		uk_pr_err("Not enough space for allocator\n");
		return NULL;
	}

	if (unlikely(!IS_ALIGNED((uintptr_t)base, __alignof(*ar)))) {
		uk_pr_err("Heap base requires alignment %lu\n", __alignof(*ar));
		return NULL;
	}

	ar = (struct uk_allocregion *)base;

	uk_pr_info("Initialize allocregion allocator @ %p, len %lu\n", ar, len);

	ar->heap_pos  = (void *)((uintptr_t)base + len);
	ar->heap_base = (void *)((uintptr_t)base + sizeof(*ar));

	/* use exclusively "compat" wrappers for calloc, realloc, memalign,
	 * palloc and pfree as those do not add additional metadata.
	 */
	uk_alloc_init_malloc(&ar->a, allocregion_malloc, uk_calloc_compat,
			     uk_realloc_compat, allocregion_free,
			     allocregion_posix_memalign,
			     uk_memalign_compat, allocregion_availmem,
			     allocregion_availmem,
			     allocregion_addmem);

	return ar;
}
