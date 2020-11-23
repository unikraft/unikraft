/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Simon Kuenzer <simon.kuenzer@neclab.eu>
 *
 * Copyright (c) 2020, NEC Laboratories Europe GmbH, NEC Corporation.
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

/*
 * Per library statistics
 * ----------------------
 * This file is compiled together with each library. Provided symbols are set
 * private to each library.
 * The idea is that this file hooks into `uk_alloc_get_default()`. Instead of
 * returning the actual default allocator, a per-library wrapper is returned
 * for keeping the statistics. Allocator requests are forwarded to the actual
 * default allocator while changes of memory usage are observed. These changes
 * are accounted to the library-local statistics.
 */

#include <uk/print.h>
#include <uk/alloc_impl.h>
#include <uk/essentials.h>
#include <uk/preempt.h>

static inline struct uk_alloc *_uk_alloc_get_actual_default(void)
{
	return _uk_alloc_head;
}

#define WATCH_STATS_START(p)						\
	__ssz _before_mem_use;						\
	__sz _before_nb_allocs;						\
	__sz _before_tot_nb_allocs;					\
	__sz _before_nb_enomem;						\
									\
	uk_preempt_disable();						\
	_before_mem_use       = (p)->_stats.cur_mem_use;		\
	_before_nb_allocs     = (p)->_stats.cur_nb_allocs;		\
	_before_tot_nb_allocs = (p)->_stats.tot_nb_allocs;		\
	_before_nb_enomem     = (p)->_stats.nb_enomem;

#define WATCH_STATS_END(p, nb_allocs_diff, nb_enomem_diff,		\
			mem_use_diff, alloc_size)			\
	__sz _nb_allocs = (p)->_stats.tot_nb_allocs			\
			  - _before_tot_nb_allocs;			\
									\
	/* NOTE: We assume that an allocator call does at
	 * most one allocation. Otherwise we cannot currently
	 * keep track of `last_alloc_size` properly
	 */								\
	UK_ASSERT(_nb_allocs <= 1);					\
									\
	*(mem_use_diff)   = (p)->_stats.cur_mem_use			\
			    - _before_mem_use;				\
	*(nb_allocs_diff) = (__ssz) (p)->_stats.cur_nb_allocs		\
			    - _before_nb_allocs;			\
	*(nb_enomem_diff) = (__ssz) (p)->_stats.nb_enomem		\
			    - _before_nb_enomem;			\
	if (_nb_allocs > 0)						\
		*(alloc_size) = (p)->_stats.last_alloc_size;		\
	else								\
		*(alloc_size) = 0; /* there was no new allocation */	\
	uk_preempt_enable();

static inline void update_stats(struct uk_alloc_stats *stats,
				__ssz nb_allocs_diff,
				__ssz nb_enomem_diff,
				__ssz mem_use_diff,
				__sz last_alloc_size)
{
	uk_preempt_disable();
	if (nb_allocs_diff >= 0)
		stats->tot_nb_allocs += nb_allocs_diff;
	else
		stats->tot_nb_frees  += -nb_allocs_diff;
	stats->cur_nb_allocs += nb_allocs_diff;
	stats->nb_enomem     += nb_enomem_diff;
	stats->cur_mem_use   += mem_use_diff;
	if (last_alloc_size)
		stats->last_alloc_size = last_alloc_size;

	/*
	 * NOTE: Because we apply a diff to the library stats
	 *       instead of counting events, we need to call
	 *       `_uk_alloc_stats_refresh_minmax()` from here.
	 */
	_uk_alloc_stats_refresh_minmax(stats);
	uk_preempt_enable();
}

