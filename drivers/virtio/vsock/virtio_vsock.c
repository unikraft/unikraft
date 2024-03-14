/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */
#include <uk/essentials.h>
#include <uk/assert.h>
#include <uk/errptr.h>
#include <linux/vm_sockets.h>
#include <uk/vsockdev.h>
#include <uk/sglist.h>
#include <uk/bitops.h>
#include <uk/netbuf.h>
#include <uk/print.h>
#include <virtio/virtio_config.h>
#include <virtio/virtqueue.h>
#include <virtio/virtio_bus.h>
#include <virtio/virtio_ids.h>

#include <virtio/virtio_vsock.h>

#include <string.h>
#include <sys/socket.h>
#include <uk/sched.h>

#define VSOCK_MAX_FRAGMENTS    ((__U16_MAX >> __PAGE_SHIFT) + 2)

/*
 * Upper bound on the payload we place into a single TX packet. This bounds
 * the size of any allocation and memcpy driven by the (peer-controlled)
 * credit window. Without it a malicious peer could advertise a huge
 * buf_alloc (up to 4 GiB) and force us into enormous per-send
 * allocations/copies. Capping here also removes any reliance on
 * integer-promotion subtleties in the size arithmetic below.
 */
#define VSOCK_MAX_PKT_SIZE	(1U << 16)

struct virtio_vsockdev;

static __u32 virtio_vsockdev_get_cid(struct uk_vsockdev *vd);
static int virtio_vsock_create(struct uk_vsockdev *vd,
			       int sock_type, int protocol,
			       struct uk_vsock **out);
static int virtio_vsock_accept(struct uk_vsockdev *vd __unused,
			       struct uk_vsock *sock,
			       struct uk_vsock_accept_entry *entry,
			       struct uk_vsock **out);
static void virtio_vsock_free_accept_entry(struct uk_vsockdev *vd __unused,
					   struct uk_vsock *sock __maybe_unused,
					   struct uk_vsock_accept_entry *entry);
static __ssz virtio_vsock_send(struct uk_vsockdev *vd __unused,
			       struct uk_vsock *sock, const char *buf,
			       __sz size);
static int virtio_vsock_connect(struct uk_vsockdev *vd __unused,
				struct uk_vsock *sock);

static int virtio_vsock_shutdown(struct uk_vsockdev *vd __unused,
				 struct uk_vsock *sock, int rx, int tx);
static int virtio_vsock_reset(struct uk_vsockdev *vd __unused,
			      struct uk_vsock *sock);
static int virtio_vsock_destroy(struct uk_vsockdev *vd __unused,
				struct uk_vsock *sock);

struct virtio_vsock_accept_entry {
	struct uk_vsock_accept_entry entry;
	__u32 peer_buf_alloc;
	__u32 peer_fwd_count;
};

#define to_virtio_vsock_accept_entry(_entry) \
	__containerof(_entry, struct virtio_vsock_accept_entry, entry)

struct virtio_vsock {
	struct uk_vsock vsock;
	struct virtio_vsockdev *vvd;
	int type;
	__u32 peer_buf_alloc;
	__u32 peer_fwd_count;
	__u32 tx_count;
};

#define to_virtio_vsock(_vsock) \
	__containerof(_vsock, struct virtio_vsock, vsock)

#define VIRTIO_VSOCKDEV_STATUS_SUCCESS	UK_BIT(0)
#define VIRTIO_VSOCKDEV_STATUS_MORE	UK_BIT(1)
#define VIRTIO_VSOCKDEV_STATUS_UNDERRUN	UK_BIT(2)

#define VTVSOCK_INTR_EN			UK_BIT(0)
#define VTVSOCK_INTR_USR_EN		UK_BIT(1)

struct virtio_vsockdev_queue {
	/* The virtqueue reference */
	struct virtqueue *vq;
	/* The virtqueue hw identifier */
	__u16 hwvq_id;
	/* The libukvsock queue identifier */
	__u16 lqueue_id;
	/* The nr. of descriptor limit */
	__u16 max_nb_desc;
	/* The nr. of descriptor user configured */
	__u16 nb_desc;
	/* The flag to interrupt on the transmit queue */
	__u8 intr_enabled;
	/* Reference to the uk_vsockdev */
	struct uk_vsockdev *vd;
	/* The scatter list and its associated fragements */
	struct uk_sglist sg;
	struct uk_sglist_seg sgsegs[VSOCK_MAX_FRAGMENTS];
};

enum virtio_vsockdev_queue_id {
	VIRTIO_VSOCKDEV_QUEUE_RX	= 0,
	VIRTIO_VSOCKDEV_QUEUE_TX	= 1,
	VIRTIO_VSOCKDEV_QUEUE_EVENT	= 2,

	VIRTIO_VSOCKDEV_QUEUE_COUNT	= 3,
};

struct virtio_vsockdev {
	struct uk_vsockdev vd;
	struct virtio_dev *vdev;
	__bool seqpacket_supported;

	__u32 cid;
	struct virtio_vsockdev_queue *queues;
};

#define to_virtio_vsockdev(_vd) \
	__containerof(_vd, struct virtio_vsockdev, vd)

static const struct uk_vsockdev_ops virtio_vsockdev_ops = {
	.get_cid = virtio_vsockdev_get_cid,
	.create = virtio_vsock_create,
	.accept = virtio_vsock_accept,
	.free_accept_entry = virtio_vsock_free_accept_entry,
	.send = virtio_vsock_send,
	.connect = virtio_vsock_connect,
	.shutdown = virtio_vsock_shutdown,
	.reset = virtio_vsock_reset,
	.destroy = virtio_vsock_destroy,
};

static struct uk_alloc *drv_alloc;

/*
 * Return the effective TX window (in bytes) for this connection, i.e. the
 * minimum of:
 * - peer_buf_alloc: the peer's advertised RX buffer (buf_alloc), and
 * - our own local buffer size (uk_vsock::buf_size), the value we ourselves
 *   advertise to the peer and which is clamped to max_buf_size.
 */
static inline __u32 virtio_vsock_tx_buf_size(struct virtio_vsock *vv)
{
	__u32 peer_buf, local_capped;
	__sz local_buf;

	peer_buf = vv->peer_buf_alloc;
	local_buf = vv->vsock.buf_size;

	if (local_buf > __U32_MAX)
		local_capped = __U32_MAX;
	else
		local_capped = (__u32)local_buf;

	return (peer_buf < local_capped) ? peer_buf : local_capped;
}

/*
 * Return the number of bytes the peer can still receive on this connection,
 * i.e. our current TX credit window: the effective TX window (see
 * virtio_vsock_tx_buf_size()) minus what is already in flight
 * (tx_count - peer_fwd_count).
 */
static inline __u32 virtio_vsock_peer_rx_free(struct virtio_vsock *vv)
{
	__u32 tx_window;
	__s64 bytes;

	tx_window = virtio_vsock_tx_buf_size(vv);
	bytes = (__s64)tx_window - (vv->tx_count - vv->peer_fwd_count);
	if (bytes < 0)
		bytes = 0;
	else if (bytes > __U32_MAX)
		bytes = __U32_MAX;

	return (__u32)bytes;
}

static int virtio_vsockdev_rxq_enqueue(struct virtio_vsockdev *vvd,
				       struct uk_netbuf *pkt)
{
	struct virtio_vsockdev_queue *queue;
	struct uk_sglist *sg;
	int rc;

	UK_ASSERT(vvd);
	UK_ASSERT(pkt);

	uk_pr_debug("vvd=%p pkt=%p pkt->len=%u\n", vvd, pkt, pkt->len);

	queue = &vvd->queues[VIRTIO_VSOCKDEV_QUEUE_RX];

	if (unlikely(virtqueue_is_full(queue->vq))) {
		uk_pr_debug("RX virtqueue is full on vvd=%p\n", vvd);
		return -ENOBUFS;
	}

	sg = &queue->sg;
	uk_sglist_reset(sg);

	/* Appending the packet to the sglist
	 * In contrast to virtio-net we can put the full packet into one
	 * descriptor without negotiating ANY_LAYOUT or VERSION_1
	 */
	rc = uk_sglist_append(sg, pkt->data, pkt->len);
	if (unlikely(rc < 0)) {
		uk_pr_debug("sglist append failed: %d\n", rc);
		return rc;
	}

	uk_pr_debug("enqueuing pkt=%p (%u bytes) to RX vq, nseg=%u\n",
		    pkt, pkt->len, sg->sg_nseg);

	return virtqueue_buffer_enqueue(queue->vq, pkt, sg, 0, sg->sg_nseg);
}

static int virtio_vsockdev_rxq_fillup(struct virtio_vsockdev *vvd,
				      __u16 nb_desc, int notify)
{
	struct uk_netbuf *pkt;
	struct uk_alloc *a;
	int status = 0;
	__u16 i;
	int rc;

	uk_pr_debug("vvd=%p nb_desc=%u notify=%d\n", vvd, nb_desc, notify);

	a = drv_alloc;

	for (i = 0; i < nb_desc; i++) {
		/* TODO: - proper allocator,
		 *       - get buffer size from somewhere
		 *         (also consider that payload_size = buflen - header)
		 */
		pkt = uk_netbuf_alloc_buf(a, __PAGE_SIZE, 8, 0, 0, __NULL);
		if (unlikely(!pkt)) {
			uk_pr_warn("Couldn't alloc RX packet\n");
			status |= VIRTIO_VSOCKDEV_STATUS_UNDERRUN;
			break;
		}

		pkt->len = pkt->buflen;

		uk_pr_debug("allocated RX pkt=%p buflen=%zu, enqueuing [%u/%u]\n",
			    pkt, pkt->buflen, i, nb_desc);

		rc = virtio_vsockdev_rxq_enqueue(vvd, pkt);
		if (unlikely(rc < 0)) {
			uk_pr_debug("Failed to add a buffer to RX vq of %p at nb_desc %d: %d\n",
				    vvd, i, rc);
			uk_netbuf_free(pkt);
			break;
		}
	}

	/**
	 * Notify the host, when we submit new descriptor(s).
	 */
	if (notify && i) {
		uk_pr_debug("notifying host RX vq on vvd=%p (%u descs added)\n",
			    vvd, i);
		virtqueue_host_notify(vvd->queues[VIRTIO_VSOCKDEV_QUEUE_RX].vq);
	}

	uk_pr_debug("fillup done: added %u descs, status=0x%x\n", i, status);

	return status;
}

