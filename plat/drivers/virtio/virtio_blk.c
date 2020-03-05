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
#include <stdbool.h>
#include <virtio/virtio_bus.h>
#include <virtio/virtio_ids.h>
#include <uk/blkdev.h>
#include <virtio/virtio_blk.h>
#include <uk/sglist.h>
#include <uk/blkdev_driver.h>

#define DRIVER_NAME		"virtio-blk"
#define DEFAULT_SECTOR_SIZE	512

#define	VTBLK_INTR_EN		(1 << 0)
#define	VTBLK_INTR_EN_MASK	(1)
#define	VTBLK_INTR_USR_EN	(1 << 1)
#define	VTBLK_INTR_USR_EN_MASK	(2)

#define to_virtioblkdev(bdev) \
	__containerof(bdev, struct virtio_blk_device, blkdev)

/* Features are:
 *	Access Mode
 *	Sector_size;
 *	Multi-queue,
 *	Maximum size of a segment for requests,
 *	Maximum number of segments per request,
 *	Flush
 **/
#define VIRTIO_BLK_DRV_FEATURES(features) \
	(VIRTIO_FEATURES_UPDATE(features, VIRTIO_BLK_F_RO | \
	VIRTIO_BLK_F_BLK_SIZE | VIRTIO_BLK_F_MQ | \
	VIRTIO_BLK_F_SEG_MAX | VIRTIO_BLK_F_SIZE_MAX | \
	VIRTIO_BLK_F_CONFIG_WCE | VIRTIO_BLK_F_FLUSH))

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
	/* Maximum number of segments for a request */
	__u32 max_segments;
	/* Maximum size of a segment */
	__u32 max_size_segment;
	/* If it is set then flush request is allowed */
	__u8 writeback;
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
	/* The nr. of descriptor user configured */
	uint16_t nb_desc;
	/* The flag to interrupt on the queue */
	uint8_t intr_enabled;
	/* Reference to virtio_blk_device  */
	struct virtio_blk_device *vbd;
	/* The scatter list and its associated fragments */
	struct uk_sglist sg;
	struct uk_sglist_seg *sgsegs;
};

struct virtio_blkdev_request {
	struct uk_blkreq *req;
	struct virtio_blk_outhdr virtio_blk_outhdr;
	uint8_t status;
};

static int virtio_blkdev_request_set_sglist(struct uk_blkdev_queue *queue,
		struct virtio_blkdev_request *virtio_blk_req,
		__sector sector_size,
		bool have_data)
{
	struct virtio_blk_device *vbdev;
	struct uk_blkreq *req;
	size_t data_size = 0;
	size_t segment_size;
	size_t segment_max_size;
	size_t idx;
	uintptr_t start_data;
	int rc = 0;

	UK_ASSERT(queue);
	UK_ASSERT(virtio_blk_req);

	req = virtio_blk_req->req;
	vbdev = queue->vbd;
	start_data = (uintptr_t)req->aio_buf;
	data_size = req->nb_sectors * sector_size;
	segment_max_size = vbdev->max_size_segment;

	/* Prepare the sglist */
	uk_sglist_reset(&queue->sg);
	rc = uk_sglist_append(&queue->sg, &virtio_blk_req->virtio_blk_outhdr,
			sizeof(struct virtio_blk_outhdr));
	if (unlikely(rc != 0)) {
		uk_pr_err("Failed to append to sg list %d\n", rc);
		goto out;
	}

	/* Append to sglist chunks of `segment_max_size` size
	 * Only for read / write operations
	 **/
	if (have_data)
		for (idx = 0; idx < data_size; idx += segment_max_size) {
			segment_size = data_size - idx;
			segment_size = (segment_size > segment_max_size) ?
					segment_max_size : segment_size;
			rc = uk_sglist_append(&queue->sg,
					(void *)(start_data + idx),
					segment_size);
			if (unlikely(rc != 0)) {
				uk_pr_err("Failed to append to sg list %d\n",
						rc);
				goto out;
			}
		}

	rc = uk_sglist_append(&queue->sg, &virtio_blk_req->status,
			sizeof(uint8_t));
	if (unlikely(rc != 0)) {
		uk_pr_err("Failed to append to sg list %d\n", rc);
		goto out;
	}

out:
	return rc;
}

