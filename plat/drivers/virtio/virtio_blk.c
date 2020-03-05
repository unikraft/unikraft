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

#include <virtio/virtio_bus.h>
#include <virtio/virtio_ids.h>

#define DRIVER_NAME		"virtio-blk"

static struct uk_alloc *a;

static int virtio_blk_add_dev(struct virtio_dev *vdev)
{
	int rc = 0;

	UK_ASSERT(vdev != NULL);

	return rc;
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
