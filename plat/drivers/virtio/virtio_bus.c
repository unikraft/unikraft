/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Sharan Santhanam <sharan.santhanam@neclab.eu>
 *
 * Copyright (c) 2018, NEC Europe Ltd., NEC Corporation. All rights reserved.
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
#include <uk/arch/types.h>
#include <uk/list.h>
#include <uk/alloc.h>
#include <uk/bus.h>
#include <virtio/virtio_ids.h>
#include <virtio/virtio_config.h>
#include <virtio/virtio_bus.h>

UK_TAILQ_HEAD(virtio_driver_list, struct virtio_driver);
/**
 * Module local data structure(s).
 */
static struct virtio_driver_list virtio_drvs =
			UK_TAILQ_HEAD_INITIALIZER(virtio_drvs);
static struct uk_alloc *a;

/**
 *   Driver module local function(s).
 */
static int virtio_device_reinit(struct virtio_dev *vdev);
static struct virtio_driver *find_match_drv(struct virtio_dev *vdev);
static int virtio_bus_init(struct uk_alloc *mem_alloc);
static int virtio_bus_probe(void);

static inline int virtio_device_id_match(const struct virtio_dev_id *id0,
					 const struct virtio_dev_id *id1)
{
	int rc = 0;

	if (id0->virtio_device_id == id1->virtio_device_id)
		rc = 1;

	return rc;
}

/**
 * Find a match driver
 * @param vdev
 *	Reference to the virtio device.
 */
static struct virtio_driver *find_match_drv(struct virtio_dev *vdev)
{
	int i = 0;
	struct virtio_driver *drv = NULL;

	UK_TAILQ_FOREACH(drv, &virtio_drvs, next) {
		i = 0;
		while (drv->dev_ids[i].virtio_device_id != VIRTIO_ID_INVALID) {
			if (virtio_device_id_match(&drv->dev_ids[i],
						   &vdev->id)) {
				return drv;
			}
			i++;
		}
	}
	return NULL;
}

/**
 * Reinitialize the virtio device
 * @param vdev
 *	Reference to the virtio device.
 */
static int virtio_device_reinit(struct virtio_dev *vdev)
{
	int rc = 0;

	/**
	 * Resetting the virtio device
	 * This may not be necessary while initializing the device for the first
	 * time.
	 */
	if (vdev->cops->device_reset) {
		vdev->cops->device_reset(vdev);
		/* Set the device status */
		vdev->status = VIRTIO_DEV_RESET;
	}
	/* Acknowledge the virtio device */
	rc = virtio_dev_status_update(vdev, VIRTIO_CONFIG_STATUS_ACK);
	if (rc != 0) {
		uk_pr_err("Failed to acknowledge the virtio device %p: %d\n",
			  vdev, rc);
		return rc;
	}

	/* Acknowledge the virtio driver */
	rc = virtio_dev_status_update(vdev, VIRTIO_CONFIG_STATUS_DRIVER);
	if (rc != 0) {
		uk_pr_err("Failed to acknowledge the virtio driver %p: %d\n",
			  vdev, rc);
		return rc;
	}
	vdev->status = VIRTIO_DEV_INITIALIZED;
	uk_pr_info("Virtio device %p initialized\n", vdev);
	return rc;
}

int virtio_bus_register_device(struct virtio_dev *vdev)
{
	struct virtio_driver *drv = NULL;
	int rc = 0;

	UK_ASSERT(vdev);
	/* Check for the dev with the driver list */
	drv = find_match_drv(vdev);
	if (!drv) {
		uk_pr_err("Failed to find the driver for the virtio device %p (id:%"__PRIu16")\n",
			  vdev, vdev->id.virtio_device_id);
		return -EFAULT;
	}
	vdev->vdrv = drv;

	/* Initialize the device */
	rc = virtio_device_reinit(vdev);
	if (rc != 0) {
		uk_pr_err("Failed to initialize the virtio device %p (id:%"__PRIu16": %d\n",
			  vdev, vdev->id.virtio_device_id, rc);
		return rc;
	}

	/* Initialize the virtqueue list */
	UK_TAILQ_INIT(&vdev->vqs);

	/* Calling the driver add device */
	rc = drv->add_dev(vdev);
	if (rc != 0) {
		uk_pr_err("Failed to add the virtio device %p: %d\n", vdev, rc);
		goto virtio_dev_fail_set;
	}
exit:
	return rc;

virtio_dev_fail_set:
	/**
	 * We set the status to fail. We can ignore the exit status from the
	 * status update.
	 */
	virtio_dev_status_update(vdev, VIRTIO_CONFIG_STATUS_FAIL);
	goto exit;
}

/**
 * Probe for the virtio device.
 */
static int virtio_bus_probe(void)
{
	return 0;
}

/**
 * Initialize the virtio bus driver(s).
 * @param mem_alloc
 *	Reference to the mem_allocator.
 * @return
 *	(int) On successful initialization return the count of device
 *	initialized.
 *	On error return -1.
 */
static int virtio_bus_init(struct uk_alloc *mem_alloc)
{
	struct virtio_driver *drv = NULL, *ndrv = NULL;
	int ret = 0, dev_count = 0;

	a = mem_alloc;
	UK_TAILQ_FOREACH_SAFE(drv, &virtio_drvs, next, ndrv) {
		if (drv->init) {
			ret = drv->init(a);
			if (unlikely(ret)) {
				uk_pr_err("Failed to initialize virtio driver %p: %d\n",
					  drv, ret);
				UK_TAILQ_REMOVE(&virtio_drvs, drv, next);
			} else
				dev_count++;
		}
	}
	return (dev_count > 0) ? dev_count : -1;
}

/**
 * Register the virtio driver(s).
 * @param vdrv
 *	Reference to the virtio_driver
 */
void _virtio_bus_register_driver(struct virtio_driver *vdrv)
{
	UK_TAILQ_INSERT_TAIL(&virtio_drvs, vdrv, next);
}

static struct uk_bus virtio_bus = {
	.init = virtio_bus_init,
	.probe = virtio_bus_probe,
};
UK_BUS_REGISTER(&virtio_bus);
