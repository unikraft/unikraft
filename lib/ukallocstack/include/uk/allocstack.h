/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UKALLOCSTACK_H__
#define __UKALLOCSTACK_H__

#include <uk/alloc.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize a stack allocator.
 * Given a parent allocator for the allocation of internal structures, return
 * an allocator that allocates consistent stacks of a given configuration.
 * Notably, if CONFIG_LIBUKVMEM is enabled, the Virtual Address Space that will
 * contain the stacks is required as an argument and, additionaly, the initial,
 * pre-paged-in, length of the stack may be given (may be 0). The latter may
 * be used to reduce page faults on initial stack access.
 *
 * NOTE: When using the page guards configuration, the maximum allowed
 * alignment for functions such as memalign/posix_memalign is PAGE_SIZE.
 * See comment in `stack_memalign()`. Additionaly, just like any `memalign()` or
 * `posix_memalign()` calls, the alignment must be a power of 2.
 *
 * @param a
 *    The parent allocator used to allocate internal structures
 * @param vas
 *    The Virtual Address Space that manages the virtual mapping's of these
 *    stacks
 * @initial_size
 *    The initial paged-in size from top to bottom, i.e. last
 *    initial_size / PAGE_SIZE pages will be paged-in.
 *     E.g.:
 *     - initial_size == PAGE_SIZE * 2 => the top two pages (stacks grow top to
 *     bottom) of returned stacks will be paged-in and thus no page faults on
 *     them
 *     - initial_size == 0 => no page of any of the allocated stacks is
 *     pre-mapped
 *     - initial_size == <SOME_STACK_SIZE> then a uk_malloc(<SOME_STACK_SIZE>)
 *     would return a fully paged-in stack and a
 *     uk_malloc(<SOME_STACK_SIZE> + PAGE_SIZE) would return a stack whose all
 *     pages, except the first one, are paged-in. Consequently if allocating
 *     a stack whose size is smaller than initial_size, it means the whole stack
 *     will be paged-in.
 * @return
 *     Returns the stack allocator or NULL on error
 */
struct uk_alloc *uk_allocstack_init(struct uk_alloc *a
#if CONFIG_LIBUKVMEM
				     , struct uk_vas __maybe_unused *vas,
				     __sz initial_size
#endif /* CONFIG_LIBUKVMEM */
				     );

/**
 * Terminate a stack allocator created with `uk_allocstack_init()`.
 *
 * @param a
 *    The stack allocator returned by `uk_allocstack_init()`
 */
void uk_allocstack_term(struct uk_alloc *a);

#ifdef __cplusplus
}
#endif

#endif /* __UKALLOCSTACK_H__ */
