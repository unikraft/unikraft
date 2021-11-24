/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Simon Kuenzer <simon.kuenzer@neclab.eu>
 *          Florian Schmidt <florian.schmidt@neclab.eu>
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
 */

#ifndef __UK_ALLOC_H__
#define __UK_ALLOC_H__

#include <uk/arch/types.h>
#include <uk/config.h>
#include <uk/assert.h>
#include <uk/essentials.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

struct uk_alloc;

typedef void* (*uk_alloc_malloc_func_t)
		(struct uk_alloc *a, __sz size);
typedef void* (*uk_alloc_calloc_func_t)
		(struct uk_alloc *a, __sz nmemb, __sz size);
typedef int   (*uk_alloc_posix_memalign_func_t)
		(struct uk_alloc *a, void **memptr, __sz align, __sz size);
typedef void* (*uk_alloc_memalign_func_t)
		(struct uk_alloc *a, __sz align, __sz size);
typedef void* (*uk_alloc_realloc_func_t)
		(struct uk_alloc *a, void *ptr, __sz size);
typedef void  (*uk_alloc_free_func_t)
		(struct uk_alloc *a, void *ptr);
typedef void* (*uk_alloc_palloc_func_t)
		(struct uk_alloc *a, unsigned long num_pages);
typedef void  (*uk_alloc_pfree_func_t)
		(struct uk_alloc *a, void *ptr, unsigned long num_pages);
typedef int   (*uk_alloc_addmem_func_t)
		(struct uk_alloc *a, void *base, __sz size);
typedef __ssz (*uk_alloc_getsize_func_t)
		(struct uk_alloc *a);
typedef long  (*uk_alloc_getpsize_func_t)
		(struct uk_alloc *a);

#if CONFIG_LIBUKALLOC_IFSTATS
struct uk_alloc_stats {
	__sz last_alloc_size; /* size of the last allocation */
	__sz max_alloc_size; /* biggest satisfied allocation size */
	__sz min_alloc_size; /* smallest satisfied allocation size */

	__u64 tot_nb_allocs; /* total number of satisfied allocations */
	__u64 tot_nb_frees;  /* total number of satisfied free operations */
	__s64 cur_nb_allocs; /* current number of active allocations */
	__s64 max_nb_allocs; /* maximum number of active allocations */

	__ssz cur_mem_use; /* current used memory by allocations */
	__ssz max_mem_use; /* maximum amount of memory used by allocations */

	__u64 nb_enomem; /* number of times failing allocation requests */
};
#endif /* CONFIG_LIBUKALLOC_IFSTATS */

struct uk_alloc {
	/* memory allocation */
	uk_alloc_malloc_func_t malloc;
	uk_alloc_calloc_func_t calloc;
	uk_alloc_realloc_func_t realloc;
	uk_alloc_posix_memalign_func_t posix_memalign;
	uk_alloc_memalign_func_t memalign;
	uk_alloc_free_func_t free;

#if CONFIG_LIBUKALLOC_IFMALLOC
	uk_alloc_free_func_t free_backend;
	uk_alloc_malloc_func_t malloc_backend;
#endif

	/* page allocation interface */
	uk_alloc_palloc_func_t palloc;
	uk_alloc_pfree_func_t pfree;
	/* optional interfaces, but recommended */
	uk_alloc_getsize_func_t maxalloc; /* biggest alloc req. (bytes) */
	uk_alloc_getsize_func_t availmem; /* total memory available (bytes) */
	uk_alloc_getpsize_func_t pmaxalloc; /* biggest alloc req. (pages) */
	uk_alloc_getpsize_func_t pavailmem; /* total pages available */
	/* optional interface */
	uk_alloc_addmem_func_t addmem;

#if CONFIG_LIBUKALLOC_IFSTATS
	struct uk_alloc_stats _stats;
#endif

	/* internal */
	struct uk_alloc *next;
	__u8 priv[];
};

extern struct uk_alloc *_uk_alloc_head;

/* Iterate over all registered allocators */
#define uk_alloc_foreach(iter)			\
	for (iter = _uk_alloc_head;		\
	     iter != __NULL;			\
	     iter = iter->next)

#if CONFIG_LIBUKALLOC_IFSTATS_PERLIB
struct uk_alloc *uk_alloc_get_default(void);
#else /* !CONFIG_LIBUKALLOC_IFSTATS_PERLIB */
static inline struct uk_alloc *uk_alloc_get_default(void)
{
	return _uk_alloc_head;
}
#endif /* !CONFIG_LIBUKALLOC_IFSTATS_PERLIB */

/* wrapper functions */
static inline void *uk_do_malloc(struct uk_alloc *a, __sz size)
{
	UK_ASSERT(a);
	return a->malloc(a, size);
}

static inline void *uk_malloc(struct uk_alloc *a, __sz size)
{
	if (unlikely(!a)) {
		errno = ENOMEM;
		return __NULL;
	}
	return uk_do_malloc(a, size);
}

static inline void *uk_do_calloc(struct uk_alloc *a,
				 __sz nmemb, __sz size)
{
	UK_ASSERT(a);
	return a->calloc(a, nmemb, size);
}

static inline void *uk_calloc(struct uk_alloc *a,
			      __sz nmemb, __sz size)
{
	if (unlikely(!a)) {
		errno = ENOMEM;
		return __NULL;
	}
	return uk_do_calloc(a, nmemb, size);
}

