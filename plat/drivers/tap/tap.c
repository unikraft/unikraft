/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Sharan Santhanam <sharan.santhanam@neclab.eu>
 *
 * Copyright (c) 2020, NEC Europe Ltd., NEC Corporation. All rights reserved.
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
 */
#include <errno.h>
#include <uk/alloc.h>
#include <uk/arch/types.h>
#include <uk/netdev_core.h>
#include <uk/netdev_driver.h>
#include <uk/netbuf.h>
#include <uk/errptr.h>
#include <uk/bus.h>

struct tap_net_drv {
	/* allocator to initialize the driver data structure */
	struct uk_alloc *a;
};

/**
 * Module level variables
 */
static struct tap_net_drv tap_drv = {0};

/**
 * Module functions
 */
static int tap_netdev_xmit(struct uk_netdev *dev,
			   struct uk_netdev_tx_queue *queue,
			   struct uk_netbuf *pkt);
static int tap_netdev_recv(struct uk_netdev *dev,
			   struct uk_netdev_rx_queue *queue,
			   struct uk_netbuf **pkt);
static struct uk_netdev_rx_queue *tap_netdev_rxq_setup(struct uk_netdev *dev,
					__u16 queue_id, __u16 nb_desc,
					struct uk_netdev_rxqueue_conf *conf);
static struct uk_netdev_tx_queue *tap_netdev_txq_setup(struct uk_netdev *dev,
					__u16 queue_id, __u16 nb_desc,
					struct uk_netdev_txqueue_conf *conf);
static int tap_netdev_configure(struct uk_netdev *n,
				const struct uk_netdev_conf *conf);
static int tap_netdev_start(struct uk_netdev *n);
static const struct uk_hwaddr *tap_netdev_mac_get(struct uk_netdev *n);
static int tap_netdev_mac_set(struct uk_netdev *n,
			      const struct uk_hwaddr *hwaddr);
static __u16 tap_netdev_mtu_get(struct uk_netdev *n);
static int tap_netdev_mtu_set(struct uk_netdev *n,  __u16 mtu);
static unsigned int tap_netdev_promisc_get(struct uk_netdev *n);
static void tap_netdev_info_get(struct uk_netdev *dev,
				struct uk_netdev_info *dev_info);
static int tap_netdev_rxq_info_get(struct uk_netdev *dev, __u16 queue_id,
				   struct uk_netdev_queue_info *qinfo);
static int tap_netdev_txq_info_get(struct uk_netdev *dev, __u16 queue_id,
				   struct uk_netdev_queue_info *qinfo);

/**
 * Local function definitions
 */

static int tap_netdev_recv(struct uk_netdev *dev,
			   struct uk_netdev_rx_queue *queue,
			   struct uk_netbuf **pkt)
{
	int rc = -EINVAL;

	UK_ASSERT(dev);
	UK_ASSERT(queue && pkt);

	return rc;
}

static int tap_netdev_xmit(struct uk_netdev *dev,
			   struct uk_netdev_tx_queue *queue,
			   struct uk_netbuf *pkt)
{
	int rc = -EINVAL;

	UK_ASSERT(dev);
	UK_ASSERT(queue && pkt);

	return rc;
}

static int tap_netdev_txq_info_get(struct uk_netdev *dev __unused,
				   __u16 queue_id __unused,
				   struct uk_netdev_queue_info *qinfo __unused)
{
	return -EINVAL;
}

static int tap_netdev_rxq_info_get(struct uk_netdev *dev __unused,
				   __u16 queue_id __unused,
				   struct uk_netdev_queue_info *qinfo __unused)
{
	return -EINVAL;
}

static struct uk_netdev_rx_queue *tap_netdev_rxq_setup(struct uk_netdev *dev,
						       __u16 queue_id __unused,
						       __u16 nb_desc __unused,
					struct uk_netdev_rxqueue_conf *conf)
{
	int rc = -EINVAL;
	struct uk_netdev_rx_queue *rxq = NULL;

	UK_ASSERT(dev && conf);

	rxq = ERR2PTR(rc);
	return rxq;
}

static struct uk_netdev_tx_queue *tap_netdev_txq_setup(struct uk_netdev *dev,
						       __u16 queue_id __unused,
						       __u16 nb_desc __unused,
					struct uk_netdev_txqueue_conf *conf)
{
	int rc = -EINVAL;
	struct uk_netdev_tx_queue *txq = NULL;

	UK_ASSERT(dev && conf);

	txq = ERR2PTR(rc);
	return txq;
}

static int tap_netdev_start(struct uk_netdev *n)
{
	int rc = -EINVAL;

	UK_ASSERT(n);
	return rc;
}

static void tap_netdev_info_get(struct uk_netdev *dev __unused,
				struct uk_netdev_info *dev_info)
{
	UK_ASSERT(dev_info);
}

static unsigned int tap_netdev_promisc_get(struct uk_netdev *n)
{

	UK_ASSERT(n);

	return 0;
}

static __u16 tap_netdev_mtu_get(struct uk_netdev *n)
{
	UK_ASSERT(n);
	return 0;
}

static int tap_netdev_mtu_set(struct uk_netdev *n,  __u16 mtu __unused)
{
	int rc = -EINVAL;

	UK_ASSERT(n);

	return rc;
}

static const struct uk_hwaddr *tap_netdev_mac_get(struct uk_netdev *n)
{
	UK_ASSERT(n);
	return NULL;
}

static int tap_netdev_mac_set(struct uk_netdev *n,
			      const struct uk_hwaddr *hwaddr)
{
	int rc = -EINVAL;

	UK_ASSERT(n && hwaddr);
	return rc;
}

static int tap_netdev_configure(struct uk_netdev *n,
				const struct uk_netdev_conf *conf)
{
	int rc = -EINVAL;

	UK_ASSERT(n && conf);
	return rc;
}

static const struct uk_netdev_ops tap_netdev_ops = {
	.configure = tap_netdev_configure,
	.rxq_configure = tap_netdev_rxq_setup,
	.txq_configure = tap_netdev_txq_setup,
	.start = tap_netdev_start,
	.info_get = tap_netdev_info_get,
	.promiscuous_get = tap_netdev_promisc_get,
	.hwaddr_get = tap_netdev_mac_get,
	.hwaddr_set = tap_netdev_mac_set,
	.mtu_get = tap_netdev_mtu_get,
	.mtu_set = tap_netdev_mtu_set,
	.txq_info_get = tap_netdev_txq_info_get,
	.rxq_info_get = tap_netdev_rxq_info_get,
};

/**
 * Register a tap driver as bus. Currently in Unikraft, the uk_bus interface
 * provides the necessary to provide callbacks for bring a pseudo device. In the
 * future we might provide interface to support the pseudo device.
 */
static int tap_drv_probe(void)
{
	return 0;
}

static int tap_drv_init(struct uk_alloc *_a)
{
	tap_drv.a = _a;
	return 0;
}

static struct uk_bus tap_bus = {
	.init = tap_drv_init,
	.probe = tap_drv_probe,
};
UK_BUS_REGISTER(&tap_bus);
