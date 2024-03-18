/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <stddef.h>
#include <string.h>
#include <uk/alloc.h>
#include <uk/alloc_impl.h>
#include <uk/arch/ctx.h>
#include <uk/arch/paging.h>
#include <uk/assert.h>
#include <uk/essentials.h>
#if CONFIG_LIBUKVMEM
#include <uk/vmem.h>
#endif /* CONFIG_LIBUKVMEM */

struct uk_allocstack {
	struct uk_alloc a;
	struct uk_alloc *og_a;
#if CONFIG_LIBUKVMEM
	__sz initial_size;
	struct uk_vas *vas;
#endif /* CONFIG_LIBUKVMEM */
};

static struct uk_allocstack *to_allocstack(struct uk_alloc *a)
{
	UK_ASSERT(a);

	return __containerof(a, struct uk_allocstack, a);
}

static void *stack_malloc(struct uk_alloc *a, __sz size)
{
	__vaddr_t sp __maybe_unused;
	struct uk_allocstack *sa;
	int rc __maybe_unused;
	__vaddr_t vaddr;
	__sz real_size;

	UK_ASSERT(a);

	if (!size)
		return NULL;

	sa = to_allocstack(a);

#if CONFIG_LIBUKALLOCSTACK_PAGE_GUARDS
	real_size = PAGE_ALIGN_UP(size) + PAGE_SIZE;
	vaddr = __VADDR_ANY;
	rc = uk_vma_map_stack(sa->vas,
			      &vaddr,
			      real_size,
			      UK_VMA_MAP_UNINITIALIZED,
			      NULL,
			      sa->initial_size);
	if (unlikely(rc)) {
		uk_pr_err("Failed to create stack VMA of size 0x%lx\n",
			  real_size);
		return NULL;
	}
#else /* !CONFIG_LIBUKALLOCSTACK_PAGE_GUARDS */
	real_size = size;
	vaddr = (__vaddr_t)uk_memalign(sa->og_a, UKARCH_SP_ALIGN, real_size);
	if (unlikely(!vaddr)) {
		uk_pr_err("Failed to allocate stack of sizegth 0x%lx from "
			  "allocator %p\n", real_size, sa->og_a);
		return NULL;
	}

#if CONFIG_LIBUKVMEM
	sp = ukarch_gen_sp(vaddr, real_size);
	rc = uk_vma_advise(sa->vas,
			   PAGE_ALIGN_DOWN((__vaddr_t)
					   (PAGE_ALIGN_UP(sp) -
					    PAGE_ALIGN_UP(sa->initial_size))),
			   PAGE_ALIGN_UP(sa->initial_size),
			   UK_VMA_ADV_WILLNEED,
			   UK_VMA_FLAG_UNINITIALIZED);
	if (unlikely(rc)) {
		uk_pr_err("Could not pre-map stack initial size\n");
		return NULL;
	}
#endif /* CONFIG_LIBUKVMEM */
#endif /* !CONFIG_LIBUKALLOCSTACK_PAGE_GUARDS */

	return (void *)vaddr;
}

static void stack_free(struct uk_alloc *a, void *ptr)
{
#if CONFIG_LIBUKALLOCSTACK_PAGE_GUARDS
	const struct uk_vma *vma __maybe_unused;
#endif /* CONFIG_LIBUKALLOCSTACK_PAGE_GUARDS */
	__vaddr_t vaddr __maybe_unused;
	struct uk_allocstack *sa;
	int rc __maybe_unused;

	UK_ASSERT(a);

	if (!ptr)
		return;

	sa = to_allocstack(a);

#if CONFIG_LIBUKALLOCSTACK_PAGE_GUARDS
	vaddr = (__vaddr_t)ptr;
	vma = uk_vma_find(sa->vas, vaddr);
	/* Crash if pointer given to free is not part of any VMA. This would
	 * normally lead to a heap corruption.
	 */
	UK_ASSERT(vma);

	rc = uk_vma_unmap(sa->vas,
			  vaddr,
			  vma->end - vma->start,
			  0);
	UK_ASSERT(!rc);
#else /* !CONFIG_LIBUKALLOCSTACK_PAGE_GUARDS */
	uk_free(sa->og_a, ptr);
#endif /* !CONFIG_LIBUKALLOCSTACK_PAGE_GUARDS */
}

