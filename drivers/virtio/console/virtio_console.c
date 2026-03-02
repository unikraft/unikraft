/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <string.h>
#include <uk/assert.h>
#include <uk/print.h>
#include <uk/alloc.h>
#include <uk/essentials.h>
#include <uk/console.h>
#include <uk/console/driver.h>
#include <uk/paging.h>
#include <uk/sglist.h>

#include <virtio/virtio_bus.h>
#include <virtio/virtqueue.h>
#include <virtio/virtio_console.h>

/* Virtqueue indices for port 0 */
#define VTCONS_RXQ_IDX				0
#define VTCONS_TXQ_IDX				1

/*
 * Cap on how many descriptor slots we actually use from the device maximum.
 * Each slot requires a pre-allocated VTCONS_RX_BUFSZ receive buffer, so this
 * bounds the memory committed at probe time. Without this cap we would use
 * whatever the device advertises (up to 1024), wasting megabytes of memory
 * for what is essentially a serial console.
 */
#define VTCONS_NB_DESC							\
	CONFIG_LIBVIRTIO_CONSOLE_NB_DESC

/*
 * Size of each pre-allocated RX buffer. One buffer is posted per descriptor
 * slot; the host fills it with incoming bytes and reports the used length.
 */
#define VTCONS_RX_BUFSZ				4096

/* Allocator handed to us by the virtio bus at driver init time. */
static struct uk_alloc *vtcons_a;

/* Cookie stored in the virtqueue for each posted RX descriptor. */
struct vtcons_rxbuf {
	char data[VTCONS_RX_BUFSZ];
};

struct vtcons_rxq {
	/* The virtqueue used for receiving data from the host. */
	struct virtqueue *vq;
	/* Number of descriptor slots in use. */
	__u16 nb_desc;
	/* Maximum number of descriptor slots the device supports. */
	__u16 max_nb_desc;
};

struct vtcons_txq {
	/* The virtqueue used for sending data to the host. */
	struct virtqueue *vq;
	/* Number of descriptor slots in use. */
	__u16 nb_desc;
	/* Maximum number of descriptor slots the device supports. */
	__u16 max_nb_desc;
};

struct vtcons_dev {
	/* Underlying virtio device. */
	struct virtio_dev *vdev;
	/* Receive virtqueue state. */
	struct vtcons_rxq rxq;
	/* Transmit virtqueue state. */
	struct vtcons_txq txq;
	/* ukconsole async device representation. */
	struct uk_console_async con_drv;
};

/* Obtain the enclosing vtcons_dev from any pointer to its con_drv member. */
static inline struct vtcons_dev *to_vtcons_dev(struct uk_console *cons)
{
	struct uk_console_async *acons;

	acons = __containerof(cons, struct uk_console_async, cons);
	return __containerof(acons, struct vtcons_dev, con_drv);
}

static __ssz vtcons_in(struct uk_console *con, char *buf, __sz len);
static __ssz vtcons_out(struct uk_console *con, const char *buf, __sz len);
static __ssz vtcons_emerg_out(struct uk_console *con,
			      const char *buf, __sz len);

static const struct uk_console_ops vtcons_ops = {
	.out = vtcons_out,
	.in = vtcons_in,
	.emerg_out = __NULL,
};

static const struct uk_console_ops vtcons_ops_emerg = {
	.out = vtcons_out,
	.in = vtcons_in,
	.emerg_out = vtcons_emerg_out,
};

/*
 * Post up to @nb fresh writable descriptors to the RX virtqueue.
 * Notifies the host if @notify is non-zero and at least one buffer was added.
 * Returns the number of descriptors successfully posted.
 */
static __u16 vtcons_rxq_fillup(struct vtcons_dev *dev, __u16 nb, int notify)
{
	struct uk_sglist_seg sgsegs[2];
	struct vtcons_rxbuf *rxbuf;
	struct uk_sglist sg;
	__u16 filled = 0;
	int rc;

	while (filled < nb) {
		if (virtqueue_is_full(dev->rxq.vq))
			break;

		rxbuf = uk_malloc(vtcons_a, sizeof(*rxbuf));
		if (unlikely(!rxbuf))
			break;

		uk_sglist_init(&sg, 2, sgsegs);
		rc = uk_sglist_append(&sg, rxbuf->data, VTCONS_RX_BUFSZ);
		if (unlikely(rc)) {
			uk_free(vtcons_a, rxbuf);
			break;
		}

		/* 0 readable segments (host writes into this buffer),
		 * sg.sg_nseg writable segments.
		 */
		rc = virtqueue_buffer_enqueue(dev->rxq.vq, rxbuf,
					      &sg, 0, sg.sg_nseg);
		if (unlikely(rc < 0)) {
			uk_free(vtcons_a, rxbuf);
			break;
		}
		filled++;
	}

	if (notify && filled)
		virtqueue_host_notify(dev->rxq.vq);

	return filled;
}

