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
#include <uk/config.h>
#include <uk/alloc.h>
#include <uk/assert.h>
#include <uk/essentials.h>
#include <uk/list.h>
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

static void p9front_handler(evtchn_port_t evtchn __unused,
			    struct __regs *regs __unused,
			    void *arg __unused)
{
}

static void p9front_free_dev_ring(struct p9front_dev *p9fdev, int idx)
{
	struct p9front_dev_ring *ring = &p9fdev->rings[idx];
	int i;

	UK_ASSERT(ring->initialized);

	unbind_evtchn(ring->evtchn);
	for (i = 0; i < (1 << p9fdev->ring_order); i++)
		gnttab_end_access(ring->intf->ref[i]);
	uk_pfree(a, ring->data.in,
		p9fdev->ring_order + XEN_PAGE_SHIFT - PAGE_SHIFT);
	gnttab_end_access(ring->ref);
	uk_pfree(a, ring->intf, 0);
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
	ring->intf = uk_palloc(a, 0);
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
	data_bytes = uk_palloc(a,
			p9fdev->ring_order + XEN_PAGE_SHIFT - PAGE_SHIFT);
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

	/* Allocate event channel. */
	rc = evtchn_alloc_unbound(xendev->otherend_id, p9front_handler, ring,
				&ring->evtchn);
	if (rc) {
		uk_pr_err(DRIVER_NAME": Error creating evt channel: %d\n", rc);
		goto out_free_grants;
	}

	unmask_evtchn(ring->evtchn);

	/* Mark ring as initialized. */
	ring->initialized = true;

	return 0;

out_free_grants:
	for (i = 0; i < (1 << p9fdev->ring_order); i++)
		gnttab_end_access(ring->intf->ref[i]);
	uk_pfree(a, data_bytes,
		p9fdev->ring_order + XEN_PAGE_SHIFT - PAGE_SHIFT);
out_free_intf:
	gnttab_end_access(ring->ref);
	uk_pfree(a, ring->intf, 0);
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

static int p9front_connect(struct uk_9pdev *p9dev __unused,
			   const char *device_identifier __unused,
			   const char *mount_args __unused)
{
	return 0;
}

static int p9front_disconnect(struct uk_9pdev *p9dev __unused)
{
	return 0;
}

static int p9front_request(struct uk_9pdev *p9dev __unused,
			   struct uk_9preq *req __unused)
{
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
	unsigned long flags;

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
	ukplat_spin_lock_irqsave(&p9front_device_list_lock, flags);
	uk_list_add(&p9fdev->_list, &p9front_device_list);
	ukplat_spin_unlock_irqrestore(&p9front_device_list_lock, flags);

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
