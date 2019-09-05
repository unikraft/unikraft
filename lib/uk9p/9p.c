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

struct uk_9preq *uk_9p_version(struct uk_9pdev *dev,
		const char *requested, struct uk_9p_str *received)
{
	struct uk_9p_str requested_str;
	struct uk_9preq *req;
	int rc;
	uint32_t new_msize;

	uk_9p_str_init(&requested_str, requested);

	uk_pr_debug("TVERSION msize %u version %s\n",
			dev->msize, requested);

	req = uk_9pdev_call(dev, UK_9P_TVERSION, __PAGE_SIZE, "ds",
			dev->msize, &requested_str);
	if (PTRISERR(req))
		return req;

	rc = uk_9preq_deserialize(req, "ds", &new_msize, received);

	if (rc)
		return ERR2PTR(rc);

	uk_pr_debug("RVERSION msize %u version %.*s\n", new_msize,
			received->size, received->data);

	/*
	 * Note: the 9P specification mentions that new_msize <= dev->msize.
	 * Howevver, execution can continue even if the invariant is violated
	 * and set_msize() fails, as the old message size is always within the
	 * accepted limit.
	 */
	if (!uk_9pdev_set_msize(dev, new_msize))
		uk_pr_debug("Invalid new message size.\n");

	return req;
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

	uk_pr_debug("TATTACH fid %u afid %u uname %s aname %s n_uname %u\n",
			fid->fid, afid, uname, aname, n_uname);

	req = uk_9pdev_call(dev, UK_9P_TATTACH, __PAGE_SIZE, "ddssd",
			fid->fid, afid, &uname_str, &aname_str, n_uname);
	if (PTRISERR(req)) {
		uk_9pdev_fid_release(fid);
		return (void *)req;
	}

	rc = uk_9preq_deserialize(req, "Q", &fid->qid);
	uk_9pdev_req_remove(dev, req);

	uk_pr_debug("RATTACH qid type %u version %u path %lu\n",
			fid->qid.type, fid->qid.version, fid->qid.path);

	if (rc < 0) {
		uk_9pdev_fid_release(fid);
		return ERR2PTR(rc);
	}

	return fid;
}

int uk_9p_flush(struct uk_9pdev *dev, uint16_t oldtag)
{
	struct uk_9preq *req;

	uk_pr_debug("TFLUSH oldtag %u\n", oldtag);
	req = uk_9pdev_call(dev, UK_9P_TFLUSH, __PAGE_SIZE, "w", oldtag);
	if (PTRISERR(req))
		return PTR2ERR(req);

	uk_pr_debug("RFLUSH\n");
	uk_9pdev_req_remove(dev, req);

	return 0;
}

struct uk_9pfid *uk_9p_walk(struct uk_9pdev *dev, struct uk_9pfid *fid,
		const char *name)
{
	struct uk_9preq *req;
	struct uk_9pfid *newfid;
	struct uk_9p_str name_str;
	uint16_t nwqid;
	uint16_t nwname;
	int rc;

	uk_9p_str_init(&name_str, name);

	newfid = uk_9pdev_fid_create(dev);
	if (PTRISERR(newfid))
		return newfid;

	nwname = name ? 1 : 0;

	if (name) {
		uk_pr_debug("TWALK fid %u newfid %u nwname %d name %s\n",
				fid->fid, newfid->fid, nwname, name);
		req = uk_9pdev_call(dev, UK_9P_TWALK, __PAGE_SIZE, "ddws",
				fid->fid, newfid->fid, nwname, &name_str);
	} else {
		uk_pr_debug("TWALK fid %u newfid %u nwname %d\n",
				fid->fid, newfid->fid, nwname);
		req = uk_9pdev_call(dev, UK_9P_TWALK, __PAGE_SIZE, "ddw",
				fid->fid, newfid->fid, nwname);
	}

	if (PTRISERR(req)) {
		/*
		 * Don't clunk if request has finished with error, as the fid
		 * is invalid.
		 */
		newfid->was_removed = 1;
		rc = PTR2ERR(req);
		goto out;
	}

	rc = uk_9preq_deserialize(req, "w", &nwqid);
	if (rc < 0)
		goto out_req;

	uk_pr_debug("RWALK nwqid %u\n", nwqid);

	if (nwqid != nwname) {
		rc = -ENOENT;
		goto out_req;
	}


	if (nwname) {
		rc = uk_9preq_deserialize(req, "Q", &newfid->qid);
		if (rc < 0)
			goto out_req;
	} else
		newfid->qid = fid->qid;

	rc = 0;
out_req:
	uk_9pdev_req_remove(dev, req);
out:
	if (rc) {
		uk_9pdev_fid_release(newfid);
		return ERR2PTR(rc);
	}

	return newfid;
}

int uk_9p_open(struct uk_9pdev *dev, struct uk_9pfid *fid, uint8_t mode)
{
	struct uk_9preq *req;
	int rc;

	uk_pr_debug("TOPEN fid %u mode %u\n", fid->fid, mode);

	req = uk_9pdev_call(dev, UK_9P_TOPEN, __PAGE_SIZE, "db",
			fid->fid, mode);
	if (PTRISERR(req))
		return PTR2ERR(req);

	rc = uk_9preq_deserialize(req, "Qd", &fid->qid, &fid->iounit);
	uk_9pdev_req_remove(dev, req);

	uk_pr_debug("ROPEN qid type %u version %u path %lu iounit %u\n",
			fid->qid.type, fid->qid.version, fid->qid.path,
			fid->iounit);

	return rc;
}

