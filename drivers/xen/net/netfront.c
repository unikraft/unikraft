/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Costin Lupu <costin.lupu@cs.pub.ro>
 *          Razvan Cojocaru <razvan.cojocaru93@gmail.com>
 *          Simon Kuenzer <simon.kuenzer@neclab.eu>
 *
 * Copyright (c) 2020, University Politehnica of Bucharest. All rights reserved.
 * Copyright (c) 2020, NEC Laboratories Europe GmbH, NEC Corporation.
 *                     All rights reserved.
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
 */

#include <string.h>
#include <uk/assert.h>
#include <uk/print.h>
#include <uk/alloc.h>
#include <uk/netdev_driver.h>
#if defined(__i386__) || defined(__x86_64__)
#include <xen-x86/mm.h>
#include <xen-x86/irq.h>
#elif defined(__aarch64__)
#include <xen-arm/mm.h>
#include <arm/irq.h>
#else
#error "Unsupported architecture"
#endif
#include <uk/xenbus/xenbus.h>
#include <uk/essentials.h>
#include "netfront.h"
#include "netfront_xb.h"


#define DRIVER_NAME  "xen-netfront"

/* TODO Same interrupt macros we use in virtio-net */
#define NETFRONT_INTR_EN             (1 << 0)
#define NETFRONT_INTR_EN_MASK        (1)
#define NETFRONT_INTR_USR_EN         (1 << 1)
#define NETFRONT_INTR_USR_EN_MASK    (2)

#define to_netfront_dev(dev) \
	__containerof(dev, struct netfront_dev, netdev)

static struct uk_alloc *drv_allocator;


static uint16_t xennet_rxidx(RING_IDX idx)
{
	return (uint16_t) (idx & (NET_RX_RING_SIZE - 1));
}

static void add_id_to_freelist(uint16_t id, uint16_t *freelist)
{
	freelist[id + 1] = freelist[0];
	freelist[0]  = id;
}

static uint16_t get_id_from_freelist(uint16_t *freelist)
{
	uint16_t id;

	id = freelist[0];
	freelist[0] = freelist[id + 1];
	return id;
}

static int network_tx_buf_gc(struct uk_netdev_tx_queue *txq)
{
	RING_IDX prod, cons;
	netif_tx_response_t *tx_rsp;
	uint16_t id;
	int count = 0;

	prod = txq->ring.sring->rsp_prod;
	rmb(); /* Ensure we see responses up to 'rp'. */

	for (cons = txq->ring.rsp_cons; cons != prod; cons++) {
		tx_rsp = RING_GET_RESPONSE(&txq->ring, cons);

		if (tx_rsp->status == NETIF_RSP_NULL)
			continue;

		if (tx_rsp->status == NETIF_RSP_ERROR)
			uk_pr_err("netdev%u: Transmission error on txq %u\n",
				  txq->netfront_dev->netdev._data->id,
				  txq->lqueue_id);


		id  = tx_rsp->id;
		UK_ASSERT(id < NET_TX_RING_SIZE);

		uk_netbuf_free_single(txq->nbuf[id]);

		add_id_to_freelist(id, txq->freelist);

		count++;
	}
	txq->ring.rsp_cons = prod;

	return count;
}

static int netfront_xmit(struct uk_netdev *n,
		struct uk_netdev_tx_queue *txq,
		struct uk_netbuf *pkt)
{
	struct netfront_dev *nfdev;
	unsigned long flags;
	uint16_t id;
	RING_IDX req_prod;
	netif_tx_request_t *tx_req;
	bool more_to_do;
	int notify;
	int status;

	UK_ASSERT(n != NULL);
	UK_ASSERT(txq != NULL);
	UK_ASSERT(pkt != NULL);
	UK_ASSERT(pkt->len < PAGE_SIZE);
	UK_ASSERT(!pkt->next); /* TODO: Support for netbuf chains missing */
	UK_ASSERT(((unsigned long) pkt->buf & ~PAGE_MASK) == 0);

