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
#include <virtio/virtio_blk.h>
#include <uk/blkdev_driver.h>

#define DRIVER_NAME		"virtio-blk"
#define DEFAULT_SECTOR_SIZE	512

#define to_virtioblkdev(bdev) \
	__containerof(bdev, struct virtio_blk_device, blkdev)

/* Features are:
 *	Access Mode
 *	Sector_size;
 *	Multi-queue,
 **/
#define VIRTIO_BLK_DRV_FEATURES(features) \
	(VIRTIO_FEATURES_UPDATE(features, VIRTIO_BLK_F_RO | \
	VIRTIO_BLK_F_BLK_SIZE | VIRTIO_BLK_F_MQ))

static struct uk_alloc *a;
static const char *drv_name = DRIVER_NAME;

struct virtio_blk_device {
	/* Pointer to Unikraft Block Device */
	struct uk_blkdev blkdev;
	/* The blkdevice identifier */
	__u16 uid;
	/* Virtio Device */
	struct virtio_dev *vdev;
	/* List of all the virtqueue in the pci device */
	struct virtqueue *vq;
	/* Nb of max_queues supported by device */
	__u16 max_vqueue_pairs;
	/* This is used when the user has decided the nb_queues to use */
	__u16    nb_queues;
	/* List of queues */
	struct   uk_blkdev_queue *qs;
};

struct uk_blkdev_queue {
	/* The virtqueue reference */
	struct virtqueue *vq;
	/* The libukblkdev queue identifier */
	/* It is also the virtqueue identifier */
	uint16_t lqueue_id;
	/* Allocator */
	struct uk_alloc *a;
	/* The nr. of descriptor limit */
	uint16_t max_nb_desc;
	/* Reference to virtio_blk_device  */
	struct virtio_blk_device *vbd;
};


static int virtio_blkdev_queues_alloc(struct virtio_blk_device *vbdev,
				    const struct uk_blkdev_conf *conf)
{
	int rc = 0;
	uint16_t i = 0;
	int vq_avail = 0;
	__u16 qdesc_size[conf->nb_queues];

	if (conf->nb_queues > vbdev->max_vqueue_pairs) {
		uk_pr_err("Queue number not supported: %"__PRIu16"\n",
				conf->nb_queues);
		return -ENOTSUP;
	}

	vbdev->nb_queues = conf->nb_queues;
	vq_avail = virtio_find_vqs(vbdev->vdev, conf->nb_queues, qdesc_size);
	if (unlikely(vq_avail != conf->nb_queues)) {
		uk_pr_err("Expected: %d queues, Found: %d queues\n",
				conf->nb_queues, vq_avail);
		rc = -ENOMEM;
		goto exit;
	}

	/**
	 * TODO:
	 * The virtio device management data structure are allocated using the
	 * allocator from the blkdev configuration. In the future it might be
	 * wiser to move it to the allocator of each individual queue. This
	 * would better considering NUMA support.
	 */
	vbdev->qs = uk_calloc(a, conf->nb_queues, sizeof(*vbdev->qs));
	if (unlikely(vbdev->qs == NULL)) {
		uk_pr_err("Failed to allocate memory for queue management\n");
		rc = -ENOMEM;
		goto exit;
	}

	for (i = 0; i < conf->nb_queues; ++i)
		vbdev->qs[i].max_nb_desc = qdesc_size[i];

exit:
	return rc;
}

static int virtio_blkdev_configure(struct uk_blkdev *dev,
		const struct uk_blkdev_conf *conf)
{
	int rc = 0;
	struct virtio_blk_device *vbdev = NULL;

	UK_ASSERT(dev != NULL);
	UK_ASSERT(conf != NULL);

	vbdev = to_virtioblkdev(dev);
	rc = virtio_blkdev_queues_alloc(vbdev, conf);
	if (rc) {
		uk_pr_err("Failed to allocate the queues %d\n", rc);
		goto exit;
	}

	uk_pr_info(DRIVER_NAME": %"__PRIu16" configured\n", vbdev->uid);
exit:
	return rc;
}

static int virtio_blkdev_unconfigure(struct uk_blkdev *dev)
{
	struct virtio_blk_device *d;

	UK_ASSERT(dev != NULL);
	d = to_virtioblkdev(dev);
	uk_free(a, d->qs);

	return 0;
}

static void virtio_blkdev_get_info(struct uk_blkdev *dev,
		struct uk_blkdev_info *dev_info)
{
	struct virtio_blk_device *vbdev = NULL;

