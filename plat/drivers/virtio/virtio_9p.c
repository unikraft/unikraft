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

#include <inttypes.h>
#include <uk/alloc.h>
#include <uk/essentials.h>
#include <virtio/virtio_bus.h>
#include <virtio/virtio_9p.h>
#include <uk/plat/spinlock.h>

#define DRIVER_NAME	"virtio-9p"
#define NUM_SEGMENTS	128 /** The number of virtqueue descriptors. */
static struct uk_alloc *a;

/* List of initialized virtio 9p devices. */
static UK_LIST_HEAD(virtio_9p_device_list);
static DEFINE_SPINLOCK(virtio_9p_device_list_lock);

struct virtio_9p_device {
	/* Virtio device. */
	struct virtio_dev *vdev;
	/* Mount tag for this virtio device. */
	char *tag;
	/* Entry within the virtio devices' list. */
	struct uk_list_head _list;
	/* Virtqueue reference. */
	struct virtqueue *vq;
	/* Hw queue identifier. */
	uint16_t hwvq_id;
};

static int virtio_9p_recv(struct virtqueue *vq __unused, void *priv __unused)
{
	return 0;
}

static int virtio_9p_vq_alloc(struct virtio_9p_device *d)
{
	int vq_avail = 0;
	int rc = 0;
	__u16 qdesc_size;

	vq_avail = virtio_find_vqs(d->vdev, 1, &qdesc_size);
	if (unlikely(vq_avail != 1)) {
		uk_pr_err(DRIVER_NAME": Expected: %d queues, found %d\n",
			  1, vq_avail);
		rc = -ENOMEM;
		goto exit;
	}

	d->hwvq_id = 0;
	if (unlikely(qdesc_size != NUM_SEGMENTS)) {
		uk_pr_err(DRIVER_NAME": Expected %d descriptors, found %d (virtqueue %"
			  PRIu16")\n", NUM_SEGMENTS, qdesc_size, d->hwvq_id);
		rc = -EINVAL;
		goto exit;
	}

	d->vq = virtio_vqueue_setup(d->vdev,
				    d->hwvq_id,
				    qdesc_size,
				    virtio_9p_recv,
				    a);
	if (unlikely(PTRISERR(d->vq))) {
		uk_pr_err(DRIVER_NAME": Failed to set up virtqueue %"PRIu16"\n",
			  d->hwvq_id);
		rc = PTR2ERR(d->vq);
	}

	d->vq->priv = d;

exit:
	return rc;
}

static int virtio_9p_feature_negotiate(struct virtio_9p_device *d)
{
	__u64 host_features;
	__u16 tag_len;
	int rc = 0;

	host_features = virtio_feature_get(d->vdev);
	if (!virtio_has_features(host_features, VIRTIO_9P_F_MOUNT_TAG)) {
		uk_pr_err(DRIVER_NAME": Host system does not offer MOUNT_TAG feature\n");
		rc = -EINVAL;
		goto out;
	}

	virtio_config_get(d->vdev,
			  __offsetof(struct virtio_9p_config, tag_len),
			  &tag_len, 1, sizeof(tag_len));

	d->tag = uk_calloc(a, tag_len + 1, sizeof(*d->tag));
	if (!d->tag) {
		rc = -ENOMEM;
		goto out;
	}

	virtio_config_get(d->vdev,
			  __offsetof(struct virtio_9p_config, tag),
			  d->tag, tag_len, 1);
	d->tag[tag_len] = '\0';

	d->vdev->features &= host_features;
	virtio_feature_set(d->vdev, d->vdev->features);

out:
	return rc;
}

static inline void virtio_9p_feature_set(struct virtio_9p_device *d)
{
	d->vdev->features = 0;
	VIRTIO_FEATURES_UPDATE(d->vdev->features, VIRTIO_9P_F_MOUNT_TAG);
}

static int virtio_9p_configure(struct virtio_9p_device *d)
{
	int rc = 0;

	rc = virtio_9p_feature_negotiate(d);
	if (rc != 0) {
		uk_pr_err(DRIVER_NAME": Failed to negotiate the device feature %d\n",
			rc);
		rc = -EINVAL;
		goto out_status_fail;
	}

	rc = virtio_9p_vq_alloc(d);
	if (rc) {
		uk_pr_err(DRIVER_NAME": Could not allocate virtqueue\n");
		goto out_status_fail;
	}

	uk_pr_info(DRIVER_NAME": Configured: features=0x%lx tag=%s\n",
			d->vdev->features, d->tag);
out:
	return rc;

out_status_fail:
	virtio_dev_status_update(d->vdev, VIRTIO_CONFIG_STATUS_FAIL);
	goto out;
}

static int virtio_9p_start(struct virtio_9p_device *d)
{
	virtqueue_intr_enable(d->vq);
	virtio_dev_drv_up(d->vdev);
	uk_pr_info(DRIVER_NAME": %s started\n", d->tag);

	return 0;
}

static int virtio_9p_add_dev(struct virtio_dev *vdev)
{
	struct virtio_9p_device *d;
	unsigned long flags;
	int rc = 0;

	UK_ASSERT(vdev != NULL);

	d = uk_calloc(a, 1, sizeof(*d));
	if (!d) {
		rc = -ENOMEM;
		goto out;
	}
	d->vdev = vdev;
	virtio_9p_feature_set(d);
	rc = virtio_9p_configure(d);
	if (rc)
		goto out_free;
	rc = virtio_9p_start(d);
	if (rc)
		goto out_free;

	ukplat_spin_lock_irqsave(&virtio_9p_device_list_lock, flags);
	uk_list_add(&d->_list, &virtio_9p_device_list);
	ukplat_spin_unlock_irqrestore(&virtio_9p_device_list_lock, flags);
out:
	return rc;
out_free:
	uk_free(a, d);
	goto out;
}

static int virtio_9p_drv_init(struct uk_alloc *drv_allocator)
{
	int rc = 0;

	if (!drv_allocator) {
		rc = -EINVAL;
		goto out;
	}

	a = drv_allocator;
out:
	return rc;
}

static const struct virtio_dev_id v9p_dev_id[] = {
	{VIRTIO_ID_9P},
	{VIRTIO_ID_INVALID} /* List Terminator */
};

static struct virtio_driver v9p_drv = {
	.dev_ids = v9p_dev_id,
	.init    = virtio_9p_drv_init,
	.add_dev = virtio_9p_add_dev
};
VIRTIO_BUS_REGISTER_DRIVER(&v9p_drv);