/* Dequeue and free all TX buffers the host has already consumed. */
static void vtcons_txq_reclaim(struct vtcons_dev *dev)
{
	char *txbuf;
	int rc;

	for (;;) {
		rc = virtqueue_buffer_dequeue(dev->txq.vq,
					      (void **)&txbuf, __NULL);
		if (rc < 0)
			break;

		UK_ASSERT(txbuf);
		uk_free(vtcons_a, txbuf);
	}
}

/*
 * Dequeue as many completed RX buffers as needed to fill @buf[@len].
 * Refills consumed slots in one batch at the end and re-enables the RX
 * interrupt so new data triggers a notification.
 *
 * Returns the number of bytes actually read (0 if the queue was empty).
 */
static __ssz vtcons_in(struct uk_console *con, char *buf, __sz len)
{
	struct vtcons_dev *dev = to_vtcons_dev(con);
	struct vtcons_rxbuf *rxbuf;
	__u16 reclaimed = 0;
	__sz total = 0;
	__u32 rxlen;
	int rc;

	while (total < len) {
		rc = virtqueue_buffer_dequeue(dev->rxq.vq,
					      (void **)&rxbuf, &rxlen);
		if (rc < 0)
			break;

		UK_ASSERT(rxbuf);

		/* The host may have filled fewer bytes than VTCONS_RX_BUFSZ;
		 * also clamp to the remaining space in the caller's buffer.
		 */
		rxlen = (__u32)MIN((__sz)rxlen, len - total);
		memcpy(buf + total, rxbuf->data, rxlen);
		total += rxlen;
		reclaimed++;

		uk_free(vtcons_a, rxbuf);
	}

	/* Refill all consumed slots in one pass and notify the host. */
	if (reclaimed)
		vtcons_rxq_fillup(dev, reclaimed, 1);

	/* Re-enable the RX interrupt now that the queue is drained.
	 * If data arrived between the last dequeue and this enable,
	 * the host will fire the interrupt again immediately.
	 */
	virtqueue_intr_enable(dev->rxq.vq);

	return (__ssz)total;
}

/*
 * Copy @buf[@len] into a freshly allocated TX buffer and enqueue it.
 *
 * We never hand the caller's buffer directly to the virtqueue because the
 * caller may reuse or free it before the host has consumed the descriptor
 * (non-blocking path).
 */
static __ssz vtcons_out(struct uk_console *con, const char *buf, __sz len)
{
	/* This is fine in practice as most VMM have sensible vq sizes */
	const __sz nsegs = UK_PAGING_PAGE_COUNT(len) + 1;
	struct vtcons_dev *dev = to_vtcons_dev(con);
	struct uk_sglist_seg sgsegs[nsegs];
	struct uk_sglist sg;
	char *txbuf;
	int rc;

	vtcons_txq_reclaim(dev);

	txbuf = uk_malloc(vtcons_a, len);
	if (unlikely(!txbuf))
		return -ENOMEM;

	memcpy(txbuf, buf, len);

	uk_sglist_init(&sg, nsegs, sgsegs);
	rc = uk_sglist_append(&sg, txbuf, len);
	if (unlikely(rc)) {
		uk_free(vtcons_a, txbuf);
		return rc;
	}

	/* sg.sg_nseg readable descriptors (guest -> host), 0 writable.
	 * txbuf is the cookie returned by virtqueue_buffer_dequeue during
	 * reclaim, so we know what to free.
	 */
	rc = virtqueue_buffer_enqueue(dev->txq.vq, txbuf,
				      &sg, sg.sg_nseg, 0);
	if (unlikely(rc < 0)) {
		uk_free(vtcons_a, txbuf);
		return rc;
	}

	virtqueue_host_notify(dev->txq.vq);

	return (__ssz)len;
}