static int virtio_blkdev_request_write(struct uk_blkdev_queue *queue,
		struct virtio_blkdev_request *virtio_blk_req,
		__u16 *read_segs, __u16 *write_segs)
{
	struct virtio_blk_device *vbdev;
	struct uk_blkdev_cap *cap;
	struct uk_blkreq *req;
	int rc = 0;

	UK_ASSERT(queue);
	UK_ASSERT(virtio_blk_req);

	vbdev = queue->vbd;
	cap = &vbdev->blkdev.capabilities;
	req = virtio_blk_req->req;
	if (req->operation == UK_BLKDEV_WRITE &&
			cap->mode == O_RDONLY)
		return -EPERM;

	if (req->aio_buf == NULL)
		return -EINVAL;

	if (req->nb_sectors == 0)
		return -EINVAL;

	if (req->start_sector + req->nb_sectors > cap->sectors)
		return -EINVAL;

	if (req->nb_sectors > cap->max_sectors_per_req)
		return -EINVAL;

	rc = virtio_blkdev_request_set_sglist(queue, virtio_blk_req,
			cap->ssize, true);
	if (rc) {
		uk_pr_err("Failed to set sglist %d\n", rc);
		goto out;
	}

	if (req->operation == UK_BLKDEV_WRITE) {
		*read_segs = queue->sg.sg_nseg - 1;
		*write_segs = 1;
		virtio_blk_req->virtio_blk_outhdr.type = VIRTIO_BLK_T_OUT;
	} else if (req->operation == UK_BLKDEV_READ) {
		*read_segs = 1;
		*write_segs = queue->sg.sg_nseg - 1;
		virtio_blk_req->virtio_blk_outhdr.type = VIRTIO_BLK_T_IN;
	}

out:
	return rc;
}

static int virtio_blkdev_request_flush(struct uk_blkdev_queue *queue,
		struct virtio_blkdev_request *virtio_blk_req,
		__u16 *read_segs, __u16 *write_segs)
{
	struct virtio_blk_device *vbdev;
	int rc = 0;

	UK_ASSERT(queue);
	UK_ASSERT(virtio_blk_req);

	vbdev = queue->vbd;
	if (!vbdev->writeback)
		return -ENOTSUP;

	if (virtio_blk_req->virtio_blk_outhdr.sector) {
		uk_pr_warn("Start sector should be 0 for flush request\n");
		virtio_blk_req->virtio_blk_outhdr.sector = 0;
	}

	rc = virtio_blkdev_request_set_sglist(queue, virtio_blk_req, 0, false);
	if (rc) {
		uk_pr_err("Failed to set sglist %d\n", rc);
		goto out;
	}

	*read_segs = 1;
	*write_segs = 1;
	virtio_blk_req->virtio_blk_outhdr.type = VIRTIO_BLK_T_FLUSH;

out:
	return rc;
}

static int virtio_blkdev_queue_enqueue(struct uk_blkdev_queue *queue,
		struct uk_blkreq *req)
{
	struct virtio_blkdev_request *virtio_blk_req;
	__u16 write_segs = 0;
	__u16 read_segs = 0;
	int rc = 0;

	UK_ASSERT(queue);
	UK_ASSERT(req);

	if (virtqueue_is_full(queue->vq)) {
		uk_pr_debug("The virtqueue is full\n");
		return -ENOSPC;
	}

	virtio_blk_req = uk_malloc(a, sizeof(*virtio_blk_req));
	if (!virtio_blk_req)
		return -ENOMEM;

	virtio_blk_req->req = req;
	virtio_blk_req->virtio_blk_outhdr.sector = req->start_sector;
	if (req->operation == UK_BLKDEV_WRITE ||
			req->operation == UK_BLKDEV_READ)
		rc = virtio_blkdev_request_write(queue, virtio_blk_req,
				&read_segs, &write_segs);
	else if (req->operation == UK_BLKDEV_FFLUSH)
		rc = virtio_blkdev_request_flush(queue, virtio_blk_req,
				&read_segs, &write_segs);
	else
		return -EINVAL;

	if (rc)
		goto out;

	rc = virtqueue_buffer_enqueue(queue->vq, virtio_blk_req, &queue->sg,
				      read_segs, write_segs);

out:
	return rc;
}

