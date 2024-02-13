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
	struct uk_alloc *parent_a;
#if CONFIG_LIBUKVMEM
	__sz premapped_len;
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
	__sz alloc_size, premapped_len __maybe_unused;
	__vaddr_t sp __maybe_unused;
	struct uk_allocstack *sa;
	int rc __maybe_unused;
	__vaddr_t vaddr;

	UK_ASSERT(a);

	if (unlikely(!size))
		return NULL;

	sa = to_allocstack(a);

#if CONFIG_LIBUKALLOCSTACK_PAGE_GUARDS
	alloc_size = PAGE_ALIGN_UP(size);
	vaddr = __VADDR_ANY;
	rc = uk_vma_map_stack(sa->vas,
			      &vaddr,
			      alloc_size,
			      UK_VMA_MAP_UNINITIALIZED,
			      NULL,
			      sa->premapped_len);
	if (unlikely(rc)) {
		uk_pr_err("Failed to create stack VMA of size 0x%lx\n",
			  alloc_size);
		return NULL;
	}
#else /* !CONFIG_LIBUKALLOCSTACK_PAGE_GUARDS */
	alloc_size = size;
	vaddr = (__vaddr_t)uk_memalign(sa->parent_a,
				       UKARCH_SP_ALIGN, alloc_size);
	if (unlikely(!vaddr)) {
		uk_pr_err("Failed to allocate stack of sizegth 0x%lx from allocator %p\n",
			  alloc_size, sa->parent_a);
		return NULL;
	}

#if CONFIG_LIBUKVMEM
	premapped_len = sa->premapped_len > alloc_size ? alloc_size :
						       sa->premapped_len;
	sp = ukarch_gen_sp(vaddr, alloc_size);
	rc = uk_vma_advise(sa->vas,
			   PAGE_ALIGN_DOWN((__vaddr_t)
					   (PAGE_ALIGN_UP(sp) - premapped_len)),
			   premapped_len,
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

	if (unlikely(!ptr))
		return;

	sa = to_allocstack(a);

#if CONFIG_LIBUKALLOCSTACK_PAGE_GUARDS
	vma = uk_vma_find(sa->vas, (__vaddr_t)ptr);
	vaddr = vma->start;
	/* Crash if pointer given to free is not part of any VMA. This would
	 * normally lead to a heap corruption.
	 */
	UK_BUGON(!vma);

	rc = uk_vma_unmap(sa->vas,
			  vaddr,
			  vma->end - vma->start,
			  0);
	UK_BUGON(rc);
#else /* !CONFIG_LIBUKALLOCSTACK_PAGE_GUARDS */
	uk_free(sa->parent_a, ptr);
#endif /* !CONFIG_LIBUKALLOCSTACK_PAGE_GUARDS */
}

static void *stack_memalign(struct uk_alloc *a, __sz align, __sz size)
{
	__sz alloc_size, premapped_len __maybe_unused;
	__vaddr_t sp __maybe_unused;
	struct uk_allocstack *sa;
	int rc __maybe_unused;
	__vaddr_t vaddr;

	UK_ASSERT(a);
	UK_ASSERT(align >= UKARCH_SP_ALIGN);
	UK_ASSERT(IS_ALIGNED(align,  UKARCH_SP_ALIGN));
#if CONFIG_LIBUKALLOCSTACK_PAGE_GUARDS
	/* If using page guards then we allocate stack VMA's instead. So,
	 * the return address will always be page-aligned and the maximum
	 * allowed alignment is PAGE_SIZE because we cannot satisfy higher
	 * alignments and smaller than PAGE_SIZE alignments must be a
	 * divisor of PAGE_SIZE (memalign alignments are always powers of 2 and
	 * all powers of 2 below PAGE_SIZE are divisors of PAGE_SIZE).
	 * One could argue we could do the following:
	 * alloc_size = PAGE_ALIGN_UP(size) + align
	 * ... uk_vma_map_stack ... // create the stack VMA of size alloc_size
	 * vaddr = ALIGN_UP(vaddr, align)
	 *
	 * And the above will work. However, this implies that the stack VMA
	 * will be a bit bigger to satisfy the ALIGN_UP and thus have more room
	 * before the page guards are reached. This is still fine but results
	 * in weird semantics being applied that the caller would somehow have
	 * to be aware of. It is not worth sacrificing expectations in favor
	 * of an exceptional case that may never happen: as of this moment of
	 * writing, I cannot see any case where one would need stacks with
	 * an alignment higher than PAGE_SIZE.
	 * Therefore, make this constraint clear when using page guards
	 * configuration: alignment must not be bigger than PAGE_SIZE.
	 */
	UK_ASSERT(align <= PAGE_SIZE);
#endif /* CONFIG_LIBUKALLOCSTACK_PAGE_GUARDS */

	if (unlikely(!size))
		return NULL;

	sa = to_allocstack(a);

#if CONFIG_LIBUKALLOCSTACK_PAGE_GUARDS
	alloc_size = PAGE_ALIGN_UP(size);
	vaddr = __VADDR_ANY;
	rc = uk_vma_map_stack(sa->vas,
			      &vaddr,
			      alloc_size,
			      UK_VMA_MAP_UNINITIALIZED,
			      NULL,
			      sa->premapped_len);
	if (unlikely(rc)) {
		uk_pr_err("Failed to create stack VMA of size 0x%lx\n",
			  alloc_size);
		return NULL;
	}
#else /* !CONFIG_LIBUKALLOCSTACK_PAGE_GUARDS */
	alloc_size = size;
	vaddr = (__vaddr_t)uk_memalign(sa->parent_a, align, alloc_size);
	if (unlikely(!vaddr)) {
		uk_pr_err("Failed to allocate stack of sizegth 0x%lx from allocator %p\n",
			  alloc_size, sa->parent_a);
		return NULL;
	}
#if CONFIG_LIBUKVMEM
	premapped_len = sa->premapped_len > alloc_size ? alloc_size :
						       sa->premapped_len;
	sp = ukarch_gen_sp(vaddr, alloc_size);
	rc = uk_vma_advise(sa->vas,
			   PAGE_ALIGN_DOWN((__vaddr_t)
					   (PAGE_ALIGN_UP(sp) - premapped_len)),
			   premapped_len,
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

	if (unlikely(!nmemb || !size))
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
				     __sz premapped_len
#endif /* CONFIG_LIBUKVMEM */
				     )
{
	struct uk_allocstack *sa;

	UK_ASSERT(a);
#if CONFIG_LIBUKVMEM
	UK_ASSERT(vas);
	UK_ASSERT(PAGE_ALIGNED(premapped_len));
#endif /* CONFIG_LIBUKVMEM */

	sa = uk_malloc(a, sizeof(*sa));
	if (unlikely(!sa)) {
		uk_pr_err("Failed to allocate stack allocator\n");
		return NULL;
	}
#if CONFIG_LIBUKVMEM
	sa->vas = vas;
	sa->premapped_len = premapped_len;
#endif /* CONFIG_LIBUKVMEM */
	sa->parent_a = a;

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
