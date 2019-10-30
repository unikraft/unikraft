/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Roxana Nicolescu <nicolescu.roxana1996@gmail.com>
 *
 * Copyright (c) 2019, University Politehnica of Bucharest.
 * All rights reserved.
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
#include <inttypes.h>
#include <string.h>
#include <fcntl.h>
#include <uk/assert.h>
#include <uk/print.h>
#include <uk/alloc.h>
#include <uk/essentials.h>
#include <uk/arch/limits.h>
#include <uk/page.h>
#include <uk/blkdev_driver.h>
#include <xen-x86/mm.h>
#include <xen-x86/mm_pv.h>
#include <xenbus/xenbus.h>
#include "blkfront.h"
#include "blkfront_xb.h"

#define DRIVER_NAME		"xen-blkfront"

#define SECTOR_INDEX_IN_PAGE(a, sector_size) \
	(((a) & ~PAGE_MASK) / (sector_size))

/* TODO Same interrupt macros we use in virtio-blk */
#define BLKFRONT_INTR_EN             (1 << 0)
#define BLKFRONT_INTR_EN_MASK        (1)
#define BLKFRONT_INTR_USR_EN         (1 << 1)
#define BLKFRONT_INTR_USR_EN_MASK    (2)

/* Get blkfront_dev* which contains blkdev */
#define to_blkfront(blkdev) \
	__containerof(blkdev, struct blkfront_dev, blkdev)

static struct uk_alloc *drv_allocator;


static void blkif_request_init(struct blkif_request *ring_req,
		__sector sector_size)
{
	uintptr_t start_sector, end_sector;
	uint16_t nb_segments;
	struct blkfront_request *blkfront_req;
	struct uk_blkreq *req;
	uintptr_t start_data, end_data;
	uint16_t seg;

	UK_ASSERT(ring_req);
	blkfront_req = (struct blkfront_request *)ring_req->id;
	req = blkfront_req->req;
	start_data = (uintptr_t)req->aio_buf;
	end_data = (uintptr_t)req->aio_buf + req->nb_sectors * sector_size;

	/* Can't io non-sector-aligned buffer */
	UK_ASSERT(!(start_data & (sector_size - 1)));

	/*
	 * Find number of segments (pages)
	 * Being sector-size aligned buffer, it may not be aligned
	 * to page_size. If so, it is necessary to find the start and end
	 * of the pages the buffer is allocated, in order to calculate the
	 * number of pages the request has.
	 **/
	start_sector = round_pgdown(start_data);
	end_sector = round_pgup(end_data);
	nb_segments = (end_sector - start_sector) / PAGE_SIZE;
	UK_ASSERT(nb_segments <= BLKIF_MAX_SEGMENTS_PER_REQUEST);

	/* Set ring request */
	ring_req->operation = (req->operation == UK_BLKDEV_WRITE) ?
			BLKIF_OP_WRITE : BLKIF_OP_READ;
	ring_req->nr_segments = nb_segments;
	ring_req->sector_number = req->start_sector;

	/* Set for each page the offset of sectors used for request */
	for (seg = 0; seg < nb_segments; ++seg) {
		ring_req->seg[seg].first_sect = 0;
		ring_req->seg[seg].last_sect = PAGE_SIZE / sector_size - 1;
	}

	ring_req->seg[0].first_sect =
			SECTOR_INDEX_IN_PAGE(start_data, sector_size);
	ring_req->seg[nb_segments - 1].last_sect =
			SECTOR_INDEX_IN_PAGE(end_data - 1, sector_size);
}

static int blkfront_request_write(struct blkfront_request *blkfront_req,
		struct blkif_request *ring_req)
{
	struct blkfront_dev *dev;
	struct uk_blkreq *req;
	struct uk_blkdev_cap *cap;
	__sector sector_size;
	int rc = 0;

	UK_ASSERT(blkfront_req);
	req = blkfront_req->req;
	dev = blkfront_req->queue->dev;
	cap = &dev->blkdev.capabilities;
	sector_size = cap->ssize;
	if (req->operation == UK_BLKDEV_WRITE && cap->mode == O_RDONLY)
		return -EPERM;

	if (req->aio_buf == NULL)
		return -EINVAL;

	if (req->nb_sectors == 0)
		return -EINVAL;

	if (req->start_sector + req->nb_sectors > cap->sectors)
		return -EINVAL;

	if (req->nb_sectors > cap->max_sectors_per_req)
		return -EINVAL;

	blkif_request_init(ring_req, sector_size);
	blkfront_req->nb_segments = ring_req->nr_segments;

	return rc;
}

static int blkfront_request_flush(struct blkfront_request *blkfront_req,
		struct blkif_request *ring_req)
{
	struct blkfront_dev *dev;
	struct uk_blkdev_queue *queue;

