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
#define _GNU_SOURCE /* for asprintf() */
#include <stdlib.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <uk/errptr.h>
#include <uk/print.h>
#include <uk/assert.h>
#include <xenbus/client.h>
#include <xenbus/xs.h>
#if defined(__aarch64__)
#include <xen-arm/mm.h>
#endif
#include "blkfront_xb.h"

static int xs_read_backend_id(const char *nodename, domid_t *domid)
{
	char *path = NULL;
	int value, err;

	UK_ASSERT(nodename != NULL);

	err = asprintf(&path, "%s/backend-id", nodename);
	if (err <= 0) {
		uk_pr_err("Failed to allocate and format path: %d.\n", err);
		goto out;
	}

	err = xs_read_integer(XBT_NIL, path, &value);
	if (err == 0)
		*domid = (domid_t) value;

out:
	free(path);
	return err;
}

static int blkfront_xb_get_nb_max_queues(struct blkfront_dev *dev)
{
	int err = 0;
	struct xenbus_device *xendev;

	UK_ASSERT(dev != NULL);
	xendev = dev->xendev;

	err = xs_scanf(XBT_NIL, xendev->otherend, "multi-queue-max-queues",
				"%"SCNu16,
				&dev->nb_queues);
	if (err < 0) {
		uk_pr_err("Failed to read multi-queue-max-queues: %d\n", err);
		return err;
	}

	return 0;
}

int blkfront_xb_init(struct blkfront_dev *dev)
{
	struct xenbus_device *xendev;
	char *nodename;
	int err = 0;

	UK_ASSERT(dev != NULL);
	xendev = dev->xendev;

	err = xs_read_backend_id(xendev->nodename, &xendev->otherend_id);
	if (err)
		goto out;

	/* Get handle */
	nodename = strrchr(xendev->nodename, '/');
	if (!nodename) {
		err = -EINVAL;
		goto out;
	}

	dev->handle = strtoul(nodename + 1, NULL, 0);
	if (!dev->handle) {
		err = -EINVAL;
		goto out;
	}

	/* Read otherend path */
	xendev->otherend = xs_read(XBT_NIL, xendev->nodename, "backend");
	if (PTRISERR(xendev->otherend)) {
		uk_pr_err("Failed to read backend path: %d.\n", err);
		err = PTR2ERR(xendev->otherend);
		xendev->otherend = NULL;
		goto out;
	}

	err = blkfront_xb_get_nb_max_queues(dev);
	if (err) {
		uk_pr_err("Failed to read multi-queue-max-queues: %d\n", err);
		goto out;
	}
out:
	return err;
}

void blkfront_xb_fini(struct blkfront_dev *dev)
{
	struct xenbus_device *xendev;

	UK_ASSERT(dev != NULL);

	xendev = dev->xendev;
	if (xendev->otherend) {
		free(xendev->otherend);
		xendev->otherend = NULL;
	}
}

int blkfront_xb_write_nb_queues(struct blkfront_dev *dev)
{
	int err;
	struct xenbus_device *xendev;

	UK_ASSERT(dev);

	xendev = dev->xendev;
	err = xs_printf(XBT_NIL, xendev->nodename,
			"multi-queue-num-queues",
			"%u",
			dev->nb_queues);
	if (err < 0) {
		uk_pr_err("Failed to write multi-queue-num-queue: %d\n", err);
		goto out;
	}

	err = 0;

out:
	return err;
}

/*
 * Get device capabilities (sector_size, etc)
 * from the backend after it switched to Connected State.
 */
static int blkfront_xb_get_capabilities(struct blkfront_dev *blkdev)
{
	struct xenbus_device *xendev;
	char *mode;
	int err = 0;

	UK_ASSERT(blkdev != NULL);
	xendev = blkdev->xendev;
	err = xs_scanf(XBT_NIL, xendev->otherend, "sectors",
		"%lu", &blkdev->blkdev.capabilities.sectors);
	if (err < 0) {
		uk_pr_err("Failed to read nb_sectors from xs: %d\n", err);
		return err;
	}

	err = xs_scanf(XBT_NIL, xendev->otherend, "sector-size",
			"%lu", &blkdev->blkdev.capabilities.ssize);
	if (err < 0) {
		uk_pr_err("Failed to read ssize from xs: %d\n", err);
		return err;
	}

	err = xs_scanf(XBT_NIL, xendev->otherend, "feature-flush-cache",
					"%d", &blkdev->flush);
	if (err < 0) {
		uk_pr_err("Failed to read feature-flush-cache from xs: %d\n",
				err);
		return err;
	}

	err = xs_scanf(XBT_NIL, xendev->otherend, "feature-barrier",
					"%d", &blkdev->barrier);
	if (err < 0) {
		uk_pr_err("Failed to read feature-barrier from xs: %d\n", err);
		return err;
	}

	mode = xs_read(XBT_NIL, xendev->otherend, "mode");
	if (PTRISERR(mode)) {
		uk_pr_err("Failed to read mode from xs: %d.\n", err);
		return PTR2ERR(mode);
	}

	blkdev->blkdev.capabilities.mode = (*mode == 'r') ? O_RDONLY : O_RDWR;
	blkdev->blkdev.capabilities.max_sectors_per_req =
			(BLKIF_MAX_SEGMENTS_PER_REQUEST - 1) *
			(PAGE_SIZE / blkdev->blkdev.capabilities.ssize) + 1;
	blkdev->blkdev.capabilities.ioalign = blkdev->blkdev.capabilities.ssize;

	free(mode);
	return 0;
}

