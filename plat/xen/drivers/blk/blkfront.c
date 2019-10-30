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
#include <uk/blkdev_driver.h>
#include <xenbus/xenbus.h>
#include "blkfront.h"
#include "blkfront_xb.h"

#define DRIVER_NAME		"xen-blkfront"


/* Get blkfront_dev* which contains blkdev */
#define to_blkfront(blkdev) \
	__containerof(blkdev, struct blkfront_dev, blkdev)

static struct uk_alloc *drv_allocator;


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
	.dev_unconfigure = blkfront_unconfigure,
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
