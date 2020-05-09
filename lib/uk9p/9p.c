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

#include <uk/config.h>
#include <uk/assert.h>
#include <uk/errptr.h>
#include <uk/9p.h>
#include <uk/9pdev.h>
#include <uk/9preq.h>
#include <uk/9pfid.h>
#include <uk/trace.h>

UK_TRACEPOINT(uk_9p_trace_request_create, "");
UK_TRACEPOINT(uk_9p_trace_request_allocated, "");
UK_TRACEPOINT(uk_9p_trace_ready, "tag %u", uint16_t);
UK_TRACEPOINT(uk_9p_trace_sent, "tag %u", uint16_t);
UK_TRACEPOINT(uk_9p_trace_received, "tag %u", uint16_t);

static inline int send_and_wait_zc(struct uk_9pdev *dev, struct uk_9preq *req,
		enum uk_9preq_zcdir zc_dir, void *zc_buf, uint32_t zc_size,
		uint32_t zc_offset)
{
	int rc;

	if ((rc = uk_9preq_ready(req, zc_dir, zc_buf, zc_size, zc_offset)))
		return rc;
	uk_9p_trace_ready(req->tag);

	if ((rc = uk_9pdev_request(dev, req)))
		return rc;
	uk_9p_trace_sent(req->tag);

	if ((rc = uk_9preq_waitreply(req)))
		return rc;
	uk_9p_trace_received(req->tag);

	return 0;
}

static inline int send_and_wait_no_zc(struct uk_9pdev *dev,
		struct uk_9preq *req)
{
	return send_and_wait_zc(dev, req, UK_9PREQ_ZCDIR_NONE, NULL, 0, 0);
}

static struct uk_9preq *request_create(struct uk_9pdev *dev, uint8_t type)
{
	struct uk_9preq *req;

	uk_9p_trace_request_create();
	req = uk_9pdev_req_create(dev, type);
	if (!PTRISERR(req))
		uk_9p_trace_request_allocated();

	return req;
}

struct uk_9preq *uk_9p_version(struct uk_9pdev *dev,
		const char *requested, struct uk_9p_str *received)
{
	struct uk_9p_str requested_str;
	struct uk_9preq *req;
	int rc = 0;
	uint32_t new_msize;

	uk_9p_str_init(&requested_str, requested);

	req = request_create(dev, UK_9P_TVERSION);
	if (PTRISERR(req))
		return req;

	uk_pr_debug("TVERSION msize %u version %s\n",
			dev->msize, requested);

	if ((rc = uk_9preq_write32(req, dev->msize)) ||
		(rc = uk_9preq_writestr(req, &requested_str)) ||
		(rc = send_and_wait_no_zc(dev, req)) ||
		(rc = uk_9preq_read32(req, &new_msize)) ||
		(rc = uk_9preq_readstr(req, received)))
		goto free;

	uk_pr_debug("RVERSION msize %u version %.*s\n", new_msize,
			received->size, received->data);

	/*
	 * Note: the 9P specification mentions that new_msize <= dev->msize.
	 * However, execution can continue even if the invariant is violated
	 * and set_msize() fails, as the old message size is always within the
	 * accepted limit.
	 */
	if (!uk_9pdev_set_msize(dev, new_msize))
		uk_pr_warn("Invalid new message size.\n");

	return req;

free:
	uk_9pdev_req_remove(dev, req);
	return ERR2PTR(rc);
}