static int virtio_vsockdev_rxq_dequeue(struct virtio_vsockdev *vvd,
				       struct uk_netbuf **pkt)
{
	struct uk_netbuf *buf = __NULL;
	struct virtio_vsockdev_queue *queue;
	__u32 len;
	int ret;

	UK_ASSERT(pkt);

	uk_pr_debug("vvd=%p\n", vvd);

	queue = &vvd->queues[VIRTIO_VSOCKDEV_QUEUE_RX];

	ret = virtqueue_buffer_dequeue(queue->vq, (void **)&buf, &len);
	if (ret < 0) {
		uk_pr_debug("RX vq empty on vvd=%p\n", vvd);
		*pkt = __NULL;
		return queue->nb_desc;
	}

	if (unlikely(len > buf->len)) {
		uk_pr_err("VMM sent bigger packet (%u) than RX buffer (%u); dropping packet\n",
			  len, buf->len);
		uk_netbuf_free(buf);
		*pkt = __NULL;
		return -ENOSPC;
	}

	uk_pr_debug("dequeued pkt=%p len=%u (vq ret=%d)\n", buf, len, ret);

	buf->len = len;
	*pkt = buf;
	return ret;
}

/* Raw packet receive function */
static int virtio_vsockdev_recv(struct virtio_vsockdev *vvd,
				struct uk_netbuf **pkt)
{
	struct virtio_vsockdev_queue *queue;
	int status = 0;
	int rc;

	UK_ASSERT(vvd);
	UK_ASSERT(pkt);

	uk_pr_debug("vvd=%p\n", vvd);

	queue = &vvd->queues[VIRTIO_VSOCKDEV_QUEUE_RX];

	rc = virtio_vsockdev_rxq_dequeue(vvd, pkt);
	if (unlikely(rc < 0)) {
		uk_pr_err("Failed to dequeue the packet: %d\n", rc);
		goto err_exit;
	}
	status |= (*pkt) ? VIRTIO_VSOCKDEV_STATUS_SUCCESS : 0x0;

	uk_pr_debug("dequeue result: pkt=%p status=0x%x remaining=%d\n",
		    *pkt, status, rc);

	status |= virtio_vsockdev_rxq_fillup(vvd, (queue->nb_desc - rc), 1);

	/* If the interrupts were not enabled previously just return */
	if (!(queue->intr_enabled & VTVSOCK_INTR_USR_EN)) {
		/*
		 * For polling case, we report always there are further
		 * packets unless the queue is empty.
		 */
		status |= (*pkt) ? VIRTIO_VSOCKDEV_STATUS_MORE : 0;
		uk_pr_debug("polling mode: status=0x%x\n", status);
		return status;
	}

	/* Try to enable interrupts and return if we either got a packet or we
	 * were able to activate the interrupts.
	 */
	rc = virtqueue_intr_enable(queue->vq);
	uk_pr_debug("intr_enable returned %d, pkt=%p\n", rc, *pkt);
	if (rc == 0 || *pkt) {
		status |= (rc == 1) ? VIRTIO_VSOCKDEV_STATUS_MORE : 0;
		return status;
	}

	/* We received a packet when trying to enable the interrupts and we did
	 * not have one so far => fetch the new packet.
	 */
	uk_pr_debug("packet arrived while enabling interrupts, fetching\n");
	rc = virtio_vsockdev_rxq_dequeue(vvd, pkt);
	if (unlikely(rc < 0)) {
		uk_pr_err("Failed to dequeue the packet: %d\n", rc);
		goto err_exit;
	}
	status |= VIRTIO_VSOCKDEV_STATUS_SUCCESS;
	status |= virtio_vsockdev_rxq_fillup(vvd, (queue->nb_desc - rc), 1);

	rc = virtqueue_intr_enable(queue->vq);
	status |= (rc == 1) ? VIRTIO_VSOCKDEV_STATUS_MORE : 0;

	uk_pr_debug("recv done: pkt=%p status=0x%x\n", *pkt, status);

	return status;

err_exit:
	return rc;
}

__isr static int virtio_vsockdev_rx_intr_enable(struct virtio_vsockdev *vvd)
{
	struct virtio_vsockdev_queue *queue;
	int rc;

	queue = &vvd->queues[VIRTIO_VSOCKDEV_QUEUE_RX];

	if (queue->intr_enabled & VTVSOCK_INTR_EN)
		return 0;

	/*
	 * Enable the user configuration bit. This would cause the interrupt to
	 * be enabled automatically, if the interrupt could not be enabled now
	 * due to data in the queue.
	 */
	queue->intr_enabled = VTVSOCK_INTR_USR_EN;
	rc = virtqueue_intr_enable(queue->vq);
	if (!rc)
		queue->intr_enabled |= VTVSOCK_INTR_EN;

	return rc;
}

__isr static void virtio_vsockdev_rx_intr_disable(struct virtio_vsockdev *vvd)
{
	struct virtio_vsockdev_queue *queue;

	queue = &vvd->queues[VIRTIO_VSOCKDEV_QUEUE_RX];

	virtqueue_intr_disable(queue->vq);
	queue->intr_enabled &= ~(VTVSOCK_INTR_USR_EN | VTVSOCK_INTR_EN);
}

__isr static int virtio_vsockdev_recv_event(struct virtqueue *vq, void *priv)
{
	struct virtio_vsockdev_queue *queue;

	UK_ASSERT(vq && priv);
	queue = priv;

	/* Disable the interrupts for the virtqueue */
	virtqueue_intr_disable(vq);
	queue->intr_enabled &= ~(VTVSOCK_INTR_EN);

	uk_vsockdev_notify_rxq(queue->vd);
	return 1;
}

static int virtio_vsockdev_evq_enqueue(struct virtio_vsockdev *vvd,
				       struct virtio_vsock_event *ev)
{
	struct virtio_vsockdev_queue *queue;
	struct uk_sglist *sg;
	int rc;

	UK_ASSERT(vvd);
	UK_ASSERT(ev);

	uk_pr_debug("vvd=%p ev=%p\n", vvd, ev);

	queue = &vvd->queues[VIRTIO_VSOCKDEV_QUEUE_EVENT];

	if (unlikely(virtqueue_is_full(queue->vq))) {
		uk_pr_err("The virtqueue is full\n");
		return -ENOSPC;
	}

	sg = &queue->sg;
	uk_sglist_reset(sg);

	/* Appending the event to the sglist */
	rc = uk_sglist_append(sg, ev, sizeof(struct virtio_vsock_event));
	if (unlikely(rc)) {
		uk_pr_err("Failed to append to evq: %d\n", rc);
		return rc;
	}

	uk_pr_debug("enqueuing event ev=%p to evq, nseg=%u\n",
		    ev, sg->sg_nseg);

	return virtqueue_buffer_enqueue(queue->vq, ev, sg, 0, sg->sg_nseg);
}

static int virtio_vsockdev_evq_fillup(struct virtio_vsockdev *vvd,
				      __u16 nb_desc, int notify)
{
	const enum virtio_vsockdev_queue_id qidx = VIRTIO_VSOCKDEV_QUEUE_EVENT;
	struct virtio_vsock_event *ev;
	int status = 0;
	__u16 i;
	int rc;

	uk_pr_debug("vvd=%p nb_desc=%u notify=%d\n", vvd, nb_desc, notify);

	for (i = 0; i < nb_desc; i++) {
		ev = uk_calloc(drv_alloc, 1, sizeof(*ev));
		if (unlikely(!ev)) {
			uk_pr_info("Could not allocate event buffer\n");
			status |= VIRTIO_VSOCKDEV_STATUS_UNDERRUN;
			break;
		}

		uk_pr_debug("allocated ev=%p, enqueuing [%u/%u]\n",
			    ev, i, nb_desc);

		rc = virtio_vsockdev_evq_enqueue(vvd, ev);
		if (unlikely(rc < 0)) {
			uk_pr_err("Failed to add a buffer to receive virtqueue of %p: %d\n",
				  vvd, rc);
			uk_free(drv_alloc, ev);
			break;
		}
	}

	/*
	 * Notify the host, when we submit new descriptor(s).
	 */
	if (notify && i) {
		uk_pr_debug("notifying host event vq on vvd=%p (%u descs added)\n",
			    vvd, i);
		virtqueue_host_notify(vvd->queues[qidx].vq);
	}

	uk_pr_debug("evq fillup done: added %u descs, status=0x%x\n",
		    i, status);

	return status;
}

static int virtio_vsockdev_evq_dequeue(struct virtio_vsockdev *vvd,
				       struct virtio_vsock_event **event)
{
	struct virtio_vsock_event *buf = __NULL;
	struct virtio_vsockdev_queue *queue;
	__u32 len;
	int ret;

	UK_ASSERT(event);

	uk_pr_debug("vvd=%p\n", vvd);

	queue = &vvd->queues[VIRTIO_VSOCKDEV_QUEUE_EVENT];

	ret = virtqueue_buffer_dequeue(queue->vq, (void **)&buf, &len);
	if (ret < 0) {
		uk_pr_debug("event vq empty on vvd=%p\n", vvd);
		*event = __NULL;
		return queue->nb_desc;
	}

	if (unlikely(len != sizeof(*buf))) {
		uk_pr_err("VMM sent unexpected vsock event structure\n");
		*event = __NULL;
		uk_free(drv_alloc, buf);
		return -ENOSPC;
	}

	uk_pr_debug("dequeued event=%p id=%u (vq ret=%d)\n",
		    buf, buf->id, ret);

	*event = buf;
	return ret;
}

static int virtio_vsockdev_evq_recv(struct virtio_vsockdev *vvd,
				    struct virtio_vsock_event **event)
{
	struct virtio_vsockdev_queue *queue;
	int status = 0;
	int rc;

	UK_ASSERT(vvd);
	UK_ASSERT(event);

	uk_pr_debug("vvd=%p\n", vvd);

	queue = &vvd->queues[VIRTIO_VSOCKDEV_QUEUE_EVENT];

	rc = virtio_vsockdev_evq_dequeue(vvd, event);
	if (unlikely(rc < 0)) {
		uk_pr_err("failed to dequeue the event: %d\n", rc);
		goto err_exit;
	}
	status |= (*event) ? VIRTIO_VSOCKDEV_STATUS_SUCCESS : 0x0;

