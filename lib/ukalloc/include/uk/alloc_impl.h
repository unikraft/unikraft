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

/* Functions that can be used by allocators that implement palloc(),
 * pfree() and potentially pavail(), pmaxalloc() only
 */
void *uk_malloc_ifpages(struct uk_alloc *a, __sz size);
void *uk_realloc_ifpages(struct uk_alloc *a, void *ptr, __sz size);
int uk_posix_memalign_ifpages(struct uk_alloc *a, void **memptr,
				__sz align, __sz size);
void uk_free_ifpages(struct uk_alloc *a, void *ptr);
__ssz uk_alloc_availmem_ifpages(struct uk_alloc *a);
__ssz uk_alloc_maxalloc_ifpages(struct uk_alloc *a);

#if CONFIG_LIBUKALLOC_IFMALLOC
void *uk_malloc_ifmalloc(struct uk_alloc *a, __sz size);
void *uk_realloc_ifmalloc(struct uk_alloc *a, void *ptr, __sz size);
int uk_posix_memalign_ifmalloc(struct uk_alloc *a, void **memptr,
				     __sz align, __sz size);
void uk_free_ifmalloc(struct uk_alloc *a, void *ptr);
#endif

/* Functionality that is provided based on malloc() and posix_memalign() */
void *uk_calloc_compat(struct uk_alloc *a, __sz num, __sz len);
void *uk_realloc_compat(struct uk_alloc *a, void *ptr, __sz size);
void *uk_memalign_compat(struct uk_alloc *a, __sz align, __sz len);
void *uk_palloc_compat(struct uk_alloc *a, unsigned long num_pages);
void uk_pfree_compat(struct uk_alloc *a, void *ptr, unsigned long num_pages);
long uk_alloc_pavailmem_compat(struct uk_alloc *a);
long uk_alloc_pmaxalloc_compat(struct uk_alloc *a);

#if CONFIG_LIBUKALLOC_IFSTATS
#include <string.h>
#include <uk/preempt.h>

#if CONFIG_LIBUKALLOC_IFSTATS_GLOBAL
extern struct uk_alloc_stats _uk_alloc_stats_global;
#endif /* CONFIG_LIBUKALLOC_IFSTATS_GLOBAL */

/* NOTE: Please do not use this function directly */
static inline void _uk_alloc_stats_refresh_minmax(struct uk_alloc_stats *stats)
{
	if (stats->cur_nb_allocs > stats->max_nb_allocs)
		stats->max_nb_allocs = stats->cur_nb_allocs;

	if (stats->cur_mem_use > stats->max_mem_use)
		stats->max_mem_use = stats->cur_mem_use;

	if (stats->last_alloc_size) {
		/* Force storing the minimum allocation size
		 * with the first allocation request
		 */
		if ((stats->tot_nb_allocs == 1)
		    || (stats->last_alloc_size < stats->min_alloc_size))
			stats->min_alloc_size = stats->last_alloc_size;
		if (stats->last_alloc_size > stats->max_alloc_size)
			stats->max_alloc_size = stats->last_alloc_size;
	}
}

/* NOTE: Please do not use this function directly */
static inline void _uk_alloc_stats_count_alloc(struct uk_alloc_stats *stats,
					       void *ptr, __sz size)
{
	/* TODO: SMP safety */
	uk_preempt_disable();
	if (likely(ptr)) {
		stats->tot_nb_allocs++;

		stats->cur_nb_allocs++;
		stats->cur_mem_use += size;
		stats->last_alloc_size = size;
		_uk_alloc_stats_refresh_minmax(stats);
	} else {
		stats->nb_enomem++;
	}
	uk_preempt_enable();
}

/* NOTE: Please do not use this function directly */
static inline void _uk_alloc_stats_count_free(struct uk_alloc_stats *stats,
					      void *ptr, __sz size)
{
	/* TODO: SMP safety */
	uk_preempt_disable();
	if (likely(ptr)) {
		stats->tot_nb_frees++;

		stats->cur_nb_allocs--;
		stats->cur_mem_use -= size;
		_uk_alloc_stats_refresh_minmax(stats);
	}
	uk_preempt_enable();
}

#if CONFIG_LIBUKALLOC_IFSTATS_GLOBAL
#define _uk_alloc_stats_global_count_alloc(ptr, size) \
	_uk_alloc_stats_count_alloc(&_uk_alloc_stats_global, (ptr), (size))
#define _uk_alloc_stats_global_count_free(ptr, freed_size) \
	_uk_alloc_stats_count_free(&_uk_alloc_stats_global, (ptr), (freed_size))
#else /* !CONFIG_LIBUKALLOC_IFSTATS_GLOBAL */
#define _uk_alloc_stats_global_count_alloc(ptr, size) \
	do {} while (0)
#define _uk_alloc_stats_global_count_free(ptr, freed_size) \
	do {} while (0)
#endif /* !CONFIG_LIBUKALLOC_IFSTATS_GLOBAL */

/*
 * The following macros should be used to instrument an allocator for
 * statistics:
 */
/* NOTE: If ptr is NULL, an ENOMEM event is counted */
#define uk_alloc_stats_count_alloc(a, ptr, size)			\
	do {								\
		_uk_alloc_stats_count_alloc(&((a)->_stats),		\
					    (ptr), (size));		\
		_uk_alloc_stats_global_count_alloc((ptr), (size));	\
	} while (0)