	nfdev = to_netfront_dev(n);

	local_irq_save(flags);
	if (unlikely(RING_FULL(&txq->ring))) {
		/* try some cleanup */
		network_tx_buf_gc(txq);
		if (unlikely(RING_FULL(&txq->ring))) {
			uk_pr_debug("tx queue is full\n");
			local_irq_restore(flags);
			return 0x0;
		}
	}

	/* get request id */
	id = get_id_from_freelist(txq->freelist);

	/* get request */
	req_prod = txq->ring.req_prod_pvt;
	tx_req = RING_GET_REQUEST(&txq->ring, req_prod);

	/* setup grant for buffer data */
	if (unlikely(txq->gref[id] == GRANT_INVALID_REF)) {
		/* allocating of a new grant needed */
		txq->gref[id] = gnttab_grant_access(nfdev->xendev->otherend_id,
						    virt_to_mfn(pkt->buf),
						    0);
	} else {
		/* re-use grant (update it) */
		gnttab_update_grant(txq->gref[id],
				    nfdev->xendev->otherend_id,
				    virt_to_mfn(pkt->buf),
				    0);
	}
	UK_ASSERT(txq->gref[id] != GRANT_INVALID_REF);

	/* remember netbuf reference for free'ing it later */
	txq->nbuf[id] = pkt;
	tx_req->gref = txq->gref[id];
	tx_req->offset = (uint16_t) uk_netbuf_headroom(pkt);
	tx_req->size = (uint16_t) pkt->len;
	tx_req->flags  = (pkt->flags & UK_NETBUF_F_PARTIAL_CSUM)
			 ? NETTXF_csum_blank : 0x0;
	tx_req->flags |= (pkt->flags & UK_NETBUF_F_DATA_VALID)
			 ? NETTXF_data_validated : 0x0;
	tx_req->id = id;
	status = UK_NETDEV_STATUS_SUCCESS;

	txq->ring.req_prod_pvt = req_prod + 1;
	wmb(); /* Ensure backend sees requests */

	RING_PUSH_REQUESTS_AND_CHECK_NOTIFY(&txq->ring, notify);
	if (notify)
		notify_remote_via_evtchn(txq->evtchn);


	/* some cleanup */
	do {
		network_tx_buf_gc(txq);
		RING_FINAL_CHECK_FOR_RESPONSES(&txq->ring, more_to_do);
	} while (more_to_do);

	status |= (RING_FULL(&txq->ring)) ? 0x0 : UK_NETDEV_STATUS_MORE;
	local_irq_restore(flags);

	return status;
}

static int netfront_rxq_enqueue(struct uk_netdev_rx_queue *rxq,
		struct uk_netbuf *netbuf)
{
	RING_IDX req_prod;
	uint16_t id;
	netif_rx_request_t *rx_req;
	struct netfront_dev *nfdev;
	int notify;

	/* buffer must be page aligned */
	UK_ASSERT(((unsigned long) netbuf->buf & ~PAGE_MASK) == 0);

	if (unlikely(RING_FULL(&rxq->ring))) {
		uk_pr_debug("rx queue is full\n");
		return -ENOSPC;
	}

	/* get request */
	req_prod = rxq->ring.req_prod_pvt;
	id = xennet_rxidx(req_prod);
	rx_req = RING_GET_REQUEST(&rxq->ring, req_prod);
	rx_req->id = id;

	/* save buffer */
	rxq->netbuf[id] = netbuf;
	/* setup grant for buffer data */
	nfdev = rxq->netfront_dev;
	if (unlikely(rxq->gref[id] == GRANT_INVALID_REF)) {
		/* allocating of a new grant needed */
		rxq->gref[id] = gnttab_grant_access(nfdev->xendev->otherend_id,
						    virt_to_mfn(netbuf->buf),
						    0);
	} else {
		/* re-use grant (update it) */
		gnttab_update_grant(rxq->gref[id],
				    nfdev->xendev->otherend_id,
				    virt_to_mfn(netbuf->buf),
				    0);
	}
	UK_ASSERT(rxq->gref[id] != GRANT_INVALID_REF);