	uk_pr_debug("dequeue result: event=%p status=0x%x remaining=%d\n",
		    *event, status, rc);

	status |= virtio_vsockdev_evq_fillup(vvd, (queue->nb_desc - rc), 1);

	/* If the interrupts were not enabled previously just return */
	if (!(queue->intr_enabled & VTVSOCK_INTR_USR_EN)) {
		/*
		 * For polling case, we report always there are further
		 * events unless the queue is empty.
		 */
		status |= (*event) ? VIRTIO_VSOCKDEV_STATUS_MORE : 0;
		uk_pr_debug("polling mode: status=0x%x\n", status);
		return status;
	}

	/* Try to enable interrupts and return if we either got an event or we
	 * were able to activate the interrupts.
	 */
	rc = virtqueue_intr_enable(queue->vq);
	uk_pr_debug("intr_enable returned %d, event=%p\n", rc, *event);
	if (rc == 0 || *event) {
		status |= (rc == 1) ? VIRTIO_VSOCKDEV_STATUS_MORE : 0;
		return status;
	}

	/* We received an event when trying to enable the interrupts and we did
	 * not have one so far => fetch the new event.
	 */
	uk_pr_debug("event arrived while enabling interrupts, fetching\n");
	rc = virtio_vsockdev_evq_dequeue(vvd, event);
	if (unlikely(rc < 0)) {
		uk_pr_err("failed to dequeue the event: %d\n", rc);
		goto err_exit;
	}
	status |= VIRTIO_VSOCKDEV_STATUS_SUCCESS;
	status |= virtio_vsockdev_evq_fillup(vvd, (queue->nb_desc - rc), 1);

	rc = virtqueue_intr_enable(queue->vq);
	status |= (rc == 1) ? VIRTIO_VSOCKDEV_STATUS_MORE : 0;

	uk_pr_debug("evq recv done: event=%p status=0x%x\n", *event, status);

	return status;

err_exit:
	return rc;
}

__isr static int virtio_vsockdev_evq_event(struct virtqueue *vq, void *priv)
{
	struct virtio_vsockdev_queue *queue;

	UK_ASSERT(vq && priv);
	queue = priv;

	/* Disable the interrupts for the virtqueue */
	virtqueue_intr_disable(vq);
	queue->intr_enabled &= ~(VTVSOCK_INTR_EN);

	uk_vsockdev_notify_evq(queue->vd);
	return 1;
}

static int virtio_vsockdev_evq_intr_enable(struct virtio_vsockdev *vvd)
{
	struct virtio_vsockdev_queue *queue;
	int rc;

	uk_pr_debug("vvd=%p\n", vvd);

	queue = &vvd->queues[VIRTIO_VSOCKDEV_QUEUE_EVENT];

	if (queue->intr_enabled & VTVSOCK_INTR_EN) {
		uk_pr_debug("event intr already enabled on vvd=%p\n", vvd);
		return 0;
	}

	/*
	 * Enable the user configuration bit. This would cause the interrupt to
	 * be enabled automatically, if the interrupt could not be enabled now
	 * due to data in the queue.
	 */
	queue->intr_enabled = VTVSOCK_INTR_USR_EN;
	rc = virtqueue_intr_enable(queue->vq);
	if (!rc) {
		queue->intr_enabled |= VTVSOCK_INTR_EN;
		uk_pr_debug("event intr enabled on vvd=%p\n", vvd);
	} else {
		uk_pr_debug("event intr enable deferred (queue non-empty) on vvd=%p rc=%d\n",
			    vvd, rc);
	}

	return rc;
}

static void virtio_vsockdev_evq_intr_disable(struct virtio_vsockdev *vvd)
{
	struct virtio_vsockdev_queue *queue;

	uk_pr_debug("vvd=%p\n", vvd);

	queue = &vvd->queues[VIRTIO_VSOCKDEV_QUEUE_EVENT];

	virtqueue_intr_disable(queue->vq);
	queue->intr_enabled &= ~(VTVSOCK_INTR_USR_EN | VTVSOCK_INTR_EN);

	uk_pr_debug("event intr disabled on vvd=%p\n", vvd);
}

static void virtio_vsockdev_xmit_free(struct virtio_vsockdev *vvd)
{
	struct virtio_vsockdev_queue *queue;
	struct uk_netbuf *pkt = __NULL;
	int cnt = 0;
	int rc;

	uk_pr_debug("vvd=%p: reclaiming completed TX descriptors\n", vvd);

	queue = &vvd->queues[VIRTIO_VSOCKDEV_QUEUE_TX];

	while (1) {
		rc = virtqueue_buffer_dequeue(queue->vq, (void **)&pkt, __NULL);
		if (rc < 0) {
			UK_ASSERT(rc == -ENOMSG);
			break;
		}

		UK_ASSERT(pkt);
		uk_pr_debug("freeing completed TX pkt=%p\n", pkt);
		uk_netbuf_free(pkt);
		cnt++;
	}
	uk_pr_debug("Freed %d TX packets\n", cnt);
}

/* Raw packet transmit function */
static int virtio_vsockdev_xmit(struct virtio_vsockdev *vvd,
				struct uk_netbuf *pkt)
{
	struct virtio_vsockdev_queue *queue;
	struct uk_sglist *sg;
	int rc;

	uk_pr_debug("vvd=%p pkt=%p pkt->len=%u\n", vvd, pkt, pkt->len);

	/**
	 * We are reclaiming the free descriptors from buffers. The function is
	 * not protected by means of locks. We need to be careful if there are
	 * multiple context through which we free the tx descriptors.
	 */
	virtio_vsockdev_xmit_free(vvd);

	queue = &vvd->queues[VIRTIO_VSOCKDEV_QUEUE_TX];
	sg = &queue->sg;
	uk_sglist_reset(sg);

	rc = uk_sglist_append(sg, pkt->data, pkt->len);
	if (unlikely(rc)) {
		uk_pr_err("Failed to append to the TX sglist: %d", rc);
		return rc;
	}

	uk_pr_debug("enqueuing TX pkt=%p len=%u nseg=%u\n",
		    pkt, pkt->len, sg->sg_nseg);

	rc = virtqueue_buffer_enqueue(queue->vq, pkt, sg, sg->sg_nseg, 0);
	if (likely(rc >= 0)) {
		uk_pr_debug("TX enqueue ok, notifying host\n");
		virtqueue_host_notify(queue->vq);
	} else if (rc == -ENOSPC) {
		uk_pr_debug("TX ring full, returning EAGAIN\n");
		return -EAGAIN;
	} else if (rc < 0) {
		uk_pr_debug("TX enqueue failed: %d\n", rc);
		return rc;
	}
	return 0;
}

/* Raw packet transmit with retry for full ring */
static int virtio_vsockdev_xmit_retry(struct virtio_vsockdev *vvd,
				      struct uk_netbuf *pkt)
{
	const unsigned int max_retries = 1024;
	unsigned int retries = 0;
	int rc;

	uk_pr_debug("vvd=%p pkt=%p\n", vvd, pkt);

	rc = virtio_vsockdev_xmit(vvd, pkt);
	while (rc == -EAGAIN && retries++ < max_retries) {
		uk_pr_debug("TX ring full, yielding and retrying pkt=%p\n",
			    pkt);
		uk_sched_yield();
		rc = virtio_vsockdev_xmit(vvd, pkt);
	}

	if (unlikely(rc == -EAGAIN))
		uk_pr_err("TX ring stayed full for %u retries, giving up on pkt=%p\n",
			  retries, pkt);

	uk_pr_debug("xmit_retry done: pkt=%p rc=%d\n", pkt, rc);

	return rc;
}

static void virtio_vsockdev_rst_no_sock(struct virtio_vsockdev *vvd,
					const struct virtio_vsock_hdr *req)
{
	struct virtio_vsock_hdr hdr;
	struct uk_netbuf *pkt;
	int rc;

	pkt = uk_netbuf_alloc_buf(drv_alloc, sizeof(hdr), 8, 0, 0, NULL);
	if (unlikely(!pkt)) {
		uk_pr_err("Unable to allocate memory for no-socket RST\n");
		return;
	}

	pkt->len = sizeof(hdr);
	hdr = *req;
	hdr.op = VIRTIO_VSOCK_OP_RST;
	hdr.src_cid = vvd->cid;
	hdr.src_port = req->dst_port;
	hdr.dst_cid = req->src_cid;
	hdr.dst_port = req->src_port;
	hdr.len	= 0;
	hdr.flags = 0;
	hdr.buf_alloc = 0;
	hdr.fwd_cnt = 0;

	memcpy(pkt->data, &hdr, sizeof(hdr));

	rc = virtio_vsockdev_xmit_retry(vvd, pkt);
	if (unlikely(rc)) {
		uk_pr_err("Error sending no-socket RST: %d\n", rc);
		uk_netbuf_free(pkt);
	}
}

static int virtio_vsock_work_credit_request(struct virtio_vsockdev *vvd,
					    struct virtio_vsock *vv)
{
	struct virtio_vsock_hdr *hdr;
	struct uk_netbuf *pkt;
	int rc;

	uk_pr_debug("vvd=%p vv=%p local={cid=%u port=%u} peer={cid=%u port=%u}\n",
		    vvd, vv,
		    vv->vsock.local_addr.cid, vv->vsock.local_addr.port,
		    vv->vsock.peer_addr.cid, vv->vsock.peer_addr.port);

	pkt = uk_netbuf_alloc_buf(drv_alloc, sizeof(*hdr), 8, 0,
				  0, __NULL);
	if (unlikely(!pkt)) {
		uk_pr_err("Unable to allocate memory for credit update\n");
		return -ENOMEM;
	}

	pkt->len = sizeof(*hdr);
	hdr = pkt->data;

	hdr->op = VIRTIO_VSOCK_OP_CREDIT_UPDATE;
	hdr->src_cid = vvd->cid;
	hdr->src_port = vv->vsock.local_addr.port;
	hdr->dst_cid = vv->vsock.peer_addr.cid;
	hdr->dst_port = vv->vsock.peer_addr.port;

