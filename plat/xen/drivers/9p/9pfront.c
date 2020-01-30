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
#include <stdio.h>
#include <uk/config.h>
#include <uk/alloc.h>
#include <uk/assert.h>
#include <uk/essentials.h>
#include <uk/errptr.h>
#include <uk/list.h>
#if CONFIG_LIBUKSCHED
#include <uk/thread.h>
#endif
#include <uk/9pdev.h>
#include <uk/9preq.h>
#include <uk/9pdev_trans.h>
#include <uk/plat/spinlock.h>
#include <xen-x86/mm.h>
#include <xen-x86/irq.h>
#include <xenbus/xenbus.h>

#include "9pfront_xb.h"

#define DRIVER_NAME	"xen-9pfront"

static struct uk_alloc *a;
static UK_LIST_HEAD(p9front_device_list);
static DEFINE_SPINLOCK(p9front_device_list_lock);

struct p9front_header {
	uint32_t size;
	uint8_t type;
	uint16_t tag;
} __packed;

static void p9front_recv(struct p9front_dev_ring *ring);

#if CONFIG_LIBUKSCHED

static void p9front_bh_handler(void *arg)
{
	struct p9front_dev_ring *ring = arg;

	while (1) {
		uk_waitq_wait_event(&ring->bh_wq,
				UK_READ_ONCE(ring->data_avail));
		p9front_recv(ring);
	}
}

#endif

static void p9front_recv(struct p9front_dev_ring *ring)
{
	struct p9front_dev *p9fdev = ring->dev;
	evtchn_port_t evtchn = ring->evtchn;
	RING_IDX cons, prod, masked_cons, masked_prod;
	int ring_size, rc;
	struct p9front_header hdr;
	struct uk_9preq *req;
	uint32_t buf_cnt, zc_buf_cnt;

	ring_size = XEN_FLEX_RING_SIZE(p9fdev->ring_order);

	while (1) {
		cons = ring->intf->in_cons;
		prod = ring->intf->in_prod;
		xen_rmb();

		if (xen_9pfs_queued(prod, cons, ring_size) < sizeof(hdr)) {
#if CONFIG_LIBUKSCHED
			UK_WRITE_ONCE(ring->data_avail, false);
#endif
			notify_remote_via_evtchn(evtchn);
			return;
		}

		masked_prod = xen_9pfs_mask(prod, ring_size);
		masked_cons = xen_9pfs_mask(cons, ring_size);

		xen_9pfs_read_packet(&hdr, ring->data.in, sizeof(hdr),
				masked_prod, &masked_cons, ring_size);

		req = uk_9pdev_req_lookup(p9fdev->p9dev, hdr.tag);
		if (PTRISERR(req)) {
			uk_pr_warn("Found invalid tag=%u\n", hdr.tag);
			cons += hdr.size;
			xen_mb();
			ring->intf->in_cons = cons;
			continue;
		}

		masked_cons = xen_9pfs_mask(cons, ring_size);

		/*
		 * Compute amount of data to read into request buffer and into
		 * zero-copy buffer.
		 */
		buf_cnt = hdr.size;
		if (hdr.type != UK_9P_RERROR && req->recv.zc_buf)
			buf_cnt = MIN(buf_cnt, req->recv.zc_offset);
		zc_buf_cnt = hdr.size - buf_cnt;

		xen_9pfs_read_packet(req->recv.buf, ring->data.in, buf_cnt,
				masked_prod, &masked_cons, ring_size);
		xen_9pfs_read_packet(req->recv.zc_buf, ring->data.in,
				zc_buf_cnt, masked_prod, &masked_cons,
				ring_size);
		cons += hdr.size;
		xen_mb();
		ring->intf->in_cons = cons;

		rc = uk_9preq_receive_cb(req, hdr.size);
		if (rc)
			uk_pr_warn("Could not receive reply: %d\n", rc);

		/* Release reference held by uk_9pdev_req_lookup(). */
		uk_9preq_put(req);
	}
}