struct uk_9pfid *uk_9p_attach(struct uk_9pdev *dev, uint32_t afid,
		const char *uname, const char *aname, uint32_t n_uname)
{
	struct uk_9preq *req;
	struct uk_9pfid *fid;
	struct uk_9p_str uname_str;
	struct uk_9p_str aname_str;
	int rc;

	uk_9p_str_init(&uname_str, uname);
	uk_9p_str_init(&aname_str, aname);

	fid = uk_9pdev_fid_create(dev);
	if (PTRISERR(fid))
		return fid;

	req = request_create(dev, UK_9P_TATTACH);
	if (PTRISERR(req)) {
		uk_9pdev_fid_release(fid);
		return (void *)req;
	}

	uk_pr_debug("TATTACH fid %u afid %u uname %s aname %s n_uname %u\n",
			fid->fid, afid, uname, aname, n_uname);

	rc = 0;
	if ((rc = uk_9preq_write32(req, fid->fid)) ||
		(rc = uk_9preq_write32(req, afid)) ||
		(rc = uk_9preq_writestr(req, &uname_str)) ||
		(rc = uk_9preq_writestr(req, &aname_str)) ||
		(rc = uk_9preq_write32(req, n_uname)) ||
		(rc = send_and_wait_no_zc(dev, req)) ||
		(rc = uk_9preq_readqid(req, &fid->qid)))
		goto free;

	uk_9pdev_req_remove(dev, req);

	uk_pr_debug("RATTACH qid type %u version %u path %lu\n",
			fid->qid.type, fid->qid.version, fid->qid.path);

	return fid;

free:
	uk_9pdev_fid_release(fid);
	uk_9pdev_req_remove(dev, req);
	return ERR2PTR(rc);
}

int uk_9p_flush(struct uk_9pdev *dev, uint16_t oldtag)
{
	struct uk_9preq *req;
	int rc = 0;

	req = request_create(dev, UK_9P_TFLUSH);
	if (PTRISERR(req))
		return PTR2ERR(req);

	uk_pr_debug("TFLUSH oldtag %u\n", oldtag);
	if ((rc = uk_9preq_write16(req, oldtag)) ||
		(rc = send_and_wait_no_zc(dev, req)))
		goto out;
	uk_pr_debug("RFLUSH\n");

out:
	uk_9pdev_req_remove(dev, req);
	return rc;
}

struct uk_9pfid *uk_9p_walk(struct uk_9pdev *dev, struct uk_9pfid *fid,
		const char *name)
{
	struct uk_9preq *req;
	struct uk_9pfid *newfid;
	struct uk_9p_str name_str;
	uint16_t nwqid;
	uint16_t nwname;
	int rc = 0;

	uk_9p_str_init(&name_str, name);

	newfid = uk_9pdev_fid_create(dev);
	if (PTRISERR(newfid))
		return newfid;

	nwname = name ? 1 : 0;

	req = request_create(dev, UK_9P_TWALK);
	if (PTRISERR(req)) {
		rc = PTR2ERR(req);
		goto out;
	}

	uk_pr_debug("TWALK fid %u newfid %u nwname %d name %s\n",
		fid->fid, newfid->fid, nwname, name ? name : "<NULL>");

	if ((rc = uk_9preq_write32(req, fid->fid)) ||
		(rc = uk_9preq_write32(req, newfid->fid)) ||
		(rc = uk_9preq_write16(req, nwname)))
		goto out;
	if (name && (rc = uk_9preq_writestr(req, &name_str)))
		goto out;

	if ((rc = send_and_wait_no_zc(dev, req))) {
		/*
		 * Don't clunk if request has finished with error, as the fid
		 * is invalid.
		 */
		newfid->was_removed = 1;
		goto out;
	}

	if ((rc = uk_9preq_read16(req, &nwqid)))
		goto out;

	uk_pr_debug("RWALK nwqid %u\n", nwqid);

	if (nwqid != nwname) {
		rc = -ENOENT;
		goto out;
	}

	if (nwname) {
		if ((rc = uk_9preq_readqid(req, &newfid->qid)))
			goto out;
	} else
		newfid->qid = fid->qid;

out:
	uk_9pdev_req_remove(dev, req);
	if (rc) {
		uk_9pdev_fid_release(newfid);
		return ERR2PTR(rc);
	}

	return newfid;
}

