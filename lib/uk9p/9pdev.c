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

#include <stdbool.h>
#include <string.h>
#include <uk/config.h>
#include <uk/plat/spinlock.h>
#include <uk/alloc.h>
#include <uk/essentials.h>
#include <uk/errptr.h>
#include <uk/bitmap.h>
#include <uk/refcount.h>
#include <uk/wait.h>
#include <uk/9p.h>
#include <uk/9pdev.h>
#include <uk/9pdev_trans.h>
#include <uk/9preq.h>
#include <uk/9pfid.h>
#if CONFIG_LIBUKSCHED
#include <uk/sched.h>
#include <uk/wait.h>
#endif

static void _fid_mgmt_init(struct uk_9pdev_fid_mgmt *fid_mgmt)
{
	ukarch_spin_lock_init(&fid_mgmt->spinlock);
	fid_mgmt->next_fid = 0;
	UK_INIT_LIST_HEAD(&fid_mgmt->fid_free_list);
	UK_INIT_LIST_HEAD(&fid_mgmt->fid_active_list);
}

static int _fid_mgmt_next_fid_locked(struct uk_9pdev_fid_mgmt *fid_mgmt,
				struct uk_9pdev *dev,
				struct uk_9pfid **fid)
{
	struct uk_9pfid *result = NULL;

	if (!uk_list_empty(&fid_mgmt->fid_free_list)) {
		result = uk_list_first_entry(&fid_mgmt->fid_free_list,
				struct uk_9pfid, _list);
		uk_list_del(&result->_list);
	} else {
		result = uk_9pfid_alloc(dev);
		if (!result)
			return -ENOMEM;
		result->fid = fid_mgmt->next_fid++;
	}

	uk_refcount_init(&result->refcount, 1);
	result->was_removed = 0;
	*fid = result;

	return 0;
}

static void _fid_mgmt_add_fid_locked(struct uk_9pdev_fid_mgmt *fid_mgmt,
				struct uk_9pfid *fid)
{
	uk_list_add(&fid->_list, &fid_mgmt->fid_active_list);
}

static void _fid_mgmt_del_fid_locked(struct uk_9pdev_fid_mgmt *fid_mgmt,
				struct uk_9pfid *fid,
				bool move_to_freelist)
{
	uk_list_del(&fid->_list);

	if (move_to_freelist)
		uk_list_add(&fid->_list, &fid_mgmt->fid_free_list);
	else {
		/*
		 * Free the memory associated. This fid will never be used
		 * again.
		 */
		uk_pr_warn("Could not move fid to freelist, freeing memory.\n");
		uk_free(fid->_dev->a, fid);
	}
}

static void _fid_mgmt_cleanup(struct uk_9pdev_fid_mgmt *fid_mgmt)
{
	unsigned long flags;
	struct uk_9pfid *fid, *fidn;

	ukplat_spin_lock_irqsave(&fid_mgmt->spinlock, flags);
	/*
	 * Every fid should have been clunked *before* destroying the
	 * connection.
	 */
	UK_ASSERT(uk_list_empty(&fid_mgmt->fid_active_list));
	uk_list_for_each_entry_safe(fid, fidn, &fid_mgmt->fid_free_list,
			_list) {
		uk_list_del(&fid->_list);
		uk_free(fid->_dev->a, fid);
	}
	ukplat_spin_unlock_irqrestore(&fid_mgmt->spinlock, flags);
}

static void _req_mgmt_init(struct uk_9pdev_req_mgmt *req_mgmt)
{
	ukarch_spin_lock_init(&req_mgmt->spinlock);
	uk_bitmap_zero(req_mgmt->tag_bm, UK_9P_NUMTAGS);
	UK_INIT_LIST_HEAD(&req_mgmt->req_list);
	UK_INIT_LIST_HEAD(&req_mgmt->req_free_list);
}

static void _req_mgmt_add_req_locked(struct uk_9pdev_req_mgmt *req_mgmt,
				struct uk_9preq *req)
{
	uk_bitmap_set(req_mgmt->tag_bm, req->tag, 1);
	uk_list_add(&req->_list, &req_mgmt->req_list);
}

static struct uk_9preq *
_req_mgmt_from_freelist_locked(struct uk_9pdev_req_mgmt *req_mgmt)
{
	struct uk_9preq *req;

	if (uk_list_empty(&req_mgmt->req_free_list))
		return NULL;

	req = uk_list_first_entry(&req_mgmt->req_free_list,
			struct uk_9preq, _list);
	uk_list_del(&req->_list);

	return req;
}

static void _req_mgmt_del_req_locked(struct uk_9pdev_req_mgmt *req_mgmt,
				struct uk_9preq *req)
{
	uk_bitmap_clear(req_mgmt->tag_bm, req->tag, 1);
	uk_list_del(&req->_list);
}

static void _req_mgmt_req_to_freelist_locked(struct uk_9pdev_req_mgmt *req_mgmt,
				struct uk_9preq *req)
{
	uk_list_add(&req->_list, &req_mgmt->req_free_list);
}

