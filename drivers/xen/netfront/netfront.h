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

#ifndef __NETFRONT_H__
#define __NETFRONT_H__

#include <uk/netdev.h>
#include <uk/netbuf.h>
#include <uk/semaphore.h>
#include <xen/io/netif.h>
#include <common/gnttab.h>
#include <common/events.h>
#if defined(__aarch64__)
#include <xen-arm/mm.h>
#endif


#define NET_TX_RING_SIZE __CONST_RING_SIZE(netif_tx, PAGE_SIZE)
#define NET_RX_RING_SIZE __CONST_RING_SIZE(netif_rx, PAGE_SIZE)

/**
 * internal structure to represent the transmit queue.
 */
struct uk_netdev_tx_queue {
	/* The netfront device */
	struct netfront_dev *netfront_dev;
	/* The libuknet queue identifier */
	uint16_t lqueue_id;
	/* True if initialized */
	bool initialized;

	/* Shared ring size */
	uint16_t ring_size;
	/* Shared ring */
	netif_tx_front_ring_t ring;
	/* Shared ring grant ref */
	grant_ref_t ring_ref;
	/* Queue event channel */
	evtchn_port_t evtchn;

	/* Free list of transmitting request IDs */
	uint16_t freelist[NET_TX_RING_SIZE + 1];
	/* Ring of inflight netbufs */
	struct uk_netbuf *nbuf[NET_TX_RING_SIZE];
	/* Grants for transmit buffers */
	grant_ref_t gref[NET_TX_RING_SIZE];
	/* Transmit packets addresses */
	struct uk_netbuf *netbuf[NET_TX_RING_SIZE];
};

/**
 * internal structure to represent the receive queue.
 */
struct uk_netdev_rx_queue {
	/* The netfront device */
	struct netfront_dev *netfront_dev;
	/* The libuknet queue identifier */
	uint16_t lqueue_id;
	/* True if initialized */
	bool initialized;

	/* Shared ring size */
	uint16_t ring_size;
	/* Shared ring */
	netif_rx_front_ring_t ring;
	/* Shared ring grant ref */
	grant_ref_t ring_ref;
	/* Queue event channel */
	evtchn_port_t evtchn;

	/* The flag to interrupt on the transmit queue */
	uint8_t intr_enabled;

	/* User-provided receive buffer allocation function */
	uk_netdev_alloc_rxpkts alloc_rxpkts;
	void *alloc_rxpkts_argp;

	/* Receive buffers for incoming packets */
	struct uk_netbuf *netbuf[NET_RX_RING_SIZE];
	/* Grants for receive buffers */
	grant_ref_t gref[NET_RX_RING_SIZE];
};

struct xs_econf {
	char *ipv4addr;
	char *ipv4mask;
	char *ipv4gw;
};

struct netfront_dev {
	/* Xenbus device */
	struct xenbus_device *xendev;
	/* Network device */
	struct uk_netdev netdev;

	/* List of the Rx/Tx queues */
	uint16_t txqs_num;
	uint16_t rxqs_num;
	struct uk_netdev_tx_queue *txqs;
	struct uk_netdev_rx_queue *rxqs;
	/* Maximum number of queue pairs */
	uint16_t  max_queue_pairs;
	/* True if using split event channels */
	bool split_evtchn;

	/* Configuration parameters */
	struct xs_econf econf;

	/* The netdevice identifier */
	uint16_t uid;
	/* The mtu */
	uint16_t mtu;
	/* The hw address of the netdevice */
	struct uk_hwaddr hw_addr;
	/* RX promiscuous mode. */
	uint8_t promisc : 1;
};

#endif /* __NETFRONT_H__ */
