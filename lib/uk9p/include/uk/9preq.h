/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Cristian Banu <cristb@gmail.com>
 *
 * Copyright (c) 2019, University Politehnica of Bucharest. All rights reserved.
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

#ifndef __UK_9PREQ__
#define __UK_9PREQ__

#include <inttypes.h>
#include <uk/config.h>
#include <uk/alloc.h>
#include <uk/essentials.h>
#include <uk/list.h>
#include <uk/refcount.h>
#if CONFIG_LIBUKSCHED
#include <uk/wait_types.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The header consists of the following fields: size (4 bytes), type (1) and
 * tag (2).
 */
#define UK_9P_HEADER_SIZE               7U

/*
 * The maximum buffer size for an error reply is given by the header (7), the
 * string size (2), the error string (128) and the error code (4): in total,
 * 141.
 */
#define UK_9P_RERROR_MAXSIZE            141U

/**
 * @internal
 *
 * Describes the 9p zero-copy direction.
 */
enum uk_9preq_zcdir {
	UK_9PREQ_ZCDIR_NONE,
	UK_9PREQ_ZCDIR_READ,
	UK_9PREQ_ZCDIR_WRITE,
};

/**
 * @internal
 *
 * Describes a 9p fcall structure.
 */
struct uk_9preq_fcall {
	/*
	 * Total size of the fcall. Initially, this is the buffer size.
	 * After ready (on xmit) or reply (on recv), this will be the size of
	 * the sent/received data.
	 */
	uint32_t                        size;
	/* Type of the fcall. Should be T* for transmit, R* for receive. */
	uint8_t                         type;
	/* Offset while serializing or deserializing. */
	uint32_t                        offset;
	/* Buffer pointer. */
	void                            *buf;

	/* Zero-copy buffer pointer. */
	void                            *zc_buf;
	/* Zero-copy buffer size. */
	uint32_t                        zc_size;
	/* Zero-copy buffer offset in the 9P message. */
	uint32_t                        zc_offset;
};

/**
 * Describes the possible states in which a request may be.
 *
 * - NONE: Right after allocating.
 * - INITIALIZED: Request is ready to receive serialization data.
 * - READY: Request is ready to be sent.
 * - RECEIVED: Transport layer has received the reply and important data such
 *   as the tag, type and size have been validated.
 */
enum uk_9preq_state {
	UK_9PREQ_NONE = 0,
	UK_9PREQ_INITIALIZED,
	UK_9PREQ_READY,
	UK_9PREQ_SENT,
	UK_9PREQ_RECEIVED
};

/**
 *  Describes a 9P request.
 *
 *  This gets allocated via uk_9pdev_req_create(), and freed when it is not
 *  referenced anymore. A call to uk_9pdev_req_remove() is mandatory to
 *  correctly free this and remove it from the list of requests managed
 *  by the 9p device.
 */
struct uk_9preq {
	/* Transmit fcall. */
	struct uk_9preq_fcall           xmit;
	/* Receive fcall. */
	struct uk_9preq_fcall           recv;
	/* State of the request. See the state enum for details. */
	enum uk_9preq_state             state;
	/* Tag allocated to this request. */
	uint16_t                        tag;
	/* Entry into the list of requests (API-internal). */
	struct uk_list_head             _list;
	/* @internal Allocator used to allocate this request. */
	struct uk_alloc                 *_a;
	/* Tracks the number of references to this structure. */
	__atomic                        refcount;
#if CONFIG_LIBUKSCHED
	/* Wait-queue for state changes. */
	struct uk_waitq                 wq;
#endif
};

/**
 * @internal
 * Allocates a 9p request.
 * Should not be used directly, use uk_9pdev_req_create() instead.
 *
 * @param a
 *   Allocator to use.
 * @param size
 *   Minimum size of the receive and transmit buffers.
 * @return
 *   - (==NULL): Out of memory.
 *   - (!=NULL): Successful.
 */
struct uk_9preq *uk_9preq_alloc(struct uk_alloc *a, uint32_t size);

/**
 * Gets the 9p request, incrementing the reference count.
 *
 * @param req
 *   Reference to the 9p request.
 */