	rx_req->gref = rxq->gref[id];
	wmb(); /* Ensure backend sees requests */
	rxq->ring.req_prod_pvt = req_prod + 1;

	RING_PUSH_REQUESTS_AND_CHECK_NOTIFY(&rxq->ring, notify);
	if (notify)
		notify_remote_via_evtchn(rxq->evtchn);

	return 0;
}

static int netfront_rxq_dequeue(struct uk_netdev_rx_queue *rxq,
		struct uk_netbuf **netbuf)
{
	RING_IDX prod, cons;
	netif_rx_response_t *rx_rsp;
	uint16_t len, id;
	struct uk_netbuf *buf = NULL;
	int count = 0;

	UK_ASSERT(rxq != NULL);
	UK_ASSERT(netbuf != NULL);

	prod = rxq->ring.sring->rsp_prod;
	rmb(); /* Ensure we see queued responses up to 'rp'. */
	cons = rxq->ring.rsp_cons;
	/* No new descriptor since last dequeue operation */
	if (cons == prod) {
		*netbuf = NULL;
		goto out;
	}

	/* get response */
	rx_rsp = RING_GET_RESPONSE(&rxq->ring, cons);
	UK_ASSERT(rx_rsp->status > NETIF_RSP_NULL);
	id = rx_rsp->id;
	UK_ASSERT(id < NET_RX_RING_SIZE);

	/* NOTE: we keep the last grant for re-use */

	buf = rxq->netbuf[id];
	if (unlikely(rx_rsp->status < 0)) {
		uk_pr_err("rxq %p: Receive error %d!\n", rxq, rx_rsp->status);
		buf->len = 0;
	} else {
		len = (uint16_t) rx_rsp->status;
		if (len > UK_ETH_FRAME_MAXLEN)
			len = UK_ETH_FRAME_MAXLEN;

		buf->data = (void *)((__uptr) buf->buf + rx_rsp->offset);
		buf->len = len;
		UK_ASSERT(IN_RANGE(buf->data, buf->buf, buf->buflen));

		buf->flags |= (rx_rsp->flags & NETTXF_csum_blank)
			      ? UK_NETBUF_F_PARTIAL_CSUM : 0x0;
		buf->flags |= (rx_rsp->flags & NETTXF_data_validated)
			      ? UK_NETBUF_F_DATA_VALID : 0x0;

		/* netfront does not tell us where the checksum is located */
		buf->csum_start  = 0;
		buf->csum_offset = 0;
	}

	*netbuf = buf;

	rxq->ring.rsp_cons++;
	count = 1;

out:
	return count;
}

static int netfront_rx_fillup(struct uk_netdev_rx_queue *rxq, uint16_t nb_desc)
{
	struct uk_netbuf *netbuf[nb_desc];
	int rc, status = 0;
	uint16_t cnt;

	cnt = rxq->alloc_rxpkts(rxq->alloc_rxpkts_argp, netbuf, nb_desc);

	for (uint16_t i = 0; i < cnt; i++) {
		rc = netfront_rxq_enqueue(rxq, netbuf[i]);
		if (unlikely(rc < 0)) {
			uk_pr_err("Failed to add a buffer to rx queue %p: %d\n",
				rxq, rc);

			/*
			 * Release netbufs that we are not going
			 * to use anymore
			 */
			for (uint16_t j = i; j < cnt; j++)
				uk_netbuf_free(netbuf[j]);

			status |= UK_NETDEV_STATUS_UNDERRUN;

			goto out;
		}
	}

	if (unlikely(cnt < nb_desc))
		status |= UK_NETDEV_STATUS_UNDERRUN;

out:
	return status;
}