static void *wrapper_malloc(struct uk_alloc *a, __sz size)
{
	struct uk_alloc *p = _uk_alloc_get_actual_default();
	__ssz nb_allocs, mem_use, nb_enomem;
	__sz alloc_size;
	void *ret;

	if (unlikely(!p)) {
		update_stats(&a->_stats, 0, 1, 0, 0);
		errno = ENOMEM;
		return NULL;
	}

	WATCH_STATS_START(p);
	ret = uk_do_malloc(p, size);
	WATCH_STATS_END(p, &nb_allocs, &nb_enomem, &mem_use, &alloc_size);

	/* NOTE: We record `alloc_size` only when allocation was successful */
	update_stats(&a->_stats, nb_allocs, nb_enomem, mem_use,
		     ret != NULL ? alloc_size : 0);
	return ret;
}

static void *wrapper_calloc(struct uk_alloc *a, __sz nmemb, __sz size)
{
	struct uk_alloc *p = _uk_alloc_get_actual_default();
	__ssz nb_allocs, mem_use, nb_enomem;
	__sz alloc_size;
	void *ret;

	if (unlikely(!p)) {
		update_stats(&a->_stats, 0, 1, 0, 0);
		errno = ENOMEM;
		return NULL;
	}

	WATCH_STATS_START(p);
	ret = uk_do_calloc(p, nmemb, size);
	WATCH_STATS_END(p, &nb_allocs, &nb_enomem, &mem_use, &alloc_size);

	update_stats(&a->_stats, nb_allocs, nb_enomem, mem_use,
		     ret != NULL ? alloc_size : 0);
	return ret;
}

static int wrapper_posix_memalign(struct uk_alloc *a, void **memptr,
				  __sz align, __sz size)
{
	struct uk_alloc *p = _uk_alloc_get_actual_default();
	__ssz nb_allocs, mem_use, nb_enomem;
	__sz alloc_size;
	int ret;

	if (unlikely(!p)) {
		update_stats(&a->_stats, 0, 1, 0, 0);
		return ENOMEM;
	}

	WATCH_STATS_START(p);
	ret = uk_do_posix_memalign(p, memptr, align, size);
	WATCH_STATS_END(p, &nb_allocs, &nb_enomem, &mem_use, &alloc_size);

	update_stats(&a->_stats, nb_allocs, nb_enomem, mem_use,
		     ret == 0 ? alloc_size : 0);
	return ret;
}

static void *wrapper_memalign(struct uk_alloc *a, __sz align, __sz size)
{
	struct uk_alloc *p = _uk_alloc_get_actual_default();
	__ssz nb_allocs, mem_use, nb_enomem;
	__sz alloc_size;
	void *ret;

	if (unlikely(!p)) {
		update_stats(&a->_stats, 0, 1, 0, 0);
		errno = ENOMEM;
		return NULL;
	}

	WATCH_STATS_START(p);
	ret = uk_do_memalign(p, align, size);
	WATCH_STATS_END(p, &nb_allocs, &nb_enomem, &mem_use, &alloc_size);

	update_stats(&a->_stats, nb_allocs, nb_enomem, mem_use,
		     ret != NULL ? alloc_size : 0);
	return ret;
}

static void *wrapper_realloc(struct uk_alloc *a, void *ptr, __sz size)
{
	struct uk_alloc *p = _uk_alloc_get_actual_default();
	__ssz nb_allocs, mem_use, nb_enomem;
	__sz alloc_size;
	void *ret;

	if (unlikely(!p)) {
		update_stats(&a->_stats, 0, 1, 0, 0);
		errno = ENOMEM;
		return NULL;
	}

	WATCH_STATS_START(p);
	ret = uk_do_realloc(p, ptr, size);
	WATCH_STATS_END(p, &nb_allocs, &nb_enomem, &mem_use, &alloc_size);

	update_stats(&a->_stats, nb_allocs, nb_enomem, mem_use,
		     ret != NULL ? alloc_size : 0);
	return ret;
}

