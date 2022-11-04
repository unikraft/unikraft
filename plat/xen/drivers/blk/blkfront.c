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
#if defined(__i386__) || defined(__x86_64__)
#include <xen-x86/mm.h>
#include <xen-x86/mm_pv.h>
#elif defined(__aarch64__)
#include <xen-arm/mm.h>
#else
#error "Unsupported architecture"
#endif
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

/* This function gets from pool gref_elems or allocates new ones
 */
static int blkfront_request_set_grefs(struct blkfront_request *blkfront_req)
{
	struct blkfront_gref *ref_elem;
	uint16_t nb_segments;
	int grefi = 0, grefj;
	int err = 0;

#if CONFIG_XEN_BLKFRONT_GREFPOOL
	struct uk_blkdev_queue *queue;
	struct blkfront_grefs_pool *grefs_pool;
	int rc = 0;
#endif

	UK_ASSERT(blkfront_req != NULL);
	nb_segments = blkfront_req->nb_segments;

#if CONFIG_XEN_BLKFRONT_GREFPOOL
	queue = blkfront_req->queue;
	grefs_pool = &queue->ref_pool;
	uk_semaphore_down(&grefs_pool->sem);
	for (grefi = 0; grefi < nb_segments &&
		!UK_STAILQ_EMPTY(&grefs_pool->grefs_list); ++grefi) {
		ref_elem = UK_STAILQ_FIRST(&grefs_pool->grefs_list);
		UK_STAILQ_REMOVE_HEAD(&grefs_pool->grefs_list, _list);
		blkfront_req->gref[grefi] = ref_elem;
	}

	uk_semaphore_up(&grefs_pool->sem);
#endif
	/* we allocate new ones */
	for (; grefi < nb_segments; ++grefi) {
		ref_elem = uk_malloc(drv_allocator, sizeof(*ref_elem));
		if (!ref_elem) {
			err = -ENOMEM;
			goto err;
		}

#if CONFIG_XEN_BLKFRONT_GREFPOOL
		ref_elem->reusable_gref = false;
#endif
		blkfront_req->gref[grefi] = ref_elem;
	}

out:
	return err;
err:
	/* Free all the elements from 0 index to where the error happens */
	for (grefj = 0; grefj < grefi; ++grefj) {
		ref_elem = blkfront_req->gref[grefj];
#if CONFIG_XEN_BLKFRONT_GREFPOOL
		if (ref_elem->reusable_gref) {
			rc = gnttab_end_access(ref_elem->ref);
			UK_ASSERT(rc);
		}
#endif
		uk_free(drv_allocator, ref_elem);
	}
	goto out;
}

/* First gref_elems from blkfront_request were popped from the pool.
 * All this elements has the reusable_gref flag set.
 * We continue transferring elements from blkfront_request to the pool
 * of grant_refs until we encounter an element with the reusable flag unset.
 **/
static void blkfront_request_reset_grefs(struct blkfront_request *req)
{
	uint16_t gref_id = 0;
	struct blkfront_gref *gref_elem;
	uint16_t nb_segments;
	int rc;
#if CONFIG_XEN_BLKFRONT_GREFPOOL
	struct uk_blkdev_queue *queue;
	struct blkfront_grefs_pool *grefs_pool;
#endif

	UK_ASSERT(req);
	nb_segments = req->nb_segments;

#if CONFIG_XEN_BLKFRONT_GREFPOOL
	queue = req->queue;
	grefs_pool = &queue->ref_pool;
	uk_semaphore_down(&grefs_pool->sem);
	for (; gref_id < nb_segments; ++gref_id) {
		gref_elem = req->gref[gref_id];
		if (!gref_elem->reusable_gref)
			break;

		UK_STAILQ_INSERT_TAIL(&grefs_pool->grefs_list,
			gref_elem,
			_list);
	}

	uk_semaphore_up(&grefs_pool->sem);
#endif
	for (; gref_id < nb_segments; ++gref_id) {
		gref_elem = req->gref[gref_id];
		if (gref_elem->ref != GRANT_INVALID_REF) {
			rc = gnttab_end_access(gref_elem->ref);
			UK_ASSERT(rc);
		}

		uk_free(drv_allocator, gref_elem);
	}
}

/* This function sets the grant references from pool to point to
 * data set at request.
 * Otherwise, new blkfront_gref elems are allocated and new grant refs
 * as well.
 **/