/* Returns 1 if more packets available */
static int netfront_rxq_intr_enable(struct uk_netdev_rx_queue *rxq)
{
	int more;

	/* Check if there are no more packets enabled */
	RING_FINAL_CHECK_FOR_RESPONSES(&rxq->ring, more);
	if (!more) {
		/* No more packets, we can enable interrupts */
		rxq->intr_enabled |= NETFRONT_INTR_EN;
		unmask_evtchn(rxq->evtchn);
	}

	return (more > 0);
}

static int netfront_recv(struct uk_netdev *n __unused,
		struct uk_netdev_rx_queue *rxq,
		struct uk_netbuf **pkt)
{
	int rc, status = 0;
	int more;

	UK_ASSERT(n != NULL);
	UK_ASSERT(rxq != NULL);
	UK_ASSERT(pkt != NULL);

	/* Queue interrupts have to be off when calling receive */
	UK_ASSERT(!(rxq->intr_enabled & NETFRONT_INTR_EN));

	rc = netfront_rxq_dequeue(rxq, pkt);
	UK_ASSERT(rc >= 0);

	status |= (*pkt) ? UK_NETDEV_STATUS_SUCCESS : 0x0;
	status |= netfront_rx_fillup(rxq, rc);

	/* Enable interrupt only when user had previously enabled it */
	if (rxq->intr_enabled & NETFRONT_INTR_USR_EN_MASK) {
		/* Need to enable the interrupt on the last packet */
		rc = netfront_rxq_intr_enable(rxq);
		if (rc == 1 && !(*pkt)) {
			/**
			 * Packet arrive after reading the queue and before
			 * enabling the interrupt
			 */
			rc = netfront_rxq_dequeue(rxq, pkt);
			UK_ASSERT(rc >= 0);
			status |= UK_NETDEV_STATUS_SUCCESS;

			/*
			 * Since we received something, we need to fillup
			 * and notify
			 */
			status |= netfront_rx_fillup(rxq, rc);

			/* Need to enable the interrupt on the last packet */
			rc = netfront_rxq_intr_enable(rxq);
			status |= (rc == 1) ? UK_NETDEV_STATUS_MORE : 0x0;
		} else if (*pkt) {
			/* When we originally got a packet and there is more */
			status |= (rc == 1) ? UK_NETDEV_STATUS_MORE : 0x0;
		}
	} else if (*pkt) {
		/**
		 * For polling case, we report always there are further
		 * packets unless the queue is empty.
		 */
		RING_FINAL_CHECK_FOR_RESPONSES(&rxq->ring, more);
		status |= (more) ? UK_NETDEV_STATUS_MORE : 0x0;
	}

	return status;
}

static struct uk_netdev_tx_queue *netfront_txq_setup(struct uk_netdev *n,
		uint16_t queue_id,
		uint16_t nb_desc __unused,
		struct uk_netdev_txqueue_conf *conf)
{
	int rc;
	struct netfront_dev *nfdev;
	struct uk_netdev_tx_queue *txq;
	netif_tx_sring_t *sring;

	UK_ASSERT(n != NULL);

	nfdev = to_netfront_dev(n);
	if (queue_id >= nfdev->max_queue_pairs) {
		uk_pr_err("Invalid queue identifier: %"__PRIu16"\n", queue_id);
		return ERR2PTR(-EINVAL);
	}

	txq  = &nfdev->txqs[queue_id];
	UK_ASSERT(!txq->initialized);
	txq->netfront_dev = nfdev;
	txq->lqueue_id = queue_id;

	/* Setup shared ring */
	sring = uk_palloc(conf->a, 1);
	if (!sring)
		return ERR2PTR(-ENOMEM);
	memset(sring, 0, PAGE_SIZE);
	SHARED_RING_INIT(sring);
	FRONT_RING_INIT(&txq->ring, sring, PAGE_SIZE);
	txq->ring_size = NET_TX_RING_SIZE;
	txq->ring_ref = gnttab_grant_access(nfdev->xendev->otherend_id,
		virt_to_mfn(sring), 0);
	UK_ASSERT(txq->ring_ref != GRANT_INVALID_REF);