#define uk_alloc_stats_count_palloc(a, ptr, num_pages)			\
	uk_alloc_stats_count_alloc((a), (ptr),				\
				   ((__sz) (num_pages)) << __PAGE_SHIFT)

#define uk_alloc_stats_count_enomem(a, size)			\
	uk_alloc_stats_count_alloc((a), NULL, (size))
#define uk_alloc_stats_count_penomem(a, num_pages)			\
	uk_alloc_stats_count_enomem((a),				\
				    ((__sz) (num_pages)) << __PAGE_SHIFT)

/* Note: if ptr is NULL, nothing is counted */
#define uk_alloc_stats_count_free(a, ptr, freed_size)			\
	do {								\
		_uk_alloc_stats_count_free(&((a)->_stats),		\
					   (ptr), (freed_size));	\
		_uk_alloc_stats_global_count_free((ptr), (freed_size));	\
	} while (0)
#define uk_alloc_stats_count_pfree(a, ptr, num_pages)			\
	uk_alloc_stats_count_free((a), (ptr),				\
				  ((__sz) (num_pages)) << __PAGE_SHIFT)

#define uk_alloc_stats_reset(a)						\
	memset(&(a)->_stats, 0, sizeof((a)->_stats))

#else /* !CONFIG_LIBUKALLOC_IFSTATS */
#define uk_alloc_stats_count_alloc(a, ptr, size) do {} while (0)
#define uk_alloc_stats_count_palloc(a, ptr, num_pages) do {} while (0)
#define uk_alloc_stats_count_enomem(a, size) do {} while (0)
#define uk_alloc_stats_count_penomem(a, num_pages) do {} while (0)
#define uk_alloc_stats_count_free(a, ptr, freed_size) do {} while (0)
#define uk_alloc_stats_count_pfree(a, ptr, num_pages) do {} while (0)
#define uk_alloc_stats_reset(a) do {} while (0)
#endif /* !CONFIG_LIBUKALLOC_IFSTATS */

/* Shortcut for doing a registration of an allocator that does not implement
 * palloc() or pfree()
 */
#define uk_alloc_init_malloc(a, malloc_f, calloc_f, realloc_f, free_f,	\
			     posix_memalign_f, memalign_f, maxalloc_f,	\
			     availmem_f, addmem_f)			\
	do {								\
		(a)->malloc         = (malloc_f);			\
		(a)->calloc         = (calloc_f);			\
		(a)->realloc        = (realloc_f);			\
		(a)->posix_memalign = (posix_memalign_f);		\
		(a)->memalign       = (memalign_f);			\
		(a)->free           = (free_f);				\
		(a)->palloc         = uk_palloc_compat;			\
		(a)->pfree          = uk_pfree_compat;			\
		(a)->availmem       = (availmem_f);			\
		(a)->pavailmem      = (availmem_f != NULL)		\
				      ? uk_alloc_pavailmem_compat : NULL; \
		(a)->maxalloc       = (maxalloc_f);			\
		(a)->pmaxalloc      = (maxalloc_f != NULL)		\
				      ? uk_alloc_pmaxalloc_compat : NULL; \
		(a)->addmem         = (addmem_f);			\
									\
		uk_alloc_stats_reset((a));				\
		uk_alloc_register((a));					\
	} while (0)

#if CONFIG_LIBUKALLOC_IFMALLOC
#define uk_alloc_init_malloc_ifmalloc(a, malloc_f, free_f, maxalloc_f,	\
				      availmem_f, addmem_f)		\
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
		(a)->availmem       = (availmem_f);			\
		(a)->pavailmem      = (availmem_f != NULL)		\
				      ? uk_alloc_pavailmem_compat : NULL; \
		(a)->maxalloc       = (maxalloc_f);			\
		(a)->pmaxalloc      = (maxalloc_f != NULL)		\
				      ? uk_alloc_pmaxalloc_compat : NULL; \
		(a)->addmem         = (addmem_f);			\
									\
		uk_alloc_stats_reset((a));				\
		uk_alloc_register((a));					\
	} while (0)
#endif

/* Shortcut for doing a registration of an allocator that only
 * implements palloc(), pfree(), pmaxalloc(), pavailmem(), addmem()
 */
#define uk_alloc_init_palloc(a, palloc_func, pfree_func, pmaxalloc_func, \
			     pavailmem_func, addmem_func)		\
	do {								\
		(a)->malloc         = uk_malloc_ifpages;		\
		(a)->calloc         = uk_calloc_compat;			\
		(a)->realloc        = uk_realloc_ifpages;		\
		(a)->posix_memalign = uk_posix_memalign_ifpages;	\
		(a)->memalign       = uk_memalign_compat;		\
		(a)->free           = uk_free_ifpages;			\
		(a)->palloc         = (palloc_func);			\
		(a)->pfree          = (pfree_func);			\
		(a)->pavailmem      = (pavailmem_func);			\
		(a)->availmem       = (pavailmem_func != NULL)		\
				      ? uk_alloc_availmem_ifpages : NULL; \
		(a)->pmaxalloc      = (pmaxalloc_func);			\
		(a)->maxalloc       = (pmaxalloc_func != NULL)		\
				      ? uk_alloc_maxalloc_ifpages : NULL; \
		(a)->addmem         = (addmem_func);			\
									\
		uk_alloc_stats_reset((a));				\
		uk_alloc_register((a));					\
	} while (0)

#ifdef __cplusplus
}
#endif

#endif /* __UK_ALLOC_IMPL_H__ */