static void wrapper_free(struct uk_alloc *a, void *ptr)
{
	struct uk_alloc *p = _uk_alloc_get_actual_default();
	__ssz nb_allocs, mem_use, nb_enomem;
	__sz alloc_size;

	UK_ASSERT(p);

	WATCH_STATS_START(p);
	uk_do_free(p, ptr);
	WATCH_STATS_END(p, &nb_allocs, &nb_enomem, &mem_use, &alloc_size);

	update_stats(&a->_stats, nb_allocs, nb_enomem, mem_use, alloc_size);
}

static void *wrapper_palloc(struct uk_alloc *a, unsigned long num_pages)
{
	struct uk_alloc *p = _uk_alloc_get_actual_default();
	__ssz nb_allocs, mem_use, nb_enomem;
	__sz alloc_size;
	void *ret;

	UK_ASSERT(p);

	WATCH_STATS_START(p);
	ret = uk_do_palloc(p, num_pages);
	WATCH_STATS_END(p, &nb_allocs, &nb_enomem, &mem_use, &alloc_size);

	update_stats(&a->_stats, nb_allocs, nb_enomem, mem_use,
		     ret != NULL ? alloc_size : 0);
	return ret;
}

static void wrapper_pfree(struct uk_alloc *a, void *ptr,
			  unsigned long num_pages)
{
	struct uk_alloc *p = _uk_alloc_get_actual_default();
	__ssz nb_allocs, mem_use, nb_enomem;
	__sz alloc_size;

	UK_ASSERT(p);

	WATCH_STATS_START(p);
	uk_do_pfree(p, ptr, num_pages);
	WATCH_STATS_END(p, &nb_allocs, &nb_enomem, &mem_use, &alloc_size);

	update_stats(&a->_stats, nb_allocs, nb_enomem, mem_use, alloc_size);
}

/* The following interfaces do not change allocation statistics,
 * this is why we just forward the calls
 */
static int wrapper_addmem(struct uk_alloc *a __unused, void *base, __sz size)
{
	struct uk_alloc *p = _uk_alloc_get_actual_default();

	UK_ASSERT(p);
	return uk_alloc_addmem(p, base, size);
}

static __ssz wrapper_maxalloc(struct uk_alloc *a __unused)
{
	struct uk_alloc *p = _uk_alloc_get_actual_default();

	UK_ASSERT(p);
	return uk_alloc_maxalloc(p);
}

static __ssz wrapper_availmem(struct uk_alloc *a __unused)
{
	struct uk_alloc *p = _uk_alloc_get_actual_default();

	UK_ASSERT(p);
	return uk_alloc_availmem(p);
}

/*
 * Allocator layer that hooks in between the actual default allocator and the
 * library.
 * allocators but replaces the default allocator - for each library another one.
 * It forwards requests to the default allocator but keeps statistics for the
 * library.
 */
static struct uk_alloc _uk_alloc_lib_default = {
	.malloc         = wrapper_malloc,
	.calloc         = wrapper_calloc,
	.realloc        = wrapper_realloc,
	.posix_memalign = wrapper_posix_memalign,
	.memalign       = wrapper_memalign,
	.free           = wrapper_free,
	.palloc         = wrapper_palloc,
	.pfree          = wrapper_pfree,
	.maxalloc       = wrapper_maxalloc,
	.pmaxalloc      = uk_alloc_pmaxalloc_compat,
	.availmem       = wrapper_availmem,
	.pavailmem      = uk_alloc_pavailmem_compat,
	.addmem         = wrapper_addmem,

	._stats         = { 0 },
};

static __used __section(".uk_alloc_libstats") __align(8)
struct uk_alloc_libstats_entry _uk_alloc_libstats_entry = {
	.libname = STRINGIFY(__LIBNAME__),
	.a       = &_uk_alloc_lib_default,
};

/* Return this wrapper allocator instead of the actual default allocator */
struct uk_alloc *uk_alloc_get_default(void)
{
	uk_pr_debug("Wrap default allocator %p with allocator %p\n",
		    _uk_alloc_get_actual_default(),
		    &_uk_alloc_lib_default);
	return &_uk_alloc_lib_default;
}