	/* Setup event channel */
	if (nfdev->split_evtchn || !nfdev->rxqs[queue_id].initialized) {
		rc = evtchn_alloc_unbound(nfdev->xendev->otherend_id,
				NULL, NULL,
				&txq->evtchn);
		if (rc) {
			uk_pr_err("Error creating event channel: %d\n", rc);
			gnttab_end_access(txq->ring_ref);
			uk_pfree(conf->a, sring, 1);
			return ERR2PTR(rc);
		}
	} else
		txq->evtchn = nfdev->rxqs[queue_id].evtchn;

	/* Events are always disabled for tx queue */
	mask_evtchn(txq->evtchn);

	/* Initialize list of request ids */
	for (uint16_t i = 0; i < NET_TX_RING_SIZE; i++) {
		add_id_to_freelist(i, txq->freelist);
		txq->gref[i] = GRANT_INVALID_REF;
		txq->netbuf[i] = NULL;
	}

	txq->initialized = true;
	nfdev->txqs_num++;

	return txq;
}

static void netfront_rxq_handler(evtchn_port_t port __unused,
		struct __regs *regs __unused, void *arg)
{
	struct uk_netdev_rx_queue *rxq = arg;

	/* Disable the interrupt for the ring */
	rxq->intr_enabled &= ~(NETFRONT_INTR_EN);
	mask_evtchn(rxq->evtchn);

	/* Indicate to the network stack about an event */
	uk_netdev_drv_rx_event(&rxq->netfront_dev->netdev, rxq->lqueue_id);
}

static struct uk_netdev_rx_queue *netfront_rxq_setup(struct uk_netdev *n,
		uint16_t queue_id,
		uint16_t nb_desc __unused,
		struct uk_netdev_rxqueue_conf *conf)
{
	int rc;
	struct netfront_dev *nfdev;
	struct uk_netdev_rx_queue *rxq;
	netif_rx_sring_t *sring;

	UK_ASSERT(n != NULL);
	UK_ASSERT(conf != NULL);

	nfdev = to_netfront_dev(n);
	if (queue_id >= nfdev->max_queue_pairs) {
		uk_pr_err("Invalid queue identifier: %"__PRIu16"\n", queue_id);
		return ERR2PTR(-EINVAL);
	}

	rxq = &nfdev->rxqs[queue_id];
	UK_ASSERT(!rxq->initialized);
	rxq->netfront_dev = nfdev;
	rxq->lqueue_id = queue_id;

	/* Setup shared ring */
	sring = uk_palloc(conf->a, 1);
	if (!sring)
		return ERR2PTR(-ENOMEM);
	memset(sring, 0, PAGE_SIZE);
	SHARED_RING_INIT(sring);
	FRONT_RING_INIT(&rxq->ring, sring, PAGE_SIZE);
	rxq->ring_size = NET_RX_RING_SIZE;
	rxq->ring_ref = gnttab_grant_access(nfdev->xendev->otherend_id,
		virt_to_mfn(sring), 0);
	UK_ASSERT(rxq->ring_ref != GRANT_INVALID_REF);

	/* Setup event channel */
	if (nfdev->split_evtchn || !nfdev->txqs[queue_id].initialized) {
		rc = evtchn_alloc_unbound(nfdev->xendev->otherend_id,
				netfront_rxq_handler, rxq,
				&rxq->evtchn);
		if (rc) {
			uk_pr_err("Error creating event channel: %d\n", rc);
			gnttab_end_access(rxq->ring_ref);
			uk_pfree(conf->a, sring, 1);
			return ERR2PTR(rc);
		}
	} else {
		rxq->evtchn = nfdev->txqs[queue_id].evtchn;
		/* overwriting event handler */
		bind_evtchn(rxq->evtchn, netfront_rxq_handler, rxq);
	}
	/*
	 * By default, events are disabled and it is up to the user or
	 * network stack to explicitly enable them.
	 */
	mask_evtchn(rxq->evtchn);
	rxq->intr_enabled = 0;