/* Write info for a specific queue in xenstore.
 * If there is only one queue, the path does not
 * include queue-<queue-id>.
 **/
static int blkfront_xb_write_ring_info(struct blkfront_dev *dev,
		uint16_t queue_id,
		xenbus_transaction_t xbt)
{
	struct xenbus_device *xendev;
	char *node_ring_ref = NULL;
	char *node_evtchn = NULL;
	int err = 0;

	UK_ASSERT(dev != NULL);
	xendev = dev->xendev;

	if (queue_id == 0 && dev->nb_queues == 1) {
		err = asprintf(&node_ring_ref, "ring-ref");
		if (err <= 0) {
			uk_pr_err("Failed to format ring_ref path: %d\n", err);
			goto out;
		}

		err = asprintf(&node_evtchn, "event-channel");
		if (err <= 0) {
			uk_pr_err("Failed to format event-channel path: %d\n",
					err);
			goto out;
		}
	} else {
		err = asprintf(&node_ring_ref, "queue-%"PRIu16"/ring-ref",
				queue_id);
		if (err <= 0) {
			uk_pr_err("Failed to format ring_ref path: %d\n", err);
			goto out;
		}

		err = asprintf(&node_evtchn, "queue-%"PRIu16"/event-channel",
				queue_id);
		if (err <= 0) {
			uk_pr_err("Failed to format event-channel path: %d\n",
					err);
			goto out;
		}
	}

	err = xs_printf(xbt, xendev->nodename,
				node_evtchn,
				"%u",
				dev->blkdev._queue[queue_id]->evtchn);
	if (err <= 0) {
		uk_pr_err("Failed to write event-channel: %d\n", err);
		goto out;
	}

	err = xs_printf(xbt, xendev->nodename,
			node_ring_ref,
			"%u",
			dev->blkdev._queue[queue_id]->ring_ref);
	if (err <= 0) {
		uk_pr_err("Failed to write ring_ref: %d\n", err);
		goto out;
	}

	err = 0;

out:
	free(node_evtchn);
	free(node_ring_ref);
	return err;
}

/* Delete ring entry */
static int blkfront_xb_delete_ring_info(struct blkfront_dev *dev,
		uint16_t queue_id)
{
	struct xenbus_device *xendev;
	char *node_ring_ref = NULL;
	char *node_evtchn = NULL;
	int err = 0;

	UK_ASSERT(dev);
	xendev = dev->xendev;

	if (queue_id == 0 && dev->nb_queues == 1) {
		err = asprintf(&node_ring_ref, "%s/ring-ref",
				xendev->nodename);
		if (err <= 0) {
			uk_pr_err("Failed to format ring_ref_path: %d\n", err);
			goto out;
		}

		err = asprintf(&node_evtchn, "%s/event-channel",
				xendev->nodename);
		if (err <= 0) {
			uk_pr_err("Failed to format event-channel path: %d\n",
					err);
			goto out;
		}
	} else {
		err = asprintf(&node_ring_ref, "%s/queue-%"PRIu16"/ring-ref",
				xendev->nodename,
				queue_id);
		if (err <= 0) {
			uk_pr_err("Failed to format ring_ref_path: %d\n", err);
			goto out;
		}

		err = asprintf(&node_evtchn, "%s/queue-%"PRIu16"/event-channel",
				xendev->nodename,
				queue_id);
		if (err <= 0) {
			uk_pr_err("Failed to format event-channel path: %d\n",
					err);
			goto out;
		}
	}

	err = xs_rm(XBT_NIL, node_ring_ref);
	if (err)
		uk_pr_err("Failed to remove ring_ref from xs: %d\n", err);

	err = xs_rm(XBT_NIL, node_evtchn);
	if (err)
		uk_pr_err("Failed to remove event-channel from xs: %d\n", err);
out:
	free(node_evtchn);
	free(node_ring_ref);
	return err;
}

static int blkfront_xb_write_rings_info(struct blkfront_dev *dev)
{
	xenbus_transaction_t xbt = 0;
	int err = 0;

	UK_ASSERT(dev != NULL);

again:
	err = xs_transaction_start(&xbt);
	if (err)
		goto abort_transaction;

	for (uint16_t queue_id = 0; queue_id < dev->nb_queues; ++queue_id) {
		err = blkfront_xb_write_ring_info(dev, queue_id, xbt);
		if (err) {
			uk_pr_err("Failed to write queue%"PRIu16" to xs: %d\n",
					queue_id,
					err);
			goto abort_transaction;
		}
	}

	err = xs_transaction_end(xbt, 0);
	if (err == -EAGAIN)
		goto again;

	if (err)
		uk_pr_err("Failed to end transaction: %d\n", err);

	return err;

abort_transaction:
	xs_transaction_end(xbt, 1);
	return err;
}