	hdr->fwd_cnt = uk_vsock_total_processed(&vv->vsock.rx);
	hdr->buf_alloc = uk_vsock_buffer_capacity(&vv->vsock.rx);

	hdr->len = 0;
	hdr->flags = 0;
	hdr->type = vv->type;

	uk_pr_debug("sending CREDIT_UPDATE fwd_cnt=%u buf_alloc=%u\n",
		    hdr->fwd_cnt, hdr->buf_alloc);

	rc = virtio_vsockdev_xmit_retry(vvd, pkt);
	if (unlikely(rc)) {
		uk_pr_err("Unable to send credit update request: %d\n", rc);
		uk_netbuf_free(pkt);
		return rc;
	}

	uk_pr_debug("credit update sent successfully\n");

	return 0;
}

static void virtio_vsockdev_reject_conn(struct virtio_vsock *vv,
					struct virtio_vsock_accept_entry *acc)
{
	struct virtio_vsock_hdr *hdr;
	struct uk_netbuf *pkt;
	int rc;

	uk_pr_debug("vv=%p rejecting conn from peer cid=%u port=%u\n",
		    vv, acc->entry.peer.cid, acc->entry.peer.port);

	pkt = uk_netbuf_alloc_buf(drv_alloc, sizeof(*hdr), 8, 0, 0, __NULL);
	if (unlikely(!pkt)) {
		uk_pr_err("Unable to allocate memory for connect rejection: %d\n",
			  -ENOMEM);
		return;
	}

	pkt->len = sizeof(*hdr);
	hdr = pkt->data;
	hdr->op = VIRTIO_VSOCK_OP_RST;
	hdr->src_cid = vv->vsock.local_addr.cid;
	hdr->src_port = vv->vsock.local_addr.port;
	hdr->dst_cid = acc->entry.peer.cid;
	hdr->dst_port = acc->entry.peer.port;

	hdr->buf_alloc = 0;
	hdr->fwd_cnt = 0;
	hdr->type = vv->type;
	hdr->len = 0;
	hdr->flags = 0;

	uk_pr_debug("sending RST to reject conn from cid=%u port=%u\n",
		    acc->entry.peer.cid, acc->entry.peer.port);

	rc = virtio_vsockdev_xmit_retry(vv->vvd, pkt);
	if (unlikely(rc)) {
		uk_pr_err("Error sending connect rejection: %d\n", rc);
		uk_netbuf_free(pkt);
	} else {
		uk_pr_debug("rejection RST sent successfully\n");
	}
}

static int virtio_vsockdev_work_conn_request(struct virtio_vsock *vv,
					     struct virtio_vsock_hdr *hdr)
{
	struct virtio_vsock_accept_entry *accept_entry;
	int rc;

	uk_pr_debug("vv=%p conn request from cid=%llu port=%u buf_alloc=%u fwd_cnt=%u\n",
		    vv, (unsigned long long)hdr->src_cid, hdr->src_port,
		    hdr->buf_alloc, hdr->fwd_cnt);

	accept_entry = uk_calloc(drv_alloc, 1, sizeof(*accept_entry));
	if (unlikely(!accept_entry)) {
		uk_pr_err("Unable to allocate memory for the accept entry\n");
		return -ENOMEM;
	}

	accept_entry->entry.peer.cid = hdr->src_cid;
	accept_entry->entry.peer.port = hdr->src_port;
	accept_entry->peer_fwd_count = hdr->fwd_cnt;
	accept_entry->peer_buf_alloc = hdr->buf_alloc;

	uk_pr_debug("queuing accept entry=%p for vv=%p\n", accept_entry, vv);

	rc = uk_vsock_conn_request(&vv->vsock, &accept_entry->entry);
	if (unlikely(rc)) {
		uk_pr_debug("conn request rejected (rc=%d), sending RST\n", rc);
		virtio_vsockdev_reject_conn(vv, accept_entry);
		uk_free(drv_alloc, accept_entry);
		/* Normalize to 0 — not a fatal RX error */
		return (rc == -ECONNREFUSED) ? 0 : rc;
	}

	uk_pr_debug("conn request accepted, entry=%p enqueued\n",
		    accept_entry);

	return 0;
}

static int virtio_vsockdev_work_rx(struct virtio_vsockdev *vvd)
{
	struct virtio_vsock_hdr *hdr;
	struct virtio_vsock *vv;
	struct uk_vsock *sock;
	struct uk_netbuf *pkt;
	int socket_type;
	int status;
	int rc = 0;

	uk_pr_debug("vvd=%p processing RX\n", vvd);

	status = virtio_vsockdev_recv(vvd, &pkt);
	if (unlikely(status < 0)) {
		uk_pr_debug("recv failed: %d\n", status);
		return status;
	}
	if (!(status & VIRTIO_VSOCKDEV_STATUS_SUCCESS)) {
		uk_pr_debug("no packet available (status=0x%x)\n", status);
		return 0;
	}
	UK_ASSERT(pkt);

	hdr = pkt->data;
	rc = uk_netbuf_header(pkt, -(__u16)sizeof(struct virtio_vsock_hdr));
	if (unlikely(rc != 1)) {
		uk_pr_err("Not enough room to prepend virtio vsock header\n");
		/* Release this packet and try to RX something again. */
		rc = 1;
		goto out_free;
	}

	/* We do not use netbuf chains for receive buffers, there the whole
	 * payload has to be in the netbuf
	 */
	if (unlikely(hdr->len > pkt->len)) {
		uk_pr_err("VMM indicated longer packet than actually arrived\n");
		goto out_free;
	}

	/* Filter out packets for other CIDs. The handler functions can then
	 * assume hdr->dst_cid == vd->cid.
	 */
	if (unlikely(hdr->dst_cid != vvd->cid)) {
		uk_pr_info("Ignoring packet for foreign VM (cid=%llu, port=%u)\n",
			   (unsigned long long)hdr->dst_cid, hdr->dst_port);
		goto out_free;
	}

	switch (hdr->type) {
	case VIRTIO_VSOCK_TYPE_STREAM:
		socket_type = SOCK_STREAM;
		break;
	case VIRTIO_VSOCK_TYPE_SEQPACKET:
		socket_type = SOCK_SEQPACKET;
		break;
	default:
		uk_pr_err("Packet with invalid type %d\n", hdr->type);
		rc = -EPROTONOSUPPORT;
		goto out_free;
	}

	uk_pr_debug("packet op=%u scid=%llu scport=%u dscid=%llu dsport=%u\n",
		    hdr->op, (unsigned long long)hdr->src_cid, hdr->src_port,
		    (unsigned long long)hdr->dst_cid, hdr->dst_port);

	sock = uk_vsock_lookup(socket_type,
			       (struct uk_vsockaddr){
					.cid = hdr->dst_cid,
					.port = hdr->dst_port,
				},
				(struct uk_vsockaddr){
					.cid = hdr->src_cid,
					.port = hdr->src_port,
				});
	if (unlikely(!sock)) {
		uk_pr_info("Got packet to port %u without socket\n",
			   hdr->dst_port);
		/**
		 * Only send RST for REQUEST packets with no listening socket,
		 * as required by the spec. For all other ops, just drop.
		 * If we don't check for this, then we have a high risk of
		 * ending up in an RST ping-pong with the host, e.g.:
		 * 1. Host sends some control packet (RST/SHUTDOWN/etc) to a
		 * port without a socket.
		 * 2. Guest sees !sock and sends OP_RST back via
		 * virtio_vsockdev_rst_no_sock().
		 * 3. Host sees that RST as unexpected for its own state and
		 * sends yet another packet back.
		 * 4. Repeat with infinite packets to that port...
		 */
		if (hdr->op == VIRTIO_VSOCK_OP_REQUEST)
			virtio_vsockdev_rst_no_sock(vvd, hdr);
		goto out_free;
	}
	vv = to_virtio_vsock(sock);

	uk_pr_debug("dispatching to sock=%p vv=%p state=%d\n",
		    sock, vv, sock->state);

	/* Update the buffer information for all packets (except connection
	 * requests which are directed at listening sockets which don't have a
	 * useful buffer concept)
	 */
	if (hdr->op != VIRTIO_VSOCK_OP_REQUEST &&
	    sock->state != UK_VSOCK_STATE_LISTENING) {
		uk_pr_debug("updating peer credit: buf_alloc=%u fwd_cnt=%u\n",
			    hdr->buf_alloc, hdr->fwd_cnt);
		vv->peer_buf_alloc = hdr->buf_alloc;
		vv->peer_fwd_count = hdr->fwd_cnt;
		uk_vsock_notify_writable(sock,
					 virtio_vsock_peer_rx_free(vv) > 0);
	}

	/* In the listening state the only reasonable packet op is a connection
	 * request
	 */
	if (sock->state == UK_VSOCK_STATE_LISTENING) {
		if (hdr->op == VIRTIO_VSOCK_OP_REQUEST) {
			uk_pr_debug("dispatching connection request to listening sock=%p\n",
				    sock);
			rc = virtio_vsockdev_work_conn_request(vv, hdr);
		} else {
			uk_pr_debug("got unexpected op %u to listening socket\n",
				    hdr->op);
		}
		goto out_free;
	}

	switch ((enum virtio_vsock_op)hdr->op) {
	case VIRTIO_VSOCK_OP_REQUEST:
		uk_pr_warn("got unexpected connection request to non-listening socket\n");
		/**
		 * Spec requires an RST reply if no listening socket exists
		 * or we cannot accept the connection.  Here we have a socket,
		 * but it is not in LISTEN state, so treat as a rejected
		 * connection.
		 */
		rc = virtio_vsock_reset(&vvd->vd, &vv->vsock);
		break;
	case VIRTIO_VSOCK_OP_RESPONSE:
		uk_pr_debug("RESPONSE received for sock=%p\n", sock);
		rc = uk_vsock_conn_response(&vv->vsock);
		break;
	case VIRTIO_VSOCK_OP_RST:
		uk_pr_debug("RST received for sock=%p\n", sock);
		rc = uk_vsock_conn_reset(&vv->vsock);
		break;
	case VIRTIO_VSOCK_OP_SHUTDOWN:
		uk_pr_debug("SHUTDOWN received for sock=%p flags=0x%x\n",
			    sock, hdr->flags);
		rc = uk_vsock_conn_shutdown(&vv->vsock,
			hdr->flags & VIRTIO_VSOCK_SHUTDOWN_RCV,
			hdr->flags & VIRTIO_VSOCK_SHUTDOWN_SEND);
		break;
	case VIRTIO_VSOCK_OP_RW:
		uk_pr_debug("RW payload received for sock=%p len=%u\n",
			    sock, hdr->len);
		rc = uk_vsock_rx_payload(&vv->vsock, &pkt);
		break;
	case VIRTIO_VSOCK_OP_CREDIT_REQUEST:
		uk_pr_debug("CREDIT_REQUEST received for sock=%p\n", sock);
		rc = virtio_vsock_work_credit_request(vvd, vv);
		break;
	case VIRTIO_VSOCK_OP_CREDIT_UPDATE:
		/* We already processed the buf_alloc and fwd_cnt fields, so
		 * nothing to do here
		 */
		uk_pr_debug("CREDIT_UPDATE received for sock=%p (already processed)\n",
			    sock);
		break;
	default:
		/* FIXME: The virtio spec requires sending a RST answer */
		uk_pr_err("Got invalid virtio operation: %x\n", hdr->op);
		break;
	}

out_free:
	if (pkt)
		uk_netbuf_free(pkt);
	if (unlikely(rc < 0))
		return rc;
	return 1;
}