static int virtio_blkdev_submit_request(struct uk_blkdev *dev,
		struct uk_blkdev_queue *queue,
		struct uk_blkreq *req)
{
	int rc = 0;
	int status = 0x0;

	UK_ASSERT(req);
	UK_ASSERT(queue);
	UK_ASSERT(dev);

	rc = virtio_blkdev_queue_enqueue(queue, req);
	if (likely(rc >= 0)) {
		uk_pr_debug("Success and more descriptors available\n");
		status |= UK_BLKDEV_STATUS_SUCCESS;
		/**
		 * Notify the host the new buffer.
		 */
		virtqueue_host_notify(queue->vq);
		/**
		 * When there is further space available in the ring
		 * return UK_BLKDEV_STATUS_MORE.
		 */
		status |= likely(rc > 0) ? UK_BLKDEV_STATUS_MORE : 0x0;
	} else if (rc == -ENOSPC) {
		uk_pr_debug("No more descriptors available\n");
		goto err;
	} else {
		uk_pr_err("Failed to enqueue descriptors into the ring: %d\n",
			  rc);
		goto err;
	}

	return status;

err:
	return rc;
}

static int virtio_blkdev_queue_dequeue(struct uk_blkdev_queue *queue,
		struct uk_blkreq **req)
{
	int ret = 0;
	__u32 len;
	struct virtio_blkdev_request *response_req;

	UK_ASSERT(req);
	*req = NULL;

	ret = virtqueue_buffer_dequeue(queue->vq, (void **) &response_req,
			&len);
	if (ret < 0) {
		uk_pr_info("No data available in the queue\n");
		return 0;
	}

	/* We need at least one byte for the result status */
	if (unlikely(len < 1)) {
		uk_pr_err("Received invalid response size: %u\n", len);
		ret = -EINVAL;
		goto out;
	}

	*req = response_req->req;
	(*req)->result = -response_req->status;

out:
	uk_free(a, response_req);
	return ret;
}

static int virtio_blkdev_complete_reqs(struct uk_blkdev *dev,
		struct uk_blkdev_queue *queue)
{
	struct uk_blkreq *req;
	int rc = 0;

	UK_ASSERT(dev);

