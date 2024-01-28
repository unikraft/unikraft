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

/* Used to align netbuf's priv and data areas to `long long` data type */
#define NETBUF_ADDR_ALIGNMENT (sizeof(long long))
#define NETBUF_ADDR_ALIGN_UP(x)   ALIGN_UP((__uptr) (x), \
					   NETBUF_ADDR_ALIGNMENT)
#define NETBUF_ADDR_ALIGN_DOWN(x) ALIGN_DOWN((__uptr) (x), \
					     NETBUF_ADDR_ALIGNMENT)

#if CONFIG_LIBUKSGLIST
int uk_netbuf_sglist_append(struct uk_sglist *sg, struct uk_netbuf *netbuf)
{
	struct uk_sgsave save;
	struct uk_netbuf *nb;
	int rc = 0;

	UK_ASSERT(sg);
	UK_ASSERT(netbuf);

	if (sg->sg_maxseg == 0)
		return -EINVAL;

	UK_SGLIST_SAVE(sg, save);
	UK_NETBUF_CHAIN_FOREACH(nb, netbuf) {
		if (likely(nb->len > 0)) {
			rc = uk_sglist_append(sg, nb->data, nb->len);
			if (unlikely(rc)) {
				UK_SGLIST_RESTORE(sg, save);
				return rc;
			}
		}
	}
	return 0;
}
#endif /* CONFIG_LIBUKSGLIST */

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
	if (meta_len > NETBUF_ADDR_ALIGN_DOWN((__uptr) mem + size))
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