/*
 * Emergency output via the VIRTIO_CONSOLE_F_EMERG_WRITE config register.
 * One byte is written at a time as a 32-bit config register write; this path
 * bypasses the virtqueues entirely and is safe during early boot and crash.
 */
static __ssz vtcons_emerg_out(struct uk_console *con,
			      const char *buf, __sz len)
{
	struct vtcons_dev *dev = to_vtcons_dev(con);
	__u32 val;
	__sz i;

	for (i = 0; i < len; i++) {
		/* The spec says a single byte written to this 32-bit register
		 * is transmitted as if over a serial port; upper bytes are
		 * ignored by the device.
		 */
		val = (__u32)(unsigned char)buf[i];
		virtio_config_set(dev->vdev,
				  __offsetof(struct virtio_console_config,
					     emerg_wr),
				  &val, sizeof(val));
	}

	return (__ssz)len;
}

/*
 * Virtqueue RX callback, called from interrupt context when the host has
 * placed data into one or more of our RX descriptors.
 *
 * Disables further RX interrupts (re-enabled by vtcons_in after draining)
 * and wakes all registered async-RX callbacks.
 */
static int vtcons_rxq_recv_done(struct virtqueue *vq, void *priv)
{
	struct vtcons_dev *dev = (struct vtcons_dev *)priv;

	UK_ASSERT(vq && priv);

	virtqueue_intr_disable(vq);

	uk_console_async_in_handle(&dev->con_drv);

	return 1;
}

static int vtcons_vqueue_setup(struct vtcons_dev *dev)
{
	struct virtqueue *vq;
	__u16 qdesc_size[2];
	int vq_avail;

	vq_avail = virtio_find_vqs(dev->vdev, 2, qdesc_size);
	if (unlikely(vq_avail != 2)) {
		uk_pr_err("Expected 2 virtqueues, found %d\n", vq_avail);
		return -ENODEV;
	}

	dev->rxq.max_nb_desc = qdesc_size[VTCONS_RXQ_IDX];
	dev->txq.max_nb_desc = qdesc_size[VTCONS_TXQ_IDX];

	/* Legacy virtio has queue_size register as RO, while modern has it RW.
	 * This means the vring layout is immutable and setting up the queues
	 * with anything less than the reported vq size will result in layout
	 * mismatch between the host paravirtualized device and the
	 * guest device driver.
	 */
	if (VIRTIO_FEATURE_HAS(dev->vdev->features, VIRTIO_F_VERSION_1)) {
		dev->txq.nb_desc = MIN((__u16)VTCONS_NB_DESC,
				       dev->txq.max_nb_desc);
		dev->rxq.nb_desc = MIN((__u16)VTCONS_NB_DESC,
				       dev->rxq.max_nb_desc);
	} else {
		dev->txq.nb_desc = dev->txq.max_nb_desc;
		dev->rxq.nb_desc = dev->rxq.max_nb_desc;
	}

	vq = virtio_vqueue_setup(dev->vdev, VTCONS_RXQ_IDX,
				 dev->rxq.nb_desc,
				 vtcons_rxq_recv_done, vtcons_a);
	if (unlikely(PTRISERR(vq))) {
		uk_pr_err("Failed to set up RX virtqueue: %d\n",
			  (int)PTR2ERR(vq));
		return PTR2ERR(vq);
	}
	vq->priv = dev;
	dev->rxq.vq = vq;

	vq = virtio_vqueue_setup(dev->vdev, VTCONS_TXQ_IDX,
				 dev->txq.nb_desc,
				 __NULL, vtcons_a);
	if (unlikely(PTRISERR(vq))) {
		uk_pr_err("Failed to set up TX virtqueue: %d\n",
			  (int)PTR2ERR(vq));
		virtio_vqueue_release(dev->vdev, dev->rxq.vq,
				      vtcons_a);
		return PTR2ERR(vq);
	}
	dev->txq.vq = vq;

	/* TX is purely poll/lazy — no interrupt needed. */
	virtqueue_intr_disable(dev->txq.vq);

	return 0;
}