static void blkfront_request_map_grefs(struct blkif_request *ring_req,
		domid_t otherend_id)
{
	uint16_t gref_index;
	struct blkfront_request *blkfront_req;
	struct uk_blkreq *req;
	uint16_t nb_segments;
	uintptr_t data;
	uintptr_t start_sector;
	struct blkfront_gref *ref_elem;
#if CONFIG_XEN_BLKFRONT_GREFPOOL
	int rc;
#endif

	UK_ASSERT(ring_req);

	blkfront_req = (struct blkfront_request *)ring_req->id;
	req = blkfront_req->req;
	start_sector = round_pgdown((uintptr_t)req->aio_buf);
	nb_segments = blkfront_req->nb_segments;

	for (gref_index = 0; gref_index < nb_segments; ++gref_index) {
		data = start_sector + gref_index * PAGE_SIZE;
		ref_elem = blkfront_req->gref[gref_index];

#if CONFIG_XEN_BLKFRONT_GREFPOOL
		if (ref_elem->reusable_gref) {
			rc = gnttab_update_grant(ref_elem->ref, otherend_id,
				virtual_to_mfn(data), ring_req->operation);
			UK_ASSERT(rc);
		} else
#endif
		ref_elem->ref = gnttab_grant_access(otherend_id,
				virtual_to_mfn(data), ring_req->operation);

		UK_ASSERT(ref_elem->ref != GRANT_INVALID_REF);
		ring_req->seg[gref_index].gref = ref_elem->ref;
	}
}

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
	ring_req->operation = (req->operation == UK_BLKREQ_WRITE) ?
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
	if (req->operation == UK_BLKREQ_WRITE && cap->mode == O_RDONLY)
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

	/* Get blkfront_grefs from pool or allocate new ones */
	rc = blkfront_request_set_grefs(blkfront_req);
	if (rc)
		goto out;

	/* Map grant references to ring_req */
	blkfront_request_map_grefs(ring_req, dev->xendev->otherend_id);

out:
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

	if (req->operation == UK_BLKREQ_READ ||
			req->operation == UK_BLKREQ_WRITE)
		rc = blkfront_request_write(blkfront_req, ring_req);
	else if (req->operation == UK_BLKREQ_FFLUSH)
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

#define CHECK_STATUS(req, status, operation) \
	do { \
		if (status != BLKIF_RSP_OKAY) \
			uk_pr_err("Failed to "operation" %lu sector: %d\n", \
				req->start_sector,	\
				status);	\
		else	\
			uk_pr_debug("Succeed to "operation " %lu sector: %d\n",\
				req->start_sector, \
				status); \
	} while (0)

static int blkfront_queue_dequeue(struct uk_blkdev_queue *queue,
		struct uk_blkreq **req)
{
	RING_IDX prod, cons;
	struct blkif_response *rsp;
	struct uk_blkreq *req_from_q = NULL;
	struct blkfront_request *blkfront_req;
	struct blkif_front_ring *ring;
	uint8_t status;
	int rc = 0;

	UK_ASSERT(queue);
	UK_ASSERT(req);

	ring = &queue->ring;
	prod = ring->sring->rsp_prod;
	rmb(); /* Ensure we see queued responses up to 'rp'. */
	cons = ring->rsp_cons;

	/* No new descriptor since last dequeue operation */
	if (cons == prod)
		goto out;

	rsp = RING_GET_RESPONSE(ring, cons);
	blkfront_req = (struct blkfront_request *) rsp->id;
	UK_ASSERT(blkfront_req);
	req_from_q = blkfront_req->req;
	UK_ASSERT(req_from_q);
	status = rsp->status;
	switch (rsp->operation) {
	case BLKIF_OP_READ:
		CHECK_STATUS(req_from_q, status, "read");
		blkfront_request_reset_grefs(blkfront_req);
		break;
	case BLKIF_OP_WRITE:
		CHECK_STATUS(req_from_q, status, "write");
		blkfront_request_reset_grefs(blkfront_req);
		break;
	case BLKIF_OP_WRITE_BARRIER:
		if (status != BLKIF_RSP_OKAY)
			uk_pr_err("Write barrier error %d\n", status);
		break;
	case BLKIF_OP_FLUSH_DISKCACHE:
		if (status != BLKIF_RSP_OKAY)
			uk_pr_err("Flush_diskcache error %d\n", status);
		break;
	default:
		uk_pr_err("Unrecognized block operation %d (rsp %d)\n",
				rsp->operation, status);
		break;
	}

