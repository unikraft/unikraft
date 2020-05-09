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

#include <string.h>
#include <uk/config.h>
#include <uk/9preq.h>
#include <uk/9pdev.h>
#include <uk/9p_core.h>
#include <uk/list.h>
#include <uk/refcount.h>
#include <uk/essentials.h>
#include <uk/alloc.h>
#if CONFIG_LIBUKSCHED
#include <uk/sched.h>
#include <uk/wait.h>
#endif

void uk_9preq_init(struct uk_9preq *req)
{
	req->xmit.buf = req->xmit_buf;
	req->recv.buf = req->recv_buf;
	req->xmit.size = req->recv.size = UK_9P_BUFSIZE;
	req->xmit.zc_buf = req->recv.zc_buf = NULL;
	req->xmit.zc_size = req->recv.zc_size = 0;
	req->xmit.zc_offset = req->recv.zc_offset = 0;

	/*
	 * Assume the header has already been written.
	 * The header itself will be written on uk_9preq_ready(), when the
	 * actual message size is known.
	 */
	req->xmit.offset = UK_9P_HEADER_SIZE;
	req->recv.offset = 0;

	UK_INIT_LIST_HEAD(&req->_list);
	uk_refcount_init(&req->refcount, 1);
#if CONFIG_LIBUKSCHED
	uk_waitq_init(&req->wq);
#endif
}

void uk_9preq_get(struct uk_9preq *req)
{
	uk_refcount_acquire(&req->refcount);
}

int uk_9preq_put(struct uk_9preq *req)
{
	int last;

	last = uk_refcount_release(&req->refcount);
	if (last)
		uk_9pdev_req_to_freelist(req->_dev, req);

	return last;
}

int uk_9preq_ready(struct uk_9preq *req, enum uk_9preq_zcdir zc_dir,
		void *zc_buf, uint32_t zc_size, uint32_t zc_offset)
{
	int rc;
	uint32_t total_size;
	uint32_t total_size_with_zc;

	UK_ASSERT(req);

	if (UK_READ_ONCE(req->state) != UK_9PREQ_INITIALIZED)
		return -EIO;

	/* Save current offset as the size of the message. */
	total_size = req->xmit.offset;

	total_size_with_zc = total_size;
	if (zc_dir == UK_9PREQ_ZCDIR_WRITE)
		total_size_with_zc += zc_size;

	/* Serialize the header. */
	req->xmit.offset = 0;
	if ((rc = uk_9preq_write32(req, total_size_with_zc)) < 0 ||
		(rc = uk_9preq_write8(req, req->xmit.type)) < 0 ||
		(rc = uk_9preq_write16(req, req->tag)) < 0)
		return rc;

	/* Reset offset and size to sane values. */
	req->xmit.offset = 0;
	req->xmit.size = total_size;

	/* Update zero copy buffers. */
	if (zc_dir == UK_9PREQ_ZCDIR_WRITE) {
		req->xmit.zc_buf = zc_buf;
		req->xmit.zc_size = zc_size;
		/* Zero-copy offset for writes must start at the end of buf. */
		req->xmit.zc_offset = req->xmit.size;
	} else if (zc_dir == UK_9PREQ_ZCDIR_READ) {
		req->recv.zc_buf = zc_buf;
		req->recv.zc_size = zc_size;
		req->recv.zc_offset = zc_offset;
		/* The receive buffer must end before the zc buf. */
		req->recv.size = zc_offset;
	}

	/* Update the state. */
	UK_WRITE_ONCE(req->state, UK_9PREQ_READY);

	return 0;
}

int uk_9preq_receive_cb(struct uk_9preq *req, uint32_t recv_size)
{
	uint32_t size;
	uint16_t tag;
	int rc;

	UK_ASSERT(req);

	/* Check state and the existence of the header. */
	if (UK_READ_ONCE(req->state) != UK_9PREQ_SENT)
		return -EIO;
	if (recv_size < UK_9P_HEADER_SIZE)
		return -EIO;

	/* Deserialize the header into request fields. */
	req->recv.offset = 0;
	req->recv.size = recv_size;
	if ((rc = uk_9preq_read32(req, &size)) < 0 ||
		(rc = uk_9preq_read8(req, &req->recv.type)) < 0 ||
		(rc = uk_9preq_read16(req, &tag)) < 0)
		return rc;

	/* Check sanity of deserialized values. */
	if (rc < 0)
		return rc;
	if (size > recv_size)
		return -EIO;
	if (req->tag != tag)
		return -EIO;

	/* Fix the receive size for zero-copy requests. */
	if (req->recv.zc_buf && req->recv.type != UK_9P_RERROR)
		req->recv.size = req->recv.zc_offset;
	else
		req->recv.size = size;

	/* Update the state. */
	UK_WRITE_ONCE(req->state, UK_9PREQ_RECEIVED);

#if CONFIG_LIBUKSCHED
	/* Notify any waiting threads. */
	uk_waitq_wake_up(&req->wq);
#endif

	return 0;
}

int uk_9preq_waitreply(struct uk_9preq *req)
{
	int rc;

#if CONFIG_LIBUKSCHED
	uk_waitq_wait_event(&req->wq, req->state == UK_9PREQ_RECEIVED);
#else
	while (UK_READ_ONCE(req->state) != UK_9PREQ_RECEIVED)
		;
#endif

	/* Check for 9P server-side errors. */
	rc = uk_9preq_error(req);

	return rc;
}

int uk_9preq_error(struct uk_9preq *req)
{
	uint32_t errcode;
	struct uk_9p_str error;
	int rc = 0;

	if (UK_READ_ONCE(req->state) != UK_9PREQ_RECEIVED)
		return -EIO;
	if (req->recv.type != UK_9P_RERROR)
		return 0;

	/*
	 * The request should not have had any data deserialized from it prior
	 * to this call.
	 */
	UK_BUGON(req->recv.offset != UK_9P_HEADER_SIZE);

	if ((rc = uk_9preq_readstr(req, &error)) < 0 ||
		(rc = uk_9preq_read32(req, &errcode)) < 0)
		return rc;

	uk_pr_debug("RERROR %.*s %d\n", error.size, error.data, errcode);
	if (errcode == 0 || errcode >= 512)
		return -EIO;

	return -errcode;
}