	rxq->alloc_rxpkts = conf->alloc_rxpkts;
	rxq->alloc_rxpkts_argp = conf->alloc_rxpkts_argp;

	for (uint16_t i = 0; i < NET_RX_RING_SIZE; i++)
		rxq->gref[i] = GRANT_INVALID_REF;

	/* Allocate receive buffers for this queue */
	netfront_rx_fillup(rxq, rxq->ring_size);

	rxq->initialized = true;
	nfdev->rxqs_num++;

	return rxq;
}

static int netfront_rxtx_alloc(struct netfront_dev *nfdev,
		const struct uk_netdev_conf *conf)
{
	int rc = 0, i;

	if (conf->nb_tx_queues != conf->nb_rx_queues) {
		uk_pr_err("Different number of queues not supported\n");
		rc = -ENOTSUP;
		goto err_free_txrx;
	}

	nfdev->max_queue_pairs =
		MIN(nfdev->max_queue_pairs, conf->nb_tx_queues);

	nfdev->txqs = uk_calloc(drv_allocator,
		nfdev->max_queue_pairs, sizeof(*nfdev->txqs));
	if (unlikely(!nfdev->txqs)) {
		uk_pr_err("Failed to allocate memory for tx queues\n");
		rc = -ENOMEM;
		goto err_free_txrx;
	}
	for (i = 0; i < nfdev->max_queue_pairs; i++)
		nfdev->txqs[i].ring_size = NET_TX_RING_SIZE;

	nfdev->rxqs = uk_calloc(drv_allocator,
		nfdev->max_queue_pairs, sizeof(*nfdev->rxqs));
	if (unlikely(!nfdev->rxqs)) {
		uk_pr_err("Failed to allocate memory for rx queues\n");
		rc = -ENOMEM;
		goto err_free_txrx;
	}
	for (i = 0; i < nfdev->max_queue_pairs; i++)
		nfdev->rxqs[i].ring_size = NET_RX_RING_SIZE;

	return rc;

err_free_txrx:
	if (!nfdev->rxqs)
		uk_free(drv_allocator, nfdev->rxqs);
	if (!nfdev->txqs)
		uk_free(drv_allocator, nfdev->txqs);

	return rc;
}

static int netfront_rx_intr_enable(struct uk_netdev *n __unused,
		struct uk_netdev_rx_queue *rxq)
{
	int rc;

	UK_ASSERT(n != NULL);
	UK_ASSERT(rxq != NULL);
	UK_ASSERT(&rxq->netfront_dev->netdev == n);

	/* If the interrupt is enabled */
	if (rxq->intr_enabled & NETFRONT_INTR_EN)
		return 0;

	/**
	 * Enable the user configuration bit. This would cause the interrupt to
	 * be enabled automatically if the interrupt could not be enabled now
	 * due to data in the queue.
	 */
	rxq->intr_enabled = NETFRONT_INTR_USR_EN;
	rc = netfront_rxq_intr_enable(rxq);
	if (!rc)
		rxq->intr_enabled |= NETFRONT_INTR_EN;

	return rc;
}

static int netfront_rx_intr_disable(struct uk_netdev *n __unused,
		struct uk_netdev_rx_queue *rxq)
{
	UK_ASSERT(n != NULL);
	UK_ASSERT(rxq != NULL);
	UK_ASSERT(&rxq->netfront_dev->netdev == n);

	rxq->intr_enabled &= ~(NETFRONT_INTR_USR_EN | NETFRONT_INTR_EN);
	mask_evtchn(rxq->evtchn);

