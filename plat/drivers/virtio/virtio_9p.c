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
#include <uk/sglist.h>
#include <uk/9pdev.h>
#include <uk/9preq.h>
#include <uk/9pdev_trans.h>
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
	/* libuk9p associated device (NULL if the device is not in use). */
	struct uk_9pdev *p9dev;
	/* Scatter-gather list. */
	struct uk_sglist sg;
	struct uk_sglist_seg sgsegs[NUM_SEGMENTS];
	/* Spinlock protecting the sg list and the vq. */
	spinlock_t spinlock;
};

static int virtio_9p_connect(struct uk_9pdev *p9dev,
			     const char *device_identifier,
			     const char *mount_args __unused)
{
	struct virtio_9p_device *dev = NULL;
	int rc = 0;
	int found = 0;

	ukarch_spin_lock(&virtio_9p_device_list_lock);
	uk_list_for_each_entry(dev, &virtio_9p_device_list, _list) {
		if (!strcmp(dev->tag, device_identifier)) {
			if (dev->p9dev != NULL) {
				rc = -EBUSY;
				goto out;
			}
			found = 1;
			break;
		}
	}

	if (!found) {
		rc = -EINVAL;
		goto out;
	}

	/*
	 * The maximum message size is given by the number of segments.
	 *
	 * For read requests, the reply will be able to make use of the large
	 * msize, and the request will not exceed one segment.
	 * Similarly for write requests, but in reverse. Other requests should
	 * not exceed one page for both recv and xmit fcalls.
	 */
	p9dev->max_msize = (NUM_SEGMENTS - 1) * __PAGE_SIZE;

	dev->p9dev = p9dev;
	p9dev->priv = dev;

out:
	ukarch_spin_unlock(&virtio_9p_device_list_lock);
	return rc;
}

static int virtio_9p_disconnect(struct uk_9pdev *p9dev)
{
	struct virtio_9p_device *dev;

	UK_ASSERT(p9dev);
	dev = p9dev->priv;

	ukarch_spin_lock(&virtio_9p_device_list_lock);
	dev->p9dev = NULL;
	ukarch_spin_unlock(&virtio_9p_device_list_lock);

	return 0;
}

static int virtio_9p_request(struct uk_9pdev *p9dev,
			     struct uk_9preq *req)
{
	struct virtio_9p_device *dev;
	int rc, host_notified = 0;
	unsigned long flags;
	size_t read_segs, write_segs;
	bool failed = false;

	UK_ASSERT(p9dev);
	UK_ASSERT(req);
	UK_ASSERT(UK_READ_ONCE(req->state) == UK_9PREQ_READY);

	/*
	 * Get the request such that it won't get freed while it's
	 * used as a cookie for the virtqueue.
	 */
	uk_9preq_get(req);
	dev = p9dev->priv;
	ukplat_spin_lock_irqsave(&dev->spinlock, flags);
	uk_sglist_reset(&dev->sg);

	rc = uk_sglist_append(&dev->sg, req->xmit.buf, req->xmit.size);
	if (rc < 0) {
		failed = true;
		goto out_unlock;
	}

	if (req->xmit.zc_buf) {
		rc = uk_sglist_append(&dev->sg, req->xmit.zc_buf,
				req->xmit.zc_size);
		if (rc < 0) {
			failed = true;
			goto out_unlock;
		}
	}

	read_segs = dev->sg.sg_nseg;

	rc = uk_sglist_append(&dev->sg, req->recv.buf, req->recv.size);
	if (rc < 0) {
		failed = true;
		goto out_unlock;
	}

	if (req->recv.zc_buf) {
		uint32_t recv_size = req->recv.size + req->recv.zc_size;

		rc = uk_sglist_append(&dev->sg, req->recv.zc_buf,
				req->recv.zc_size);
		if (rc < 0) {
			failed = true;
			goto out_unlock;
		}

		/* Make eure there is sufficient space for Rerror replies. */
		if (recv_size < UK_9P_RERROR_MAXSIZE) {
			uint32_t leftover = UK_9P_RERROR_MAXSIZE - recv_size;

			rc = uk_sglist_append(&dev->sg,
					req->recv.buf + recv_size, leftover);
			if (rc < 0) {
				failed = true;
				goto out_unlock;
			}
		}
	}

