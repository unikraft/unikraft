/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Simon Kuenzer <simon.kuenzer@neclab.eu>
 *          Florian Schmidt <florian.schmidt@neclab.eu>
 *
 * Copyright (c) 2019, NEC Europe Ltd., NEC Corporation. All rights reserved.
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

/*
 * NOTE: This header should only be used by actual allocator implementations.
 *       These functions are not part of the public ukalloc API.
 */

#ifndef __UK_ALLOC_IMPL_H__
#define __UK_ALLOC_IMPL_H__

#include <uk/alloc.h>

#ifdef __cplusplus
extern "C" {
#endif

int uk_alloc_register(struct uk_alloc *a);

/**
 * Compatibility functions that can be used by allocator implementations to
 * fill out callback functions in `struct uk_alloc` when just a subset of the
 * API functionality is actually implemented.
 */

/* Functions that can be used by allocators that implement palloc(), pfree() only */
void *uk_malloc_ifpages(struct uk_alloc *a, size_t size);
void *uk_realloc_ifpages(struct uk_alloc *a, void *ptr, size_t size);
int uk_posix_memalign_ifpages(struct uk_alloc *a, void **memptr,
				size_t align, size_t size);
void uk_free_ifpages(struct uk_alloc *a, void *ptr);

#if CONFIG_LIBUKALLOC_IFMALLOC
void *uk_malloc_ifmalloc(struct uk_alloc *a, size_t size);
void *uk_realloc_ifmalloc(struct uk_alloc *a, void *ptr, size_t size);
int uk_posix_memalign_ifmalloc(struct uk_alloc *a, void **memptr,
				     size_t align, size_t size);
void uk_free_ifmalloc(struct uk_alloc *a, void *ptr);
#endif

/* Functionality that is provided based on malloc() and posix_memalign() */
void *uk_calloc_compat(struct uk_alloc *a, size_t num, size_t len);
void *uk_realloc_compat(struct uk_alloc *a, void *ptr, size_t size);
void *uk_memalign_compat(struct uk_alloc *a, size_t align, size_t len);
void *uk_palloc_compat(struct uk_alloc *a, unsigned long num_pages);
void uk_pfree_compat(struct uk_alloc *a, void *ptr, unsigned long num_pages);

/* Shortcut for doing a registration of an allocator that does not implement
 * palloc() or pfree()
 */
#define uk_alloc_init_malloc(a, malloc_f, calloc_f, realloc_f, free_f,	\
				posix_memalign_f, memalign_f, addmem_f) \
	do {								\
		(a)->malloc         = (malloc_f);			\
		(a)->calloc         = (calloc_f);			\
		(a)->realloc        = (realloc_f);			\
		(a)->posix_memalign = (posix_memalign_f);		\
		(a)->memalign       = (memalign_f);			\
		(a)->free           = (free_f);				\
		(a)->palloc         = uk_palloc_compat;			\
		(a)->pfree          = uk_pfree_compat;			\
		(a)->addmem         = (addmem_f);			\
									\
		uk_alloc_register((a));					\
	} while (0)

#if CONFIG_LIBUKALLOC_IFMALLOC
#define uk_alloc_init_malloc_ifmalloc(a, malloc_f, free_f, addmem_f) 	\
	do {								\
		(a)->malloc         = uk_malloc_ifmalloc;		\
		(a)->calloc         = uk_calloc_compat;			\
		(a)->realloc        = uk_realloc_ifmalloc;		\
		(a)->posix_memalign = uk_posix_memalign_ifmalloc;	\
		(a)->memalign       = uk_memalign_compat;		\
		(a)->malloc_backend = (malloc_f);			\
		(a)->free_backend   = (free_f);				\
		(a)->free           = uk_free_ifmalloc;			\
		(a)->palloc         = uk_palloc_compat;			\
		(a)->pfree          = uk_pfree_compat;			\
		(a)->addmem         = (addmem_f);			\
									\
		uk_alloc_register((a));					\
	} while (0)
#endif

/* Shortcut for doing a registration of an allocator that only
 * implements palloc(), pfree(), addmem()
 */
#define uk_alloc_init_palloc(a, palloc_func, pfree_func, addmem_func)	\
	do {								\
		(a)->malloc         = uk_malloc_ifpages;		\
		(a)->calloc         = uk_calloc_compat;			\
		(a)->realloc        = uk_realloc_ifpages;		\
		(a)->posix_memalign = uk_posix_memalign_ifpages;	\
		(a)->memalign       = uk_memalign_compat;		\
		(a)->free           = uk_free_ifpages;			\
		(a)->palloc         = (palloc_func);			\
		(a)->pfree          = (pfree_func);			\
		(a)->addmem         = (addmem_func);			\
									\
		uk_alloc_register((a));					\
	} while (0)

#ifdef __cplusplus
}
#endif

#endif /* __UK_ALLOC_IMPL_H__ */