	return 0;
}

static int netfront_txq_info_get(struct uk_netdev *n,
		uint16_t queue_id,
		struct uk_netdev_queue_info *qinfo)
{
	struct netfront_dev *nfdev;
	struct uk_netdev_tx_queue *txq;
	int rc = 0;

	UK_ASSERT(n != NULL);
	UK_ASSERT(qinfo != NULL);

	nfdev = to_netfront_dev(n);
	if (unlikely(queue_id >= nfdev->max_queue_pairs)) {
		uk_pr_err("Invalid queue_id %"__PRIu16"\n", queue_id);
		rc = -EINVAL;
		goto exit;
	}
	txq = &nfdev->txqs[queue_id];
	qinfo->nb_min = txq->ring_size;
	qinfo->nb_max = txq->ring_size;
	qinfo->nb_align = 1;
	qinfo->nb_is_power_of_two = 1;

exit:
	return rc;
}

static int netfront_rxq_info_get(struct uk_netdev *n,
		uint16_t queue_id,
		struct uk_netdev_queue_info *qinfo)
{
	struct netfront_dev *nfdev;
	struct uk_netdev_rx_queue *rxq;
	int rc = 0;

	UK_ASSERT(n != NULL);
	UK_ASSERT(qinfo != NULL);

	nfdev = to_netfront_dev(n);
	if (unlikely(queue_id >= nfdev->max_queue_pairs)) {
		uk_pr_err("Invalid queue id: %"__PRIu16"\n", queue_id);
		rc = -EINVAL;
		goto exit;
	}
	rxq = &nfdev->rxqs[queue_id];
	qinfo->nb_min = rxq->ring_size;
	qinfo->nb_max = rxq->ring_size;
	qinfo->nb_align = 1;
	qinfo->nb_is_power_of_two = 1;

exit:
	return rc;
}

static int netfront_configure(struct uk_netdev *n,
		const struct uk_netdev_conf *conf)
{
	int rc;
	struct netfront_dev *nfdev;

	UK_ASSERT(n != NULL);
	UK_ASSERT(conf != NULL);

	nfdev = to_netfront_dev(n);

	rc = netfront_rxtx_alloc(nfdev, conf);
	if (rc != 0) {
		uk_pr_err("Failed to allocate rx and tx rings %d\n", rc);
		goto out;
	}

out:
	return rc;
}

static int netfront_start(struct uk_netdev *n)
{
	struct netfront_dev *nfdev;
	int rc;

	UK_ASSERT(n != NULL);
	nfdev = to_netfront_dev(n);

	rc = netfront_xb_connect(nfdev);
	if (rc) {
		uk_pr_err("Error connecting to backend: %d\n", rc);
		return rc;
	}

	return rc;
}

static void netfront_info_get(struct uk_netdev *n,
		struct uk_netdev_info *dev_info)
{
	struct netfront_dev *nfdev;

	UK_ASSERT(n != NULL);
	UK_ASSERT(dev_info != NULL);

	nfdev = to_netfront_dev(n);
	dev_info->max_rx_queues = nfdev->max_queue_pairs;
	dev_info->max_tx_queues = nfdev->max_queue_pairs;
	dev_info->max_mtu = nfdev->mtu;
	dev_info->nb_encap_tx = 0;
	dev_info->nb_encap_rx = 0;
	dev_info->ioalign = PAGE_SIZE;
	dev_info->features = UK_NETDEV_F_RXQ_INTR | UK_NETDEV_F_PARTIAL_CSUM;
}

static const void *netfront_einfo_get(struct uk_netdev *n,
		enum uk_netdev_einfo_type einfo_type)
{
	struct netfront_dev *nfdev;

	UK_ASSERT(n != NULL);

