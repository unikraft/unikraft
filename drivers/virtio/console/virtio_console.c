/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <stdio.h>
#include <string.h>
#include <uk/assert.h>
#include <uk/print.h>
#include <uk/essentials.h>
#include <uk/paging.h>
#include <uk/sglist.h>

#include <virtio/virtio_bus.h>
#include <virtio/virtqueue.h>
#include <virtio/virtio_console.h>

#include "virtio_console_priv.h"

/* Virtqueue indices for port 0 */
#define VTCONS_RXQ_IDX				0
#define VTCONS_TXQ_IDX				1

/* Virtqueue indices for the multiport control queues */
#define VTCONS_CTLRXQ_IDX                      2
#define VTCONS_CTLTXQ_IDX                      3

/*
 * Cap on how many descriptor slots we actually use from the device maximum.
 * Each slot requires a pre-allocated VTCONS_RX_BUFSZ receive buffer, so this
 * bounds the memory committed at probe time. Without this cap we would use
 * whatever the device advertises (up to 1024), wasting megabytes of memory
 * for what is essentially a serial console.
 */
#define VTCONS_NB_DESC							\
	CONFIG_LIBVIRTIO_CONSOLE_NB_DESC

/* Allocator handed to us by the virtio bus at driver init time. */
static struct uk_alloc *vtcons_a;

/*
 * Monotonically increasing counter used to give each probed virtio-console
 * device a unique numeric suffix in its control-thread name.
 */
static __u32 vtcons_dev_cnt;

#if CONFIG_LIBUKFS_DEVFS
/*
 * List of all probed vtcons_dev instances.  Used by the devfs initcall
 * to create device nodes and symlinks that were pending while devfs was
 * not yet available.
 */
UK_LIST_HEAD(vtcons_dev_list);
#endif /* CONFIG_LIBUKFS_DEVFS */

/*
 * Virtqueue index helpers for per-port data queues.
 *
 * Port 0 occupies queues 0 (RX) and 1 (TX).  Control queues occupy 2 and 3.
 * Each additional port n (n >= 1) occupies queues 2n+2 (RX) and 2n+3 (TX).
 */
static inline __u16 vtcons_port_rxq_idx(__u32 port_id)
{
	return (port_id == 0) ? 0 : (__u16)(2 * port_id + 2);
}

static inline __u16 vtcons_port_txq_idx(__u32 port_id)
{
	return (port_id == 0) ? 1 : (__u16)(2 * port_id + 3);
}

/* Obtain the enclosing vtcons_port from any pointer to its con_drv member. */
static inline struct vtcons_port *to_vtcons_port(struct uk_console *cons)
{
	struct uk_console_async *acons;

	acons = __containerof(cons, struct uk_console_async, cons);
	return __containerof(acons, struct vtcons_port, con_drv);
}