static void p9front_handler(evtchn_port_t evtchn,
			    struct __regs *regs __unused,
			    void *arg)
{
	struct p9front_dev_ring *ring = arg;

	UK_ASSERT(ring);
	UK_ASSERT(ring->evtchn == evtchn);

	/*
	 * A new interrupt means that there is a response to be received, which
	 * means that a previously sent request has been removed from the out
	 * ring. Thus, the API can be notified of the possibility of retrying to
	 * send requests blocked on ENOSPC errors.
	 */
	if (ring->dev->p9dev)
		uk_9pdev_xmit_notify(ring->dev->p9dev);
#if CONFIG_LIBUKSCHED
	UK_WRITE_ONCE(ring->data_avail, true);
	uk_waitq_wake_up(&ring->bh_wq);
#else
	p9front_recv(ring);
#endif
}

static void p9front_free_dev_ring(struct p9front_dev *p9fdev, int idx)
{
	struct p9front_dev_ring *ring = &p9fdev->rings[idx];
	int i;

	UK_ASSERT(ring->initialized);

	if (ring->bh_thread_name)
		free(ring->bh_thread_name);
	uk_thread_kill(ring->bh_thread);
	unbind_evtchn(ring->evtchn);
	for (i = 0; i < (1 << p9fdev->ring_order); i++)
		gnttab_end_access(ring->intf->ref[i]);
	uk_pfree(a, ring->data.in,
		 1ul << (p9fdev->ring_order + XEN_PAGE_SHIFT - PAGE_SHIFT));
	gnttab_end_access(ring->ref);
	uk_pfree(a, ring->intf, 1);
	ring->initialized = false;
}

static void p9front_free_dev_rings(struct p9front_dev *p9fdev)
{
	int i;

	for (i = 0; i < p9fdev->nb_rings; i++) {
		if (!p9fdev->rings[i].initialized)
			continue;
		p9front_free_dev_ring(p9fdev, i);
	}

	uk_free(a, p9fdev->rings);
}

static int p9front_allocate_dev_ring(struct p9front_dev *p9fdev, int idx)
{
	struct xenbus_device *xendev = p9fdev->xendev;
	struct p9front_dev_ring *ring;
	int rc, i;
	void *data_bytes;

	/* Sanity checks. */
	UK_ASSERT(idx >= 0 && idx < p9fdev->nb_rings);

	ring = &p9fdev->rings[idx];
	UK_ASSERT(!ring->initialized);

	ukarch_spin_lock_init(&ring->spinlock);
	ring->dev = p9fdev;

	/* Allocate ring intf page. */
	ring->intf = uk_palloc(a, 1);
	if (!ring->intf) {
		rc = -ENOMEM;
		goto out;
	}
	memset(ring->intf, 0, PAGE_SIZE);

	/* Grant access to the allocated page to the backend. */
	ring->ref = gnttab_grant_access(xendev->otherend_id,
			virt_to_mfn(ring->intf), 0);
	UK_ASSERT(ring->ref != GRANT_INVALID_REF);

	/* Allocate memory for the data. */
	data_bytes = uk_palloc(a, 1ul << (p9fdev->ring_order +
					  XEN_PAGE_SHIFT - PAGE_SHIFT));
	if (!data_bytes) {
		rc = -ENOMEM;
		goto out_free_intf;
	}
	memset(data_bytes, 0, XEN_FLEX_RING_SIZE(p9fdev->ring_order) * 2);

	/* Grant refs to the entire data. */
	for (i = 0; i < (1 << p9fdev->ring_order); i++) {
		ring->intf->ref[i] = gnttab_grant_access(xendev->otherend_id,
				virt_to_mfn(data_bytes) + i, 0);
		UK_ASSERT(ring->intf->ref[i] != GRANT_INVALID_REF);
	}

	ring->intf->ring_order = p9fdev->ring_order;
	ring->data.in = data_bytes;
	ring->data.out = data_bytes + XEN_FLEX_RING_SIZE(p9fdev->ring_order);

#if CONFIG_LIBUKSCHED
	/* Allocate bottom-half thread. */
	ring->data_avail = false;
	uk_waitq_init(&ring->bh_wq);

	rc = asprintf(&ring->bh_thread_name, DRIVER_NAME"-recv-%s-%u",
			p9fdev->tag, idx);
	ring->bh_thread = uk_thread_create(ring->bh_thread_name,
			p9front_bh_handler, ring);
	if (!ring->bh_thread) {
		rc = -ENOMEM;
		goto out_free_grants;
	}
#endif

	/* Allocate event channel. */
	rc = evtchn_alloc_unbound(xendev->otherend_id, p9front_handler, ring,
				&ring->evtchn);
	if (rc) {
		uk_pr_err(DRIVER_NAME": Error creating evt channel: %d\n", rc);
		goto out_free_thread;
	}

	unmask_evtchn(ring->evtchn);

	/* Mark ring as initialized. */
	ring->initialized = true;

	return 0;

out_free_thread:
	if (ring->bh_thread_name)
		free(ring->bh_thread_name);
	uk_thread_kill(ring->bh_thread);
out_free_grants:
	for (i = 0; i < (1 << p9fdev->ring_order); i++)
		gnttab_end_access(ring->intf->ref[i]);
	uk_pfree(a, data_bytes,
		 1ul << (p9fdev->ring_order + XEN_PAGE_SHIFT - PAGE_SHIFT));
out_free_intf:
	gnttab_end_access(ring->ref);
	uk_pfree(a, ring->intf, 1);
out:
	return rc;
}