	nfdev = to_netfront_dev(n);
	switch (einfo_type) {
	case UK_NETDEV_IPV4_ADDR:
		return nfdev->econf.ipv4addr;
	case UK_NETDEV_IPV4_MASK:
		return nfdev->econf.ipv4mask;
	case UK_NETDEV_IPV4_GW:
		return nfdev->econf.ipv4gw;
	default:
		break;
	}

	/* type not supported */
	return NULL;
}

static const struct uk_hwaddr *netfront_mac_get(struct uk_netdev *n)
{
	struct netfront_dev *nfdev;

	UK_ASSERT(n != NULL);
	nfdev = to_netfront_dev(n);
	return &nfdev->hw_addr;
}

static uint16_t netfront_mtu_get(struct uk_netdev *n)
{
	struct netfront_dev *nfdev;

	UK_ASSERT(n != NULL);
	nfdev = to_netfront_dev(n);
	return nfdev->mtu;
}

static unsigned int netfront_promisc_get(struct uk_netdev *n)
{
	struct netfront_dev *nfdev;

	UK_ASSERT(n != NULL);
	nfdev = to_netfront_dev(n);
	return nfdev->promisc;
}

static int netfront_probe(struct uk_netdev *n)
{
	struct netfront_dev *nfdev;
	int rc;

	UK_ASSERT(n != NULL);

	nfdev = to_netfront_dev(n);

	/* Xenbus initialization */
	rc = netfront_xb_init(nfdev, drv_allocator);
	if (rc) {
		uk_pr_err("Error initializing Xenbus data: %d\n", rc);
		goto out;
	}

out:
	return rc;
}

static const struct uk_netdev_ops netfront_ops = {
	.probe = netfront_probe,
	.configure = netfront_configure,
	.start = netfront_start,
	.txq_configure = netfront_txq_setup,
	.rxq_configure = netfront_rxq_setup,
	.rxq_intr_enable = netfront_rx_intr_enable,
	.rxq_intr_disable = netfront_rx_intr_disable,
	.txq_info_get = netfront_txq_info_get,
	.rxq_info_get = netfront_rxq_info_get,
	.info_get = netfront_info_get,
	.einfo_get = netfront_einfo_get,
	.hwaddr_get = netfront_mac_get,
	.mtu_get = netfront_mtu_get,
	.promiscuous_get = netfront_promisc_get,
};

static int netfront_add_dev(struct xenbus_device *xendev)
{
	struct netfront_dev *nfdev;
	int rc = 0;

	UK_ASSERT(xendev != NULL);

	nfdev = uk_calloc(drv_allocator, 1, sizeof(*nfdev));
	if (!nfdev) {
		rc = -ENOMEM;
		goto err_out;
	}

	nfdev->xendev = xendev;
	nfdev->mtu = UK_ETH_PAYLOAD_MAXLEN;
	nfdev->max_queue_pairs = 1;
	nfdev->netdev.tx_one = netfront_xmit;
	nfdev->netdev.rx_one = netfront_recv;
	nfdev->netdev.ops = &netfront_ops;
	rc = uk_netdev_drv_register(&nfdev->netdev, drv_allocator, DRIVER_NAME);
	if (rc < 0) {
		uk_pr_err("Failed to register %s device with libuknetdev\n",
			DRIVER_NAME);
		goto err_register;
	}
	nfdev->uid = rc;
	rc = 0;

out:
	return rc;
err_register:
	uk_free(drv_allocator, nfdev);
err_out:
	goto out;
}

static int netfront_drv_init(struct uk_alloc *allocator)
{
	/* driver initialization */
	if (unlikely(!allocator))
		return -EINVAL;

	drv_allocator = allocator;
	return 0;
}

static const xenbus_dev_type_t netfront_devtypes[] = {
	xenbus_dev_vif,
	xenbus_dev_none
};

static struct xenbus_driver netfront_driver = {
	.device_types = netfront_devtypes,
	.init         = netfront_drv_init,
	.add_dev      = netfront_add_dev
};
XENBUS_REGISTER_DRIVER(&netfront_driver);