	write_segs = dev->sg.sg_nseg - read_segs;

	rc = virtqueue_buffer_enqueue(dev->vq, req, &dev->sg,
				      read_segs, write_segs);
	if (likely(rc >= 0)) {
		UK_WRITE_ONCE(req->state, UK_9PREQ_SENT);
		virtqueue_host_notify(dev->vq);
		host_notified = 1;
		rc = 0;
	}

out_unlock:
	if (failed)
		uk_pr_err(DRIVER_NAME": Failed to append to the sg list.\n");
	ukplat_spin_unlock_irqrestore(&dev->spinlock, flags);
	/*
	 * Release the reference to the 9P request if it was not successfully
	 * sent.
	 */
	if (!host_notified)
		uk_9preq_put(req);
	return rc;
}

static const struct uk_9pdev_trans_ops v9p_trans_ops = {
	.connect        = virtio_9p_connect,
	.disconnect     = virtio_9p_disconnect,
	.request        = virtio_9p_request
};

static struct uk_9pdev_trans v9p_trans = {
	.name           = "virtio",
	.ops            = &v9p_trans_ops,
	.a              = NULL /* Set by the driver initialization. */
};

static int virtio_9p_recv(struct virtqueue *vq, void *priv)
{
	struct virtio_9p_device *dev;
	struct uk_9preq *req = NULL;
	uint32_t len;
	int rc = 0;
	int handled = 0;

	UK_ASSERT(vq);
	UK_ASSERT(priv);

	dev = priv;
	UK_ASSERT(vq == dev->vq);

	while (1) {
		/*
		 * Protect against data races with virtio_9p_request() calls
		 * which are trying to enqueue to the same vq.
		 */
		ukarch_spin_lock(&dev->spinlock);
		rc = virtqueue_buffer_dequeue(dev->vq, (void **)&req, &len);
		ukarch_spin_unlock(&dev->spinlock);
		if (rc < 0)
			break;

		/*
		 * Notify the 9P API that this request has been successfully
		 * received, release the reference to the request.
		 */
		uk_9preq_receive_cb(req, len);

		/*
		 * Check for Rerror messages, fixup the error message if
		 * needed.
		 */
		if (req->recv.type == UK_9P_RERROR) {
			memcpy(req->recv.buf + req->recv.zc_offset,
			       req->recv.zc_buf,
			       MIN(req->recv.zc_size, len - UK_9P_HEADER_SIZE));
		}

		uk_9preq_put(req);
		handled = 1;

		/* Break if there are no more buffers on the virtqueue. */
		if (rc == 0)
			break;
	}

	/*
	 * As the virtqueue might have empty slots now, notify any threads
	 * blocked on ENOSPC errors.
	 */
	if (handled)
		uk_9pdev_xmit_notify(dev->p9dev);

	return handled;
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

	uk_sglist_init(&d->sg, ARRAY_SIZE(d->sgsegs), &d->sgsegs[0]);

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
	int rc = 0;

	UK_ASSERT(vdev != NULL);

	d = uk_calloc(a, 1, sizeof(*d));
	if (!d) {
		rc = -ENOMEM;
		goto out;
	}
	ukarch_spin_lock_init(&d->spinlock);
	d->vdev = vdev;
	virtio_9p_feature_set(d);
	rc = virtio_9p_configure(d);
	if (rc)
		goto out_free;
	rc = virtio_9p_start(d);
	if (rc)
		goto out_free;

	ukarch_spin_lock(&virtio_9p_device_list_lock);
	uk_list_add(&d->_list, &virtio_9p_device_list);
	ukarch_spin_unlock(&virtio_9p_device_list_lock);
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
	v9p_trans.a = a;

	rc = uk_9pdev_trans_register(&v9p_trans);
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