static uint16_t _req_mgmt_next_tag_locked(struct uk_9pdev_req_mgmt *req_mgmt)
{
	return uk_find_next_zero_bit(req_mgmt->tag_bm, UK_9P_NUMTAGS, 0);
}

static void _req_mgmt_cleanup(struct uk_9pdev_req_mgmt *req_mgmt __unused)
{
	unsigned long flags;
	uint16_t tag;
	struct uk_9preq *req, *reqn;

	ukplat_spin_lock_irqsave(&req_mgmt->spinlock, flags);
	uk_list_for_each_entry_safe(req, reqn, &req_mgmt->req_list, _list) {
		tag = req->tag;
		_req_mgmt_del_req_locked(req_mgmt, req);
		if (!uk_9preq_put(req)) {
			/* If in the future these references get released, mark
			 * _dev as NULL so uk_9pdev_req_to_freelist doesn't
			 * attempt to place them in an invalid memory region.
			 *
			 * As _dev is not used for any other purpose, this
			 * doesn't impact any other logic related to 9p request
			 * processing.
			 */
			req->_dev = NULL;
			uk_pr_err("Tag %d still has references on cleanup.\n",
				tag);
		}
	}
	uk_list_for_each_entry_safe(req, reqn, &req_mgmt->req_free_list,
			_list) {
		uk_list_del(&req->_list);
		uk_free(req->_a, req);
	}
	ukplat_spin_unlock_irqrestore(&req_mgmt->spinlock, flags);
}

struct uk_9pdev *uk_9pdev_connect(const struct uk_9pdev_trans *trans,
				const char *device_identifier,
				const char *mount_args,
				struct uk_alloc *a)
{
	struct uk_9pdev *dev;
	int rc = 0;

	UK_ASSERT(trans);
	UK_ASSERT(device_identifier);

	if (a == NULL)
		a = trans->a;

	dev = uk_calloc(a, 1, sizeof(*dev));
	if (dev == NULL) {
		rc = -ENOMEM;
		goto out;
	}

	dev->ops = trans->ops;
	dev->a = a;

#if CONFIG_LIBUKSCHED
	uk_waitq_init(&dev->xmit_wq);
#endif

	_req_mgmt_init(&dev->_req_mgmt);
	_fid_mgmt_init(&dev->_fid_mgmt);

	rc = dev->ops->connect(dev, device_identifier, mount_args);
	if (rc < 0)
		goto free_dev;

	/*
	 * By default, the maximum message size is equal to the maximum allowed
	 * message size. This can be changed with the _set_msize() and
	 * _get_msize() functions.
	 */
	UK_ASSERT(dev->max_msize != 0);
	dev->msize = dev->max_msize;
	dev->state = UK_9PDEV_CONNECTED;

	return dev;

free_dev:
	_fid_mgmt_cleanup(&dev->_fid_mgmt);
	_req_mgmt_cleanup(&dev->_req_mgmt);
	uk_free(a, dev);
out:
	return ERR2PTR(rc);
}

int uk_9pdev_disconnect(struct uk_9pdev *dev)
{
	int rc = 0;

	UK_ASSERT(dev);
	UK_ASSERT(dev->state == UK_9PDEV_CONNECTED);

	dev->state = UK_9PDEV_DISCONNECTING;

	/* Clean up the requests before closing the channel. */
	_fid_mgmt_cleanup(&dev->_fid_mgmt);
	_req_mgmt_cleanup(&dev->_req_mgmt);

	/*
	 * Even if the disconnect from the transport layer fails, the memory
	 * allocated for the 9p device is freed.
	 */
	rc = dev->ops->disconnect(dev);

	uk_free(dev->a, dev);
	return rc;
}

int uk_9pdev_request(struct uk_9pdev *dev, struct uk_9preq *req)
{
	int rc;

	UK_ASSERT(dev);
	UK_ASSERT(req);

	if (UK_READ_ONCE(req->state) != UK_9PREQ_READY) {
		rc = -EINVAL;
		goto out;
	}

	if (dev->state != UK_9PDEV_CONNECTED) {
		rc = -EIO;
		goto out;
	}

#if CONFIG_LIBUKSCHED
	uk_waitq_wait_event(&dev->xmit_wq,
		(rc = dev->ops->request(dev, req)) != -ENOSPC);
#else
	do {
		/*
		 * Retry the request while it has no space available on the
		 * transport layer.
		 */
		rc = dev->ops->request(dev, req);
	} while (rc == -ENOSPC);
#endif

out:
	return rc;
}

void uk_9pdev_xmit_notify(struct uk_9pdev *dev)
{
#if CONFIG_LIBUKSCHED
	uk_waitq_wake_up(&dev->xmit_wq);
#endif
}

struct uk_9preq *uk_9pdev_req_create(struct uk_9pdev *dev, uint8_t type)
{
	struct uk_9preq *req;
	int rc = 0;
	uint16_t tag;
	unsigned long flags;

	UK_ASSERT(dev);

