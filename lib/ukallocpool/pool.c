/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Simple memory pool using LIFO principle
 *
 * Authors: Simon Kuenzer <simon.kuenzer@neclab.eu>
 *
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

#include <uk/essentials.h>
#include <uk/alloc_impl.h>
#include <uk/allocpool.h>
#include <uk/list.h>
#include <string.h>
#include <errno.h>

/*
 * POOL: MEMORY LAYOUT
 *
 *          ++---------------------++
 *          || struct uk_allocpool ||
 *          ||                     ||
 *          ++---------------------++
 *          |    // padding //      |
 *          +=======================+
 *          |       OBJECT 1        |
 *          +=======================+
 *          |       OBJECT 2        |
 *          +=======================+
 *          |       OBJECT 3        |
 *          +=======================+
 *          |         ...           |
 *          v                       v
 */

#define MIN_OBJ_ALIGN sizeof(void *)
#define MIN_OBJ_LEN   sizeof(struct uk_list_head)

struct uk_allocpool {
	struct uk_alloc self;

	struct uk_list_head free_obj;
	unsigned int free_obj_count;

	__sz obj_align;
	__sz obj_len;
	unsigned int obj_count;

	struct uk_alloc *parent;
	void *base;
};

struct free_obj {
	struct uk_list_head list;
};

static inline struct uk_allocpool *ukalloc2pool(struct uk_alloc *a)
{
	UK_ASSERT(a);
	return __containerof(a, struct uk_allocpool, self);
}

#define allocpool2ukalloc(p) \
	(&(p)->self)

struct uk_alloc *uk_allocpool2ukalloc(struct uk_allocpool *p)
{
	UK_ASSERT(p);
	return allocpool2ukalloc(p);
}

static inline void _prepend_free_obj(struct uk_allocpool *p, void *obj)
{
	struct uk_list_head *entry;

	UK_ASSERT(p);
	UK_ASSERT(obj);
	UK_ASSERT(p->free_obj_count < p->obj_count);

	entry = &((struct free_obj *) obj)->list;
	uk_list_add(entry, &p->free_obj);
	p->free_obj_count++;
}

static inline void *_take_free_obj(struct uk_allocpool *p)
{
	struct free_obj *obj;

	UK_ASSERT(p);
	UK_ASSERT(p->free_obj_count > 0);

	/* get object from list head */
	obj = uk_list_first_entry(&p->free_obj, struct free_obj, list);
	uk_list_del(&obj->list);
	p->free_obj_count--;
	return (void *) obj;
}

static void pool_free(struct uk_alloc *a, void *ptr)
{
	struct uk_allocpool *p = ukalloc2pool(a);

	if (likely(ptr)) {
		_prepend_free_obj(p, ptr);
		uk_alloc_stats_count_free(a, ptr, p->obj_len);
	}
}

static void *pool_malloc(struct uk_alloc *a, __sz size)
{
	struct uk_allocpool *p = ukalloc2pool(a);
	void *obj;

	if (unlikely((size > p->obj_len)
		     || uk_list_empty(&p->free_obj))) {
		uk_alloc_stats_count_enomem(a, p->obj_len);
		errno = ENOMEM;
		return NULL;
	}

	obj = _take_free_obj(p);
	uk_alloc_stats_count_alloc(a, obj, p->obj_len);
	return obj;
}

static int pool_posix_memalign(struct uk_alloc *a, void **memptr, __sz align,
			       __sz size)
{
	struct uk_allocpool *p = ukalloc2pool(a);

	if (unlikely((size > p->obj_len)
		     || (align > p->obj_align)
		     || uk_list_empty(&p->free_obj))) {
		uk_alloc_stats_count_enomem(a, p->obj_len);
		return ENOMEM;
	}

	*memptr = _take_free_obj(p);
	uk_alloc_stats_count_alloc(a, *memptr, p->obj_len);
	return 0;
}

void *uk_allocpool_take(struct uk_allocpool *p)
{
	void *obj;

	UK_ASSERT(p);

	if (unlikely(uk_list_empty(&p->free_obj))) {
		uk_alloc_stats_count_enomem(allocpool2ukalloc(p),
					    p->obj_len);
		return NULL;
	}

	obj = _take_free_obj(p);
	uk_alloc_stats_count_alloc(allocpool2ukalloc(p),
				   obj, p->obj_len);
	return obj;
}

unsigned int uk_allocpool_take_batch(struct uk_allocpool *p,
				     void *obj[], unsigned int count)
{
	unsigned int i;

	UK_ASSERT(p);
	UK_ASSERT(obj);

	for (i = 0; i < count; ++i) {
		if (unlikely(uk_list_empty(&p->free_obj)))
			break;
		obj[i] = _take_free_obj(p);
		uk_alloc_stats_count_alloc(allocpool2ukalloc(p),
					   obj[i], p->obj_len);
	}

	if (unlikely(i == 0))
		uk_alloc_stats_count_enomem(allocpool2ukalloc(p),
					    p->obj_len);

	return i;
}

void uk_allocpool_return(struct uk_allocpool *p, void *obj)
{
	UK_ASSERT(p);

	_prepend_free_obj(p, obj);
	uk_alloc_stats_count_free(allocpool2ukalloc(p),
				  obj, p->obj_len);
}