int uk_9p_create(struct uk_9pdev *dev, struct uk_9pfid *fid,
		const char *name, uint32_t perm, uint8_t mode,
		const char *extension)
{
	struct uk_9preq *req;
	struct uk_9p_str name_str;
	struct uk_9p_str extension_str;
	int rc;

	uk_9p_str_init(&name_str, name);
	uk_9p_str_init(&extension_str, extension);

	uk_pr_debug("TCREATE fid %u name %s perm %u mode %u ext %s\n",
			fid->fid, name, perm, mode, extension);

	req = uk_9pdev_call(dev, UK_9P_TCREATE, __PAGE_SIZE, "dsdbs",
			fid->fid, &name_str, perm, mode, &extension_str);
	if (PTRISERR(req))
		return PTR2ERR(req);

	rc = uk_9preq_deserialize(req, "Qd", &fid->qid, &fid->iounit);
	uk_9pdev_req_remove(dev, req);

	uk_pr_debug("RCREATE qid type %u version %u path %lu iounit %u\n",
			fid->qid.type, fid->qid.version, fid->qid.path,
			fid->iounit);

	return rc;
}

int uk_9p_remove(struct uk_9pdev *dev, struct uk_9pfid *fid)
{
	struct uk_9preq *req;

	/* The fid is considered invalid even if the remove fails. */
	fid->was_removed = 1;

	uk_pr_debug("TREMOVE fid %u\n", fid->fid);
	req = uk_9pdev_call(dev, UK_9P_TREMOVE, __PAGE_SIZE, "d", fid->fid);
	if (PTRISERR(req))
		return PTR2ERR(req);

	uk_9pdev_req_remove(dev, req);
	uk_pr_debug("RREMOVE\n");

	return 0;
}

int uk_9p_clunk(struct uk_9pdev *dev, struct uk_9pfid *fid)
{
	struct uk_9preq *req;

	if (fid->was_removed)
		return 0;

	uk_pr_debug("TCLUNK fid %u\n", fid->fid);
	req = uk_9pdev_call(dev, UK_9P_TCLUNK, __PAGE_SIZE, "d", fid->fid);
	if (PTRISERR(req))
		return PTR2ERR(req);

	uk_9pdev_req_remove(dev, req);
	uk_pr_debug("RCLUNK\n");

	return 0;
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

	req = uk_9pdev_req_create(dev, UK_9P_TREAD, __PAGE_SIZE);
	if (PTRISERR(req))
		return PTR2ERR(req);

	rc = uk_9preq_serialize(req, "dqd", fid->fid, offset, count);
	if (rc < 0)
		goto out;

	rc = uk_9preq_ready(req, UK_9PREQ_ZCDIR_READ, buf, count, 11);
	if (rc < 0)
		goto out;

	rc = uk_9pdev_request(dev, req);
	if (rc < 0)
		goto out;

	rc = uk_9preq_waitreply(req);
	if (rc < 0)
		goto out;

	rc = uk_9preq_deserialize(req, "d", &count);
	if (rc < 0)
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
	req = uk_9pdev_req_create(dev, UK_9P_TWRITE, __PAGE_SIZE);
	if (PTRISERR(req))
		return PTR2ERR(req);

	rc = uk_9preq_serialize(req, "dqd", fid->fid, offset, count);
	if (rc < 0)
		goto out;

	rc = uk_9preq_ready(req, UK_9PREQ_ZCDIR_WRITE, (void *)buf, count, 23);
	if (rc < 0)
		goto out;

	rc = uk_9pdev_request(dev, req);
	if (rc < 0)
		goto out;

	rc = uk_9preq_waitreply(req);
	if (rc < 0)
		goto out;

	rc = uk_9preq_deserialize(req, "d", &count);
	if (rc < 0)
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
	int rc;
	uint16_t dummy;

	uk_pr_debug("TSTAT fid %u\n", fid->fid);
	req = uk_9pdev_call(dev, UK_9P_TSTAT, __PAGE_SIZE, "d", fid->fid);
	if (PTRISERR(req))
		return req;

	rc = uk_9preq_deserialize(req, "wS", &dummy, stat);
	if (rc)
		return ERR2PTR(rc);
	uk_pr_debug("RSTAT\n");

	return req;
}

int uk_9p_wstat(struct uk_9pdev *dev, struct uk_9pfid *fid,
		struct uk_9p_stat *stat)
{
	struct uk_9preq *req;

	/*
	 * The packed size of stat is 61 bytes + the size occupied by the
	 * strings.
	 */
	stat->size = 61;
	stat->size += stat->name.size;
	stat->size += stat->uid.size;
	stat->size += stat->gid.size;
	stat->size += stat->muid.size;
	stat->size += stat->extension.size;

	uk_pr_debug("TWSTAT fid %u\n", fid->fid);
	req = uk_9pdev_call(dev, UK_9P_TWSTAT, __PAGE_SIZE, "dwS", fid->fid,
			stat->size + 2, stat);
	if (PTRISERR(req))
		return PTR2ERR(req);
	uk_9pdev_req_remove(dev, req);
	uk_pr_debug("RWSTAT");

	return 0;
}
