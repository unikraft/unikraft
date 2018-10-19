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
#ifndef __UK_NETBUF__
#define __UK_NETBUF__

#include <sys/types.h>
#include <inttypes.h>
#include <stddef.h>
#include <limits.h>
#include <errno.h>
#include <uk/assert.h>
#include <uk/refcount.h>
#include <uk/alloc.h>
#include <uk/essentials.h>

#ifdef __cplusplus
extern "C" {
#endif

struct uk_netbuf;

typedef void (*uk_netbuf_dtor_t)(struct uk_netbuf *);

/**
 * The netbuf structure is used to describe a single contiguous packet buffer.
 * The structure can be chained to describe a packet with multiple scattered
 * buffers.
 *
 * NETBUF
 *                  +----------------------+
 *                  |   struct uk_netbuf   |
 *                  |                      |
 *                  +----------------------+
 *
 * PRIVATE META DATA
 *         *priv -> +----------------------+
 *                  |      private meta    |
 *                  |       data area      |
 *                  +----------------------+
 *
 * PACKET BUFFER
 *          *buf -> +----------------------+ \
 *                  |      HEAD ROOM       |  |
 *                  |   ^              ^   |  |
 *         *data -> +---|--------------|---+  | contiguous
 *                  |   v              v   |  | buffer
 *                  |     PACKET DATA      |  | area
 *                  |   ^              ^   |  |
 *   *data + len -> +---|--------------|---+  |
 *                  |   v              v   |  |
 *                  |      TAIL ROOM       |  |
 * *buf + buflen -> +----------------------+ /
 *
 *
 * The private data area is intended for glue code that wants to embed stack-
 * specific data to the netbuf (e.g., `struct pbuf` for lwIP). This avoids
 * separate memory management for stack-specific packet meta-data. libuknetdev
 * or device drivers are not inspecting or modifying these data.
 *
 * The buffer region contains the packet data. `struct uk_netbuf` is pointing
 * with `*data` to the first byte of the packet data within the buffer.
 * `len` describes the number of bytes used. All packet data has to fit into the
 * given buffer. The buffer is described with the `buf` and `buflen` field.
 * These two fields should not change during the netbuf life time.
 * The available headroom bytes are calculated by subtracting the `*data`
 * pointer from `*buf` pointer. The available tailroom bytes are calculated
 * with: (*buf + buflen) - (*data + len).
 * When more packet data space is required or whenever packet data is scattered
 * in memory, netbufs can be chained.
 *
 * The netbuf structure, private meta data area, and buffer area can be backed
 * by independent memory allocations. uk_netbuf_alloc_buf() and
 * uk_netbuf_prepare_buf() are placing all these three regions into a single
 * allocation.
 */
struct uk_netbuf {
	struct uk_netbuf *next;
	struct uk_netbuf *prev;

	void *data;            /**< Payload start, is part of buf. */
	uint16_t len;          /**< Payload length (should be <= buflen). */
	__atomic refcount;     /**< Reference counter */

	void *priv;            /**< Reference to user-provided private data */

	void *buf;             /**< Start address of contiguous buffer. */
	size_t buflen;         /**< Length of buffer. */

	uk_netbuf_dtor_t dtor; /**< Destructor callback */
	struct uk_alloc *_a;   /**< @internal Allocator for free'ing */
};

/*
 * Iterator helpers for netbuf chains
 */
/* head -> tail */
#define UK_NETBUF_CHAIN_FOREACH(var, head)				\
	for ((var) = (head); (var) != NULL; (var) = (var)->next)

/* tail -> head (reverse) */
#define UK_NETBUF_CHAIN_FOREACH_R(var, tail)				\
	for ((var) = (tail); (var) != NULL; (var) = (var)->prev)

/**
 * Initializes an external allocated netbuf.
 * This netbuf has no data area assigned. It is intended
 * that m->buf, m->buflen, and m->data is initialized by the caller.
 * m->len is initialized with 0.
 * Note: On the last free, only the destructor is called,
 *       no memory is released by the API.
 * @param m
 *   reference to uk_netbuf that shall be initialized
 * @param priv
 *   Reference to external (meta) data that corresponds to this netbuf.
 * @param dtor
 *   Destructor that is called when netbuf's refcount reaches zero (recommended)
 */
#define uk_netbuf_init(m, priv, dtor)				\
	uk_netbuf_init_indir((m), NULL, 0, 0, (priv), (dtor))

/**
 * Initializes an external allocated netbuf.
 * This netbuf will point to an user-provided contiguous buffer area.
 * m->len is initialized with 0.
 * Note: On the last free, only the destructor is called,
 *       no memory is released by the API.
 * @param m
 *   reference to uk_netbuf that shall be initialized
 * @param buf
 *   Reference to buffer area that is belonging to this netbuf
 * @param buflen
 *   Size of the buffer area
 * @param headroom
 *   Number of bytes reserved as headroom from buf, `m->data` will point to
 *   to the first byte in `buf` after the headroom. `headroom` has to be smaller
 *   or equal to `buflen`.
 *   Note: Some drivers may require extra headroom space in the first netbuf of
 *         a chain in order to do a packet transmission.
 * @param priv
 *   Reference to external (meta) data that corresponds to this netbuf.
 * @param dtor
 *   Destructor that is called when netbuf's refcount reaches zero (recommended)
 */
void uk_netbuf_init_indir(struct uk_netbuf *m,
			  void *buf, size_t buflen, uint16_t headroom,
			  void *priv, uk_netbuf_dtor_t dtor);

/**
 * Allocates and initializes a netbuf.
 * This netbuf has no buffer area assigned. It is intended
 * that m->buf, m->buflen, and m->data is initialized by the caller.
 * m->len is initialized with 0.
 * Note: On the last free, the buffer area is not free'd. It is intended
 *       to provide a destructor for this operation.
 * @param a
 *   Allocator to use for allocating `struct uk_netbuf` and the
 *   corresponding private data area.
 * @param dtor
 *   Destructor that is called when netbuf is free'd (recommended)
 * @param privlen
 *   Length for reserved memory to store private data. This memory is free'd
 *   together with this netbuf. If privlen is 0, either no private data is
 *   required or external meta data corresponds to this netbuf. m->priv can be
 *   modified after the allocation.
 * @returns
 *   - (NULL): Allocation failed
 *   - initialized uk_netbuf
 */
#define uk_netbuf_alloc(a, privlen, dtor)			\
	uk_netbuf_alloc_indir((a), NULL, 0, 0, (privlen), (dtor))

/**
 * Allocate and initialize netbuf to reference to an existing
 * contiguous data area
 * m->len is initialized with 0.
 * It will not be free'd together with this netbuf on the last free call.
 * @param a
 *   Allocator to be used for allocating `struct uk_netbuf`
 *   On uk_netdev_free() and refcount == 0 the allocation is free'd
 *   to this allocator.
 * @param buf
 *   Reference to the buffer area that is belonging to this netbuf
 * @param buflen
 *   Size of the buffer area
 * @param headroom
 *   Number of bytes reserved as headroom from buf.
 *   headroom has to be smaller or equal to buflen.
 *   Please note that m->data is aligned when reserved headroom is 0.
 *   In order to keep this property align up the headroom value.
 * @param privlen
 *   Length for reserved memory to store private data. This memory is free'd
 *   together with this netbuf. If privlen is 0, either no private data is
 *   required or external meta data corresponds to this netbuf. m->priv can be
 *   modified after the allocation.
 * @param dtor
 *   Destructor that is called when netbuf is free'd (recommended)
 * @returns
 *   - (NULL): Allocation failed
 *   - initialized uk_netbuf
 */
struct uk_netbuf *uk_netbuf_alloc_indir(struct uk_alloc *a,
					void *buf, size_t buflen,
					uint16_t headroom,
					size_t privlen, uk_netbuf_dtor_t dtor);

/**
 * Allocate and initialize netbuf with data buffer area.
 * m->len is initialized with 0.
 * @param a
 *   Allocator to be used for allocating `struct uk_netbuf` and the
 *   corresponding buffer area (single allocation).
 *   On uk_netbuf_free() and refcount == 0 the allocation is free'd
 *   to this allocator.
 * @param buflen
 *   Size of the buffer area
 * @param headroom
 *   Number of bytes reserved as headroom from the buffer area.
 *   `headroom` has to be smaller or equal to `buflen`.
 *   Please note that `m->data` is aligned when `headroom` is 0.
 *   In order to keep this property when a headroom is used,
 *   it is recommended to align up the required headroom.
 * @param privlen
 *   Length for reserved memory to store private data. This memory is free'd
 *   together with this netbuf. If privlen is 0, either no private data is
 *   required or external meta data corresponds to this netbuf. m->priv can be
 *   modified after the allocation.
 * @param dtor
 *   Destructor that is called when netbuf is free'd (optional)
 * @returns
 *   - (NULL): Allocation failed
 *   - initialized uk_netbuf
 */
struct uk_netbuf *uk_netbuf_alloc_buf(struct uk_alloc *a, size_t buflen,
				      uint16_t headroom,
				      size_t privlen, uk_netbuf_dtor_t dtor);

/**
 * Initialize netbuf with data buffer on a user given allocated memory area
 * m->len is initialized with 0.
 * @param mem
 *   Reference to user provided memory region
 * @param buflen
 *   Size of the data that shall be allocated together with this netbuf
 * @param headroom
 *   Number of bytes reserved as headroom from the buffer area.
 *   `headroom` has to be smaller or equal to `buflen`.
 *   Please note that `m->data` is aligned when `headroom` is 0.
 *   In order to keep this property when a headroom is used,
 *   it is recommended to align up the required headroom.
 * @param privlen
 *   Length for reserved memory to store private data. This memory will be
 *   embedded together with this netbuf. If privlen is 0, either no private data
 *   is required or external meta data corresponds to this netbuf. m->priv can
 *   be modified after the preparation.
 * @param dtor
 *   Destructor that is called when netbuf's refcount reaches zero (recommended)
 * @returns
 *   - (NULL): given failed
 *   - initialized uk_netbuf
 */
struct uk_netbuf *uk_netbuf_prepare_buf(void *mem, size_t size,
					uint16_t headroom,
					size_t privlen, uk_netbuf_dtor_t dtor);

/**
 * Retrieves the last element of a netbuf chain
 * @param m
 *   uk_netbuf that is part of a chain
 * @returns
 *   Last uk_netbuf of the chain
 */
#define uk_netbuf_chain_last(m)						\
	({								\
		struct uk_netbuf *__ret = NULL;				\
		struct uk_netbuf *__iter;				\
		UK_NETBUF_CHAIN_FOREACH(__iter, (m))			\
			__ret = __iter;					\
									\
		(__ret);						\
	})

/**
 * Retrieves the first element of a netbuf chain
 * @param m
 *   uk_netbuf that is part of a chain
 * @returns
 *   First uk_netbuf of the chain
 */
#define uk_netbuf_chain_first(m)					\
	({								\
		struct uk_netbuf *__ret = NULL;				\
		struct uk_netbuf *__iter;				\
		UK_NETBUF_CHAIN_FOREACH_R(__iter, (m))			\
			__ret = __iter;					\
									\
		(__ret);						\
	})

/**
 * Returns the reference to private data of a netbuf
 * @param m
 *   uk_netbuf to return private data of
 */
static inline void *uk_netbuf_get_priv(struct uk_netbuf *m)
{
	UK_ASSERT(m);

	return (m)->priv;
}

/**
 * Connects two netbuf chains
 * Note: The reference count of each buffer is not checked nor modified.
 * @param headtail
 *   Last netbuf element of chain that should come first.
 *   It can also be just a single netbuf.
 * @param tail
 *   Head element of the second netbuf chain.
 *   It can also be just a single netbuf.
 */
void uk_netbuf_connect(struct uk_netbuf *headtail,
		       struct uk_netbuf *tail);

/**
 * Connects two netbuf chains
 * Note: The reference count of each buffer is not checked nor modified.
 * @param head
 *   Heading netbuf element of chain that should come first.
 *   It can also be just a single netbuf.
 * @param tail
 *   Head element of the second netbuf chain.
 *   It can also be just a single netbuf.
 */
void uk_netbuf_append(struct uk_netbuf *head,
		      struct uk_netbuf *tail);

/**
 * Disconnects a netbuf from its chain. The chain will remain
 * without the removed element.
 * Note: The reference count of each buffer is not checked nor modified.
 * @param m
 *   uk_netbuf to be removed from its chain
 * @returns
 *   - (NULL) Chain consisted only of m
 *   - Head of the remaining netbuf chain
 */
struct uk_netbuf *uk_netbuf_disconnect(struct uk_netbuf *m);

/**
 * Increase reference count of a single netbuf (irrespective of a chain)
 * @param m
 *   uk_netbuf to increase refcount of
 * @returns
 *   Reference to m
 */
static inline struct uk_netbuf *uk_netbuf_ref_single(struct uk_netbuf *m)
{
	UK_ASSERT(m);

	uk_refcount_acquire(&m->refcount);
	return m;
}

/**
 * Increase reference count of a netbuf including each successive netbuf
 * element of a chain. Note, preceding chain elements are not visited.
 * @param m
 *   head of the uk_netbuf chain to increase refcount of
 * @returns
 *   Reference to m
 */
static inline struct uk_netbuf *uk_netbuf_ref(struct uk_netbuf *m)
{
	struct uk_netbuf *iter;

	UK_ASSERT(m);

	UK_NETBUF_CHAIN_FOREACH(iter, m)
		uk_netbuf_ref_single(iter);

	return m;
}

/**
 * Returns the current reference count of a single netbuf
 * @param m
 *   uk_netbuf to return the reference count of
 */
static inline uint32_t uk_netbuf_refcount_single_get(struct uk_netbuf *m)
{
	UK_ASSERT(m);

	return uk_refcount_read(&m->refcount);
}

/**
 * Decreases the reference count of each element of the chain.
 * If a refcount becomes 0, the netbuf is disconnected from its chain,
 * its destructor is called and the memory is free'd according to its
 * allocation.
 * @param m
 *   head of uk_netbuf chain to release
 * @returns
 *   Reference to m
 */
void uk_netbuf_free(struct uk_netbuf *m);

/**
 * Decreases the reference count of a single netbuf. If refcount becomes 0,
 * the netbuf is disconnected from its chain, its destructor is called and
 * the memory is free'd according to its allocation.
 * @param m
 *   uk_netbuf to release
 * @returns
 *   Reference to m
 */
void uk_netbuf_free_single(struct uk_netbuf *m);

/**
 * Calculates the current available headroom bytes of a netbuf
 * @param m
 *   uk_netbuf to return headroom bytes of
 * @returns
 *   available headroom bytes
 */
static inline size_t uk_netbuf_headroom(struct uk_netbuf *m)
{
	UK_ASSERT(m);
	UK_ASSERT(m->buf);
	UK_ASSERT(m->data);

	return (size_t) ((uintptr_t) (m)->data - (uintptr_t) (m)->buf);
}

/**
 * Calculates the current available tailroom bytes of a netbuf
 * @param m
 *   uk_netbuf to return tailroom bytes of
 * @returns
 *   available tailroom bytes
 */
static inline size_t uk_netbuf_tailroom(struct uk_netbuf *m)
{
	UK_ASSERT(m);
	UK_ASSERT(m->buf);
	UK_ASSERT(m->data);

	return ((size_t) (((uintptr_t) (m)->buf + (uintptr_t) (m)->buflen)
			  - ((uintptr_t) (m)->data + (uintptr_t) (m)->len)));
}

/**
 * Prepends (or returns) bytes from the headroom (back) to the data area.
 * It basically moves head->data.
 * @param head
 *   uk_netbuf to modify (has to be the first netbuf of a chain)
 * @param len
 *   If > 0 takes bytes from the headroom and prepends them to data
 *   If < 0 releases the -len first bytes from data to headroom
 * @returns
 *   - (-ENOSPC): Operation could not be perform within buffer bounds
 *   - (-EFAULT): Operation could not be performed because data would get > 64kB
 *   - (1): netbuf successfully modified
 */
static inline int uk_netbuf_header(struct uk_netbuf *head, int16_t len)
{
	UK_ASSERT(head);
	UK_ASSERT(head->buf);
	UK_ASSERT(head->data);

	/* head has to be the first element of a netbuf chain. */
	UK_ASSERT(head->prev == NULL);

	/* If len > 0, we take bytes from the headroom and add
	 * them to data. We are limited to the available headroom size.
	 * If len < 0, we return bytes back to the headroom.
	 * We are limited to the available data length.
	 */
	if (unlikely((len > (ssize_t) uk_netbuf_headroom(head))
		     || (-len > head->len)))
		return -ENOSPC;

	/* We should never make the packet bigger than 64kB */
	if (unlikely((int32_t) len + (int32_t) head->len
		     > (int32_t) UINT16_MAX))
		return -EFAULT;

	head->data   -= len;
	head->len    += len;
	return 1;
}

#ifdef __cplusplus
}
#endif

#endif /* __UK_NETBUF__ */