	req_from_q->result = -status;
	uk_free(drv_allocator, blkfront_req);
	ring->rsp_cons++;

out:
	*req = req_from_q;
	return rc;
}

static int blkfront_complete_reqs(struct uk_blkdev *blkdev,
		struct uk_blkdev_queue *queue)
{
	struct uk_blkreq *req;
	int rc;
	int more;

	UK_ASSERT(blkdev);
	UK_ASSERT(queue);

	/* Queue interrupts have to be off when calling receive */
	UK_ASSERT(!(queue->intr_enabled & BLKFRONT_INTR_EN));
moretodo:
	for (;;) {
		rc = blkfront_queue_dequeue(queue, &req);
		if (rc < 0) {
			uk_pr_err("Failed to dequeue the request: %d\n", rc);
			goto err_exit;
		}

		if (!req)
			break;

		uk_blkreq_finished(req);
		if (req->cb)
			req->cb(req, req->cb_cookie);
	}

	/* Enable interrupt only when user had previously enabled it */
	if (queue->intr_enabled & BLKFRONT_INTR_USR_EN_MASK) {
		/* Need to enable the interrupt on the last packet */
		rc = blkfront_xen_ring_intr_enable(queue);
		if (rc == 1)
			goto moretodo;
	} else {
		RING_FINAL_CHECK_FOR_RESPONSES(&queue->ring, more);
		if (more)
			goto moretodo;
	}

	return 0;

err_exit:
	return rc;

}

static int blkfront_ring_init(struct uk_blkdev_queue *queue)
{
	struct blkif_sring *sring = NULL;
	struct blkfront_dev *dev;

	UK_ASSERT(queue);
	dev = queue->dev;
	sring = uk_palloc(queue->a, BLK_RING_PAGES_NUM);
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
		uk_pfree(queue->a, queue->ring.sring, BLK_RING_PAGES_NUM);
}

#if CONFIG_XEN_BLKFRONT_GREFPOOL
static void blkfront_queue_gref_pool_release(struct uk_blkdev_queue *queue)
{
	struct blkfront_grefs_pool *grefs_pool;
	struct blkfront_gref *ref_elem;
	int rc;

	UK_ASSERT(queue);
	grefs_pool = &queue->ref_pool;

	while (!UK_STAILQ_EMPTY(&grefs_pool->grefs_list)) {
		ref_elem = UK_STAILQ_FIRST(&grefs_pool->grefs_list);
		if (ref_elem->ref != GRANT_INVALID_REF) {
			rc = gnttab_end_access(ref_elem->ref);
			UK_ASSERT(rc);
		}

		uk_free(queue->a, ref_elem);
		UK_STAILQ_REMOVE_HEAD(&grefs_pool->grefs_list, _list);
	}
}

static int blkfront_queue_gref_pool_setup(struct uk_blkdev_queue *queue)
{
	int ref_idx;
	struct blkfront_gref *gref_elem;
	struct blkfront_dev *dev;
	int rc = 0;

	UK_ASSERT(queue);
	dev = queue->dev;
	uk_semaphore_init(&queue->ref_pool.sem, 1);
	UK_STAILQ_INIT(&queue->ref_pool.grefs_list);

	for (ref_idx = 0; ref_idx < BLKIF_MAX_SEGMENTS_PER_REQUEST; ++ref_idx) {
		gref_elem = uk_malloc(queue->a, sizeof(*gref_elem));
		if (!gref_elem) {
			rc = -ENOMEM;
			goto err;
		}

		gref_elem->ref = gnttab_grant_access(dev->xendev->otherend_id,
				0, 1);
		UK_ASSERT(gref_elem->ref != GRANT_INVALID_REF);
		gref_elem->reusable_gref = true;
		UK_STAILQ_INSERT_TAIL(&queue->ref_pool.grefs_list, gref_elem,
				_list);
	}

out:
	return rc;
err:
	blkfront_queue_gref_pool_release(queue);
	goto out;
}
#endif

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

#if CONFIG_XEN_BLKFRONT_GREFPOOL
	err = blkfront_queue_gref_pool_setup(queue);
	if (err)
		goto err_out;
#endif

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

#if CONFIG_XEN_BLKFRONT_GREFPOOL
	blkfront_queue_gref_pool_release(queue);
#endif

	return 0;
}

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
	.queue_configure = blkfront_queue_setup,
	.queue_unconfigure = blkfront_queue_release,
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
	d->blkdev.finish_reqs = blkfront_complete_reqs;
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