int uk_9p_open(struct uk_9pdev *dev, struct uk_9pfid *fid, uint8_t mode)
{
	struct uk_9preq *req;
	int rc = 0;

	req = request_create(dev, UK_9P_TOPEN);
	if (PTRISERR(req))
		return PTR2ERR(req);

	uk_pr_debug("TOPEN fid %u mode %u\n", fid->fid, mode);

	if ((rc = uk_9preq_write32(req, fid->fid)) ||
		(rc = uk_9preq_write8(req, mode)) ||
		(rc = send_and_wait_no_zc(dev, req)) ||
		(rc = uk_9preq_readqid(req, &fid->qid)) ||
		(rc = uk_9preq_read32(req, &fid->iounit)))
		goto out;

	uk_pr_debug("ROPEN qid type %u version %u path %lu iounit %u\n",
			fid->qid.type, fid->qid.version, fid->qid.path,
			fid->iounit);

out:
	uk_9pdev_req_remove(dev, req);
	return rc;
}

int uk_9p_create(struct uk_9pdev *dev, struct uk_9pfid *fid,
		const char *name, uint32_t perm, uint8_t mode,
		const char *extension)
{
	struct uk_9preq *req;
	struct uk_9p_str name_str;
	struct uk_9p_str extension_str;
	int rc = 0;

	uk_9p_str_init(&name_str, name);
	uk_9p_str_init(&extension_str, extension);

	req = request_create(dev, UK_9P_TCREATE);
	if (PTRISERR(req))
		return PTR2ERR(req);

	uk_pr_debug("TOPEN fid %u mode %u\n", fid->fid, mode);

	if ((rc = uk_9preq_write32(req, fid->fid)) ||
		(rc = uk_9preq_writestr(req, &name_str)) ||
		(rc = uk_9preq_write32(req, perm)) ||
		(rc = uk_9preq_write8(req, mode)) ||
		(rc = uk_9preq_writestr(req, &extension_str)) ||
		(rc = send_and_wait_no_zc(dev, req)) ||
		(rc = uk_9preq_readqid(req, &fid->qid)) ||
		(rc = uk_9preq_read32(req, &fid->iounit)))
		goto out;

	uk_pr_debug("RCREATE qid type %u version %u path %lu iounit %u\n",
			fid->qid.type, fid->qid.version, fid->qid.path,
			fid->iounit);

out:
	uk_9pdev_req_remove(dev, req);
	return rc;
}

int uk_9p_remove(struct uk_9pdev *dev, struct uk_9pfid *fid)
{
	struct uk_9preq *req;
	int rc = 0;

	/* The fid is considered invalid even if the remove fails. */
	fid->was_removed = 1;

	req = request_create(dev, UK_9P_TREMOVE);
	if (PTRISERR(req))
		return PTR2ERR(req);

	uk_pr_debug("TREMOVE fid %u\n", fid->fid);
	if ((rc = uk_9preq_write32(req, fid->fid)) ||
		(rc = send_and_wait_no_zc(dev, req)))
		goto out;
	uk_pr_debug("RREMOVE\n");

out:
	uk_9pdev_req_remove(dev, req);
	return rc;
}

int uk_9p_clunk(struct uk_9pdev *dev, struct uk_9pfid *fid)
{
	struct uk_9preq *req;
	int rc = 0;

	if (fid->was_removed)
		return 0;

	req = request_create(dev, UK_9P_TCLUNK);
	if (PTRISERR(req))
		return PTR2ERR(req);

	uk_pr_debug("TCLUNK fid %u\n", fid->fid);
	if ((rc = uk_9preq_write32(req, fid->fid)) ||
		(rc = send_and_wait_no_zc(dev, req)))
		goto out;
	uk_pr_debug("RCLUNK\n");

out:
	uk_9pdev_req_remove(dev, req);
	return rc;
}