static int p9front_allocate_dev_rings(struct p9front_dev *p9fdev)
{
	int rc, i;

	p9fdev->rings = uk_calloc(a, p9fdev->nb_rings, sizeof(*p9fdev->rings));
	if (!p9fdev->rings) {
		rc = -ENOMEM;
		goto out;
	}

	for (i = 0; i < p9fdev->nb_rings; i++) {
		rc = p9front_allocate_dev_ring(p9fdev, i);
		if (rc)
			goto out_free;
	}

	return 0;

out_free:
	p9front_free_dev_rings(p9fdev);
out:
	return rc;
}

static int p9front_connect(struct uk_9pdev *p9dev,
			   const char *device_identifier,
			   const char *mount_args __unused)
{
	struct p9front_dev *p9fdev = NULL;
	int rc = 0;
	int found = 0;

	ukarch_spin_lock(&p9front_device_list_lock);
	uk_list_for_each_entry(p9fdev, &p9front_device_list, _list) {
		if (!strcmp(p9fdev->tag, device_identifier)) {
			if (p9fdev->p9dev != NULL) {
				rc = -EBUSY;
				goto out;
			}
			found = 1;
			break;
		}
	}

	if (!found) {
		rc = -ENODEV;
		goto out;
	}

	/* The msize is given by the size of the flex ring. */
	p9dev->max_msize = XEN_FLEX_RING_SIZE(p9fdev->ring_order);

	p9fdev->p9dev = p9dev;
	p9dev->priv = p9fdev;
	rc = 0;
	found = 1;

out:
	ukarch_spin_unlock(&p9front_device_list_lock);
	return rc;
}

static int p9front_disconnect(struct uk_9pdev *p9dev __unused)
{
	struct p9front_dev *p9fdev;

	UK_ASSERT(p9dev);
	p9fdev = p9dev->priv;

	ukarch_spin_lock(&p9front_device_list_lock);
	p9fdev->p9dev = NULL;
	ukarch_spin_unlock(&p9front_device_list_lock);

	return 0;
}

static int p9front_request(struct uk_9pdev *p9dev,
			   struct uk_9preq *req)
{
	struct p9front_dev *p9fdev;
	struct p9front_dev_ring *ring;
	int ring_idx, ring_size;
	RING_IDX masked_prod, masked_cons, prod, cons;

	UK_ASSERT(p9dev);
	UK_ASSERT(req);
	UK_ASSERT(req->state == UK_9PREQ_READY);

	p9fdev = p9dev->priv;

	ring_size = XEN_FLEX_RING_SIZE(p9fdev->ring_order);

	ring_idx = req->tag % p9fdev->nb_rings;
	ring = &p9fdev->rings[ring_idx];

