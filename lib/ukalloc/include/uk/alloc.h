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
 *
 * THIS HEADER MAY NOT BE EXTRACTED OR MODIFIED IN ANY WAY.
 */

#ifndef __UK_ALLOC_H__
#define __UK_ALLOC_H__

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#include <errno.h>
#include <uk/config.h>
#include <uk/assert.h>
#include <uk/essentials.h>

struct uk_alloc;

#ifdef __cplusplus
extern "C" {
#endif

struct uk_alloc *uk_alloc_get_default(void);
int uk_alloc_set_default(struct uk_alloc *a);

typedef void* (*uk_alloc_malloc_func_t)
		(struct uk_alloc *a, size_t size);
typedef void* (*uk_alloc_calloc_func_t)
		(struct uk_alloc *a, size_t nmemb, size_t size);
typedef int   (*uk_alloc_posix_memalign_func_t)
		(struct uk_alloc *a, void **memptr, size_t align, size_t size);
typedef void* (*uk_alloc_memalign_func_t)
		(struct uk_alloc *a, size_t align, size_t size);
typedef void* (*uk_alloc_realloc_func_t)
		(struct uk_alloc *a, void *ptr, size_t size);
typedef void  (*uk_alloc_free_func_t)
		(struct uk_alloc *a, void *ptr);
#if CONFIG_LIBUKALLOC_IFPAGES
typedef void* (*uk_alloc_palloc_func_t)
		(struct uk_alloc *a, size_t order);
typedef void  (*uk_alloc_pfree_func_t)
		(struct uk_alloc *a, void *ptr, size_t order);
#endif
typedef int   (*uk_alloc_addmem_func_t)
		(struct uk_alloc *a, void *base, size_t size);
#if CONFIG_LIBUKALLOC_IFSTATS
typedef ssize_t (*uk_alloc_availmem_func_t)
		(struct uk_alloc *a);
#endif

struct uk_alloc {
	/* memory allocation */
	uk_alloc_malloc_func_t malloc;
	uk_alloc_calloc_func_t calloc;
	uk_alloc_realloc_func_t realloc;
	uk_alloc_posix_memalign_func_t posix_memalign;
	uk_alloc_memalign_func_t memalign;
	uk_alloc_free_func_t free;

#if CONFIG_LIBUKALLOC_IFPAGES
	/* page allocation interface */
	uk_alloc_palloc_func_t palloc;
	uk_alloc_pfree_func_t pfree;
#endif
#if CONFIG_LIBUKALLOC_IFSTATS
	/* optional interface */
	uk_alloc_availmem_func_t availmem;
#endif
	/* optional interface */
	uk_alloc_addmem_func_t addmem;

	/* internal */
	struct uk_alloc *next;
	int8_t priv[];
};

/* wrapper functions */
static inline void *uk_do_malloc(struct uk_alloc *a, size_t size)
{
	UK_ASSERT(a);
	return a->malloc(a, size);
}
static inline void *uk_malloc(struct uk_alloc *a, size_t size)
{
	if (unlikely(!a)) {
		errno = ENOMEM;
		return NULL;
	}
	return uk_do_malloc(a, size);
}

static inline void *uk_do_calloc(struct uk_alloc *a,
				 size_t nmemb, size_t size)
{
	UK_ASSERT(a);
	return a->calloc(a, nmemb, size);
}
static inline void *uk_calloc(struct uk_alloc *a,
			      size_t nmemb, size_t size)
{
	if (unlikely(!a)) {
		errno = ENOMEM;
		return NULL;
	}
	return uk_do_calloc(a, nmemb, size);
}

static inline void *uk_do_realloc(struct uk_alloc *a,
				  void *ptr, size_t size)
{
	UK_ASSERT(a);
	return a->realloc(a, ptr, size);
}
static inline void *uk_realloc(struct uk_alloc *a, void *ptr, size_t size)
{
	if (unlikely(!a)) {
		errno = ENOMEM;
		return NULL;
	}
	return uk_do_realloc(a, ptr, size);
}

static inline int uk_do_posix_memalign(struct uk_alloc *a, void **memptr,
				       size_t align, size_t size)
{
	UK_ASSERT(a);
	return a->posix_memalign(a, memptr, align, size);
}
static inline int uk_posix_memalign(struct uk_alloc *a, void **memptr,
				    size_t align, size_t size)
{
	if (unlikely(!a)) {
		*memptr = NULL;
		return ENOMEM;
	}
	return uk_do_posix_memalign(a, memptr, align, size);
}

static inline void *uk_do_memalign(struct uk_alloc *a,
				   size_t align, size_t size)
{
	UK_ASSERT(a);
	return a->memalign(a, align, size);
}
static inline void *uk_memalign(struct uk_alloc *a,
				size_t align, size_t size)
{
	if (unlikely(!a))
		return NULL;
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

#if CONFIG_LIBUKALLOC_IFPAGES
static inline void *uk_do_palloc(struct uk_alloc *a, size_t order)
{
	UK_ASSERT(a);
	return a->palloc(a, order);
}
static inline void *uk_palloc(struct uk_alloc *a, size_t order)
{
	if (unlikely(!a))
		return NULL;
	return a->palloc(a, order);
}
static inline void *uk_malloc_page(struct uk_alloc *a)
{
	return uk_palloc(a, 0);
}
static inline void uk_do_pfree(struct uk_alloc *a, void *ptr, size_t order)
{
	UK_ASSERT(a);
	a->pfree(a, ptr, order);
}
static inline void uk_pfree(struct uk_alloc *a, void *ptr, size_t order)
{
	uk_do_pfree(a, ptr, order);
}
static inline void uk_free_page(struct uk_alloc *a, void *ptr)
{
	return uk_pfree(a, ptr, 0);
}
#endif
static inline int uk_alloc_addmem(struct uk_alloc *a, void *base,
				  size_t size)
{
	UK_ASSERT(a);
	if (a->addmem)
		return a->addmem(a, base, size);
	else
		return -ENOTSUP;
}
#if CONFIG_LIBUKALLOC_IFSTATS
static inline ssize_t uk_alloc_availmem(struct uk_alloc *a)
{
	UK_ASSERT(a);
	if (!a->availmem)
		return (ssize_t) -ENOTSUP;
	return a->availmem(a);
}
#endif /* CONFIG_LIBUKALLOC_IFSTATS */

#ifdef __cplusplus
}
#endif

#endif /* __UK_ALLOC_H__ */