static int virtio_vsockdev_work_evq(struct virtio_vsockdev *vvd)
{
	struct virtio_vsock_event *ev;
	int rc = 0, status;

	uk_pr_debug("vvd=%p processing event queue\n", vvd);

	status = virtio_vsockdev_evq_recv(vvd, &ev);
	if (unlikely(status < 0)) {
		uk_pr_debug("evq recv failed: %d\n", status);
		return status;
	}
	if (!(status & VIRTIO_VSOCKDEV_STATUS_SUCCESS)) {
		uk_pr_debug("no event available (status=0x%x)\n", status);
		return 0;
	}
	UK_ASSERT(ev);

	uk_pr_debug("received event id=%u on vvd=%p\n", ev->id, vvd);

	switch (ev->id) {
	case VIRTIO_VSOCK_EVENT_TRANSPORT_RESET:
		uk_pr_debug("handling TRANSPORT_RESET event\n");
		rc = uk_vsockdev_transport_reset(&vvd->vd);
		if (unlikely(rc))
			uk_pr_err("Transport reset failed: %d\n", rc);
		break;
	default:
		uk_pr_err("Got unknown virtio event: %x\n", ev->id);
		break;
	}

	uk_free(drv_alloc, ev);
	if (unlikely(rc < 0))
		return rc;
	return 1;
}

static void virtio_vsockdev_rx_work(struct uk_vsockdev *dev,
				    void *argp __unused)
{
	struct virtio_vsockdev *vvd = to_virtio_vsockdev(dev);
	const int work_batch_size = 128;
	int queue_non_empty;
	int done, rc;

	uk_pr_debug("vvd=%p starting RX work\n", vvd);

	virtio_vsockdev_rx_intr_disable(vvd);

	do {
		done = 0;
		while (done < work_batch_size) {
			rc = virtio_vsockdev_work_rx(vvd);
			if (unlikely(rc < 0)) {
				if (unlikely(rc != -ENOMEM)) {
					uk_pr_err("Error processing RX queue: %d\n",
						  rc);
					break;
				}

				uk_pr_debug("Out of memory to process RX desc, yielding and retrying...\n");
				uk_sched_yield();

				rc = virtio_vsockdev_work_rx(vvd);
				if (unlikely(rc < 0)) {
					uk_pr_err("Dropped RX desc...\n");
					continue;
				}
			} else if (rc == 0) {
				uk_pr_debug("RX queue empty after %d packets\n",
					    done);
				break;
			}

			done++;
		}

		uk_pr_debug("RX work batch done: processed %d packets\n", done);

		queue_non_empty = virtio_vsockdev_rx_intr_enable(vvd);

		uk_pr_debug("queue_non_empty=%d after re-enabling RX intr\n",
			    queue_non_empty);
	} while (queue_non_empty);

	uk_pr_debug("vvd=%p RX work complete\n", vvd);
}

static void virtio_vsockdev_ev_work(struct uk_vsockdev *dev,
				    void *argp __unused)
{
	struct virtio_vsockdev *vvd = to_virtio_vsockdev(dev);
	const int work_batch_size = 128;
	int queue_non_empty;
	int done, rc;

	uk_pr_debug("vvd=%p starting event work\n", vvd);

	virtio_vsockdev_evq_intr_disable(vvd);

	do {
		done = 0;
		while (done < work_batch_size) {
			rc = virtio_vsockdev_work_evq(vvd);
			if (unlikely(rc < 0)) {
				if (unlikely(rc != -ENOMEM)) {
					uk_pr_err("Error processing event queue: %d\n",
						  rc);
					break;
				}

				uk_pr_debug("Out of memory to process event desc, yielding and retrying...\n");
				uk_sched_yield();

				rc = virtio_vsockdev_work_evq(vvd);
				if (unlikely(rc < 0)) {
					uk_pr_err("Dropped event desc...\n");
					continue;
				}
			} else if (rc == 0) {
				uk_pr_debug("event queue empty after %d events\n",
					    done);
				break;
			}

			done++;
		}

		uk_pr_debug("event work batch done: processed %d events\n",
			    done);

		queue_non_empty = virtio_vsockdev_evq_intr_enable(vvd);

		uk_pr_debug("queue_non_empty=%d after re-enabling event intr\n",
			    queue_non_empty);
	} while (queue_non_empty);

	uk_pr_debug("vvd=%p event work complete\n", vvd);
}

static int virtio_vsock_create(struct uk_vsockdev *vd, int sock_type,
			       int protocol, struct uk_vsock **out)
{
	struct virtio_vsock *vv;
	int rc, type;

	uk_pr_debug("vd=%p sock_type=%d protocol=%d\n",
		    vd, sock_type, protocol);

	if (unlikely(protocol != 0)) {
		uk_pr_debug("unsupported protocol=%d\n", protocol);
		return -EPROTONOSUPPORT;
	}

	switch (sock_type) {
	case SOCK_STREAM:
		type = VIRTIO_VSOCK_TYPE_STREAM;
		break;
	case SOCK_SEQPACKET:
		type = VIRTIO_VSOCK_TYPE_SEQPACKET;
		if (!to_virtio_vsockdev(vd)->seqpacket_supported) {
			uk_pr_debug("SEQPACKET requested but VIRTIO_VSOCK_F_SEQPACKET was not negotiated\n");
			return -EPROTONOSUPPORT;
		}
		break;
	default:
		uk_pr_debug("unsupported sock_type=%d\n", sock_type);
		return -ESOCKTNOSUPPORT;
	}

	uk_pr_debug("allocating virtio_vsock for type=%d\n", type);

	vv = uk_malloc(drv_alloc, sizeof(*vv));
	if (unlikely(!vv)) {
		uk_pr_debug("allocation of virtio_vsock failed\n");
		return -ENOMEM;
	}

	rc = uk_vsock_init(&vv->vsock, drv_alloc, sock_type, __NULL);
	if (unlikely(rc)) {
		uk_pr_err("Failed to init vsock %d\n", rc);
		uk_free(drv_alloc, vv);
		return rc;
	}

	vv->type = type;
	vv->vvd = to_virtio_vsockdev(vd);
	vv->peer_buf_alloc = 0;
	vv->peer_fwd_count = 0;
	vv->tx_count = 0;

	uk_pr_debug("created virtio_vsock=%p type=%d\n", vv, type);

	*out = &vv->vsock;
	return 0;
}

static int virtio_vsock_conn_respond(struct virtio_vsock *vv)
{
	struct virtio_vsock_hdr *hdr;
	struct uk_netbuf *pkt;
	int rc;

	uk_pr_debug("vv=%p local={cid=%u port=%u} peer={cid=%u port=%u}\n",
		    vv,
		    vv->vsock.local_addr.cid, vv->vsock.local_addr.port,
		    vv->vsock.peer_addr.cid, vv->vsock.peer_addr.port);

	pkt = uk_netbuf_alloc_buf(drv_alloc, sizeof(*hdr), 8, 0, 0, __NULL);
	if (unlikely(!pkt)) {
		uk_pr_err("Unable to allocate memory for connect response\n");
		return -ENOMEM;
	}

	pkt->len = sizeof(*hdr);
	hdr = pkt->data;
	hdr->op = VIRTIO_VSOCK_OP_RESPONSE;
	hdr->src_cid = vv->vvd->cid;
	hdr->src_port = vv->vsock.local_addr.port;
	hdr->dst_cid = vv->vsock.peer_addr.cid;
	hdr->dst_port = vv->vsock.peer_addr.port;

	hdr->fwd_cnt = uk_vsock_total_processed(&vv->vsock.rx);
	hdr->buf_alloc = uk_vsock_buffer_capacity(&vv->vsock.rx);
	hdr->type = vv->type;
	hdr->len = 0;
	hdr->flags = 0;

	uk_pr_debug("sending RESPONSE fwd_cnt=%u buf_alloc=%u\n",
		    hdr->fwd_cnt, hdr->buf_alloc);

	rc = virtio_vsockdev_xmit_retry(vv->vvd, pkt);
	if (unlikely(rc)) {
		uk_pr_err("Error sending connect response\n");
		uk_netbuf_free(pkt);
		return rc;
	}

	uk_pr_debug("connect response sent successfully\n");

	return 0;
}

static int virtio_vsock_accept(struct uk_vsockdev *vd __unused,
			       struct uk_vsock *sock,
			       struct uk_vsock_accept_entry *entry,
			       struct uk_vsock **out)
{
	struct virtio_vsock_accept_entry *ventry;
	struct virtio_vsock *vv, *vvl;
	int rc;

	uk_pr_debug("sock=%p entry=%p\n", sock, entry);

	vvl = to_virtio_vsock(sock);
	ventry = to_virtio_vsock_accept_entry(entry);

	uk_pr_debug("accepting conn from peer cid=%u port=%u buf_alloc=%u fwd_cnt=%u\n",
		    ventry->entry.peer.cid, ventry->entry.peer.port,
		    ventry->peer_buf_alloc, ventry->peer_fwd_count);