	/* Protect against concurrent writes to the out ring. */
	ukarch_spin_lock(&ring->spinlock);
	cons = ring->intf->out_cons;
	prod = ring->intf->out_prod;
	xen_mb();

	masked_prod = xen_9pfs_mask(prod, ring_size);
	masked_cons = xen_9pfs_mask(cons, ring_size);

	if (ring_size - xen_9pfs_queued(prod, cons, ring_size) <
			req->xmit.size + req->xmit.zc_size) {
		ukarch_spin_unlock(&ring->spinlock);
		return -ENOSPC;
	}

	xen_9pfs_write_packet(ring->data.out, req->xmit.buf, req->xmit.size,
			      &masked_prod, masked_cons, ring_size);
	xen_9pfs_write_packet(ring->data.out, req->xmit.zc_buf, req->xmit.zc_size,
			      &masked_prod, masked_cons, ring_size);
	req->state = UK_9PREQ_SENT;
	xen_wmb();
	prod += req->xmit.size + req->xmit.zc_size;
	ring->intf->out_prod = prod;

	ukarch_spin_unlock(&ring->spinlock);
	notify_remote_via_evtchn(ring->evtchn);

	return 0;
}

static const struct uk_9pdev_trans_ops p9front_trans_ops = {
	.connect        = p9front_connect,
	.disconnect     = p9front_disconnect,
	.request        = p9front_request
};

static struct uk_9pdev_trans p9front_trans = {
	.name           = "xen",
	.ops            = &p9front_trans_ops,
	.a              = NULL /* Set below. */
};


static int p9front_drv_init(struct uk_alloc *drv_allocator)
{
	if (!drv_allocator)
		return -EINVAL;

	a = drv_allocator;
	p9front_trans.a = a;

	return uk_9pdev_trans_register(&p9front_trans);
}

static int p9front_add_dev(struct xenbus_device *xendev)
{
	struct p9front_dev *p9fdev;
	int rc;

	p9fdev = uk_calloc(a, 1, sizeof(*p9fdev));
	if (!p9fdev) {
		rc = -ENOMEM;
		goto out;
	}

	p9fdev->xendev = xendev;
	rc = p9front_xb_init(p9fdev);
	if (rc)
		goto out_free;

	uk_pr_info("Initialized 9pfront dev: tag=%s,maxrings=%d,maxorder=%d\n",
		p9fdev->tag, p9fdev->nb_max_rings, p9fdev->max_ring_page_order);

	p9fdev->nb_rings = MIN(CONFIG_XEN_9PFRONT_NB_RINGS,
				p9fdev->nb_max_rings);
	p9fdev->ring_order = MIN(CONFIG_XEN_9PFRONT_RING_ORDER,
				p9fdev->max_ring_page_order);

	rc = p9front_allocate_dev_rings(p9fdev);
	if (rc) {
		uk_pr_err(DRIVER_NAME": Could not initialize device rings: %d\n",
			rc);
		goto out_free;
	}

	rc = p9front_xb_connect(p9fdev);
	if (rc) {
		uk_pr_err(DRIVER_NAME": Could not connect: %d\n", rc);
		goto out_free_rings;
	}

	rc = 0;
	ukarch_spin_lock(&p9front_device_list_lock);
	uk_list_add(&p9fdev->_list, &p9front_device_list);
	ukarch_spin_unlock(&p9front_device_list_lock);

	uk_pr_info(DRIVER_NAME": Connected 9pfront dev: tag=%s,rings=%d,order=%d\n",
		p9fdev->tag, p9fdev->nb_rings, p9fdev->ring_order);

	goto out;

out_free_rings:
	p9front_free_dev_rings(p9fdev);
out_free:
	uk_free(a, p9fdev);
out:
	return rc;
}

static const xenbus_dev_type_t p9front_devtypes[] = {
	xenbus_dev_9pfs,
};

static struct xenbus_driver p9front_driver = {
	.device_types   = p9front_devtypes,
	.init           = p9front_drv_init,
	.add_dev        = p9front_add_dev
};

XENBUS_REGISTER_DRIVER(&p9front_driver);
