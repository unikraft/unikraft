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
 *
 * THIS HEADER MAY NOT BE EXTRACTED OR MODIFIED IN ANY WAY.
 */
#include <uk/netbuf.h>
#include <uk/essentials.h>

/* Used to align netbuf's priv and data areas to `long long` data type */
#define NETBUF_ADDR_ALIGNMENT (sizeof(long long))
#define NETBUF_ADDR_ALIGN_UP(x) ALIGN_UP((x), NETBUF_ADDR_ALIGNMENT)

void uk_netbuf_init_indir(struct uk_netbuf *m,
			  void *buf, size_t buflen, uint16_t headroom,
			  void *priv, uk_netbuf_dtor_t dtor)
{
	UK_ASSERT(m);
	UK_ASSERT(buf || (buf == NULL && buflen == 0));
	UK_ASSERT(headroom <= buflen);

	m->buf    = buf;
	m->buflen = buflen;
	m->data   = (void *) ((uintptr_t) buf + headroom);
	m->len    = 0;
	m->prev   = NULL;
	m->next   = NULL;

	uk_refcount_init(&m->refcount, 1);

	m->priv   = priv;
	m->dtor   = dtor;
	m->_a      = NULL;
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

	return m;
}

struct uk_netbuf *uk_netbuf_alloc_buf(struct uk_alloc *a, size_t buflen,
				      uint16_t headroom,
				      size_t privlen, uk_netbuf_dtor_t dtor)
{
	struct uk_netbuf *m;
	size_t buf_offset = 0;
	size_t priv_offset = 0;
	size_t headroom_extra = 0;

	UK_ASSERT(buflen > 0);
	UK_ASSERT(headroom <= buflen);

	m = uk_malloc(a, NETBUF_ADDR_ALIGN_UP(sizeof(*m))
		      + NETBUF_ADDR_ALIGN_UP(privlen)
		      + buflen);
	if (!m)
		return NULL;

	/* Place buf right behind `m` or `m->priv` region if privlen > 0.
	 * In order to keep `m->data - headroom` aligned the padding bytes
	 *  are added to the headroom.
	 * We can only do this if the given headroom stays within
	 *  uint16_t bounds after the operation.
	 */
	if (likely(UINT16_MAX - headroom > NETBUF_ADDR_ALIGNMENT)) {
		if (privlen == 0) {
			priv_offset    = 0;
			buf_offset     = sizeof(*m);
			headroom_extra = NETBUF_ADDR_ALIGN_UP(sizeof(*m))
					 - sizeof(*m);
		} else {
			priv_offset    = NETBUF_ADDR_ALIGN_UP(sizeof(*m));
			buf_offset     = priv_offset + privlen;
			headroom_extra = NETBUF_ADDR_ALIGN_UP(privlen)
					 - privlen;
		}
	}

	uk_netbuf_init_indir(m,
			     (void *) m + buf_offset,
			     buflen + headroom_extra,
			     headroom + headroom_extra,
			     privlen > 0 ? ((void *) m + priv_offset) : NULL,
			     dtor);

	/* Save reference to allocator that is used
	 * for free'ing this uk_netbuf.
	 */
	m->_a = a;

	return m;
}

struct uk_netbuf *uk_netbuf_prepare_buf(void *mem, size_t size,
					uint16_t headroom,
					size_t privlen, uk_netbuf_dtor_t dtor)
{
	struct uk_netbuf *m;
	size_t buf_offset = 0;
	size_t priv_offset = 0;

	UK_ASSERT(mem);
	if ((NETBUF_ADDR_ALIGN_UP(sizeof(*m))
	     + NETBUF_ADDR_ALIGN_UP(privlen)
	     + headroom) > size)
		return NULL;

	/* Place buf right behind `m` or `m->priv` region if privlen > 0.
	 * In order to keep `m->data - headroom` aligned the padding bytes
	 *  are added to the headroom.
	 * We can only do this if the given headroom stays within
	 *  uint16_t bounds after the operation.
	 */
	if (likely(UINT16_MAX - headroom > NETBUF_ADDR_ALIGNMENT)) {
		if (privlen == 0) {
			priv_offset = 0;
			buf_offset  = sizeof(*m);
			headroom   += NETBUF_ADDR_ALIGN_UP(sizeof(*m))
				      - sizeof(*m);
		} else {
			priv_offset = NETBUF_ADDR_ALIGN_UP(sizeof(*m));
			buf_offset  = priv_offset + privlen;
			headroom   += NETBUF_ADDR_ALIGN_UP(privlen)
				      - privlen;
		}
	}

	m = (struct uk_netbuf *) mem;
	uk_netbuf_init_indir(m,
			     mem + buf_offset,
			     size - buf_offset,
			     headroom,
			     privlen > 0 ? (mem + priv_offset) : NULL,
			     dtor);
	return m;
}