	UK_ASSERT(ring_req);

	queue = blkfront_req->queue;
	dev = queue->dev;
	if (dev->barrier)
		ring_req->operation = BLKIF_OP_WRITE_BARRIER;
	else if (dev->flush)
		ring_req->operation = BLKIF_OP_FLUSH_DISKCACHE;
	else
		return -ENOTSUP;

	ring_req->nr_segments = 0;
	ring_req->sector_number = 0;

	return 0;
}

static int blkfront_queue_enqueue(struct uk_blkdev_queue *queue,
		struct uk_blkreq *req)
{
	struct blkfront_request *blkfront_req;
	struct blkfront_dev *dev;
	RING_IDX ring_idx;
	struct blkif_request *ring_req;
	struct blkif_front_ring *ring;
	int rc = 0;

	UK_ASSERT(queue);
	UK_ASSERT(req);

	blkfront_req = uk_malloc(drv_allocator, sizeof(*blkfront_req));
	if (!blkfront_req)
		return -ENOMEM;

	blkfront_req->req = req;
	blkfront_req->queue = queue;
	dev = queue->dev;
	ring = &queue->ring;
	ring_idx = ring->req_prod_pvt;
	ring_req = RING_GET_REQUEST(ring, ring_idx);
	ring_req->id = (uintptr_t) blkfront_req;
	ring_req->handle = dev->handle;

	if (req->operation == UK_BLKDEV_READ ||
			req->operation == UK_BLKDEV_WRITE)
		rc = blkfront_request_write(blkfront_req, ring_req);
	else if (req->operation == UK_BLKDEV_FFLUSH)
		rc =  blkfront_request_flush(blkfront_req, ring_req);
	else
		rc = -EINVAL;

	if (rc)
		goto err_out;

	ring->req_prod_pvt = ring_idx + 1;

	/* Memory barrier */
	wmb();
out:
	return rc;

err_out:
	uk_free(drv_allocator, blkfront_req);
	goto out;
}

static int blkfront_submit_request(struct uk_blkdev *blkdev,
		struct uk_blkdev_queue *queue,
		struct uk_blkreq *req)
{
	int err = 0;
	int notify;
	int status = 0x0;

	UK_ASSERT(blkdev != NULL);
	UK_ASSERT(req != NULL);
	UK_ASSERT(queue != NULL);

	if (RING_FULL(&queue->ring)) {
		uk_pr_err("Queue %p is full\n", queue);
		return -EBUSY;
	}

	err = blkfront_queue_enqueue(queue, req);
	if (err) {
		uk_pr_err("Failed to set ring req for %d op: %d\n",
				req->operation, err);
		return err;
	}

	status |= UK_BLKDEV_STATUS_SUCCESS;
	RING_PUSH_REQUESTS_AND_CHECK_NOTIFY(&queue->ring, notify);
	if (notify) {
		err = notify_remote_via_evtchn(queue->evtchn);
		if (err)
			return err;
	}

	status |= (!RING_FULL(&queue->ring)) ? UK_BLKDEV_STATUS_MORE : 0x0;
	return status;
}

/* Returns 1 if more responses available */
static int blkfront_xen_ring_intr_enable(struct uk_blkdev_queue *queue)
{
	int more;

	/* Check if there are no more responses enabled */
	RING_FINAL_CHECK_FOR_RESPONSES(&queue->ring, more);
	if (!more) {
		/* No more responses, we can enable interrupts */
		queue->intr_enabled |= BLKFRONT_INTR_EN;
		unmask_evtchn(queue->evtchn);
	}

	return (more > 0);
}
static int blkfront_ring_init(struct uk_blkdev_queue *queue)
{
	struct blkif_sring *sring = NULL;
	struct blkfront_dev *dev;

	UK_ASSERT(queue);
	dev = queue->dev;
	sring = uk_malloc_page(queue->a);
	if (!sring)
		return -ENOMEM;

	memset(sring, 0, PAGE_SIZE);
	SHARED_RING_INIT(sring);
	FRONT_RING_INIT(&queue->ring, sring, PAGE_SIZE);

	queue->ring_ref = gnttab_grant_access(dev->xendev->otherend_id,
			virt_to_mfn(sring), 0);
	UK_ASSERT(queue->ring_ref != GRANT_INVALID_REF);

	return 0;
}

static void blkfront_ring_fini(struct uk_blkdev_queue *queue)
{
	int rc;

	if (queue->ring_ref != GRANT_INVALID_REF) {
		rc = gnttab_end_access(queue->ring_ref);
		UK_ASSERT(rc);
	}

	if (queue->ring.sring != NULL)
		uk_free_page(queue->a, queue->ring.sring);
}

