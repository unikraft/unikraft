/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Simon Kuenzer <simon.kuenzer@neclab.eu>
 *
 * Copyright (c) 2018, NEC Europe Ltd., NEC Corporation. All rights reserved.
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
#include <uk/netbuf.h>
#include <uk/essentials.h>
#include <uk/print.h>
#include <string.h>

/* Used to align netbuf's priv and data areas to `long long` data type */
#define NETBUF_ADDR_ALIGNMENT (sizeof(long long))
#define NETBUF_ADDR_ALIGN_UP(x)   ALIGN_UP((__uptr) (x), \
					   NETBUF_ADDR_ALIGNMENT)
#define NETBUF_ADDR_ALIGN_DOWN(x) ALIGN_DOWN((__uptr) (x), \
					     NETBUF_ADDR_ALIGNMENT)

void uk_netbuf_init_indir(struct uk_netbuf *m,
			  void *buf, size_t buflen, uint16_t headroom,
			  void *priv, uk_netbuf_dtor_t dtor)
{
	UK_ASSERT(m);
	UK_ASSERT(buf || (buf == NULL && buflen == 0));
	UK_ASSERT(headroom <= buflen);

	/* Reset pbuf, non-listed fields are automatically set to 0 */
	*m = (struct uk_netbuf) {
		.buf    = buf,
		.buflen = buflen,
		.data   = (void *) ((uintptr_t) buf + headroom),
		.priv   = priv,
		.dtor   = dtor
	};

	uk_refcount_init(&m->refcount, 1);
}

struct uk_netbuf *uk_netbuf_alloc_indir(struct uk_alloc *a,
					void *buf, size_t buflen,
					uint16_t headroom,
					size_t privlen, uk_netbuf_dtor_t dtor)
{
	struct uk_netbuf *m;

	if (privlen)
		m = uk_malloc(a, NETBUF_ADDR_ALIGN_UP(sizeof(*m)) + privlen);
	else
		m = uk_malloc(a, sizeof(*m));
	if (!m)
		return NULL;

	uk_netbuf_init_indir(m,
			     buf,
			     buflen,
			     headroom,
			     privlen > 0
			     ? (void *)((uintptr_t) m
					+ NETBUF_ADDR_ALIGN_UP(sizeof(*m)))
			     : NULL,
			     dtor);

	/* Save reference to allocator that is used
	 * for free'ing this uk_netbuf.
	 */
	m->_a = a;
	m->_b = m;

	return m;
}

struct uk_netbuf *uk_netbuf_alloc_buf(struct uk_alloc *a, size_t buflen,
				      size_t bufalign, uint16_t headroom,
				      size_t privlen, uk_netbuf_dtor_t dtor)
{
	void *mem;
	size_t alloc_len;
	struct uk_netbuf *m;

	UK_ASSERT(buflen > 0);
	UK_ASSERT(headroom <= buflen);

	alloc_len = NETBUF_ADDR_ALIGN_UP(buflen)
		    + NETBUF_ADDR_ALIGN_UP(sizeof(*m) + privlen);
	mem = uk_memalign(a, bufalign, alloc_len);
	if (!mem)
		return NULL;

	m = uk_netbuf_prepare_buf(mem,
				  alloc_len,
				  headroom,
				  privlen,
				  dtor);
	if (!m) {
		uk_free(a, mem);
		return NULL;
	}

	/* Save reference to allocator and allocation
	 * that is used for free'ing this uk_netbuf.
	 */
	m->_a = a;
	m->_b = mem;

	return m;
}

#if CONFIG_LIBUKALLOCPOOL
struct uk_netbuf *uk_netbuf_poolalloc_buf(struct uk_allocpool *p,
					  uint16_t headroom,
					  size_t privlen,
					  uk_netbuf_dtor_t dtor)
{
	void *obj;
	size_t obj_len;
	size_t meta_len;
	struct uk_alloc *a;
	struct uk_netbuf *m;

	UK_ASSERT(p);

	/* uk_alloc interface used for free'ing */
	a = uk_allocpool2ukalloc(p);

	UK_ASSERT(a);

	/* Place headroom and buf at the beginning of the allocation,
	 * This is done in order to forward potential alignments of the
	 * underlying allocation directly to the netbuf data area.
	 * `m` (followed by `m->priv` if privlen > 0) will be placed at
	 * the end of the given memory.
	 * If the operation does not work, we
	 */
	obj_len = uk_allocpool_objlen(p);
	meta_len = NETBUF_ADDR_ALIGN_UP(sizeof(*m) + privlen);
	if (unlikely(meta_len > NETBUF_ADDR_ALIGN_DOWN(obj_len)))
		return NULL; /* no space for meta data on pool objects */

	obj = uk_allocpool_take(p);
	if (unlikely(!obj))
		return NULL;

	m = (struct uk_netbuf *) (NETBUF_ADDR_ALIGN_DOWN((__uptr) obj
							 + obj_len)
				  - meta_len);
	uk_netbuf_init_indir(m,
			     obj,
			     (size_t) ((__uptr) m - (__uptr) obj),
			     headroom,
			     privlen > 0 ? (void *) ((__uptr) m
						     + sizeof(*m))
			     : NULL,
			     dtor);

	/* Save reference to allocator and allocation
	 * that is used for free'ing this uk_netbuf.
	 */
	m->_a = a;
	m->_b = obj;

	return m;
}