static int vtcons_dev_negotiate(struct virtio_dev *vdev)
{
	__u64 drv_features = 0;
	__u64 dev_features;
	__u8 dev_status;
	int rc;

	dev_features = virtio_feature_get(vdev);

	if (VIRTIO_FEATURE_HAS(dev_features, VIRTIO_F_VERSION_1))
		VIRTIO_FEATURE_SET(drv_features, VIRTIO_F_VERSION_1);
	else if (VIRTIO_FEATURE_HAS(dev_features, VIRTIO_F_ANY_LAYOUT))
		VIRTIO_FEATURE_SET(drv_features, VIRTIO_F_ANY_LAYOUT);

	if (VIRTIO_FEATURE_HAS(dev_features, VIRTIO_F_EVENT_IDX))
		VIRTIO_FEATURE_SET(drv_features, VIRTIO_F_EVENT_IDX);

	/* Emergency write gives us a crash-safe single-byte output path. */
	if (VIRTIO_FEATURE_HAS(dev_features, VIRTIO_CONSOLE_F_EMERG_WRITE))
		VIRTIO_FEATURE_SET(drv_features, VIRTIO_CONSOLE_F_EMERG_WRITE);

	vdev->features = drv_features;
	virtio_feature_set(vdev);

	rc = virtio_dev_status_update(vdev,
				      (VIRTIO_CONFIG_STATUS_ACK |
				       VIRTIO_CONFIG_STATUS_DRIVER |
				       VIRTIO_CONFIG_STATUS_FEATURES_OK));
	if (unlikely(rc))
		return rc;

	dev_status = virtio_dev_status_get(vdev);
	if (unlikely(!(dev_status & VIRTIO_CONFIG_STATUS_FEATURES_OK)))
		return -EINVAL;

	return 0;
}

static int vtcons_add_dev(struct virtio_dev *vdev)
{
	struct vtcons_dev *dev;
	const struct uk_console_ops *ops;
	int rc;

	UK_ASSERT(vdev);

	rc = vtcons_dev_negotiate(vdev);
	if (unlikely(rc))
		return rc;

	dev = uk_calloc(vtcons_a, 1, sizeof(*dev));
	if (unlikely(!dev)) {
		uk_pr_err("Failed to allocate device struct\n");
		rc = virtio_dev_status_update(vdev, VIRTIO_CONFIG_STATUS_FAIL);
		if (unlikely(rc))
			uk_pr_err("Failed set virtio device failure status: %d\n",
				  rc);
		return -ENOMEM;
	}

	dev->vdev = vdev;
	vdev->priv = dev;

	rc = vtcons_vqueue_setup(dev);
	if (unlikely(rc)) {
		uk_pr_err("Failed to set up virtqueues: %d\n", rc);
		goto err_free_dev;
	}

	if (VIRTIO_FEATURE_HAS(vdev->features, VIRTIO_CONSOLE_F_EMERG_WRITE))
		ops = &vtcons_ops_emerg;
	else
		ops = &vtcons_ops;

	/* The framework auto-assigns STDOUT/STDIN/EMERG_STDOUT flags based
	 * on which ops are present, so we only declare ASYNC_RX here.
	 */
	uk_console_async_init(&dev->con_drv, "virtio-console", ops,
			      UK_CONSOLE_FLAG_ASYNC_RX, UK_CONSOLE_CLASS_HVC);

	/* Pre-fill the RX ring so the host can immediately send data.
	 * Defer the host notification until after drv_up below.
	 */
	vtcons_rxq_fillup(dev, dev->rxq.nb_desc, 0);

	/* Set device DRIVER_OK — host may start sending data from here. */
	virtio_dev_drv_up(vdev);

	/* Register with ukconsole after the device is live so that any early
	 * printk that fires during registration already works.
	 */
	uk_console_register(&dev->con_drv.cons);

	/* Flush pre-filled RX descriptors and arm the interrupt. */
	virtqueue_host_notify(dev->rxq.vq);
	virtqueue_intr_enable(dev->rxq.vq);

	uk_pr_info("Virtio console ready\n");
	return 0;

err_free_dev:
	uk_free(vtcons_a, dev);
	return rc;
}

static int vtcons_drv_init(struct uk_alloc *a)
{
	vtcons_a = a;
	return 0;
}

static const struct virtio_dev_id vtcons_drv_ids[] = {
	{ VIRTIO_ID_CONSOLE },
	{ VIRTIO_ID_INVALID },
};

static struct virtio_driver vtcons_drv = {
	.dev_ids = vtcons_drv_ids,
	.init    = vtcons_drv_init,
	.add_dev = vtcons_add_dev,
};

VIRTIO_BUS_REGISTER_DRIVER(&vtcons_drv);