	/* Queue interrupts have to be off when calling receive */
	UK_ASSERT(!(queue->intr_enabled & VTBLK_INTR_EN));

moretodo:
	for (;;) {
		rc = virtio_blkdev_queue_dequeue(queue, &req);
		if (unlikely(rc < 0)) {
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
	if (queue->intr_enabled & VTBLK_INTR_USR_EN_MASK) {
		rc = virtqueue_intr_enable(queue->vq);
		if (rc == 1)
			goto moretodo;
	}

	return 0;

err_exit:
	return rc;
}

static int virtio_blkdev_recv_done(struct virtqueue *vq, void *priv)
{
	struct uk_blkdev_queue *queue = NULL;

	UK_ASSERT(vq && priv);

	queue = (struct uk_blkdev_queue *) priv;

	/* Disable the interrupt for the ring */
	virtqueue_intr_disable(vq);
	queue->intr_enabled &= ~(VTBLK_INTR_EN);

	uk_blkdev_drv_queue_event(&queue->vbd->blkdev, queue->lqueue_id);

	return 1;
}

static int virtio_blkdev_queue_intr_enable(struct uk_blkdev *dev,
		struct uk_blkdev_queue *queue)
{
	int rc = 0;

	UK_ASSERT(dev);

	/* If the interrupt is enabled */
	if (queue->intr_enabled & VTBLK_INTR_EN)
		return 0;

	/**
	 * Enable the user configuration bit. This would cause the interrupt to
	 * be enabled automatically, if the interrupt could not be enabled now
	 * due to data in the queue.
	 */
	queue->intr_enabled = VTBLK_INTR_USR_EN;
	rc = virtqueue_intr_enable(queue->vq);
	if (!rc)
		queue->intr_enabled |= VTBLK_INTR_EN;

	return rc;
}

static int virtio_blkdev_queue_intr_disable(struct uk_blkdev *dev,
		struct uk_blkdev_queue *queue)
{
	UK_ASSERT(dev);
	UK_ASSERT(queue);

	virtqueue_intr_disable(queue->vq);
	queue->intr_enabled &= ~(VTBLK_INTR_USR_EN | VTBLK_INTR_EN);

	return 0;
}

/**
 * This function setup the vring infrastructure.
 */
static int virtio_blkdev_vqueue_setup(struct uk_blkdev_queue *queue,
		uint16_t nr_desc)
{
	uint16_t max_desc;
	struct virtqueue *vq;

	UK_ASSERT(queue);
	max_desc = queue->max_nb_desc;
	if (unlikely(max_desc < nr_desc)) {
		uk_pr_err("Max desc: %"__PRIu16" Requested desc:%"__PRIu16"\n",
			  max_desc, nr_desc);
		return -ENOBUFS;
	}

	nr_desc = (nr_desc) ? nr_desc : max_desc;
	uk_pr_debug("Configuring the %d descriptors\n", nr_desc);

	/* Check if the descriptor is a power of 2 */
	if (unlikely(nr_desc & (nr_desc - 1))) {
		uk_pr_err("Expected descriptor count as a power 2\n");
		return -EINVAL;
	}

	vq = virtio_vqueue_setup(queue->vbd->vdev, queue->lqueue_id, nr_desc,
			virtio_blkdev_recv_done, a);
	if (unlikely(PTRISERR(vq))) {
		uk_pr_err("Failed to set up virtqueue %"__PRIu16"\n",
			  queue->lqueue_id);
		return PTR2ERR(vq);
	}

	queue->vq = vq;
	vq->priv = queue;

	return 0;
}

static struct uk_blkdev_queue *virtio_blkdev_queue_setup(struct uk_blkdev *dev,
		uint16_t queue_id,
		uint16_t nb_desc,
		const struct uk_blkdev_queue_conf *queue_conf)
{
	struct virtio_blk_device *vbdev;
	int rc = 0;
	struct uk_blkdev_queue *queue;

	UK_ASSERT(dev != NULL);
	UK_ASSERT(queue_conf != NULL);

	vbdev = to_virtioblkdev(dev);
	if (unlikely(queue_id >= vbdev->nb_queues)) {
		uk_pr_err("Invalid queue_id %"__PRIu16"\n", queue_id);
		rc = -EINVAL;
		goto err_exit;
	}

	queue = &vbdev->qs[queue_id];
	queue->a = queue_conf->a;

	/* Init sglist */
	queue->sgsegs = uk_malloc(queue->a,
			vbdev->max_segments * sizeof(*queue->sgsegs));
	if (unlikely(!queue->sgsegs)) {
		rc = -ENOMEM;
		goto err_exit;
	}

	uk_sglist_init(&queue->sg, vbdev->max_segments,
			queue->sgsegs);
	queue->vbd = vbdev;
	queue->nb_desc = nb_desc;
	queue->lqueue_id = queue_id;

	/* Setup the virtqueue with the descriptor */
	rc = virtio_blkdev_vqueue_setup(queue, nb_desc);
	if (rc < 0) {
		uk_pr_err("Failed to set up virtqueue %"__PRIu16": %d\n",
			  queue_id, rc);
		goto setup_err;
	}

exit:
	return queue;
setup_err:
	uk_free(queue->a, queue->sgsegs);
err_exit:
	queue = ERR2PTR(rc);
	goto exit;
}

static int virtio_blkdev_queue_release(struct uk_blkdev *dev,
		struct uk_blkdev_queue *queue)
{
	struct virtio_blk_device *vbdev;
	int rc = 0;

	UK_ASSERT(dev != NULL);
	vbdev = to_virtioblkdev(dev);

	uk_free(queue->a, queue->sgsegs);
	virtio_vqueue_release(vbdev->vdev, queue->vq, queue->a);

	return rc;
}

static int virtio_blkdev_queue_info_get(struct uk_blkdev *dev,
		uint16_t queue_id,
		struct uk_blkdev_queue_info *qinfo)
{
	struct virtio_blk_device *vbdev = NULL;
	struct uk_blkdev_queue *queue = NULL;
	int rc = 0;

	UK_ASSERT(dev);
	UK_ASSERT(qinfo);

	vbdev = to_virtioblkdev(dev);
	if (unlikely(queue_id >= vbdev->nb_queues)) {
		uk_pr_err("Invalid queue_id %"__PRIu16"\n", queue_id);
		rc = -EINVAL;
		goto exit;
	}

	queue = &vbdev->qs[queue_id];
	qinfo->nb_min = queue->max_nb_desc;
	qinfo->nb_max = queue->max_nb_desc;
	qinfo->nb_is_power_of_two = 1;

exit:
	return rc;
}

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

static int virtio_blkdev_start(struct uk_blkdev *dev)
{
	struct virtio_blk_device *d;

	UK_ASSERT(dev != NULL);

	d = to_virtioblkdev(dev);
	virtio_dev_drv_up(d->vdev);

	uk_pr_info(DRIVER_NAME": %"__PRIu16" started\n", d->uid);

	return 0;
}

/* If one queue has unconsumed responses it returns -EBUSY
 * TODO restart doesn't work
 **/
static int virtio_blkdev_stop(struct uk_blkdev *dev)
{
	struct virtio_blk_device *d;
	uint16_t q_id;
	int rc = 0;

	UK_ASSERT(dev != NULL);

	d = to_virtioblkdev(dev);
	for (q_id = 0; q_id < d->nb_queues; ++q_id) {
		if (virtqueue_hasdata(d->qs[q_id].vq)) {
			uk_pr_err("Queue:%"__PRIu16" has unconsumed responses\n",
					q_id);
			return -EBUSY;
		}
	}

	rc = virtio_dev_reset(d->vdev);
	if (rc) {
		uk_pr_info(DRIVER_NAME":%"__PRIu16" stopped", d->uid);
		goto out;
	}

	uk_pr_warn(DRIVER_NAME":%"__PRIu16" Start is not allowed!!!", d->uid);

out:
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
	__u32 max_segments;
	__u32 max_size_segment;
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

	if (virtio_has_features(host_features, VIRTIO_BLK_F_SEG_MAX)) {
		bytes_to_read = virtio_config_get(vbdev->vdev,
			__offsetof(struct virtio_blk_config, seg_max),
			&max_segments,
			sizeof(max_segments),
			1);
		if (bytes_to_read != sizeof(max_segments))  {
			uk_pr_err("Failed to get maximum nb of segments\n");
			rc = -EAGAIN;
			goto exit;
		}
	} else
		max_segments = 1;

	/* We need extra sg elements for head (header) and tail (status). */
	max_segments += 2;

	if (virtio_has_features(host_features, VIRTIO_BLK_F_SIZE_MAX)) {
		bytes_to_read = virtio_config_get(vbdev->vdev,
			__offsetof(struct virtio_blk_config, size_max),
			&max_size_segment,
			sizeof(max_size_segment),
			1);
		if (bytes_to_read != sizeof(max_size_segment))  {
			uk_pr_err("Failed to get size max from device %d\n",
					rc);
			rc = -EAGAIN;
			goto exit;
		}
	} else
		max_size_segment = __PAGE_SIZE;

	cap->ssize = ssize;
	cap->sectors = sectors;
	cap->ioalign = sizeof(void *);
	cap->mode = (virtio_has_features(
			host_features, VIRTIO_BLK_F_RO)) ? O_RDONLY : O_RDWR;
	cap->max_sectors_per_req =
			max_size_segment / ssize * (max_segments - 2);

	vbdev->max_vqueue_pairs = num_queues;
	vbdev->max_segments = max_segments;
	vbdev->max_size_segment = max_size_segment;
	vbdev->writeback = virtio_has_features(host_features,
				VIRTIO_BLK_F_FLUSH);

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
		.queue_get_info = virtio_blkdev_queue_info_get,
		.queue_setup = virtio_blkdev_queue_setup,
		.queue_intr_enable = virtio_blkdev_queue_intr_enable,
		.dev_start = virtio_blkdev_start,
		.dev_stop = virtio_blkdev_stop,
		.queue_intr_disable = virtio_blkdev_queue_intr_disable,
		.queue_release = virtio_blkdev_queue_release,
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
	vbdev->blkdev.finish_reqs = virtio_blkdev_complete_reqs;
	vbdev->blkdev.submit_one = virtio_blkdev_submit_request;
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