	vv = uk_malloc(drv_alloc, sizeof(*vv));
	if (unlikely(!vv)) {
		uk_pr_debug("allocation failed, rejecting connection\n");
		/* Reject the connection if we do not have the resources to
		 * accept the connection
		 */
		virtio_vsockdev_reject_conn(vvl, ventry);
		return -ENOMEM;
	}

	rc = uk_vsock_init(&vv->vsock, drv_alloc, vvl->vsock.sock_type,
			   &vvl->vsock);
	if (unlikely(rc)) {
		uk_pr_err("Failed to init vsock %d\n", rc);
		/* Reject the connection if we do not have the resources to
		 * accept the connection
		 */
		virtio_vsockdev_reject_conn(vvl, ventry);
		uk_free(drv_alloc, vv);
		return rc;
	}

	vv->type = vvl->type;
	vv->peer_buf_alloc = ventry->peer_buf_alloc;
	vv->peer_fwd_count = ventry->peer_fwd_count;
	vv->vsock.peer_addr = ventry->entry.peer;

	/* always use the concrete local CID */
	vv->vsock.local_addr.cid = vvl->vvd->cid;
	vv->vsock.local_addr.port = vvl->vsock.local_addr.port;
	vv->vvd = vvl->vvd;
	vv->tx_count = 0;

	uk_pr_debug("new vv=%p local={cid=%u port=%u} peer={cid=%u port=%u}\n",
		    vv,
		    vv->vsock.local_addr.cid, vv->vsock.local_addr.port,
		    vv->vsock.peer_addr.cid, vv->vsock.peer_addr.port);

	/* Send the connection response to the peer */
	rc = virtio_vsock_conn_respond(vv);
	if (unlikely(rc)) {
		uk_pr_err("Failed to respond to accepted connection: %d\n", rc);
		uk_vsock_buffer_destroy(&vv->vsock.rx);
		uk_free(drv_alloc, vv);
		return rc;
	}

	uk_pr_debug("accept complete, new vv=%p\n", vv);

	*out = &vv->vsock;
	return 0;
}

static void virtio_vsock_free_accept_entry(struct uk_vsockdev *vd __unused,
					   struct uk_vsock *sock __maybe_unused,
					   struct uk_vsock_accept_entry *entry)
{
	struct virtio_vsock_accept_entry *ventry;

	uk_pr_debug("freeing accept entry sock=%p entry=%p\n", sock, entry);

	ventry = to_virtio_vsock_accept_entry(entry);
	uk_free(drv_alloc, ventry);
}

static __ssz virtio_vsock_send(struct uk_vsockdev *vd __unused,
			       struct uk_vsock *sock, const char *buf,
			       __sz size)
{
	struct virtio_vsock_hdr *hdr;
	struct virtio_vsock *vv;
	struct uk_netbuf *pkt;
	__u32 pkt_len;
	int rc;

	uk_pr_debug("sock=%p size=%zu\n", sock, size);

	if (size == 0) {
		uk_pr_debug("send called with size=0, returning immediately\n");
		return 0;
	}

	vv = to_virtio_vsock(sock);

	/* Bound the payload by both the peer's credit window and a fixed
	 * maximum packet size. The latter caps peer-controlled allocations
	 * (peer_buf_alloc can be up to 4 GiB) and keeps pkt_len well within
	 * __u32, so the size arithmetic below cannot overflow.
	 */
	/* FIXME: Check whether this also need some other limit? (ring size?) */
	/* TODO: Possibly optimization: do not create super small packets if
	 *       limited by the buffer size.
	 */
	pkt_len = MIN3(virtio_vsock_peer_rx_free(vv), size,
		       (__sz)VSOCK_MAX_PKT_SIZE);

	uk_pr_debug("peer_rx_free=%u requested=%zu effective pkt_len=%u\n",
		    virtio_vsock_peer_rx_free(vv), size, pkt_len);

	if (pkt_len == 0) {
		uk_pr_debug("peer RX buffer full, returning EAGAIN\n");
		return -EAGAIN;
	}

	pkt = uk_netbuf_alloc_buf(drv_alloc, pkt_len + sizeof(*hdr), 8,
				  sizeof(*hdr), 0, __NULL);
	if (unlikely(!pkt)) {
		uk_pr_debug("netbuf allocation failed for pkt_len=%u\n",
			    pkt_len);
		return -ENOMEM;
	}

	/* Copy over the data */
	memcpy(pkt->data, buf, pkt_len);
	pkt->len = pkt_len;

	rc = uk_netbuf_header(pkt, sizeof(*hdr));
	UK_ASSERT(rc == 1);

	hdr = pkt->data;
	hdr->op = VIRTIO_VSOCK_OP_RW;
	hdr->src_cid = sock->local_addr.cid;
	hdr->src_port = sock->local_addr.port;
	hdr->dst_cid = sock->peer_addr.cid;
	hdr->dst_port = sock->peer_addr.port;

	hdr->fwd_cnt = uk_vsock_total_processed(&vv->vsock.rx);
	hdr->buf_alloc = uk_vsock_buffer_capacity(&vv->vsock.rx);
	hdr->type = vv->type;
	hdr->flags = 0;
	hdr->len = pkt_len;

	uk_pr_debug("sending RW pkt=%p pkt_len=%u fwd_cnt=%u buf_alloc=%u\n",
		    pkt, pkt_len, hdr->fwd_cnt, hdr->buf_alloc);

	rc = virtio_vsockdev_xmit(vv->vvd, pkt);
	if (unlikely(rc)) {
		uk_pr_debug("xmit failed: %d, freeing pkt=%p\n", rc, pkt);
		uk_netbuf_free(pkt);
		return rc;
	}

	vv->tx_count += pkt_len;

	uk_pr_debug("sent %u bytes, tx_count now %u\n",
		    pkt_len, vv->tx_count);

	return pkt_len;
}

static int virtio_vsock_connect(struct uk_vsockdev *vd __unused,
				struct uk_vsock *sock)
{
	struct virtio_vsock_hdr *hdr;
	struct virtio_vsock *vv;
	struct uk_netbuf *pkt;
	int rc;

	uk_pr_debug("sock=%p local={cid=%u port=%u} peer={cid=%u port=%u}\n",
		    sock,
		    sock->local_addr.cid, sock->local_addr.port,
		    sock->peer_addr.cid, sock->peer_addr.port);

	vv = to_virtio_vsock(sock);

	pkt = uk_netbuf_alloc_buf(drv_alloc, sizeof(*hdr), 8, 0, 0, __NULL);
	if (unlikely(!pkt)) {
		uk_pr_debug("netbuf allocation failed\n");
		return -ENOMEM;
	}

	pkt->len = sizeof(*hdr);
	hdr = pkt->data;
	hdr->op = VIRTIO_VSOCK_OP_REQUEST;
	hdr->src_cid = sock->local_addr.cid;
	hdr->src_port = sock->local_addr.port;
	hdr->dst_cid = sock->peer_addr.cid;
	hdr->dst_port = sock->peer_addr.port;

	hdr->buf_alloc = uk_vsock_buffer_capacity(&vv->vsock.rx);
	hdr->fwd_cnt = uk_vsock_total_processed(&vv->vsock.rx);
	hdr->type = vv->type;
	hdr->len = 0;
	hdr->flags = 0;

	uk_pr_debug("sending REQUEST buf_alloc=%u fwd_cnt=%u\n",
		    hdr->buf_alloc, hdr->fwd_cnt);

	rc = virtio_vsockdev_xmit_retry(vv->vvd, pkt);
	if (unlikely(rc)) {
		uk_pr_debug("xmit failed: %d, freeing pkt=%p\n", rc, pkt);
		uk_netbuf_free(pkt);
		return rc;
	}

	uk_pr_debug("connect REQUEST sent, returning EINPROGRESS\n");

	return -EINPROGRESS;
}

static __u32 virtio_vsockdev_get_cid(struct uk_vsockdev *vd)
{
	struct virtio_vsockdev *vvd;

	vvd = to_virtio_vsockdev(vd);

	uk_pr_debug("vd=%p vvd=%p returning cid=%u\n", vd, vvd, vvd->cid);

	return vvd->cid;
}

static int virtio_vsock_shutdown(struct uk_vsockdev *vd __unused,
				 struct uk_vsock *sock, int rx, int tx)
{
	struct virtio_vsock_hdr *hdr;
	struct virtio_vsock *vv;
	struct uk_netbuf *pkt;
	int rc;

	uk_pr_debug("sock=%p rx=%d tx=%d local={cid=%u port=%u} peer={cid=%u port=%u}\n",
		    sock, rx, tx,
		    sock->local_addr.cid, sock->local_addr.port,
		    sock->peer_addr.cid, sock->peer_addr.port);

	vv = to_virtio_vsock(sock);

	pkt = uk_netbuf_alloc_buf(drv_alloc, sizeof(*hdr), 8, 0, 0, __NULL);
	if (unlikely(!pkt)) {
		uk_pr_debug("netbuf allocation failed\n");
		return -ENOMEM;
	}

	pkt->len = sizeof(*hdr);
	hdr = pkt->data;
	hdr->op = VIRTIO_VSOCK_OP_SHUTDOWN;
	hdr->src_cid = sock->local_addr.cid;
	hdr->src_port = sock->local_addr.port;
	hdr->dst_cid = sock->peer_addr.cid;
	hdr->dst_port = sock->peer_addr.port;

	hdr->buf_alloc = uk_vsock_buffer_capacity(&vv->vsock.rx);
	hdr->fwd_cnt = uk_vsock_total_processed(&vv->vsock.rx);
	hdr->type = vv->type;
	hdr->len = 0;
	hdr->flags = (tx ? VIRTIO_VSOCK_SHUTDOWN_SEND : 0) |
		     (rx ? VIRTIO_VSOCK_SHUTDOWN_RCV : 0);

	uk_pr_debug("sending SHUTDOWN flags=0x%x buf_alloc=%u fwd_cnt=%u\n",
		    hdr->flags, hdr->buf_alloc, hdr->fwd_cnt);

	rc = virtio_vsockdev_xmit_retry(vv->vvd, pkt);
	if (unlikely(rc)) {
		uk_pr_debug("xmit failed: %d, freeing pkt=%p\n", rc, pkt);
		uk_netbuf_free(pkt);
		return rc;
	}

	uk_pr_debug("SHUTDOWN sent successfully\n");

	return 0;
}