	UK_ASSERT(dev != NULL);
	UK_ASSERT(dev_info != NULL);

	vbdev = to_virtioblkdev(dev);
	dev_info->max_queues = vbdev->max_vqueue_pairs;
}

static int virtio_blkdev_feature_negotiate(struct virtio_blk_device *vbdev)
{
	struct uk_blkdev_cap *cap;
	__u64 host_features = 0;
	int bytes_to_read;
	__sector sectors;
	__sector ssize;
	__u16 num_queues;
	int rc = 0;

	UK_ASSERT(vbdev);
	cap = &vbdev->blkdev.capabilities;
	host_features = virtio_feature_get(vbdev->vdev);

	/* Get size of device */
	bytes_to_read = virtio_config_get(vbdev->vdev,
			__offsetof(struct virtio_blk_config, capacity),
			&sectors,
			sizeof(sectors),
			1);
	if (bytes_to_read != sizeof(sectors))  {
		uk_pr_err("Failed to get nb of sectors from device %d\n", rc);
		rc = -EAGAIN;
		goto exit;
	}

	if (!virtio_has_features(host_features, VIRTIO_BLK_F_BLK_SIZE)) {
		ssize = DEFAULT_SECTOR_SIZE;
	} else {
		bytes_to_read = virtio_config_get(vbdev->vdev,
				__offsetof(struct virtio_blk_config, blk_size),
				&ssize,
				sizeof(ssize),
				1);
		if (bytes_to_read != sizeof(ssize))  {
			uk_pr_err("Failed to get ssize from the device %d\n",
					rc);
			rc = -EAGAIN;
			goto exit;
		}
	}

	/* If the device does not support multi-queues,
	 * we will use only one queue.
	 */
	if (virtio_has_features(host_features, VIRTIO_BLK_F_MQ)) {
		bytes_to_read = virtio_config_get(vbdev->vdev,
					__offsetof(struct virtio_blk_config,
							num_queues),
					&num_queues,
					sizeof(num_queues),
					1);
		if (bytes_to_read != sizeof(num_queues)) {
			uk_pr_err("Failed to read max-queues\n");
			rc = -EAGAIN;
			goto exit;
		}
	} else
		num_queues = 1;
	cap->ssize = ssize;
	cap->sectors = sectors;
	cap->ioalign = sizeof(void *);
	cap->mode = (virtio_has_features(
			host_features, VIRTIO_BLK_F_RO)) ? O_RDONLY : O_RDWR;

	vbdev->max_vqueue_pairs = num_queues;

	/**
	 * Mask out features supported by both driver and device.
	 */
	vbdev->vdev->features &= host_features;
	virtio_feature_set(vbdev->vdev, vbdev->vdev->features);

exit:
	return rc;
}

static inline void virtio_blkdev_feature_set(struct virtio_blk_device *vbdev)
{
	vbdev->vdev->features = 0;

	/* Setting the feature the driver support */
	VIRTIO_BLK_DRV_FEATURES(vbdev->vdev->features);
}

static const struct uk_blkdev_ops virtio_blkdev_ops = {
		.get_info = virtio_blkdev_get_info,
		.dev_configure = virtio_blkdev_configure,
		.dev_unconfigure = virtio_blkdev_unconfigure,
};

static int virtio_blk_add_dev(struct virtio_dev *vdev)
{
	struct virtio_blk_device *vbdev;
	int rc = 0;

	UK_ASSERT(vdev != NULL);

	vbdev = uk_calloc(a, 1, sizeof(*vbdev));
	if (!vbdev)
		return -ENOMEM;

	vbdev->vdev = vdev;
	vbdev->blkdev.dev_ops = &virtio_blkdev_ops;

	rc = uk_blkdev_drv_register(&vbdev->blkdev, a, drv_name);
	if (rc < 0) {
		uk_pr_err("Failed to register virtio_blk device: %d\n", rc);
		goto err_out;
	}

	vbdev->uid = rc;
	virtio_blkdev_feature_set(vbdev);
	rc = virtio_blkdev_feature_negotiate(vbdev);
	if (rc) {
		uk_pr_err("Failed to negotiate the device feature %d\n", rc);
		goto err_negotiate_feature;
	}

	uk_pr_info("Virtio-blk device registered with libukblkdev\n");

out:
	return rc;
err_negotiate_feature:
	virtio_dev_status_update(vbdev->vdev, VIRTIO_CONFIG_STATUS_FAIL);
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
