/*
 * Authors: Roxana Nicolescu <nicolescu.roxana1996@gmail.com>
 *
 * Copyright (c) 2019, University Politehnica of Bucharest.
 *
 * Permission to use, copy, modify, and/or distribute this software
 * for any purpose with or without fee is hereby granted, provided
 * that the above copyright notice and this permission notice appear
 * in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 * AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uk/print.h>
#include <errno.h>
#include <fcntl.h>
#include <virtio/virtio_bus.h>
#include <virtio/virtio_ids.h>
#include <uk/blkdev.h>
#include <uk/blkdev_driver.h>

#define DRIVER_NAME		"virtio-blk"

#define to_virtioblkdev(bdev) \
	__containerof(bdev, struct virtio_blk_device, blkdev)


static struct uk_alloc *a;
static const char *drv_name = DRIVER_NAME;

struct virtio_blk_device {
	/* Pointer to Unikraft Block Device */
	struct uk_blkdev blkdev;
	/* The blkdevice identifier */
	__u16 uid;
};

static int virtio_blk_add_dev(struct virtio_dev *vdev)
{
	struct virtio_blk_device *vbdev;
	int rc = 0;

	UK_ASSERT(vdev != NULL);

	vbdev = uk_calloc(a, 1, sizeof(*vbdev));
	if (!vbdev)
		return -ENOMEM;

	rc = uk_blkdev_drv_register(&vbdev->blkdev, a, drv_name);
	if (rc < 0) {
		uk_pr_err("Failed to register virtio_blk device: %d\n", rc);
		goto err_out;
	}

	vbdev->uid = rc;
	uk_pr_info("Virtio-blk device registered with libukblkdev\n");

out:
	return rc;
err_out:
	uk_free(a, vbdev);
	goto out;
}

static int virtio_blk_drv_init(struct uk_alloc *drv_allocator)
{
	/* driver initialization */
	if (!drv_allocator)
		return -EINVAL;

	a = drv_allocator;
	return 0;
}

static const struct virtio_dev_id vblk_dev_id[] = {
	{VIRTIO_ID_BLOCK},
	{VIRTIO_ID_INVALID} /* List Terminator */
};

static struct virtio_driver vblk_drv = {
	.dev_ids = vblk_dev_id,
	.init    = virtio_blk_drv_init,
	.add_dev = virtio_blk_add_dev
};
VIRTIO_BUS_REGISTER_DRIVER(&vblk_drv);