/* Obtain the enclosing vtcons_dev from any pointer to its con_drv member. */
static inline struct vtcons_dev *to_vtcons_dev(struct uk_console *cons)
{
	return to_vtcons_port(cons)->dev;
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
 * Post up to @nb fresh writable descriptors to the RX virtqueue of @port.
 * Notifies the host if @notify is non-zero and at least one buffer was added.
 * Returns the number of descriptors successfully posted.
 */
static __u16 vtcons_port_rxq_fillup(struct vtcons_port *port, __u16 nb,
				    int notify)
{
	/* Two segments in case the allocation crosses the page boundary */
	struct uk_sglist_seg sgsegs[2];
	struct vtcons_rxbuf *rxbuf;
	struct uk_sglist sg;
	__u16 filled = 0;
	int rc;

	while (filled < nb) {
		if (virtqueue_is_full(port->rxq.vq))
			break;

		rxbuf = uk_malloc(vtcons_a, sizeof(*rxbuf));
		if (unlikely(!rxbuf)) {
			uk_pr_warn_isr("port %u: RX buf alloc failed (%u/%u)\n",
					port->id, filled, nb);
			break;
		}

		uk_sglist_init(&sg, 2, sgsegs);
		rc = uk_sglist_append(&sg, rxbuf->data, VTCONS_RX_BUFSZ);
		if (unlikely(rc)) {
			uk_pr_err_isr("port %u: RX sglist append failed: %d\n",
				      port->id, rc);
			uk_free(vtcons_a, rxbuf);
			break;
		}

		/* 0 readable segments (host writes into this buffer),
		 * sg.sg_nseg writable segments.
		 */
		rc = virtqueue_buffer_enqueue(port->rxq.vq, rxbuf,
					      &sg, 0, sg.sg_nseg);
		if (unlikely(rc < 0)) {
			uk_pr_err_isr("port %u: RX enqueue failed: %d\n",
				      port->id, rc);
			uk_free(vtcons_a, rxbuf);
			break;
		}
		filled++;
	}

	if (notify && filled)
		virtqueue_host_notify(port->rxq.vq);

	if (unlikely(filled < nb))
		uk_pr_debug_isr("port %u: posted %u/%u RX descriptors\n",
				port->id, filled, nb);

	return filled;
}

/*
 * Post up to @nb fresh writable descriptors to the control RX virtqueue so
 * the host can deliver struct virtio_console_control messages to us.
 * Each buffer is VTCONS_CTL_RX_BUFSZ bytes to accommodate the control header
 * plus a trailing port name.
 * Returns the number of descriptors successfully posted.
 */
static __u16 vtcons_ctlrxq_fillup(struct vtcons_dev *dev, __u16 nb,
				   int notify)
{
	/* Two segments in case the allocation crosses the page boundary */
	struct uk_sglist_seg sgsegs[2];
	struct uk_sglist sg;
	__u16 filled = 0;
	void *ctlbuf;
	int rc;

	while (filled < nb) {
		if (virtqueue_is_full(dev->ctlrxq.vq))
			break;

		ctlbuf = uk_malloc(vtcons_a, VTCONS_CTL_RX_BUFSZ);
		if (unlikely(!ctlbuf)) {
			uk_pr_warn_isr("dev %u: ctl RX buf alloc failed (%u/%u)\n",
					dev->id, filled, nb);
			break;
		}

		uk_sglist_init(&sg, 2, sgsegs);
		rc = uk_sglist_append(&sg, ctlbuf, VTCONS_CTL_RX_BUFSZ);
		if (unlikely(rc)) {
			uk_pr_err_isr("dev %u: ctl RX sglist append failed: %d\n",
				      dev->id, rc);
			uk_free(vtcons_a, ctlbuf);
			break;
		}

		/* 0 readable segments (host writes into this buffer),
		 * sg.sg_nseg writable segments.
		 */
		rc = virtqueue_buffer_enqueue(dev->ctlrxq.vq, ctlbuf,
					      &sg, 0, sg.sg_nseg);
		if (unlikely(rc < 0)) {
			uk_free(vtcons_a, ctlbuf);
			uk_pr_err_isr("dev %u: ctl RX enqueue failed (%u/%u)\n",
				      dev->id, filled, nb);
			break;
		}
		filled++;
	}

	if (notify && filled)
		virtqueue_host_notify(dev->ctlrxq.vq);

	if (unlikely(filled < nb))
		uk_pr_debug_isr("dev %u: posted %u/%u RX descriptors\n",
				dev->id, filled, nb);

	return filled;
}

/* Dequeue and free all TX buffers the host has already consumed. */
static void vtcons_txq_reclaim(struct vtcons_port *port)
{
	char *txbuf;
	int rc;

	for (;;) {
		rc = virtqueue_buffer_dequeue(port->txq.vq,
					      (void **)&txbuf, __NULL);
		if (rc < 0)
			break;

		UK_ASSERT(txbuf);
		uk_free(vtcons_a, txbuf);
	}
}

/*
 * Enqueue a single struct virtio_console_control message on the control
 * transmit queue and notify the host.  The allocated buffer is reclaimed
 * the next time the control TX callback fires and the thread drains it.
 */
static int vtcons_send_ctl(struct vtcons_dev *dev, __u32 port_id,
			   __u16 event, __u16 value)
{
	struct virtio_console_control *ctlbuf;
	/* Two segments in case the allocation crosses the page boundary */
	struct uk_sglist_seg sgsegs[2];
	struct uk_sglist sg;
	int rc;

	ctlbuf = uk_malloc(vtcons_a, sizeof(*ctlbuf));
	if (unlikely(!ctlbuf))
		return -ENOMEM;

	ctlbuf->id = port_id;
	ctlbuf->event = event;
	ctlbuf->value = value;

	uk_sglist_init(&sg, 2, sgsegs);
	rc = uk_sglist_append(&sg, ctlbuf, sizeof(*ctlbuf));
	if (unlikely(rc)) {
		uk_free(vtcons_a, ctlbuf);
		return rc;
	}

	/* sg.sg_nseg readable descriptors (guest -> host), 0 writable.
	 * ctlbuf is the cookie returned by virtqueue_buffer_dequeue during
	 * reclaim in the control thread, so we know what to free.
	 */
	rc = virtqueue_buffer_enqueue(dev->ctltxq.vq, ctlbuf,
				      &sg, sg.sg_nseg, 0);
	if (unlikely(rc < 0)) {
		uk_free(vtcons_a, ctlbuf);
		return rc;
	}

	virtqueue_host_notify(dev->ctltxq.vq);
	return 0;
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
	struct vtcons_port *port = to_vtcons_port(con);
	__sz total = 0, remaining, copy;
	__u16 reclaimed = 0, refilled;
	struct vtcons_rxbuf *rxbuf;
	__u32 rxlen;
	int rc;

	/**
	 * Drain any partially consumed buffer left over from a previous call.
	 */
	if (port->cached_rxbuf) {
		remaining = port->cached_rxbuf_len - port->cached_rxbuf_pos;
		copy = MIN(remaining, len);

		uk_pr_debug_isr("port %u: draining cached rxbuf: pos=%zu remaining=%zu copy=%zu\n",
				port->id, port->cached_rxbuf_pos,
				remaining, copy);

		memcpy(buf, port->cached_rxbuf->data + port->cached_rxbuf_pos,
		       copy);
		total += copy;
		port->cached_rxbuf_pos += copy;

		if (port->cached_rxbuf_pos >= port->cached_rxbuf_len) {
			uk_pr_debug_isr("port %u: cached rxbuf fully consumed, freeing\n",
					port->id);

			uk_free(vtcons_a, port->cached_rxbuf);
			port->cached_rxbuf = __NULL;
			port->cached_rxbuf_pos = 0;
			port->cached_rxbuf_len = 0;
			/* No reclaimed++ here: this slot was already refilled
			 * when the buffer was originally dequeued and cached.
			 */
		}

		uk_pr_debug_isr("port %u: after cache drain: total=%zu len=%zu\n",
				port->id, total, len);

		if (total >= len)
			goto out;
	}

	while (total < len) {
		rc = virtqueue_buffer_dequeue(port->rxq.vq,
					      (void **)&rxbuf, &rxlen);
		if (rc < 0) {
			uk_pr_debug_isr("port %u: virtqueue empty (total=%zu)\n",
					port->id, total);
			break;
		}

		UK_ASSERT(rxbuf);

		/**
		 * The host reports how many bytes it wrote (rxlen).  If that
		 * is more than the remaining space in the caller's buffer,
		 * copy only what fits and stash the rxbuf for the next call
		 * so the rest is not lost.
		 */
		if ((__sz)rxlen > len - total) {
			copy = len - total;

			uk_pr_debug_isr("port %u: rxlen=%u > remaining=%zu, partial copy=%zu, caching rxbuf\n",
					port->id, rxlen, len - total, copy);

			memcpy(buf + total, rxbuf->data, copy);
			total += copy;

			port->cached_rxbuf = rxbuf;
			port->cached_rxbuf_pos = copy;
			port->cached_rxbuf_len = (__sz)rxlen;

			/**
			 * The slot has been dequeued and is now vacant.
			 * Increment reclaimed so the refill pass below posts a
			 * fresh buffer back to the virtqueue.  The rxbuf itself
			 * is not freed here — it is retained in the cache and
			 * freed once its remaining bytes have been consumed.
			 */
			reclaimed++;
			break;
		}

		uk_pr_debug_isr("port %u: full dequeue rxlen=%u total_so_far=%zu\n",
				port->id, rxlen, total);

		memcpy(buf + total, rxbuf->data, rxlen);
		total += rxlen;
		reclaimed++;
		uk_free(vtcons_a, rxbuf);
	}

out:
	uk_pr_debug_isr("port %u: done: total=%zu reclaimed=%u\n",
			port->id, total, reclaimed);

	/* Refill all consumed slots in one pass and notify the host. */
	if (reclaimed) {
		refilled = vtcons_port_rxq_fillup(port, reclaimed, 1);
		if (unlikely(refilled < reclaimed))
			uk_pr_warn_isr("port %u: partial RX refill (%u/%u)\n",
				       port->id, refilled, reclaimed);
	}

	/* Re-enable the RX interrupt now that the queue is drained.
	 * If data arrived between the last dequeue and this enable,
	 * the host will fire the interrupt again immediately.
	 */
	virtqueue_intr_enable(port->rxq.vq);

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
	struct vtcons_port *port = to_vtcons_port(con);
	struct uk_sglist_seg sgsegs[nsegs];
	struct uk_sglist sg;
	char *txbuf;
	int rc;

	vtcons_txq_reclaim(port);

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
	rc = virtqueue_buffer_enqueue(port->txq.vq, txbuf,
				      &sg, sg.sg_nseg, 0);
	if (unlikely(rc < 0)) {
		uk_free(vtcons_a, txbuf);
		return rc;
	}

	virtqueue_host_notify(port->txq.vq);

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
static int vtcons_port_rxq_recv_done(struct virtqueue *vq, void *priv)
{
	struct vtcons_port *port = (struct vtcons_port *)priv;

	UK_ASSERT(vq && priv);

	virtqueue_intr_disable(vq);

	uk_console_async_in_handle(&port->con_drv);

	return 1;
}

/*
 * Control RX callback, called from interrupt context when the host has
 * written one or more struct virtio_console_control messages into our
 * pre-posted control RX descriptors.
 *
 * Disables further control RX interrupts (re-enabled by the control thread
 * after draining the queue) and wakes the control-receiver thread.
 */
static int vtcons_ctlrxq_recv_done(struct virtqueue *vq, void *priv)
{
	struct vtcons_dev *dev = (struct vtcons_dev *)priv;

	UK_ASSERT(vq && priv);

	virtqueue_intr_disable(vq);

	uk_semaphore_up(&dev->ctl_sem);

	return 1;
}

/*
 * Control TX callback, called from interrupt context when the host has
 * consumed one or more of the control messages we enqueued on the control
 * transmit queue.
 *
 * Wakes the control-receiver thread so it can reclaim the sent buffers and,
 * if necessary, enqueue follow-up messages (e.g. PORT_READY after PORT_ADD).
 */
static int vtcons_ctltxq_done(struct virtqueue *vq, void *priv)
{
	struct vtcons_dev *dev = (struct vtcons_dev *)priv;

	UK_ASSERT(vq && priv);

	uk_semaphore_up(&dev->ctl_sem);

	return 1;
}

/*
 * Handle VIRTIO_CONSOLE_PORT_NAME: store the trailing name payload that
 * follows the control header into port->name.
 */
static void vtcons_handle_port_name(struct vtcons_dev *dev,
				    struct virtio_console_control *ctlbuf,
				    __u32 rxlen)
{
	struct vtcons_port *port;
	__sz namelen;

	if (ctlbuf->id >= dev->max_nr_ports)
		return;

	port = &dev->ports[ctlbuf->id];

	namelen = (rxlen > sizeof(*ctlbuf)) ? rxlen - sizeof(*ctlbuf) : 0;
	if (unlikely(namelen > VTCONS_PORT_NAME_LEN)) {
		uk_pr_err_isr("port %u: VIRTIO_CONSOLE_PORT_NAME truncated from %lu to %lu\n",
			      port->id, namelen, VTCONS_PORT_NAME_LEN);
		namelen = VTCONS_PORT_NAME_LEN;
	}


	memcpy(port->name, (const char *)(ctlbuf + 1), namelen);
	port->name[namelen] = '\0';

#if CONFIG_LIBUKFS_DEVFS
	/* Create the /dev/virtio-ports/<name> symlink now that the port has
	 * a name, but only if devfs is ready and the port is not a console
	 * port (console ports do not get a vportXportY node to link to).
	 */
	if (dev->devfs_ready && !port->is_console)
		vtcons_devfs_mksymlink(port);
#endif /* CONFIG_LIBUKFS_DEVFS */
}

/*
 * Handle VIRTIO_CONSOLE_CONSOLE_PORT: the host is nominating this port for
 * STDOUT routing.  Every port is already readable/writable and already
 * registered with uk_console from DEVICE_ADD.  This event only controls
 * whether the port gets UK_CONSOLE_FLAG_STDOUT.
 */
static void vtcons_handle_console_port(struct vtcons_dev *dev,
				       struct virtio_console_control *ctlbuf,
				       __u32 rxlen __maybe_unused)
{
	const struct uk_console_ops *ops;
	struct vtcons_port *port;
	const char *name;

	if (ctlbuf->id >= dev->max_nr_ports)
		return;

	port = &dev->ports[ctlbuf->id];

	/* If this port has already been promoted via a previous CONSOLE_PORT
	 * event, do nothing — the promotion is idempotent and does not need
	 * to be repeated.
	 */
	if (port->is_console)
		return;

#if CONFIG_LIBUKFS_DEVFS
	if (dev->devfs_ready)
		uk_pr_warn_isr("We don't handle late VIRTIO_CONSOLE_CONSOLE_PORT in devfs!\n");
#endif /* CONFIG_LIBUKFS_DEVFS */

	if (port->is_registered)
		uk_console_unregister(&port->con_drv.cons);

	if (VIRTIO_FEATURE_HAS(dev->vdev->features,
			       VIRTIO_CONSOLE_F_EMERG_WRITE))
		ops = &vtcons_ops_emerg;
	else
		ops = &vtcons_ops;

	name = port->name[0] ? port->name : "virtio-console-port";

	/**
	 * FIXME: If there were any registered callbacks on this console
	 * device then they will permanently get lost or need re-registration.
	 * In practice, this is fine as it is usually just sent once during
	 * driver setup when we are polling for control messages and tends to
	 * be static in VMM configurations - so before any higher-level library
	 * or component would attempt to register anything with this port.
	 */
	uk_console_async_init(&port->con_drv, name, ops,
			      UK_CONSOLE_FLAG_ASYNC_RX | UK_CONSOLE_FLAG_STDOUT,
			      UK_CONSOLE_CLASS_HVC);
	uk_console_register(&port->con_drv.cons);

	port->is_console = 1;
	port->is_registered = 1;
}

/*
 * Handle VIRTIO_CONSOLE_PORT_ADD: set up the port's virtqueues (lazily for
 * id > 0), register it with uk_console, and acknowledge with PORT_READY.
 */
static void vtcons_handle_device_add(struct vtcons_dev *dev,
				     struct virtio_console_control *ctlbuf,
				     __u32 rxlen __maybe_unused)
{
	__u16 rx_nb_desc, tx_nb_desc, rx_idx, tx_idx;
	__u16 rx_max_nb_desc, tx_max_nb_desc;
	const struct uk_console_ops *ops;
	struct vtcons_port *port;
	struct virtqueue *vq;
	const char *name;
	__u16 filled;
	int rc;

	if (ctlbuf->id >= dev->max_nr_ports) {
		rc = vtcons_send_ctl(dev, ctlbuf->id,
				     VIRTIO_CONSOLE_PORT_READY, 0);
		if (unlikely(rc))
			uk_pr_warn_isr("port %u: failed to send PORT_READY: %d\n",
				       ctlbuf->id, rc);
		return;
	}

	port = &dev->ports[ctlbuf->id];

	/* Restore the back-pointer and id in case this port was previously
	 * removed (memset'd) and is being re-added.
	 */
	port->id = ctlbuf->id;
	port->dev = dev;

	if (VIRTIO_FEATURE_HAS(dev->vdev->features,
			       VIRTIO_CONSOLE_F_EMERG_WRITE))
		ops = &vtcons_ops_emerg;
	else
		ops = &vtcons_ops;

	if (ctlbuf->id == 0) {
		/* Port 0 data queues are already set up at probe time. */
		rc = vtcons_send_ctl(dev, 0, VIRTIO_CONSOLE_PORT_READY, 1);
		if (unlikely(rc))
			uk_pr_warn_isr("port %u: failed to send PORT_READY: %d\n",
				       ctlbuf->id, rc);


		/**
		 * TODO: Normally, we should send PORT_OPEN with 1 on open()
		 * and 0 on close(). Implicitly send PORT_OPEN for now as we
		 * do not support intercepting open()/close() directly in the
		 * driver yet.
		 */
		 rc = vtcons_send_ctl(dev, ctlbuf->id,
				      VIRTIO_CONSOLE_PORT_OPEN, 1);
		if (unlikely(rc))
			uk_pr_warn_isr("port %u: failed to send PORT_OPEN: %d\n",
				       ctlbuf->id, rc);


		/* The framework auto-assigns STDOUT/STDIN/EMERG_STDOUT flags
		 * based on which ops are present, so we only declare ASYNC_RX
		 * here.
		 */
		uk_console_async_init(&port->con_drv, "virtio-console", ops,
				      UK_CONSOLE_FLAG_ASYNC_RX,
				      UK_CONSOLE_CLASS_NONE);
		uk_console_register(&port->con_drv.cons);
		port->is_registered = 1;
		return;
	}

	/* id > 0: set up the port's virtqueues lazily. */
	rx_idx = vtcons_port_rxq_idx(ctlbuf->id);
	tx_idx = vtcons_port_txq_idx(ctlbuf->id);

	rx_max_nb_desc = dev->qdesc_sizes[rx_idx];
	tx_max_nb_desc = dev->qdesc_sizes[tx_idx];

	/* Legacy virtio has queue_size register as RO, while modern has it RW.
	 * This means the vring layout is immutable and setting up the queues
	 * with anything less than the reported vq size will result in layout
	 * mismatch between the host paravirtualized device and the
	 * guest device driver.
	 */
	if (VIRTIO_FEATURE_HAS(dev->vdev->features, VIRTIO_F_VERSION_1)) {
		rx_nb_desc = MIN((__u16)VTCONS_NB_DESC, rx_max_nb_desc);
		tx_nb_desc = MIN((__u16)VTCONS_NB_DESC, tx_max_nb_desc);
	} else {
		rx_nb_desc = rx_max_nb_desc;
		tx_nb_desc = tx_max_nb_desc;
	}

	port->rxq.max_nb_desc = rx_max_nb_desc;
	port->rxq.nb_desc = rx_nb_desc;
	port->txq.max_nb_desc = tx_max_nb_desc;
	port->txq.nb_desc = tx_nb_desc;

	vq = virtio_vqueue_setup(dev->vdev, rx_idx, rx_nb_desc,
				 vtcons_port_rxq_recv_done, vtcons_a);
	if (unlikely(PTRISERR(vq))) {
		uk_pr_err_isr("failed to set up RX vq for port %u: %d\n",
			      ctlbuf->id, (int)PTR2ERR(vq));
		vtcons_send_ctl(dev, ctlbuf->id,
				VIRTIO_CONSOLE_PORT_READY, 0);
		return;
	}
	vq->priv = port;
	port->rxq.vq = vq;

	vq = virtio_vqueue_setup(dev->vdev, tx_idx, tx_nb_desc,
				 __NULL, vtcons_a);
	if (unlikely(PTRISERR(vq))) {
		uk_pr_err_isr("failed to set up TX vq for port %u: %d\n",
			      ctlbuf->id, (int)PTR2ERR(vq));
		vtcons_send_ctl(dev, ctlbuf->id,
				VIRTIO_CONSOLE_PORT_READY, 0);
		return;
	}
	port->txq.vq = vq;

	/* TX is purely poll/lazy — no interrupt needed. */
	virtqueue_intr_disable(port->txq.vq);

	filled = vtcons_port_rxq_fillup(port, port->rxq.nb_desc, 0);
	if (unlikely(!filled)) {
		uk_pr_err_isr("port %u: failed to post any RX buffers\n",
			      ctlbuf->id);
		rc = vtcons_send_ctl(dev, ctlbuf->id,
				     VIRTIO_CONSOLE_PORT_READY, 0);
		if (unlikely(rc))
			uk_pr_warn_isr("port %u: failed to send PORT_READY(0): %d\n",
				       ctlbuf->id, rc);
		virtio_vqueue_release(dev->vdev, port->rxq.vq, vtcons_a);
		virtio_vqueue_release(dev->vdev, port->txq.vq, vtcons_a);
		return;
	}

	virtqueue_host_notify(port->rxq.vq);
	virtqueue_intr_enable(port->rxq.vq);

	rc = vtcons_send_ctl(dev, ctlbuf->id, VIRTIO_CONSOLE_PORT_READY, 1);
	if (unlikely(rc))
		uk_pr_warn_isr("port %u: failed to send PORT_READY: %d\n",
			       ctlbuf->id, rc);


	/**
	 * TODO: Normally, we should send PORT_OPEN with 1 on open()
	 * and 0 on close(). Implicitly send PORT_OPEN for now as we
	 * do not support intercepting open()/close() directly in the
	 * driver yet.
	 */
	rc = vtcons_send_ctl(dev, ctlbuf->id, VIRTIO_CONSOLE_PORT_OPEN, 1);
	if (unlikely(rc))
		uk_pr_warn_isr("port %u: failed to send PORT_OPEN: %d\n",
			       ctlbuf->id, rc);

	name = port->name[0] ? port->name : "virtio-console-port";

	uk_console_async_init(&port->con_drv, name, ops,
			      UK_CONSOLE_FLAG_ASYNC_RX,
			      UK_CONSOLE_CLASS_NONE);
	uk_console_register(&port->con_drv.cons);
	port->is_registered = 1;

#if CONFIG_LIBUKFS_DEVFS
	if (port->dev->devfs_ready)
		vtcons_devfs_mknode(port);
#endif /* CONFIG_LIBUKFS_DEVFS */
}

/*
 * Handle VIRTIO_CONSOLE_PORT_REMOVE: unregister the port from uk_console,
 * drain its virtqueues, and release per-port queue resources for id > 0.
 */
static void vtcons_handle_device_remove(struct vtcons_dev *dev,
					struct virtio_console_control *ctlbuf,
					__u32 rxlen __maybe_unused)
{
	struct vtcons_rxbuf *rxbuf;
	struct vtcons_port *port;
	char *txbuf;
	__u32 dummy;
	int rc;

	if (ctlbuf->id >= dev->max_nr_ports)
		return;

	port = &dev->ports[ctlbuf->id];

	if (port->is_registered)
		uk_console_unregister(&port->con_drv.cons);

	/* Drain and free any outstanding RX buffers for the port. */
	if (port->rxq.vq) {
		for (;;) {
			rc = virtqueue_buffer_dequeue(port->rxq.vq,
						      (void **)&rxbuf, &dummy);
			if (rc < 0)
				break;
			UK_ASSERT(rxbuf);
			uk_free(vtcons_a, rxbuf);
		}
	}

	/* Reclaim any outstanding TX buffers. */
	if (port->txq.vq) {
		for (;;) {
			rc = virtqueue_buffer_dequeue(port->txq.vq,
						      (void **)&txbuf, __NULL);
			if (rc < 0)
				break;
			UK_ASSERT(txbuf);
			uk_free(vtcons_a, txbuf);
		}
	}

	if (ctlbuf->id != 0) {
		/* Release the per-port virtqueues. */
		if (port->rxq.vq)
			virtio_vqueue_release(dev->vdev, port->rxq.vq,
					      vtcons_a);
		if (port->txq.vq)
			virtio_vqueue_release(dev->vdev, port->txq.vq,
					      vtcons_a);
	}
	/* For id == 0: do NOT release queues 0 and 1 — they are managed by
	 * the device lifetime; just drain and mark inactive.
	 */
#if CONFIG_LIBUKFS_DEVFS
	if (port->dev->devfs_ready && !port->is_console)
		vtcons_devfs_rmnode(port);
#endif /* CONFIG_LIBUKFS_DEVFS */

	memset(port, 0, sizeof(*port));
}

/*
 * Control-receiver thread body (runs as "vtcons_ctlr<N>").
 *
 * Sleeps on ctl_sem; each control-queue interrupt callback does a single
 * uk_semaphore_up().  On wake-up the thread:
 *   1. Drains completed control TX cookies (frees them).
 *   2. Drains completed control RX messages and dispatches each event.
 *   3. Refills the control RX ring and re-arms its interrupt.
 */
__noreturn static void vtcons_ctlr_thread(void *arg)
{
	struct vtcons_dev *dev = (struct vtcons_dev *)arg;
	struct virtio_console_control *ctlbuf;
	__u16 reclaimed, refilled;
	__u32 rxlen;
	int rc;

	for (;;) {
		uk_semaphore_down_all(&dev->ctl_sem);

		/* Reclaim consumed control TX buffers. */
		for (;;) {
			rc = virtqueue_buffer_dequeue(dev->ctltxq.vq,
						      (void **)&ctlbuf,
						      __NULL);
			if (rc < 0)
				break;

			UK_ASSERT(ctlbuf);
			uk_free(vtcons_a, ctlbuf);
		}

		/* Drain and dispatch control RX messages. */
		reclaimed = 0;
		for (;;) {
			rc = virtqueue_buffer_dequeue(dev->ctlrxq.vq,
						      (void **)&ctlbuf,
						      &rxlen);
			if (rc < 0)
				break;

			UK_ASSERT(ctlbuf);
			reclaimed++;

			switch (ctlbuf->event) {
			case VIRTIO_CONSOLE_PORT_ADD:
				vtcons_handle_device_add(dev, ctlbuf, rxlen);
				break;

			case VIRTIO_CONSOLE_PORT_REMOVE:
				vtcons_handle_device_remove(dev, ctlbuf, rxlen);
				break;

			case VIRTIO_CONSOLE_CONSOLE_PORT:
				vtcons_handle_console_port(dev, ctlbuf, rxlen);
				break;

			case VIRTIO_CONSOLE_PORT_NAME:
				vtcons_handle_port_name(dev, ctlbuf, rxlen);
				break;

			case VIRTIO_CONSOLE_RESIZE:
			case VIRTIO_CONSOLE_PORT_OPEN:
				/* Informational — log and ignore. */
				uk_pr_debug_isr("ignoring ctl event %u for port %u\n",
						ctlbuf->event, ctlbuf->id);
				break;

			default:
				uk_pr_warn_isr("unknown ctl event %u\n",
					       ctlbuf->event);
				break;
			}

			uk_free(vtcons_a, ctlbuf);
		}

		/* Refill consumed RX slots in one pass and re-arm the
		 * interrupt.  If a control message arrived between the last
		 * dequeue and this enable, the host will fire again.
		 */
		if (reclaimed) {
			refilled = vtcons_ctlrxq_fillup(dev, reclaimed, 1);
			if (unlikely(refilled < reclaimed))
				uk_pr_warn_isr("dev %u: partial ctl RX refill (%u/%u)\n",
					       dev->id, refilled, reclaimed);
		}

		virtqueue_intr_enable(dev->ctlrxq.vq);
	}
}

static int vtcons_vqueue_setup(struct vtcons_dev *dev)
{
	struct virtqueue *vq;
	int nvqs, vq_avail;
	__bool multiport;

	multiport = VIRTIO_FEATURE_HAS(dev->vdev->features,
				       VIRTIO_CONSOLE_F_MULTIPORT);

	if (multiport)
		nvqs = (int)(2 * (dev->max_nr_ports + 1));
	else
		nvqs = 2;

	vq_avail = virtio_find_vqs(dev->vdev, nvqs, dev->qdesc_sizes);
	if (unlikely(vq_avail != nvqs)) {
		uk_pr_err_isr("Expected %d virtqueues, found %d\n",
			      nvqs, vq_avail);
		return -ENODEV;
	}

	dev->ports[0].rxq.max_nb_desc = dev->qdesc_sizes[VTCONS_RXQ_IDX];
	dev->ports[0].txq.max_nb_desc = dev->qdesc_sizes[VTCONS_TXQ_IDX];

	/* Legacy virtio has queue_size register as RO, while modern has it RW.
	 * This means the vring layout is immutable and setting up the queues
	 * with anything less than the reported vq size will result in layout
	 * mismatch between the host paravirtualized device and the
	 * guest device driver.
	 */
	if (VIRTIO_FEATURE_HAS(dev->vdev->features, VIRTIO_F_VERSION_1)) {
		dev->ports[0].txq.nb_desc = MIN((__u16)VTCONS_NB_DESC,
						dev->ports[0].txq.max_nb_desc);
		dev->ports[0].rxq.nb_desc = MIN((__u16)VTCONS_NB_DESC,
						dev->ports[0].rxq.max_nb_desc);
	} else {
		dev->ports[0].txq.nb_desc = dev->ports[0].txq.max_nb_desc;
		dev->ports[0].rxq.nb_desc = dev->ports[0].rxq.max_nb_desc;
	}

	vq = virtio_vqueue_setup(dev->vdev, VTCONS_RXQ_IDX,
				 dev->ports[0].rxq.nb_desc,
				 vtcons_port_rxq_recv_done, vtcons_a);
	if (unlikely(PTRISERR(vq))) {
		uk_pr_err_isr("Failed to set up RX virtqueue: %d\n",
			      (int)PTR2ERR(vq));
		return PTR2ERR(vq);
	}
	vq->priv = &dev->ports[0];
	dev->ports[0].rxq.vq = vq;

	vq = virtio_vqueue_setup(dev->vdev, VTCONS_TXQ_IDX,
				 dev->ports[0].txq.nb_desc,
				 __NULL, vtcons_a);
	if (unlikely(PTRISERR(vq))) {
		uk_pr_err_isr("Failed to set up TX virtqueue: %d\n",
			      (int)PTR2ERR(vq));
		virtio_vqueue_release(dev->vdev, dev->ports[0].rxq.vq,
				      vtcons_a);
		return PTR2ERR(vq);
	}
	dev->ports[0].txq.vq = vq;

	/* TX is purely poll/lazy — no interrupt needed. */
	virtqueue_intr_disable(dev->ports[0].txq.vq);

	if (!multiport)
		return 0;

	dev->ctlrxq.max_nb_desc = dev->qdesc_sizes[VTCONS_CTLRXQ_IDX];
	dev->ctltxq.max_nb_desc = dev->qdesc_sizes[VTCONS_CTLTXQ_IDX];

	/* Legacy virtio has queue_size register as RO, while modern has it RW.
	 * This means the vring layout is immutable and setting up the queues
	 * with anything less than the reported vq size will result in layout
	 * mismatch between the host paravirtualized device and the
	 * guest device driver.
	 */
	if (VIRTIO_FEATURE_HAS(dev->vdev->features, VIRTIO_F_VERSION_1)) {
		dev->ctlrxq.nb_desc = MIN((__u16)VTCONS_NB_DESC,
					  dev->ctlrxq.max_nb_desc);
		dev->ctltxq.nb_desc = MIN((__u16)VTCONS_NB_DESC,
					  dev->ctltxq.max_nb_desc);
	} else {
		dev->ctlrxq.nb_desc = dev->ctlrxq.max_nb_desc;
		dev->ctltxq.nb_desc = dev->ctltxq.max_nb_desc;
	}

	vq = virtio_vqueue_setup(dev->vdev, VTCONS_CTLRXQ_IDX,
				 dev->ctlrxq.nb_desc,
				 vtcons_ctlrxq_recv_done, vtcons_a);
	if (unlikely(PTRISERR(vq))) {
		uk_pr_err_isr("Failed to set up control RX virtqueue: %d\n",
			      (int)PTR2ERR(vq));
		virtio_vqueue_release(dev->vdev, dev->ports[0].rxq.vq,
				      vtcons_a);
		virtio_vqueue_release(dev->vdev, dev->ports[0].txq.vq,
				      vtcons_a);
		return PTR2ERR(vq);
	}
	vq->priv = dev;
	dev->ctlrxq.vq = vq;

	vq = virtio_vqueue_setup(dev->vdev, VTCONS_CTLTXQ_IDX,
				 dev->ctltxq.nb_desc,
				 vtcons_ctltxq_done, vtcons_a);
	if (unlikely(PTRISERR(vq))) {
		uk_pr_err_isr("Failed to set up control TX virtqueue: %d\n",
			      (int)PTR2ERR(vq));
		virtio_vqueue_release(dev->vdev, dev->ctlrxq.vq, vtcons_a);
		virtio_vqueue_release(dev->vdev, dev->ports[0].rxq.vq,
				      vtcons_a);
		virtio_vqueue_release(dev->vdev, dev->ports[0].txq.vq,
				      vtcons_a);
		return PTR2ERR(vq);
	}
	vq->priv = dev;
	dev->ctltxq.vq = vq;

	return 0;
}

static int vtcons_dev_negotiate(struct virtio_dev *vdev)
{
	__u64 dev_features, drv_features = 0;
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

	/* Multiport support: enables control queues and additional ports. */
	if (VIRTIO_FEATURE_HAS(dev_features, VIRTIO_CONSOLE_F_MULTIPORT))
		VIRTIO_FEATURE_SET(drv_features, VIRTIO_CONSOLE_F_MULTIPORT);

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

/*
 * Poll the control RX queue directly (interrupts disabled) until both
 * VIRTIO_CONSOLE_PORT_ADD and VIRTIO_CONSOLE_CONSOLE_PORT have been received
 * for port 0.  Called once during probe, before handing control to the
 * control-receiver thread.  QEMU sends these two messages synchronously after
 * receiving DEVICE_READY, so the spin is bounded.
 *
 * Any unrecognised messages that arrive in the same burst are consumed and
 * refilled silently; the control thread handles all subsequent events once
 * interrupts are enabled by the caller.
 */
static void vtcons_ctlq_probe_handshake(struct vtcons_dev *dev)
{
	struct virtio_console_control *ctlbuf;
	__u32 rxlen;
	int rc;

	for (;;) {
		rc = virtqueue_buffer_dequeue(dev->ctlrxq.vq,
					      (void **)&ctlbuf, &rxlen);
		if (rc < 0)
			break;

		UK_ASSERT(ctlbuf);

		switch (ctlbuf->event) {
		case VIRTIO_CONSOLE_PORT_ADD:
			vtcons_handle_device_add(dev, ctlbuf, rxlen);
			break;
		case VIRTIO_CONSOLE_CONSOLE_PORT:
			vtcons_handle_console_port(dev, ctlbuf, rxlen);
			break;
		case VIRTIO_CONSOLE_PORT_REMOVE:
			vtcons_handle_device_remove(dev, ctlbuf, rxlen);
			break;
		case VIRTIO_CONSOLE_PORT_NAME:
			vtcons_handle_port_name(dev, ctlbuf, rxlen);
			break;
		default:
			uk_pr_debug_isr("ignoring event %u for port %u\n",
					ctlbuf->event, ctlbuf->id);
			break;
		}

		uk_free(vtcons_a, ctlbuf);
		vtcons_ctlrxq_fillup(dev, 1, 1);
	}
}

/*
 * Single-port (non-multiport) start path: initialise and register the
 * port 0 console, mark the device DRIVER_OK, and arm the RX interrupt.
 */
static void vtcons_singleport_start(struct vtcons_dev *dev)
{
	const struct uk_console_ops *ops;

	if (VIRTIO_FEATURE_HAS(dev->vdev->features,
			       VIRTIO_CONSOLE_F_EMERG_WRITE))
		ops = &vtcons_ops_emerg;
	else
		ops = &vtcons_ops;

	/* The framework auto-assigns STDOUT/STDIN/EMERG_STDOUT flags based
	 * on which ops are present, so we only declare ASYNC_RX here.
	 */
	uk_console_async_init(&dev->ports[0].con_drv, "virtio-console",
			      ops, UK_CONSOLE_FLAG_ASYNC_RX,
			      UK_CONSOLE_CLASS_HVC);

	/* Set device DRIVER_OK — host may start sending data from here. */
	virtio_dev_drv_up(dev->vdev);

	/* Register with ukconsole after the device is live so that any early
	 * printk that fires during registration already works.
	 */
	uk_console_register(&dev->ports[0].con_drv.cons);

	/* Flush pre-filled RX descriptors and arm the interrupt. */
	virtqueue_host_notify(dev->ports[0].rxq.vq);
	virtqueue_intr_enable(dev->ports[0].rxq.vq);
}

/*
 * Multiport start path: pre-fill the control RX ring, mark the device
 * DRIVER_OK, perform the initial PORT_ADD(0)/CONSOLE_PORT(0) handshake
 * by polling (no interrupts), then create the control-receiver thread.
 */
static int vtcons_multiport_start(struct vtcons_dev *dev)
{
	__u16 filled;
	int rc;

	/* Pre-fill the control RX ring before the device is marked
	 * DRIVER_OK so that control messages sent right at startup
	 * are not lost.  Defer the host notification until after
	 * drv_up.
	 */
	filled = vtcons_ctlrxq_fillup(dev, dev->ctlrxq.nb_desc, 0);
	if (unlikely(!filled)) {
		uk_pr_err_isr("dev %u: failed to post any ctl RX buffers\n",
			      dev->id);
		return -ENOMEM;
	}
	if (unlikely(filled < dev->ctlrxq.nb_desc))
		uk_pr_warn_isr("dev %u: partial ctl RX prefill (%u/%u)\n",
			       dev->id, filled, dev->ctlrxq.nb_desc);

	/* Initialise the semaphore to 0 — the thread blocks until
	 * the first control-queue interrupt arrives. Do this very early
	 * so that any raised IRQs can up a valid semaphore.
	 */
	uk_semaphore_init(&dev->ctl_sem, 0);

	/* Set device DRIVER_OK — host may start sending data from here. */
	virtio_dev_drv_up(dev->vdev);

	/* Flush pre-filled port 0 RX descriptors and arm the interrupt. */
	virtqueue_host_notify(dev->ports[0].rxq.vq);
	virtqueue_intr_enable(dev->ports[0].rxq.vq);

	/* Flush control RX descriptors.  Do not arm the control RX interrupt
	 * yet: we poll the queue directly for the initial PORT_ADD(0) and
	 * CONSOLE_PORT(0) handshake so that port 0 is fully registered before
	 * vtcons_add_dev returns.
	 */
	virtqueue_host_notify(dev->ctlrxq.vq);

	/* The driver MUST send DEVICE_READY if VIRTIO_CONSOLE_F_MULTIPORT is
	 * negotiated.  value=1 means success.
	 */
	rc = vtcons_send_ctl(dev, 0, VIRTIO_CONSOLE_DEVICE_READY, 1);
	if (unlikely(rc))
		uk_pr_warn_isr("failed to send DEVICE_READY: %d\n", rc);

	/* Poll the control RX queue for the initial PORT_ADD(0) and
	 * CONSOLE_PORT(0) handshake before arming the interrupt and
	 * handing off to the control thread.
	 *
	 * TODO: This works nicely because most VMMs send in burst all these
	 * control message right after DEVICE_READY. If we bump into a platform
	 * that does not do this, we are going to delay booting unnecessarily,
	 * so make this configurable when that happens.
	 */
	vtcons_ctlq_probe_handshake(dev);

	/* Handshake complete.  Arm the control RX interrupt so the
	 * control thread handles all subsequent messages.
	 */
	virtqueue_intr_enable(dev->ctlrxq.vq);

	/* dev->id was assigned in vtcons_add_dev. */
	snprintf(dev->ctl_thr_name, sizeof(dev->ctl_thr_name),
		 "vtcons_ctlr%u", dev->id);

	dev->ctl_thread = uk_sched_thread_create(uk_sched_current(),
						 vtcons_ctlr_thread,
						 dev,
						 dev->ctl_thr_name);
	if (unlikely(!dev->ctl_thread)) {
		uk_pr_err_isr("Failed to create control thread %s\n",
			      dev->ctl_thr_name);
		return -ENOMEM;
	}

	return 0;
}

static int vtcons_add_dev(struct virtio_dev *vdev)
{
	struct vtcons_dev *dev;
	int rc, rc2, nvqs;
	__bool multiport;
	__u16 filled;

	UK_ASSERT(vdev);

	rc = vtcons_dev_negotiate(vdev);
	if (unlikely(rc))
		return rc;

	multiport = VIRTIO_FEATURE_HAS(vdev->features,
				       VIRTIO_CONSOLE_F_MULTIPORT);

	dev = uk_calloc(vtcons_a, 1, sizeof(*dev));
	if (unlikely(!dev)) {
		uk_pr_err_isr("Failed to allocate device struct\n");
		rc = virtio_dev_status_update(vdev, VIRTIO_CONFIG_STATUS_FAIL);
		if (unlikely(rc))
			uk_pr_err("Failed set virtio device failure status: %d\n",
				  rc);
		return -ENOMEM;
	}

	dev->vdev = vdev;
	vdev->priv = dev;

	/* Assign a stable device index before anything else so that thread
	 * naming and vport node naming are consistent.
	 */
	dev->id = vtcons_dev_cnt++;

	if (multiport) {
		virtio_config_get(vdev,
				  __offsetof(struct virtio_console_config,
					     max_nr_ports),
				  &dev->max_nr_ports,
				  sizeof(dev->max_nr_ports),
				  sizeof(dev->max_nr_ports));
		if (!dev->max_nr_ports)
			dev->max_nr_ports = 1;

		nvqs = (int)(2 * (dev->max_nr_ports + 1));
	} else {
		dev->max_nr_ports = 1;
		nvqs = 2;
	}

	dev->ports = uk_calloc(vtcons_a, dev->max_nr_ports,
			       sizeof(*dev->ports));
	if (unlikely(!dev->ports)) {
		uk_pr_err_isr("Failed to allocate ports array\n");
		rc = -ENOMEM;
		goto err_free_dev;
	}

	dev->ports[0].id = 0;
	dev->ports[0].dev = dev;

	dev->qdesc_sizes = uk_calloc(vtcons_a, nvqs,
					 sizeof(*dev->qdesc_sizes));
	if (unlikely(!dev->qdesc_sizes)) {
		uk_pr_err_isr("Failed to allocate qdesc sizes array\n");
		rc = -ENOMEM;
		goto err_free_ports;
	}

	rc = vtcons_vqueue_setup(dev);
	if (unlikely(rc)) {
		uk_pr_err_isr("Failed to set up virtqueues: %d\n", rc);
		goto err_free_qdesc;
	}

	/* Pre-fill the RX ring so the host can immediately send data.
	 * Defer the host notification until after drv_up below.
	 */
	filled = vtcons_port_rxq_fillup(&dev->ports[0],
					dev->ports[0].rxq.nb_desc, 0);
	if (unlikely(!filled)) {
		uk_pr_err_isr("failed to post any port 0 RX buffers\n");
		rc = -ENOMEM;
		goto err_free_qdesc;
	}
	if (unlikely(filled < dev->ports[0].rxq.nb_desc))
		uk_pr_warn_isr("partial port 0 RX prefill (%u/%u) — heap may be low\n",
			       filled, dev->ports[0].rxq.nb_desc);

	if (multiport) {
		rc = vtcons_multiport_start(dev);
		if (unlikely(rc)) {
			struct vtcons_rxbuf *rxbuf;

			while (virtqueue_buffer_dequeue(dev->ports[0].rxq.vq,
							(void **)&rxbuf,
							__NULL) >= 0)
				uk_free(vtcons_a, rxbuf);

			virtio_vqueue_release(dev->vdev,
					      dev->ports[0].rxq.vq, vtcons_a);
			virtio_vqueue_release(dev->vdev,
					      dev->ports[0].txq.vq, vtcons_a);
			goto err_free_qdesc;
		}
	} else {
		vtcons_singleport_start(dev);
	}

#if CONFIG_LIBUKFS_DEVFS
	uk_list_add(&dev->list, &vtcons_dev_list);
#endif /* CONFIG_LIBUKFS_DEVFS */

	uk_pr_info_isr("Virtio console ready\n");
	return 0;

err_free_qdesc:
	uk_free(vtcons_a, dev->qdesc_sizes);
err_free_ports:
	uk_free(vtcons_a, dev->ports);
err_free_dev:
	uk_free(vtcons_a, dev);

	rc2 = virtio_dev_status_update(vdev, VIRTIO_CONFIG_STATUS_FAIL);
	if (unlikely(rc2))
		uk_pr_err("Failed set virtio device failure status: %d\n",
			  rc2);
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