static int virtio_vsock_reset(struct uk_vsockdev *vd __unused,
			      struct uk_vsock *sock)
{
	struct virtio_vsock_hdr *hdr;
	struct virtio_vsock *vv;
	struct uk_netbuf *pkt;
	int rc;

	uk_pr_debug("sock=%p local={cid=%u port=%u} peer={cid=%u port=%u}\n",
		    sock,
		    sock->local_addr.cid, sock->local_addr.port,
		    sock->peer_addr.cid, sock->peer_addr.port);

	vv = to_virtio_vsock(sock);

	pkt = uk_netbuf_alloc_buf(drv_alloc, sizeof(*hdr), 8, 0, 0, __NULL);
	if (unlikely(!pkt)) {
		uk_pr_debug("netbuf allocation failed\n");
		return -ENOMEM;
	}

	pkt->len = sizeof(*hdr);
	hdr = pkt->data;
	hdr->op = VIRTIO_VSOCK_OP_RST;
	hdr->src_cid = sock->local_addr.cid;
	hdr->src_port = sock->local_addr.port;
	hdr->dst_cid = sock->peer_addr.cid;
	hdr->dst_port = sock->peer_addr.port;

	hdr->buf_alloc = uk_vsock_buffer_capacity(&vv->vsock.rx);
	hdr->fwd_cnt = uk_vsock_total_processed(&vv->vsock.rx);
	hdr->type = vv->type;
	hdr->len = 0;
	hdr->flags = 0;

	uk_pr_debug("sending RST buf_alloc=%u fwd_cnt=%u\n",
		    hdr->buf_alloc, hdr->fwd_cnt);

	rc = virtio_vsockdev_xmit_retry(vv->vvd, pkt);
	if (unlikely(rc)) {
		uk_pr_debug("xmit failed: %d, freeing pkt=%p\n", rc, pkt);
		uk_netbuf_free(pkt);
		return rc;
	}

	uk_pr_debug("RST sent successfully\n");

	return 0;
}

static int virtio_vsock_destroy(struct uk_vsockdev *vd __unused,
				struct uk_vsock *sock)
{
	struct virtio_vsock *vv;

	vv = to_virtio_vsock(sock);

	uk_pr_debug("sock=%p vv=%p freeing\n", sock, vv);

	uk_free(drv_alloc, vv);

	uk_pr_debug("vv=%p freed\n", vv);

	return 0;
}

static int
virtio_vsockdev_setup_queue(struct virtio_vsockdev *vvd,
			    enum virtio_vsockdev_queue_id queue_id,
			    __u16 nb_desc, virtqueue_callback_t callback)
{
	struct virtio_vsockdev_queue *queue;
	struct virtqueue *vq;

	uk_pr_debug("vvd=%p queue_id=%d nb_desc=%u callback=%p\n",
		    vvd, queue_id, nb_desc, callback);

	queue = &vvd->queues[queue_id];

	if (unlikely(queue->max_nb_desc < nb_desc)) {
		uk_pr_debug("requested nb_desc=%u exceeds max=%u\n",
			    nb_desc, queue->max_nb_desc);
		return -ENOBUFS;
	}

	/* Legacy virtio has queue_size register as RO, while modern has it RW.
	 * This means the vring layout is immutable and setting up the queues
	 * with anything less than the reported vq size will result in layout
	 * mismatch between the host paravirtualized device and the
	 * guest device driver.
	 */
	if (!nb_desc ||
	    !VIRTIO_FEATURE_HAS(vvd->vdev->features, VIRTIO_F_VERSION_1))
		nb_desc = queue->max_nb_desc;

	if (unlikely(!POWER_OF_2(nb_desc))) {
		uk_pr_debug("nb_desc=%u is not a power of 2\n", nb_desc);
		return -EINVAL;
	}

	uk_pr_debug("setting up vq hwvq_id=%u nb_desc=%u\n",
		    queue->hwvq_id, nb_desc);

	vq = virtio_vqueue_setup(vvd->vdev, queue->hwvq_id,
				 nb_desc, callback, drv_alloc);
	if (unlikely(PTRISERR(vq))) {
		uk_pr_debug("vqueue setup failed: %d\n", PTR2ERR(vq));
		return PTR2ERR(vq);
	}

	queue->vd = &vvd->vd;
	queue->vq = vq;
	queue->vq->priv = queue;
	queue->nb_desc = nb_desc;
	queue->lqueue_id = queue_id;

	uk_pr_debug("queue_id=%d set up: vq=%p nb_desc=%u\n",
		    queue_id, vq, nb_desc);

	return 0;
}

static void virtio_vsockdev_release_queues(struct virtio_vsockdev *vvd)
{
	struct virtqueue *vq;
	int i;

	uk_pr_debug("vvd=%p releasing %d queues\n",
		    vvd, VIRTIO_VSOCKDEV_QUEUE_COUNT);

	for (i = 0; i < VIRTIO_VSOCKDEV_QUEUE_COUNT; i++) {
		vq = vvd->queues[i].vq;
		if (!vq) {
			uk_pr_debug("queue[%d] vq is NULL, skipping\n", i);
			continue;
		}
		/* TODO: this does not free the buffers in the RX/EV ring, but
		 *       no other driver seems to care either...
		 */
		uk_pr_debug("releasing vq=%p for queue[%d]\n", vq, i);
		virtio_vqueue_release(vvd->vdev, vq, drv_alloc);
		vvd->queues[i].vq = __NULL;
	}

	uk_pr_debug("all queues released\n");
}

static int virtio_vsockdev_setup_queues(struct virtio_vsockdev *vvd)
{
	__u16 nb_desc;
	int rc;

	uk_pr_debug("vvd=%p setting up queues\n", vvd);

	/* The error handling in this function is not ideal because the
	 * underlying virtio libraries do not allow us to properly clean up the
	 * created structures.
	 */

	/* TODO: Make nb_desc configurable somehow? (uklibparam?) */
	rc = virtio_vsockdev_setup_queue(vvd, VIRTIO_VSOCKDEV_QUEUE_RX, 0,
					 virtio_vsockdev_recv_event);
	if (unlikely(rc)) {
		uk_pr_debug("RX queue setup failed: %d\n", rc);
		return rc;
	}
	uk_pr_debug("RX queue set up\n");

	rc = virtio_vsockdev_setup_queue(vvd, VIRTIO_VSOCKDEV_QUEUE_TX, 0,
					 __NULL);
	if (unlikely(rc)) {
		uk_pr_debug("TX queue setup failed: %d\n", rc);
		return rc;
	}
	uk_pr_debug("TX queue set up\n");

	rc = virtio_vsockdev_setup_queue(vvd, VIRTIO_VSOCKDEV_QUEUE_EVENT, 0,
					 virtio_vsockdev_evq_event);
	if (unlikely(rc)) {
		uk_pr_debug("event queue setup failed: %d\n", rc);
		return rc;
	}
	uk_pr_debug("event queue set up\n");

	/* Fill up guest -> host rings with buffers */
	nb_desc = vvd->queues[VIRTIO_VSOCKDEV_QUEUE_EVENT].nb_desc;
	uk_pr_debug("filling up event queue with %u descriptors\n", nb_desc);
	rc = virtio_vsockdev_evq_fillup(vvd, nb_desc, 0);
	if (unlikely(rc < 0))
		goto err_release_queues;

	nb_desc = vvd->queues[VIRTIO_VSOCKDEV_QUEUE_RX].nb_desc;
	uk_pr_debug("filling up RX queue with %u descriptors\n", nb_desc);
	rc = virtio_vsockdev_rxq_fillup(vvd, nb_desc, 0);
	if (unlikely(rc < 0))
		goto err_release_queues;

	uk_pr_debug("configuring vsockdev event queue dispatcher\n");
	rc = uk_vsockdev_evqueue_configure(&vvd->vd,
					   &(struct uk_vsockdev_queue_conf){
					    .a = drv_alloc,
					    .callback = virtio_vsockdev_ev_work,
					    .callback_cookie = __NULL,
#if CONFIG_LIBUKVSOCKDEV_DISPATCHERTHREADS
					    .s = uk_sched_current(),
#endif /* CONFIG_LIBUKVSOCKDEV_DISPATCHERTHREADS */
					   });
	if (unlikely(rc)) {
		uk_pr_debug("evqueue configure failed: %d\n", rc);
		goto err_release_queues;
	}

	uk_pr_debug("configuring vsockdev RX queue dispatcher\n");
	rc = uk_vsockdev_rxqueue_configure(&vvd->vd,
					   &(struct uk_vsockdev_queue_conf){
					    .a = drv_alloc,
					    .callback = virtio_vsockdev_rx_work,
					    .callback_cookie = __NULL,
#if CONFIG_LIBUKVSOCKDEV_DISPATCHERTHREADS
					    .s = uk_sched_current(),
#endif /* CONFIG_LIBUKVSOCKDEV_DISPATCHERTHREADS */
					   });
	if (unlikely(rc)) {
		uk_pr_debug("rxqueue configure failed: %d\n", rc);
		uk_vsockdev_evqueue_unconfigure(&vvd->vd);
		goto err_release_queues;
	}

	uk_pr_debug("vvd=%p all queues set up successfully\n", vvd);

	return 0;

err_release_queues:
	uk_pr_debug("queue setup failed, releasing queues\n");
	virtio_vsockdev_release_queues(vvd);
	return rc;
}