/* Handler for event channel notifications */
static void blkfront_handler(evtchn_port_t port __unused,
		struct __regs *regs __unused, void *arg)
{
	struct uk_blkdev_queue *queue;

	UK_ASSERT(arg);
	queue = (struct uk_blkdev_queue *)arg;

	/* Disable the interrupt for the ring */
	queue->intr_enabled &= ~(BLKFRONT_INTR_EN);
	mask_evtchn(queue->evtchn);

	uk_blkdev_drv_queue_event(&queue->dev->blkdev, queue->queue_id);
}

static struct uk_blkdev_queue *blkfront_queue_setup(struct uk_blkdev *blkdev,
		uint16_t queue_id,
		uint16_t nb_desc __unused,
		const struct uk_blkdev_queue_conf *queue_conf)
{
	struct blkfront_dev *dev;
	struct uk_blkdev_queue *queue;
	int err = 0;

	UK_ASSERT(blkdev != NULL);

	dev = to_blkfront(blkdev);
	if (queue_id >= dev->nb_queues) {
		uk_pr_err("Invalid queue identifier: %"__PRIu16"\n", queue_id);
		return ERR2PTR(-EINVAL);
	}

	queue = &dev->queues[queue_id];
	queue->a = queue_conf->a;
	queue->queue_id = queue_id;
	queue->dev = dev;
	queue->intr_enabled = 0;
	err = blkfront_ring_init(queue);
	if (err) {
		uk_pr_err("Failed to init ring: %d.\n", err);
		return ERR2PTR(err);
	}

	err = evtchn_alloc_unbound(dev->xendev->otherend_id,
			blkfront_handler, queue,
			&queue->evtchn);
	if (err) {
		uk_pr_err("Failed to create event-channel: %d.\n", err);
		err *= -1;
		goto err_out;
	}

	return queue;

err_out:
	blkfront_ring_fini(queue);
	return ERR2PTR(err);
}

static int blkfront_queue_release(struct uk_blkdev *blkdev,
		struct uk_blkdev_queue *queue)
{
	UK_ASSERT(blkdev != NULL);
	UK_ASSERT(queue != NULL);

	mask_evtchn(queue->evtchn);
	unbind_evtchn(queue->evtchn);
	blkfront_ring_fini(queue);


static int blkfront_queue_intr_enable(struct uk_blkdev *blkdev,
		struct uk_blkdev_queue *queue)
{
	int rc = 0;

	UK_ASSERT(blkdev != NULL);
	UK_ASSERT(queue != NULL);

	/* If the interrupt is enabled */
	if (queue->intr_enabled & BLKFRONT_INTR_EN)
		return 0;

	/**
	 * Enable the user configuration bit. This would cause the interrupt to
	 * be enabled automatically if the interrupt could not be enabled now
	 * due to data in the queue.
	 */
	queue->intr_enabled = BLKFRONT_INTR_USR_EN;
	rc = blkfront_xen_ring_intr_enable(queue);
	if (!rc)
		queue->intr_enabled |= BLKFRONT_INTR_EN;

	return rc;
}

static int blkfront_queue_intr_disable(struct uk_blkdev *blkdev,
		struct uk_blkdev_queue *queue)
{
	UK_ASSERT(blkdev);
	UK_ASSERT(queue);

	queue->intr_enabled &= ~(BLKFRONT_INTR_USR_EN | BLKFRONT_INTR_EN);
	mask_evtchn(queue->evtchn);

	return 0;
}

static int blkfront_queue_get_info(struct uk_blkdev *blkdev,
		uint16_t queue_id,
		struct uk_blkdev_queue_info *qinfo)
{
	struct blkfront_dev *dev;

	UK_ASSERT(blkdev);
	UK_ASSERT(qinfo);

	dev = to_blkfront(blkdev);
	if (queue_id >= dev->nb_queues) {
		uk_pr_err("Invalid queue identifier: %"__PRIu16"\n", queue_id);
		return -EINVAL;
	}

	qinfo->nb_is_power_of_two = 1;