void uk_allocpool_return_batch(struct uk_allocpool *p,
			       void *obj[], unsigned int count)
{
	unsigned int i;

	UK_ASSERT(p);
	UK_ASSERT(obj);

	for (i = 0; i < count; ++i) {
		_prepend_free_obj(p, obj[i]);
		uk_alloc_stats_count_free(allocpool2ukalloc(p),
					  obj[i], p->obj_len);
	}
}

static __ssz pool_availmem(struct uk_alloc *a)
{
	struct uk_allocpool *p = ukalloc2pool(a);

	return (__ssz) (p->free_obj_count * p->obj_len);
}

static __ssz pool_maxalloc(struct uk_alloc *a)
{
	struct uk_allocpool *p = ukalloc2pool(a);

	return (__ssz) p->obj_len;
}

__sz uk_allocpool_reqmem(unsigned int obj_count, __sz obj_len,
			 __sz obj_align)
{
	__sz obj_alen;

	UK_ASSERT(POWER_OF_2(obj_align));

	obj_len   = MAX(obj_len, MIN_OBJ_LEN);
	obj_align = MAX(obj_align, MIN_OBJ_ALIGN);
	obj_alen  = ALIGN_UP(obj_len, obj_align);
	return (sizeof(struct uk_allocpool)
		+ obj_align
		+ ((__sz) obj_count * obj_alen));
}

unsigned int uk_allocpool_availcount(struct uk_allocpool *p)
{
	return p->free_obj_count;
}

__sz uk_allocpool_objlen(struct uk_allocpool *p)
{
	return p->obj_len;
}

struct uk_allocpool *uk_allocpool_init(void *base, __sz len,
				       __sz obj_len, __sz obj_align)
{
	struct uk_allocpool *p;
	struct uk_alloc *a;
	__sz obj_alen;
	__sz left;
	void *obj_ptr;

	UK_ASSERT(POWER_OF_2(obj_align));

	if (!base || sizeof(struct uk_allocpool) > len) {
		errno = ENOSPC;
		return NULL;
	}

	/* apply minimum requirements */
	obj_len   = MAX(obj_len, MIN_OBJ_LEN);
	obj_align = MAX(obj_align, MIN_OBJ_ALIGN);

	p = (struct uk_allocpool *) base;
	memset(p, 0, sizeof(*p));
	a = allocpool2ukalloc(p);

	obj_alen = ALIGN_UP(obj_len, obj_align);
	obj_ptr = (void *) ALIGN_UP((__uptr) base + sizeof(*p),
				    obj_align);
	if ((__uptr) obj_ptr > (__uptr) base + len) {
		uk_pr_debug("%p: Empty pool: Not enough space for allocating objects\n",
			    p);
		goto out;
	}

	left = len - ((__uptr) obj_ptr - (__uptr) base);

	p->obj_count = 0;
	p->free_obj_count = 0;
	UK_INIT_LIST_HEAD(&p->free_obj);
	while (left >= obj_alen) {
		++p->obj_count;
		_prepend_free_obj(p, obj_ptr);
		obj_ptr = (void *) ((__uptr) obj_ptr + obj_alen);
		left -= obj_alen;
	}

out:
	p->obj_len         = obj_alen;
	p->obj_align       = obj_align;
	p->base            = base;
	p->parent          = NULL;

	uk_alloc_init_malloc(a,
			     pool_malloc,
			     uk_calloc_compat,
			     uk_realloc_compat,
			     pool_free,
			     pool_posix_memalign,
			     uk_memalign_compat,
			     pool_maxalloc,
			     pool_availmem,
			     NULL);

	uk_pr_debug("%p: Pool created (%"__PRIsz" B): %u objs of %"__PRIsz" B, aligned to %"__PRIsz" B\n",
		    p, len, p->obj_count, p->obj_len, p->obj_align);
	return p;
}

struct uk_allocpool *uk_allocpool_alloc(struct uk_alloc *parent,
					unsigned int obj_count,
					__sz obj_len, __sz obj_align)
{
	struct uk_allocpool *p;
	void *base;
	__sz len;

	/* uk_allocpool_reqmem computes minimum requirement */
	len = uk_allocpool_reqmem(obj_count, obj_len, obj_align);
	base = uk_malloc(parent, len);
	if (!base)
		return NULL;

	p = uk_allocpool_init(base, len, obj_len, obj_align);
	if (!p) {
		uk_free(parent, base);
		errno = ENOSPC;
		return NULL;
	}

	p->parent = parent;
	return p;
}

void uk_allocpool_free(struct uk_allocpool *p)
{
	/* If we do not have a parent, this pool was created with
	 * uk_allocpool_init(). Such a pool cannot be free'd with
	 * this function since we are not the owner of the allocation
	 */
	UK_ASSERT(p->parent);

	/* Make sure we got all objects back */
	UK_ASSERT(p->free_obj_count == p->obj_count);

	/* FIXME: Unregister `ukalloc` interface from `lib/ukalloc` */
	/* TODO: Provide unregistration interface at `lib/ukalloc` */
	UK_CRASH("Unregistering from `lib/ukalloc` not implemented.\n");

	uk_free(p->parent, p->base);
}