int uk_netbuf_poolalloc_buf_batch(struct uk_allocpool *p,
				  struct uk_netbuf *m[], unsigned int count,
				  uint16_t headroom,
				  size_t privlen, uk_netbuf_dtor_t dtor)
{
	size_t meta_len = 0;
	size_t obj_len;
	struct uk_alloc *a;
	void *obj[count];
	unsigned int i;

	UK_ASSERT(p);

	/* uk_alloc interface used for free'ing */
	a = uk_allocpool2ukalloc(p);

	UK_ASSERT(a);

	/* Place headroom and buf at the beginning of the allocation,
	 * This is done in order to forward potential alignments of the
	 * underlying allocation directly to the netbuf data area.
	 * `m` (followed by `m->priv` if privlen > 0) will be placed at
	 * the end of the given memory.
	 * If the operation does not work, we
	 */
	obj_len = uk_allocpool_objlen(p);
	meta_len = NETBUF_ADDR_ALIGN_UP(sizeof(struct uk_netbuf) + privlen);
	if (unlikely(meta_len > NETBUF_ADDR_ALIGN_DOWN(obj_len)))
		return -ENOSPC; /* no space for meta data on pool objects */

	count = uk_allocpool_take_batch(p, obj, count);
	for (i = 0; i < count; ++i) {
		m[i] = (struct uk_netbuf *) (NETBUF_ADDR_ALIGN_DOWN((__uptr) obj[i]
								    + obj_len)
					     - meta_len);
		uk_netbuf_init_indir(m[i],
				     obj[i],
				     (size_t) ((__uptr) m[i] - (__uptr) obj[i]),
				     headroom,
				     privlen > 0 ? (void *) ((__uptr) m[i]
							     + sizeof(struct uk_netbuf))
				     : NULL,
				     dtor);

		/* Save reference to allocator and allocation
		 * that is used for free'ing this uk_netbuf.
		 */
		m[i]->_a = a;
		m[i]->_b = obj[i];
	}

	return (int) count;
}
#endif /* CONFIG_LIBUKALLOCPOOL */

struct uk_netbuf *uk_netbuf_prepare_buf(void *mem, size_t size,
					uint16_t headroom,
					size_t privlen, uk_netbuf_dtor_t dtor)
{
	struct uk_netbuf *m;
	size_t meta_len = 0;

	UK_ASSERT(mem);

	/* Place headroom and buf at the beginning of the allocation,
	 * This is done in order to forward potential alignments of the
	 * underlying allocation directly to the netbuf data area.
	 * `m` (followed by `m->priv` if privlen > 0) will be placed at
	 * the end of the given memory.
	 */
	meta_len = NETBUF_ADDR_ALIGN_UP(sizeof(*m) + privlen);
	if (meta_len > NETBUF_ADDR_ALIGN_DOWN(size))
		return NULL;

	m = (struct uk_netbuf *) (NETBUF_ADDR_ALIGN_DOWN((__uptr) mem + size)
				  - meta_len);

	uk_netbuf_init_indir(m,
			     mem,
			     (size_t) ((__uptr) m - (__uptr) mem),
			     headroom,
			     privlen > 0 ? (void *) ((__uptr) m+ sizeof(*m))
					 : NULL,
			     dtor);
	return m;
}

struct uk_netbuf *uk_netbuf_disconnect(struct uk_netbuf *m)
{
	struct uk_netbuf *remhead = NULL;

	UK_ASSERT(m);

	/* We want to return the next element of m as the
	 * remaining head of the chain. If there is no next element
	 * there was no chain.
	 */
	remhead = m->next;

