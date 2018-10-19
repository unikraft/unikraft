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

#ifdef __cplusplus
}
#endif

#endif /* __UK_NETBUF__ */