#define uk_do_zalloc(a, size) uk_do_calloc((a), 1, (size))
#define uk_zalloc(a, size) uk_calloc((a), 1, (size))

static inline void *uk_do_realloc(struct uk_alloc *a,
				  void *ptr, __sz size)
{
	UK_ASSERT(a);
	return a->realloc(a, ptr, size);
}

static inline void *uk_realloc(struct uk_alloc *a, void *ptr, __sz size)
{
	if (unlikely(!a)) {
		errno = ENOMEM;
		return __NULL;
	}
	return uk_do_realloc(a, ptr, size);
}

static inline int uk_do_posix_memalign(struct uk_alloc *a, void **memptr,
				       __sz align, __sz size)
{
	UK_ASSERT(a);
	return a->posix_memalign(a, memptr, align, size);
}

static inline int uk_posix_memalign(struct uk_alloc *a, void **memptr,
				    __sz align, __sz size)
{
	if (unlikely(!a)) {
		*memptr = __NULL;
		return ENOMEM;
	}
	return uk_do_posix_memalign(a, memptr, align, size);
}

static inline void *uk_do_memalign(struct uk_alloc *a,
				   __sz align, __sz size)
{
	UK_ASSERT(a);
	return a->memalign(a, align, size);
}

static inline void *uk_memalign(struct uk_alloc *a,
				__sz align, __sz size)
{
	if (unlikely(!a))
		return __NULL;
	return uk_do_memalign(a, align, size);
}

static inline void uk_do_free(struct uk_alloc *a, void *ptr)
{
	UK_ASSERT(a);
	a->free(a, ptr);
}

static inline void uk_free(struct uk_alloc *a, void *ptr)
{
	uk_do_free(a, ptr);
}

static inline void *uk_do_palloc(struct uk_alloc *a, unsigned long num_pages)
{
	UK_ASSERT(a);
	return a->palloc(a, num_pages);
}

static inline void *uk_palloc(struct uk_alloc *a, unsigned long num_pages)
{
	if (unlikely(!a || !a->palloc))
		return __NULL;
	return uk_do_palloc(a, num_pages);
}

static inline void uk_do_pfree(struct uk_alloc *a, void *ptr,
			       unsigned long num_pages)
{
	UK_ASSERT(a);
	a->pfree(a, ptr, num_pages);
}

static inline void uk_pfree(struct uk_alloc *a, void *ptr,
			    unsigned long num_pages)
{
	uk_do_pfree(a, ptr, num_pages);
}

static inline int uk_alloc_addmem(struct uk_alloc *a, void *base,
				  __sz size)
{
	UK_ASSERT(a);
	if (a->addmem)
		return a->addmem(a, base, size);
	else
		return -ENOTSUP;
}

/* current biggest allocation request possible */
static inline __ssz uk_alloc_maxalloc(struct uk_alloc *a)
{
	UK_ASSERT(a);
	if (!a->maxalloc)
		return (__ssz) -ENOTSUP;
	return a->maxalloc(a);
}

static inline long uk_alloc_pmaxalloc(struct uk_alloc *a)
{
	UK_ASSERT(a);
	if (!a->pmaxalloc)
		return (long) -ENOTSUP;
	return a->pmaxalloc(a);
}

/* total free memory of the allocator */
static inline __ssz uk_alloc_availmem(struct uk_alloc *a)
{
	UK_ASSERT(a);
	if (!a->availmem)
		return (__ssz) -ENOTSUP;
	return a->availmem(a);
}

static inline long uk_alloc_pavailmem(struct uk_alloc *a)
{
	UK_ASSERT(a);
	if (!a->pavailmem)
		return (long) -ENOTSUP;
	return a->pavailmem(a);
}

__sz uk_alloc_availmem_total(void);

unsigned long uk_alloc_pavailmem_total(void);

#if CONFIG_LIBUKALLOC_IFSTATS
/*
 * Memory allocation statistics
 */
void uk_alloc_stats_get(struct uk_alloc *a, struct uk_alloc_stats *dst);

#if CONFIG_LIBUKALLOC_IFSTATS_GLOBAL
void uk_alloc_stats_get_global(struct uk_alloc_stats *dst);
#endif /* CONFIG_LIBUKALLOC_IFSTATS_GLOBAL */

#if CONFIG_LIBUKALLOC_IFSTATS_PERLIB
struct uk_alloc_libstats_entry {
	const char *libname;
	struct uk_alloc *a; /* default allocator wrapper for the library */
};

extern struct uk_alloc_libstats_entry _uk_alloc_libstats_start[];
extern struct uk_alloc_libstats_entry _uk_alloc_libstats_end;

#define uk_alloc_foreach_libstats(iter)					\
	for ((iter) = _uk_alloc_libstats_start;				\
	     (iter) < &_uk_alloc_libstats_end;				\
	     (iter) = (struct uk_alloc_libstats_entry *) ((__uptr)(iter) \
		      + ALIGN_UP(sizeof(struct uk_alloc_libstats_entry), 8)))

#endif /* CONFIG_LIBUKALLOC_IFSTATS_PERLIB */
#endif /* CONFIG_LIBUKALLOC_IFSTATS */

#ifdef __cplusplus
}
#endif

#endif /* __UK_ALLOC_H__ */
