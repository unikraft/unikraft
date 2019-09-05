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
#include <uk/9p_core.h>
#include <uk/list.h>
#include <uk/refcount.h>
#include <uk/essentials.h>
#include <uk/alloc.h>
#if CONFIG_LIBUKSCHED
#include <uk/sched.h>
#include <uk/wait.h>
#endif

static int _fcall_alloc(struct uk_alloc *a, struct uk_9preq_fcall *f,
			uint32_t size)
{
	UK_ASSERT(a);
	UK_ASSERT(f);
	UK_ASSERT(size > 0);

	f->buf = uk_calloc(a, size, sizeof(char));
	if (f->buf == NULL)
		return -ENOMEM;

	f->size = size;
	f->offset = 0;
	f->zc_buf = NULL;
	f->zc_size = 0;
	f->zc_offset = 0;

	return 0;
}

static void _fcall_free(struct uk_alloc *a, struct uk_9preq_fcall *f)
{
	UK_ASSERT(a);
	UK_ASSERT(f);

	if (f->buf)
		uk_free(a, f->buf);
}

struct uk_9preq *uk_9preq_alloc(struct uk_alloc *a, uint32_t size)
{
	struct uk_9preq *req;
	int rc;

	req = uk_calloc(a, 1, sizeof(*req));
	if (req == NULL)
		goto out;

	rc = _fcall_alloc(a, &req->xmit, size);
	if (rc < 0)
		goto out_free;

	rc = _fcall_alloc(a, &req->recv, MAX(size, UK_9P_RERROR_MAXSIZE));
	if (rc < 0)
		goto out_free;

	UK_INIT_LIST_HEAD(&req->_list);
	req->_a = a;
	uk_refcount_init(&req->refcount, 1);
#if CONFIG_LIBUKSCHED
	uk_waitq_init(&req->wq);
#endif

	/*
	 * Assume the header has already been written.
	 * The header itself will be written on uk_9preq_ready(), when the
	 * actual message size is known.
	 */
	req->xmit.offset = UK_9P_HEADER_SIZE;

	return req;

out_free:
	_fcall_free(a, &req->recv);
	_fcall_free(a, &req->xmit);
	uk_free(a, req);
out:
	return NULL;
}

static void _req_free(struct uk_9preq *req)
{
	_fcall_free(req->_a, &req->recv);
	_fcall_free(req->_a, &req->xmit);
	uk_free(req->_a, req);
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
		_req_free(req);

	return last;
}

static int _fcall_write(struct uk_9preq_fcall *fcall, const void *buf,
		uint32_t size)
{
	if (fcall->offset + size > fcall->size)
		return -ENOBUFS;

	memcpy((char *)fcall->buf + fcall->offset, buf, size);
	fcall->offset += size;
	return 0;
}

static int _fcall_serialize(struct uk_9preq_fcall *f, const char *fmt, ...);

static int _fcall_vserialize(struct uk_9preq_fcall *fcall, const char *fmt,
			va_list vl)
{
	int rc = 0;

	while (*fmt) {
		switch (*fmt) {
		case 'b': {
			uint8_t x;

			x = va_arg(vl, unsigned int);
			rc = _fcall_write(fcall, &x, sizeof(x));
			if (rc < 0)
				goto out;
			break;
		}
		case 'w': {
			uint16_t x;

			x = va_arg(vl, unsigned int);
			rc = _fcall_write(fcall, &x, sizeof(x));
			if (rc < 0)
				goto out;
			break;
		}
		case 'd': {
			uint32_t x;

			x = va_arg(vl, uint32_t);
			rc = _fcall_write(fcall, &x, sizeof(x));
			if (rc < 0)
				goto out;
			break;
		}
		case 'q': {
			uint64_t x;

			x = va_arg(vl, uint64_t);
			rc = _fcall_write(fcall, &x, sizeof(x));
			if (rc < 0)
				goto out;
			break;
		}
		case 's': {
			struct uk_9p_str *p;

			p = va_arg(vl, struct uk_9p_str *);
			rc = _fcall_write(fcall, &p->size, sizeof(p->size));
			if (rc < 0)
				goto out;
			rc = _fcall_write(fcall, p->data, p->size);
			if (rc < 0)
				goto out;
			break;
		}
		case 'Q': {
			struct uk_9p_qid *p;

			p = va_arg(vl, struct uk_9p_qid *);
			rc = _fcall_serialize(fcall, "bdq", p->type,
					p->version, p->path);
			if (rc < 0)
				goto out;
			break;
		}
		case 'S': {
			struct uk_9p_stat *p;

			p = va_arg(vl, struct uk_9p_stat *);
			rc = _fcall_serialize(fcall, "wwdQdddqsssssddd",
					p->size, p->type, p->dev, &p->qid,
					p->mode, p->atime, p->mtime, p->length,
					&p->name, &p->uid, &p->gid, &p->muid,
					&p->extension, p->n_uid, p->n_gid,
					p->n_muid);
			if (rc < 0)
				goto out;
			break;
		}
		default:
			rc = -EINVAL;
			goto out;
		}

		fmt++;
	}

out:
	return rc;
}

static int _fcall_serialize(struct uk_9preq_fcall *f, const char *fmt, ...)
{
	va_list vl;
	int rc;

	va_start(vl, fmt);
	rc = _fcall_vserialize(f, fmt, vl);
	va_end(vl);

	return rc;
}

int uk_9preq_vserialize(struct uk_9preq *req, const char *fmt, va_list vl)
{
	int rc;

	UK_ASSERT(req);
	UK_ASSERT(UK_READ_ONCE(req->state) == UK_9PREQ_INITIALIZED);
	rc = _fcall_vserialize(&req->xmit, fmt, vl);

	return rc;
}