	/* Reconnect the chains before and after m. */
	if (m->prev)
		m->prev->next = m->next;
	if (m->next)
		m->next->prev = m->prev;

	/* Disconnect m. */
	m->prev = NULL;
	m->next = NULL;

	return remhead;
}


void uk_netbuf_connect(struct uk_netbuf *headtail,
		       struct uk_netbuf *tail)
{
	UK_ASSERT(headtail);
	UK_ASSERT(!headtail->next);
	UK_ASSERT(tail);
	UK_ASSERT(!tail->prev);

	headtail->next = tail;
	tail->prev = headtail;
}

void uk_netbuf_append(struct uk_netbuf *head,
		      struct uk_netbuf *tail)
{
	struct uk_netbuf *headtail;

	UK_ASSERT(head);
	UK_ASSERT(!head->prev);
	UK_ASSERT(tail);
	UK_ASSERT(!tail->prev);

	headtail = uk_netbuf_chain_last(head);
	headtail->next = tail;
	tail->prev = headtail;
}

void uk_netbuf_free_single(struct uk_netbuf *m)
{
	struct uk_alloc *a;
	void *b;

	UK_ASSERT(m);

	/* Decrease refcount and call destructor and free up memory
	 * when last reference was released.
	 */
	if (uk_refcount_release(&m->refcount) == 1) {
		uk_pr_debug("Freeing netbuf %p (next: %p)\n", m, m->next);

		/* Disconnect this netbuf from the chain. */
		uk_netbuf_disconnect(m);

		/* Copy the reference of the allocator and base address
		 * in case the destructor is free'ing up our memory
		 * (e.g., uk_netbuf_init_indir() used).
		 * In such a case `a` and `b` should be (NULL),
		 * however we need to access them for a check after
		 * we have called the destructor.
		 */
		a = m->_a;
		b = m->_b;

		if (m->dtor)
			m->dtor(m);
		if (a && b)
			uk_free(a, b);
	} else {
		uk_pr_debug("Not freeing netbuf %p (next: %p): refcount greater than 1",
			    m, m->next);
	}
}

void uk_netbuf_free(struct uk_netbuf *m)
{
	struct uk_netbuf *n;

	UK_ASSERT(m);
	UK_ASSERT(!m->prev);

	while (m != NULL) {
		n = m->next;
		uk_netbuf_free_single(m);
		m = n;
	}
}

struct uk_netbuf *uk_netbuf_dup_single(struct uk_alloc *a, size_t buflen,
				       size_t bufalign, uint16_t headroom,
				       size_t privlen, uk_netbuf_dtor_t dtor,
				       const struct uk_netbuf *src)
{
	struct uk_netbuf *nb;

	UK_ASSERT(src);

	if (unlikely((size_t) src->len + headroom > buflen))
		return NULL; /* buffer would be too small to hold everything */

	nb = uk_netbuf_alloc_buf(a, buflen, bufalign, headroom, privlen, dtor);
	if (unlikely(!nb))
		return NULL;

	memcpy(nb->data, src->data, src->len);
	nb->len = src->len;
	nb->flags = src->flags;
	nb->csum_start = src->csum_start;
	nb->csum_offset = src->csum_offset;

	return nb;
}

#if CONFIG_LIBUKALLOCPOOL
struct uk_netbuf *uk_netbuf_pooldup_single(struct uk_allocpool *p,
					   uint16_t headroom,
					   size_t privlen,
					   uk_netbuf_dtor_t dtor,
					   const struct uk_netbuf *src)
{
	struct uk_netbuf *nb;

	UK_ASSERT(src);

	if (unlikely((size_t) src->len + headroom > (NETBUF_ADDR_ALIGN_DOWN(uk_allocpool_objlen(p))
						     - NETBUF_ADDR_ALIGN_UP(sizeof(*nb) + privlen))))
		return NULL; /* buffer would be too small to hold everything */

	nb = uk_netbuf_poolalloc_buf(p, headroom, privlen, dtor);
	if (unlikely(!nb))
		return NULL;

	/* double-check that target packet has really enough space to hold the source */
	UK_ASSERT(uk_netbuf_tailroom(nb) >= src->len);

	memcpy(nb->data, src->data, src->len);
	nb->len = src->len;
	nb->flags = src->flags;
	nb->csum_start = src->csum_start;
	nb->csum_offset = src->csum_offset;

	return nb;
}
#endif /* CONFIG_LIBUKALLOCPOOL */