static void *stack_memalign(struct uk_alloc *a, __sz align, __sz size)
{
	__vaddr_t sp __maybe_unused;
	struct uk_allocstack *sa;
	int rc __maybe_unused;
	__vaddr_t vaddr;
	__sz real_size;

	UK_ASSERT(a);
	UK_ASSERT(align >= UKARCH_SP_ALIGN);
	UK_ASSERT(IS_ALIGNED(align,  UKARCH_SP_ALIGN));

	if (!size)
		return NULL;

	sa = to_allocstack(a);

#if CONFIG_LIBUKALLOCSTACK_PAGE_GUARDS
	UK_ASSERT(align <= PAGE_SIZE);

	real_size = PAGE_ALIGN_UP(size) + PAGE_SIZE;
	vaddr = __VADDR_ANY;
	rc = uk_vma_map_stack(sa->vas,
			      &vaddr,
			      real_size,
			      UK_VMA_MAP_UNINITIALIZED,
			      NULL,
			      sa->initial_size);
	if (unlikely(rc)) {
		uk_pr_err("Failed to create stack VMA of size 0x%lx\n",
			  real_size);
		return NULL;
	}
#else /* !CONFIG_LIBUKALLOCSTACK_PAGE_GUARDS */
	real_size = size;
	vaddr = (__vaddr_t)uk_memalign(sa->og_a, align, real_size);
	if (unlikely(!vaddr)) {
		uk_pr_err("Failed to allocate stack of sizegth 0x%lx from "
			  "allocator %p\n", real_size, sa->og_a);
		return NULL;
	}
#if CONFIG_LIBUKVMEM
	sp = ukarch_gen_sp(vaddr, real_size);
	rc = uk_vma_advise(sa->vas,
			   PAGE_ALIGN_DOWN((__vaddr_t)
					   (PAGE_ALIGN_UP(sp) -
					    PAGE_ALIGN_UP(sa->initial_size))),
			   PAGE_ALIGN_UP(sa->initial_size),
			   UK_VMA_ADV_WILLNEED,
			   UK_VMA_FLAG_UNINITIALIZED);
	if (unlikely(rc)) {
		uk_pr_err("Could not pre-map stack initial size\n");
		return NULL;
	}
#endif /* CONFIG_LIBUKVMEM */
#endif /* !CONFIG_LIBUKALLOCSTACK_PAGE_GUARDS */

	return (void *)vaddr;
}

static int stack_posix_memalign(struct uk_alloc *a, void **memptr, __sz align,
				__sz size)
{
	void *addr;

	UK_ASSERT(a);
	UK_ASSERT(memptr);
	UK_ASSERT(align >= UKARCH_SP_ALIGN);
	UK_ASSERT(IS_ALIGNED(align,  UKARCH_SP_ALIGN));
#if CONFIG_LIBUKALLOCSTACK_PAGE_GUARDS
	UK_ASSERT(align <= PAGE_SIZE);
#endif /* CONFIG_LIBUKALLOCSTACK_PAGE_GUARDS */

	addr = stack_memalign(a, align, size);
	if (unlikely(!addr)) {
		*memptr = NULL;
		return -ENOMEM;
	}

	*memptr = addr;
	return 0;
}

static void *stack_calloc(struct uk_alloc *a, __sz nmemb, __sz size)
{
	__sz total;
	void *addr;

	UK_ASSERT(a);

	if (nmemb <= 1 || !size)
		return NULL;

	total = nmemb * size;
	addr = stack_malloc(a, total);
	if (unlikely(!addr))
		return NULL;

	memset(addr, 0, total);

	return addr;
}

struct uk_alloc *uk_allocstack_init(struct uk_alloc *a
#if CONFIG_LIBUKVMEM
				     , struct uk_vas *vas,
				     __sz initial_size
#endif /* CONFIG_LIBUKVMEM */
				     )
{
	struct uk_allocstack *sa;

	UK_ASSERT(a);
#if CONFIG_LIBUKVMEM
	UK_ASSERT(vas);
#endif /* CONFIG_LIBUKVMEM */

	sa = uk_malloc(a, sizeof(*sa));
	if (unlikely(!sa)) {
		uk_pr_err("Failed to allocate stack allocator\n");
		return NULL;
	}
#if CONFIG_LIBUKVMEM
	sa->vas = vas;
	sa->initial_size = initial_size;
#endif /* !CONFIG_LIBUKVMEM */
	sa->og_a = a;

	uk_alloc_init_malloc(&sa->a,
			     stack_malloc,
			     stack_calloc,
			     uk_realloc_compat,
			     stack_free,
			     stack_posix_memalign,
			     stack_memalign,
			     NULL,  /* maxalloc */
			     NULL,  /* availmem */
			     NULL); /* addmem */

	return &sa->a;
}

void uk_allocstack_term(struct uk_alloc *a)
{
	struct uk_allocstack *sa;

	UK_ASSERT(a);

	sa = to_allocstack(a);

	uk_free(sa->og_a, sa);
}