static int virtio_vsockdev_alloc_queues(struct virtio_vsockdev *vvd)
{
	__u16 qdesc_size[VIRTIO_VSOCKDEV_QUEUE_COUNT];
	struct virtio_vsockdev_queue *queue;
	int vq_avail;
	int i;

	uk_pr_debug("vvd=%p allocating %d queues\n", vvd,
		    VIRTIO_VSOCKDEV_QUEUE_COUNT);

	vvd->queues = uk_calloc(drv_alloc, VIRTIO_VSOCKDEV_QUEUE_COUNT,
			       sizeof(*vvd->queues));
	if (unlikely(!vvd->queues)) {
		uk_pr_debug("queue array allocation failed\n");
		return -ENOMEM;
	}

	vq_avail = virtio_find_vqs(vvd->vdev, VIRTIO_VSOCKDEV_QUEUE_COUNT,
				   qdesc_size);
	if (unlikely(vq_avail != VIRTIO_VSOCKDEV_QUEUE_COUNT)) {
		uk_pr_err("Expected %d queues, found %d queues\n",
			  VIRTIO_VSOCKDEV_QUEUE_COUNT, vq_avail);
		uk_free(drv_alloc, vvd->queues);
		return -ENOMEM;
	}

	uk_pr_debug("found %d virtqueues\n", vq_avail);

	for (i = 0; i < VIRTIO_VSOCKDEV_QUEUE_COUNT; i++) {
		queue = &vvd->queues[i];
		queue->hwvq_id = i;
		queue->max_nb_desc = qdesc_size[queue->hwvq_id];
		uk_sglist_init(&queue->sg, ARRAY_SIZE(queue->sgsegs),
			       queue->sgsegs);
		uk_pr_debug("queue[%d]: hwvq_id=%u max_nb_desc=%u\n",
			    i, queue->hwvq_id, queue->max_nb_desc);
	}

	uk_pr_debug("vvd=%p queues allocated\n", vvd);

	return 0;
}

static void virtio_vsockdev_free_queues(struct virtio_vsockdev *vvd)
{
	uk_pr_debug("vvd=%p freeing queue array\n", vvd);
	uk_free(drv_alloc, vvd->queues);
	uk_pr_debug("queue array freed\n");
}

static int virtio_vsockdev_feature_negotiate(struct virtio_vsockdev *vvd)
{
	__u64 drv_features = 0;
	__u64 host_features;
	__u64 guest_cid;
	int rc;

	uk_pr_debug("vvd=%p negotiating features\n", vvd);

	host_features = virtio_feature_get(vvd->vdev);

	uk_pr_debug("host_features=0x%llx\n",
		    (unsigned long long)host_features);

	/* VirtIO modern */
	if (VIRTIO_FEATURE_HAS(host_features, VIRTIO_F_VERSION_1)) {
		uk_pr_debug("host supports VIRTIO_F_VERSION_1\n");
		VIRTIO_FEATURE_SET(drv_features, VIRTIO_F_VERSION_1);
	}

	/* We do not rely on having implicit STREAM support */
	if (VIRTIO_FEATURE_HAS(host_features,
			       VIRTIO_VSOCK_F_NO_IMPLIED_STREAM)) {
		uk_pr_debug("host supports VIRTIO_VSOCK_F_NO_IMPLIED_STREAM\n");
		VIRTIO_FEATURE_SET(drv_features,
				   VIRTIO_VSOCK_F_NO_IMPLIED_STREAM);
	} else {
		/* TODO: Set a separate feature flag for stream support in this
		 *       case (STREAM support is implied if the device does not
		 *       offer this feature)
		 */
		uk_pr_debug("VIRTIO_VSOCK_F_NO_IMPLIED_STREAM not available, stream support implied\n");
	}

	/* SOCK_STREAM socket support */
	if (VIRTIO_FEATURE_HAS(host_features, VIRTIO_VSOCK_F_STREAM)) {
		uk_pr_debug("host supports VIRTIO_VSOCK_F_STREAM\n");
		VIRTIO_FEATURE_SET(drv_features, VIRTIO_VSOCK_F_STREAM);
	}

	/* SOCK_SEQPACKET socket support */
	vvd->seqpacket_supported = __false;
	if (VIRTIO_FEATURE_HAS(host_features, VIRTIO_VSOCK_F_SEQPACKET)) {
		uk_pr_debug("host supports VIRTIO_VSOCK_F_SEQPACKET\n");
		VIRTIO_FEATURE_SET(drv_features, VIRTIO_VSOCK_F_SEQPACKET);
		vvd->seqpacket_supported = __true;
	}

	/*
	 * Use index based event supression when it's available.
	 * This allows a more fine-grained control when the hypervisor should
	 * notify the guest. Some hypervisors such as firecracker also do not
	 * support the original flag.
	 */
	if (VIRTIO_FEATURE_HAS(host_features, VIRTIO_F_EVENT_IDX)) {
		uk_pr_debug("host supports VIRTIO_F_EVENT_IDX\n");
		VIRTIO_FEATURE_SET(drv_features, VIRTIO_F_EVENT_IDX);
	}

	uk_pr_debug("negotiated drv_features=0x%llx, reading CID\n",
		    (unsigned long long)drv_features);

	/* Fetch CID */
	rc = virtio_config_get(vvd->vdev,
			       __offsetof(struct virtio_vsock_config,
					  guest_cid),
			       &guest_cid, sizeof(guest_cid), 1);
	if (unlikely(rc)) {
		uk_pr_err("Unable to read guest CID from virtio-vsock device: read %d bytes but expected %zu bytes\n",
			  rc, sizeof(guest_cid));
		return -EAGAIN;
	}

	/**
	 * Reject reserved CIDs and VMADDR_CID_ANY:
	 *  - VMADDR_CID_HYPERVISOR (0)
	 *  - VMADDR_CID_LOCAL      (1)
	 *  - VMADDR_CID_HOST       (2)
	 *  - VMADDR_CID_ANY        (U32_MAX)
	 */
	if (unlikely(guest_cid <= VMADDR_CID_HOST ||
		     guest_cid == VMADDR_CID_ANY)) {
		uk_pr_err("Read invalid guest CID from virtio-vsock device: %llu\n",
			  (unsigned long long)guest_cid);
		return -EAGAIN;
	}

	if (unlikely(guest_cid > __U32_MAX)) {
		uk_pr_err("Guest CID %llu exceeds 32-bit range\n",
			  (unsigned long long)guest_cid);
		return -ERANGE;
	}
	vvd->cid = (__u32)guest_cid;

	uk_pr_debug("CID assigned: %u\n", vvd->cid);

	vvd->vdev->features = drv_features;
	virtio_feature_set(vvd->vdev);

	uk_pr_debug("features committed to device\n");

	return virtio_dev_status_update(vvd->vdev,
					VIRTIO_CONFIG_STATUS_ACK |
					VIRTIO_CONFIG_STATUS_DRIVER |
					VIRTIO_CONFIG_STATUS_FEATURES_OK);
}

static int virtio_vsockdev_start(struct virtio_vsockdev *vvd)
{
	int rc;

	uk_pr_debug("vvd=%p starting device\n", vvd);

	rc = virtio_vsockdev_rx_intr_enable(vvd);
	if (unlikely(rc)) {
		uk_pr_err("Vsock RX queue contained packets before starting device: %d\n",
			  rc);
		return rc;
	}
	uk_pr_debug("RX interrupts enabled\n");

	rc = virtio_vsockdev_evq_intr_enable(vvd);
	if (unlikely(rc)) {
		uk_pr_err("Vsock event queue contained events before starting device: %d\n",
			  rc);
		return rc;
	}
	uk_pr_debug("event interrupts enabled\n");

	virtio_dev_drv_up(vvd->vdev);

	uk_pr_info("Started virtio-vsock device\n");

	return 0;
}

static int virtio_vsockdev_add_dev(struct virtio_dev *vdev)
{
	struct virtio_vsockdev *vvd;
	int rc;

	uk_pr_debug("vdev=%p adding virtio-vsock device\n", vdev);

	vvd = uk_calloc(drv_alloc, 1, sizeof(*vvd));
	if (unlikely(!vvd)) {
		uk_pr_debug("allocation of virtio_vsockdev failed\n");
		return -ENOMEM;
	}

	vvd->vdev = vdev;
	vvd->vd.ops = &virtio_vsockdev_ops;

	rc = uk_vsockdev_register(&vvd->vd);
	if (unlikely(rc)) {
		uk_pr_err("Failed to register vsock dev: %d\n", rc);
		uk_free(drv_alloc, vvd);
		return rc;
	}
	uk_pr_debug("vsockdev registered, vvd=%p\n", vvd);

	rc = virtio_vsockdev_feature_negotiate(vvd);
	if (unlikely(rc)) {
		uk_pr_debug("feature negotiation failed: %d\n", rc);
		goto err_out;
	}
	uk_pr_debug("feature negotiation complete, cid=%u\n", vvd->cid);

	rc = virtio_vsockdev_alloc_queues(vvd);
	if (unlikely(rc)) {
		uk_pr_debug("queue allocation failed: %d\n", rc);
		goto err_negotiate_feature;
	}
	uk_pr_debug("queues allocated\n");

	rc = virtio_vsockdev_setup_queues(vvd);
	if (unlikely(rc)) {
		uk_pr_debug("queue setup failed: %d\n", rc);
		goto err_free_queues;
	}
	uk_pr_debug("queues set up\n");

	rc = virtio_vsockdev_start(vvd);
	if (unlikely(rc)) {
		uk_pr_debug("device start failed: %d\n", rc);
		goto err_teardown_queues;
	}

	uk_pr_debug("virtio-vsock device vvd=%p fully initialized\n", vvd);

	return 0;

err_teardown_queues:
	uk_vsockdev_rxqueue_unconfigure(&vvd->vd);
	uk_vsockdev_evqueue_unconfigure(&vvd->vd);
	virtio_vsockdev_release_queues(vvd);
err_free_queues:
	virtio_vsockdev_free_queues(vvd);
err_negotiate_feature:
	virtio_dev_status_update(vvd->vdev, VIRTIO_CONFIG_STATUS_FAIL);
err_out:
	uk_free(drv_alloc, vvd);
	return rc;
}

static int virtio_vsockdev_drv_init(struct uk_alloc *drv_allocator)
{
	uk_pr_debug("drv_allocator=%p initializing driver\n", drv_allocator);

	/* driver initialization */
	if (unlikely(!drv_allocator)) {
		uk_pr_debug("drv_allocator is NULL\n");
		return -EINVAL;
	}

	drv_alloc = drv_allocator;

	uk_pr_debug("driver initialized, drv_alloc=%p\n", drv_alloc);

	return 0;
}

static const struct virtio_dev_id virtio_vsockdev_dev_id[] = {
	{VIRTIO_ID_VSOCK},
	{VIRTIO_ID_INVALID} /* List Terminator */
};

static struct virtio_driver vsock_drv = {
	.dev_ids = virtio_vsockdev_dev_id,
	.init = virtio_vsockdev_drv_init,
	.add_dev = virtio_vsockdev_add_dev
};

VIRTIO_BUS_REGISTER_DRIVER(&vsock_drv);