	ukplat_spin_lock_irqsave(&dev->_req_mgmt.spinlock, flags);
	if (!(req = _req_mgmt_from_freelist_locked(&dev->_req_mgmt))) {
		/* Don't allocate with the spinlock held. */
		ukplat_spin_unlock_irqrestore(&dev->_req_mgmt.spinlock, flags);
		req = uk_calloc(dev->a, 1, sizeof(*req));
		if (req == NULL) {
			rc = -ENOMEM;
			goto out;
		}
		req->_dev = dev;
		/*
		 * Duplicate this, instead of using req->_dev, as we can't rely
		 * on the value of _dev at time of free. Check comment in
		 * _req_mgmt_cleanup.
		 */
		req->_a = dev->a;
		ukplat_spin_lock_irqsave(&dev->_req_mgmt.spinlock, flags);
	}

	uk_9preq_init(req);

	/*
	 * If request was from the free list, it should already belong to the
	 * dev.
	 */
	UK_ASSERT(req->_dev == dev);

	/* Shouldn't exceed the msize on non-zerocopy buffers, just in case. */
	req->recv.size = MIN(req->recv.size, dev->msize);
	req->xmit.size = MIN(req->xmit.size, dev->msize);

	if (type == UK_9P_TVERSION)
		tag = UK_9P_NOTAG;
	else
		tag = _req_mgmt_next_tag_locked(&dev->_req_mgmt);

	req->tag = tag;
	req->xmit.type = type;

	_req_mgmt_add_req_locked(&dev->_req_mgmt, req);
	ukplat_spin_unlock_irqrestore(&dev->_req_mgmt.spinlock, flags);

	req->state = UK_9PREQ_INITIALIZED;

	return req;

out:
	return ERR2PTR(rc);
}

struct uk_9preq *uk_9pdev_req_lookup(struct uk_9pdev *dev, uint16_t tag)
{
	unsigned long flags;
	struct uk_9preq *req;
	int rc = -EINVAL;

	ukplat_spin_lock_irqsave(&dev->_req_mgmt.spinlock, flags);
	uk_list_for_each_entry(req, &dev->_req_mgmt.req_list, _list) {
		if (tag != req->tag)
			continue;
		rc = 0;
		uk_9preq_get(req);
		break;
	}
	ukplat_spin_unlock_irqrestore(&dev->_req_mgmt.spinlock, flags);

	if (rc == 0)
		return req;

	return ERR2PTR(rc);
}

int uk_9pdev_req_remove(struct uk_9pdev *dev, struct uk_9preq *req)
{
	unsigned long flags;

	ukplat_spin_lock_irqsave(&dev->_req_mgmt.spinlock, flags);
	_req_mgmt_del_req_locked(&dev->_req_mgmt, req);
	ukplat_spin_unlock_irqrestore(&dev->_req_mgmt.spinlock, flags);

	return uk_9preq_put(req);
}

void uk_9pdev_req_to_freelist(struct uk_9pdev *dev, struct uk_9preq *req)
{
	unsigned long flags;

	if (!dev)
		return;

	ukplat_spin_lock_irqsave(&dev->_req_mgmt.spinlock, flags);
	_req_mgmt_req_to_freelist_locked(&dev->_req_mgmt, req);
	ukplat_spin_unlock_irqrestore(&dev->_req_mgmt.spinlock, flags);
}

struct uk_9pfid *uk_9pdev_fid_create(struct uk_9pdev *dev)
{
	struct uk_9pfid *fid = NULL;
	int rc = 0;
	unsigned long flags;

	ukplat_spin_lock_irqsave(&dev->_fid_mgmt.spinlock, flags);
	rc = _fid_mgmt_next_fid_locked(&dev->_fid_mgmt, dev, &fid);
	if (rc < 0)
		goto out;

	_fid_mgmt_add_fid_locked(&dev->_fid_mgmt, fid);

out:
	ukplat_spin_unlock_irqrestore(&dev->_fid_mgmt.spinlock, flags);
	if (rc == 0)
		return fid;
	return ERR2PTR(rc);
}

void uk_9pdev_fid_release(struct uk_9pfid *fid)
{
	struct uk_9pdev *dev = fid->_dev;
	unsigned long flags;
	bool move_to_freelist = false;
	int rc;

	/* First clunk the fid. */
	rc = uk_9p_clunk(fid->_dev, fid);
	if (rc < 0) {
		uk_pr_warn("Could not clunk fid %d: %d\n", fid->fid, rc);
		goto out;
	}

	/* If successfully clunked, move it to a freelist. */
	move_to_freelist = true;

out:
	/* Then remove it from any internal data structures. */
	ukplat_spin_lock_irqsave(&dev->_fid_mgmt.spinlock, flags);
	_fid_mgmt_del_fid_locked(&dev->_fid_mgmt, fid, move_to_freelist);
	ukplat_spin_unlock_irqrestore(&dev->_fid_mgmt.spinlock, flags);
}

bool uk_9pdev_set_msize(struct uk_9pdev *dev, uint32_t msize)
{
	if (msize > dev->max_msize)
		return false;

	dev->msize = msize;

	return true;
}

uint32_t uk_9pdev_get_msize(struct uk_9pdev *dev)
{
	return dev->msize;
}