void uk_9preq_get(struct uk_9preq *req);

/**
 * Puts the 9p request, decrementing the reference count.
 * If this was the last live reference, the memory will be freed.
 *
 * @param req
 *   Reference to the 9p request.
 * @return
 *   - 0: This was not the last live reference.
 *   - 1: This was the last live reference.
 */
int uk_9preq_put(struct uk_9preq *req);

/*
 * The following family of serialization and deserialization functions work
 * by employing a printf-like formatting mechanism for data types supported by
 * the 9p protocol:
 * - 'b': byte (uint8_t)
 * - 'w': word (uint16_t)
 * - 'd': double-word (uint32_t)
 * - 'q': quad-word (uint64_t)
 * - 's': uk_9p_str *
 * - 'S': uk_9p_stat *
 *
 * Similarly to vprintf(), the vserialize() and vdeserialize() functions take
 * a va_list instead of a variable number of arguments.
 *
 * Possible return values:
 * - 0: Operation successful.
 * - (-EINVAL): Invalid format specifier.
 * - (-ENOBUFS): End of buffer reached.
 */

int uk_9preq_vserialize(struct uk_9preq *req, const char *fmt, va_list vl);
int uk_9preq_serialize(struct uk_9preq *req, const char *fmt, ...);
int uk_9preq_vdeserialize(struct uk_9preq *req, const char *fmt, va_list vl);
int uk_9preq_deserialize(struct uk_9preq *req, const char *fmt, ...);

/**
 * Copies raw data from the request receive buffer to the provided buffer.
 *
 * @param req
 *   Reference to the 9p request.
 * @param buf
 *   Destination buffer.
 * @param size
 *   Amount to copy.
 * Possible return values:
 * - 0: Operation successful.
 * - (-ENOBUFS): End of buffer reached.
 */
int uk_9preq_copy_to(struct uk_9preq *req, void *buf, uint32_t size);

/**
 * Copies raw data from the provided buffer to the request transmission buffer.
 *
 * @param req
 *   Reference to the 9p request.
 * @param buf
 *   Source buffer.
 * @param size
 *   Amount to copy.
 * Possible return values:
 * - 0: Operation successful.
 * - (-ENOBUFS): End of buffer reached.
 */
int uk_9preq_copy_from(struct uk_9preq *req, const void *buf, uint32_t size);

/**
 * Marks the given request as being ready, transitioning between states
 * INITIALIZED and READY.
 *
 * @param req
 *   Reference to the 9p request.
 * @param zc_dir
 *   Zero-copy direction.
 * @param zc_buf
 *   Zero-copy buffer, if zc_dir is not NONE.
 * @param zc_size
 *   Zero-copy buffer size, if zc_dir is not NONE.
 * @param zc_offset
 *   Zero-copy offset within the received message, if zc_dir is READ.
 * @return
 *   - 0: Successful.
 *   - (< 0): Invalid state or request size serialization failed.
 */
int uk_9preq_ready(struct uk_9preq *req, enum uk_9preq_zcdir zc_dir,
		void *zc_buf, uint32_t zc_size, uint32_t zc_offset);

/**
 * Function called from the transport layer when a request has been received.
 * Implements the transition from the SENT to the RECEIVED state.
 *
 * @param req
 *   The 9P request.
 * @param recv_size
 *   Size of the packet received from the transport layer.
 * @return
 *   - (0): Successfully received.
 *   - (< 0): An error occurred.
 */
int uk_9preq_receive_cb(struct uk_9preq *req, uint32_t recv_size);

/**
 * Waits for the reply to be received.
 *
 * @param req
 *   The 9P request.
 * @return
 *   - (0): Successful.
 *   - (< 0): Failed. Returns the error code received from the 9P server.
 */
int uk_9preq_waitreply(struct uk_9preq *req);

/**
 * Extracts the error from the received reply.
 *
 * @param req
 *   The 9P request.
 * @return
 *   - (0): No error occurred.
 *   - (< 0): An Rerror was received, the error code is 9pfs-specific.
 */
int uk_9preq_error(struct uk_9preq *req);

#ifdef __cplusplus
}
#endif

#endif /* __UK_9PREQ__ */