static int blkfront_xb_delete_rings_info(struct blkfront_dev *dev)
{
	int err = 0;

	UK_ASSERT(dev != NULL);

	for (uint16_t queue_id = 0; queue_id < dev->nb_queues; ++queue_id) {
		err = blkfront_xb_delete_ring_info(dev, queue_id);
		if (err) {
			uk_pr_err("Failed to delete ring_info for q-%"
					PRIu16 ": %d\n",
					queue_id, err);
			return err;
		}
	}

	return err;
}

#define WAIT_BE_STATE_CHANGE_WHILE_COND(state_cond) \
	do { \
		err = xs_read_integer(XBT_NIL, back_state_path,\
			(int *) &back_state); \
		if (err) \
			goto out; \
		while (!err && (state_cond)) \
			err = xenbus_wait_for_state_change(back_state_path, \
				&back_state, \
				xendev->otherend_watch); \
		if (err) \
			goto out; \
	} while (0)


static int blkfront_xb_wait_be_connect(struct blkfront_dev *blkdev)
{
	struct xenbus_device *xendev;
	char *back_state_path = NULL;
	XenbusState back_state;
	int err = 0;

	UK_ASSERT(blkdev != NULL);
	xendev = blkdev->xendev;
	err = asprintf(&back_state_path, "%s/state", xendev->otherend);
	if (err <= 0) {
		uk_pr_err("Failed to format back_state_path: %d\n", err);
		goto out;
	}

	WAIT_BE_STATE_CHANGE_WHILE_COND(back_state < XenbusStateConnected);
	if (back_state != XenbusStateConnected) {
		uk_pr_err("Backend not available, state=%s\n",
				xenbus_state_to_str(back_state));
		err = -1;
		goto out;
	}

out:
	free(back_state_path);
	return err;
}

static int blkfront_xb_wait_be_disconnect(struct blkfront_dev *blkdev)
{
	struct xenbus_device *xendev;
	char *back_state_path = NULL;
	XenbusState back_state;
	int err = 0;

	UK_ASSERT(blkdev != NULL);
	xendev = blkdev->xendev;

	err = asprintf(&back_state_path, "%s/state", xendev->otherend);
	if (err <= 0) {
		uk_pr_err("Failed to format back_state_path: %d\n", err);
		goto out;
	}

	WAIT_BE_STATE_CHANGE_WHILE_COND(back_state < XenbusStateClosing);
	err = xenbus_switch_state(XBT_NIL, xendev, XenbusStateClosed);
	if (err) {
		uk_pr_err("Failed to switch state to Closed: %d\n", err);
		goto out;
	}

	WAIT_BE_STATE_CHANGE_WHILE_COND(back_state < XenbusStateClosed);
	err = xenbus_switch_state(XBT_NIL, xendev, XenbusStateInitialising);
	if (err) {
		uk_pr_err("Failed to switch state to Initialising: %d\n", err);
		goto out;
	}

	WAIT_BE_STATE_CHANGE_WHILE_COND(back_state < XenbusStateInitWait ||
			back_state >= XenbusStateClosed);

out:
	free(back_state_path);
	return err;
}

int blkfront_xb_connect(struct blkfront_dev *blkdev)
{
	int err;
	struct xenbus_device *xendev;

	UK_ASSERT(blkdev != NULL);
	xendev = blkdev->xendev;

	err = blkfront_xb_write_rings_info(blkdev);
	if (err) {
		uk_pr_err("Failed to write rings info to xenstore: %d\n.",
				err);
		return err;
	}

	err = xenbus_switch_state(XBT_NIL, xendev, XenbusStateConnected);
	if (err) {
		uk_pr_err("Failed to switch state to XenbusStateConnected: %d.\n",
				err);
		goto err;
	}


	err = blkfront_xb_wait_be_connect(blkdev);
	if (err) {
		uk_pr_err("Backend failed to change state: %d.\n", err);
		goto err;
	}

	err = blkfront_xb_get_capabilities(blkdev);
	if (err) {
		uk_pr_err("Failed to extract info from backend: %d.\n", err);
		goto err;
	}

err:
	return err;
}

int blkfront_xb_disconnect(struct blkfront_dev *blkdev)
{
	struct xenbus_device *xendev;
	int err;

	UK_ASSERT(blkdev != NULL);

	xendev = blkdev->xendev;

	uk_pr_info("Disconnect blkfront: backend at %s\n",
			xendev->otherend);

	err = xenbus_switch_state(XBT_NIL, xendev, XenbusStateClosing);
	if (err) {
		uk_pr_err("Failed to switch state to Closing: %d\n", err);
		goto out;
	}

	err = blkfront_xb_wait_be_disconnect(blkdev);
	if (err) {
		uk_pr_err("Failed to disconnect: %d\n", err);
		goto out;
	}

	err = blkfront_xb_delete_rings_info(blkdev);
	if (err) {
		uk_pr_err("Failed to delete rings info: %d\n", err);
		goto out;
	}

out:
	return err;
}