int uk_9preq_serialize(struct uk_9preq *req, const char *fmt, ...)
{
	va_list vl;
	int rc;

	va_start(vl, fmt);
	rc = uk_9preq_vserialize(req, fmt, vl);
	va_end(vl);

	return rc;
}

static int _fcall_read(struct uk_9preq_fcall *fcall, void *buf, uint32_t size)
{
	if (fcall->offset + size > fcall->size)
		return -ENOBUFS;

	memcpy(buf, (char *)fcall->buf + fcall->offset, size);
	fcall->offset += size;
	return 0;
}

static int _fcall_deserialize(struct uk_9preq_fcall *f, const char *fmt, ...);

static int _fcall_vdeserialize(struct uk_9preq_fcall *fcall,
			      const char *fmt,
			      va_list vl)
{
	int rc = 0;

	while (*fmt) {
		switch (*fmt) {
		case 'b': {
			uint8_t *x;

			x = va_arg(vl, uint8_t *);
			rc = _fcall_read(fcall, x, sizeof(*x));
			if (rc < 0)
				goto out;
			break;
		}
		case 'w': {
			uint16_t *x;

			x = va_arg(vl, uint16_t *);
			rc = _fcall_read(fcall, x, sizeof(*x));
			if (rc < 0)
				goto out;
			break;
		}
		case 'd': {
			uint32_t *x;

			x = va_arg(vl, uint32_t *);
			rc = _fcall_read(fcall, x, sizeof(*x));
			if (rc < 0)
				goto out;
			break;
		}
		case 'q': {
			uint64_t *x;

			x = va_arg(vl, uint64_t *);
			rc = _fcall_read(fcall, x, sizeof(*x));
			if (rc < 0)
				goto out;
			break;
		}
		case 's': {
			struct uk_9p_str *p;

			p = va_arg(vl, struct uk_9p_str *);
			rc = _fcall_read(fcall, &p->size, sizeof(p->size));
			if (rc < 0)
				goto out;
			p->data = (char *)fcall->buf + fcall->offset;
			fcall->offset += p->size;
			break;
		}
		case 'Q': {
			struct uk_9p_qid *p;

			p = va_arg(vl, struct uk_9p_qid *);
			rc = _fcall_deserialize(fcall, "bdq", &p->type,
					&p->version, &p->path);
			if (rc < 0)
				goto out;
			break;
		}
		case 'S': {
			struct uk_9p_stat *p;

			p = va_arg(vl, struct uk_9p_stat *);
			rc = _fcall_deserialize(fcall, "wwdQdddqsssssddd",
					&p->size, &p->type, &p->dev, &p->qid,
					&p->mode, &p->atime, &p->mtime,
					&p->length, &p->name, &p->uid, &p->gid,
					&p->muid, &p->extension, &p->n_uid,
					&p->n_gid, &p->n_muid);
			if (rc < 0)
				goto out;
			break;
		}
		default:
			rc = -EINVAL;
			goto out;
		}

		fmt++;
	}

out:
	return rc;
}

static int _fcall_deserialize(struct uk_9preq_fcall *f, const char *fmt, ...)
{
	va_list vl;
	int rc;

	va_start(vl, fmt);
	rc = _fcall_vdeserialize(f, fmt, vl);
	va_end(vl);

	return rc;
}

int uk_9preq_vdeserialize(struct uk_9preq *req, const char *fmt, va_list vl)
{
	int rc;

	UK_ASSERT(req);
	UK_ASSERT(UK_READ_ONCE(req->state) == UK_9PREQ_RECEIVED);
	rc = _fcall_vdeserialize(&req->recv, fmt, vl);

	return rc;
}

int uk_9preq_deserialize(struct uk_9preq *req, const char *fmt, ...)
{
	va_list vl;
	int rc;

	va_start(vl, fmt);
	rc = uk_9preq_vdeserialize(req, fmt, vl);
	va_end(vl);

	return rc;
}

int uk_9preq_copy_to(struct uk_9preq *req, void *buf, uint32_t size)
{
	return _fcall_read(&req->recv, buf, size);
}

int uk_9preq_copy_from(struct uk_9preq *req, const void *buf, uint32_t size)
{
	return _fcall_write(&req->xmit, buf, size);
}

int uk_9preq_ready(struct uk_9preq *req, enum uk_9preq_zcdir zc_dir,
		void *zc_buf, uint32_t zc_size, uint32_t zc_offset)
{
	int rc;
	uint32_t total_size;
	uint32_t total_size_with_zc;

	UK_ASSERT(req);

	if (UK_READ_ONCE(req->state) != UK_9PREQ_INITIALIZED) {
		rc = -EIO;
		goto out;
	}

	/* Save current offset as the size of the message. */
	total_size = req->xmit.offset;

	total_size_with_zc = total_size;
	if (zc_dir == UK_9PREQ_ZCDIR_WRITE)
		total_size_with_zc += zc_size;

	/* Serialize the header. */
	req->xmit.offset = 0;
	rc = uk_9preq_serialize(req, "dbw", total_size_with_zc, req->xmit.type,
			req->tag);
	if (rc < 0)
		goto out;

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

out:
	return rc;
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
	rc = _fcall_deserialize(&req->recv, "dbw", &size,
			&req->recv.type, &tag);

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

	rc = uk_9preq_deserialize(req, "sd", &error, &errcode);
	if (rc < 0)
		return rc;

	uk_pr_debug("RERROR %.*s %d\n", error.size, error.data, errcode);
	if (errcode == 0 || errcode >= 512)
		return -EIO;

	return -errcode;
}