	return 0;
}

static int blkfront_configure(struct uk_blkdev *blkdev,
		const struct uk_blkdev_conf *conf)
{
	struct blkfront_dev *dev;
	int err = 0;

	UK_ASSERT(blkdev != NULL);
	UK_ASSERT(conf != NULL);

	dev = to_blkfront(blkdev);
	dev->nb_queues = conf->nb_queues;
	dev->queues = uk_calloc(drv_allocator, dev->nb_queues,
				sizeof(*dev->queues));
	if (!dev->queues)
		return -ENOMEM;

	err = blkfront_xb_write_nb_queues(dev);
	if (err) {
		uk_pr_err("Failed to write nb of queues: %d.\n", err);
		goto out_err;
	}

	uk_pr_info(DRIVER_NAME": %"PRIu16" configured\n", dev->uid);
out:
	return err;
out_err:
	uk_free(drv_allocator, dev->queues);
	goto out;
}

static int blkfront_start(struct uk_blkdev *blkdev)
{
	struct blkfront_dev *dev;
	int err = 0;

	UK_ASSERT(blkdev != NULL);
	dev = to_blkfront(blkdev);
	err = blkfront_xb_connect(dev);
	if (err) {
		uk_pr_err("Failed to connect to backend: %d.\n", err);
		return err;
	}

	uk_pr_info(DRIVER_NAME": %"PRIu16" started\n", dev->uid);

	return err;
}

/* If one queue has unconsumed responses it returns -EBUSY */
static int blkfront_stop(struct uk_blkdev *blkdev)
{
	struct blkfront_dev *dev;
	uint16_t q_id;
	int err;

	UK_ASSERT(blkdev != NULL);
	dev = to_blkfront(blkdev);
	for (q_id = 0; q_id < dev->nb_queues; ++q_id) {
		if (RING_HAS_UNCONSUMED_RESPONSES(&dev->queues[q_id].ring)) {
			uk_pr_err("Queue:%"PRIu16" has unconsumed responses\n",
					q_id);
			return -EBUSY;
		}
	}

	err = blkfront_xb_disconnect(dev);
	if (err) {
		uk_pr_err(
			"Failed to disconnect: %d.\n", err);
		return err;
	}

	uk_pr_info(DRIVER_NAME": %"PRIu16" stopped\n", dev->uid);

	return err;
}

static int blkfront_unconfigure(struct uk_blkdev *blkdev)
{
	struct blkfront_dev *dev;

	UK_ASSERT(blkdev != NULL);
	dev = to_blkfront(blkdev);
	uk_free(drv_allocator, dev->queues);

	return 0;
}

static void blkfront_get_info(struct uk_blkdev *blkdev,
		struct uk_blkdev_info *dev_info)
{
	struct blkfront_dev *dev = NULL;

	UK_ASSERT(blkdev);
	UK_ASSERT(dev_info);

	dev = to_blkfront(blkdev);
	dev_info->max_queues = dev->nb_queues;
}

static const struct uk_blkdev_ops blkfront_ops = {
	.get_info = blkfront_get_info,
	.dev_configure = blkfront_configure,
	.queue_get_info = blkfront_queue_get_info,
	.queue_setup = blkfront_queue_setup,
	.queue_release = blkfront_queue_release,
	.dev_start = blkfront_start,
	.dev_stop = blkfront_stop,
	.dev_unconfigure = blkfront_unconfigure,
	.queue_intr_enable = blkfront_queue_intr_enable,
	.queue_intr_disable = blkfront_queue_intr_disable,
};

/**
 * Assign callbacks to uk_blkdev
 */
static int blkfront_add_dev(struct xenbus_device *dev)
{
	struct blkfront_dev *d = NULL;
	int rc = 0;

	UK_ASSERT(dev != NULL);

	d = uk_calloc(drv_allocator, 1, sizeof(struct blkfront_dev));
	if (!d)
		return -ENOMEM;

	d->xendev = dev;
	d->blkdev.submit_one = blkfront_submit_request;
	d->blkdev.dev_ops = &blkfront_ops;

	/* Xenbus initialization */
	rc = blkfront_xb_init(d);
	if (rc) {
		uk_pr_err("Error initializing Xenbus data: %d\n", rc);
		goto err_xenbus;
	}

	rc = uk_blkdev_drv_register(&d->blkdev, drv_allocator, "blkdev");
	if (rc < 0) {
		uk_pr_err("Failed to register blkfront with libukblkdev %d",
				rc);
		goto err_register;
	}

	d->uid = rc;
	uk_pr_info("Blkfront device registered with libukblkdev: %d\n", rc);
	rc = 0;
out:
	return rc;
err_register:
	blkfront_xb_fini(d);
err_xenbus:
	uk_free(drv_allocator, d);
	goto out;
}

static int blkfront_drv_init(struct uk_alloc *allocator)
{
	/* driver initialization */
	if (!allocator)
		return -EINVAL;

	drv_allocator = allocator;
	return 0;
}

static const xenbus_dev_type_t blkfront_devtypes[] = {
	xenbus_dev_vbd,
};

static struct xenbus_driver blkfront_driver = {
	.device_types = blkfront_devtypes,
	.init = blkfront_drv_init,
	.add_dev = blkfront_add_dev
};

XENBUS_REGISTER_DRIVER(&blkfront_driver);
