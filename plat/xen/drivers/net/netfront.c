/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Costin Lupu <costin.lupu@cs.pub.ro>
 *          Razvan Cojocaru <razvan.cojocaru93@gmail.com>
 *
 * Copyright (c) 2020, University Politehnica of Bucharest. All rights reserved.
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
#include <xen-x86/mm.h>
#include <xenbus/xenbus.h>
#include "netfront.h"
#include "netfront_xb.h"


#define DRIVER_NAME  "xen-netfront"

#define to_netfront_dev(dev) \
	__containerof(dev, struct netfront_dev, netdev)

static struct uk_alloc *drv_allocator;


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
	rc = evtchn_alloc_unbound(nfdev->xendev->otherend_id,
			NULL, NULL,
			&txq->evtchn);
	if (rc) {
		uk_pr_err("Error creating event channel: %d\n", rc);
		gnttab_end_access(txq->ring_ref);
		uk_pfree(conf->a, sring, 1);
		return ERR2PTR(-rc);
	}
	/* Events are always disabled for tx queue */
	mask_evtchn(txq->evtchn);

	/* Initialize list of request ids */
	uk_semaphore_init(&txq->sem, NET_TX_RING_SIZE);
	for (uint16_t i = 0; i < NET_TX_RING_SIZE; i++) {
		add_id_to_freelist(i, txq->freelist);
		txq->gref[i] = GRANT_INVALID_REF;
		txq->netbuf[i] = NULL;
	}

	txq->initialized = true;
	nfdev->txqs_num++;

	return txq;
}

static int netfront_rxtx_alloc(struct netfront_dev *nfdev,
		const struct uk_netdev_conf *conf)
{
	int rc = 0;

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

	nfdev->rxqs = uk_calloc(drv_allocator,
		nfdev->max_queue_pairs, sizeof(*nfdev->rxqs));
	if (unlikely(!nfdev->rxqs)) {
		uk_pr_err("Failed to allocate memory for rx queues\n");
		rc = -ENOMEM;
		goto err_free_txrx;
	}

	return rc;

err_free_txrx:
	if (!nfdev->rxqs)
		uk_free(drv_allocator, nfdev->rxqs);
	if (!nfdev->txqs)
		uk_free(drv_allocator, nfdev->txqs);

	return rc;
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
	dev_info->features = UK_FEATURE_RXQ_INTR_AVAILABLE;
}

static const void *netfront_einfo_get(struct uk_netdev *n,
		enum uk_netdev_einfo_type einfo_type)
{
	struct netfront_dev *nfdev;

	UK_ASSERT(n != NULL);

	nfdev = to_netfront_dev(n);
	switch (einfo_type) {
	case UK_NETDEV_IPV4_ADDR_STR:
		return nfdev->econf.ipv4addr;
	case UK_NETDEV_IPV4_MASK_STR:
		return nfdev->econf.ipv4mask;
	case UK_NETDEV_IPV4_GW_STR:
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

static const struct uk_netdev_ops netfront_ops = {
	.configure = netfront_configure,
	.txq_configure = netfront_txq_setup,
	.txq_info_get = netfront_txq_info_get,
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

	/* Xenbus initialization */
	rc = netfront_xb_init(nfdev, drv_allocator);
	if (rc) {
		uk_pr_err("Error initializing Xenbus data: %d\n", rc);
		goto err_xenbus;
	}

	/* register netdev */
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
	netfront_xb_fini(nfdev, drv_allocator);
err_xenbus:
	uk_free(drv_allocator, nfdev);
err_out:
	goto out;
}

static int netfront_drv_init(struct uk_alloc *allocator)
{
	/* driver initialization */
	if (!allocator)
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