int64_t uk_9p_read(struct uk_9pdev *dev, struct uk_9pfid *fid,
		uint64_t offset, uint32_t count, char *buf)
{
	struct uk_9preq *req;
	int64_t rc;

	if (fid->iounit != 0)
		count = MIN(count, fid->iounit);
	count = MIN(count, dev->msize - 11);

	uk_pr_debug("TREAD fid %u offset %lu count %u\n", fid->fid,
			offset, count);

	req = request_create(dev, UK_9P_TREAD);
	if (PTRISERR(req))
		return PTR2ERR(req);

	if ((rc = uk_9preq_write32(req, fid->fid)) ||
		(rc = uk_9preq_write64(req, offset)) ||
		(rc = uk_9preq_write32(req, count)) ||
		(rc = send_and_wait_zc(dev, req, UK_9PREQ_ZCDIR_READ, buf,
				       count, 11)) ||
		(rc = uk_9preq_read32(req, &count)))
		goto out;

	uk_pr_debug("RREAD count %u\n", count);

	rc = count;

out:
	uk_9pdev_req_remove(dev, req);
	return rc;
}

int64_t uk_9p_write(struct uk_9pdev *dev, struct uk_9pfid *fid,
		uint64_t offset, uint32_t count, const char *buf)
{
	struct uk_9preq *req;
	int64_t rc;

	count = MIN(count, fid->iounit);
	count = MIN(count, dev->msize - 23);

	uk_pr_debug("TWRITE fid %u offset %lu count %u\n", fid->fid,
			offset, count);
	req = request_create(dev, UK_9P_TWRITE);
	if (PTRISERR(req))
		return PTR2ERR(req);

	if ((rc = uk_9preq_write32(req, fid->fid)) ||
		(rc = uk_9preq_write64(req, offset)) ||
		(rc = uk_9preq_write32(req, count)) ||
		(rc = send_and_wait_zc(dev, req, UK_9PREQ_ZCDIR_WRITE,
				(void *)buf, count, 23)) ||
		(rc = uk_9preq_read32(req, &count)))
		goto out;

	uk_pr_debug("RWRITE count %u\n", count);

	rc = count;

out:
	uk_9pdev_req_remove(dev, req);
	return rc;
}

struct uk_9preq *uk_9p_stat(struct uk_9pdev *dev, struct uk_9pfid *fid,
		struct uk_9p_stat *stat)
{
	struct uk_9preq *req;
	int rc = 0;
	uint16_t dummy;

	req = request_create(dev, UK_9P_TSTAT);
	if (PTRISERR(req))
		return req;

	uk_pr_debug("TSTAT fid %u\n", fid->fid);

	if ((rc = uk_9preq_write32(req, fid->fid)) ||
		(rc = send_and_wait_no_zc(dev, req)) ||
		(rc = uk_9preq_read16(req, &dummy)) ||
		(rc = uk_9preq_readstat(req, stat)))
		goto out;

	uk_pr_debug("RSTAT\n");

	return req;

out:
	uk_9pdev_req_remove(dev, req);
	return ERR2PTR(rc);
}

int uk_9p_wstat(struct uk_9pdev *dev, struct uk_9pfid *fid,
		struct uk_9p_stat *stat)
{
	struct uk_9preq *req;
	int rc = 0;
	uint16_t *dummy;

	req = request_create(dev, UK_9P_TWSTAT);
	if (PTRISERR(req))
		return PTR2ERR(req);

	uk_pr_debug("TWSTAT fid %u\n", fid->fid);

	if ((rc = uk_9preq_write32(req, fid->fid)))
		goto out;

	dummy = (uint16_t *)(req->xmit.buf + req->xmit.offset);
	if ((rc = uk_9preq_write16(req, 0)) ||
		(rc = uk_9preq_writestat(req, stat)))
		goto out;
	*dummy = stat->size + 2;

	if ((rc = send_and_wait_no_zc(dev, req)))
		goto out;

	uk_pr_debug("RWSTAT\n");

out:
	uk_9pdev_req_remove(dev, req);
	return rc;
}
