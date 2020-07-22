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
#include <stdio.h>
#include <string.h>
#include <uk/alloc.h>
#include <uk/arch/types.h>
#include <uk/netdev_core.h>
#include <uk/netdev_driver.h>
#include <uk/netbuf.h>
#include <uk/errptr.h>
#include <uk/libparam.h>
#include <uk/bus.h>
#include <tap/tap.h>

/**
 * The tap driver is supported only on the linuxu platform. Since the driver is
 * part of the common codebase we add compiler guard to not include the tap
 * driver from other platforms.
 */
#ifdef CONFIG_PLAT_LINUXU
#include <linuxu/tap.h>
#else
#error "The driver is supported on linuxu platform"
#endif /* CONFIG_PLAT_LINUXU */

#define DRIVER_NAME             "tap-net"

#define ETH_PKT_PAYLOAD_LEN       1500

/**
 * TODO: Find a better way of forwarding the command line argument to the
 * driver. For now they are defined as macros from this driver.
 */

#define to_tapnetdev(dev) \
		__containerof(dev, struct tap_net_dev, ndev)

struct uk_netdev_tx_queue {
	/* tx queue identifier */
	int queue_id;
};

struct uk_netdev_rx_queue {
	/* rx queue identifier */
	int queue_id;
};

struct tap_net_dev {
	/* Net device structure */
	struct uk_netdev    ndev;
	/* max number of queues */
	__u16 max_qpairs;
	/* Number of rxq configured */
	__u16 rxq_cnt;
	/* List of rx queues */
	UK_TAILQ_HEAD(tap_rxqs, struct uk_netdev_rx_queue) rxqs;
	/* Number of txq configured */
	__u16 txq_cnt;
	/* List of tx queues */
	UK_TAILQ_HEAD(tap_txqs, struct uk_netdev_tx_queue) txqs;
	/* The list of the tap device */
	UK_TAILQ_ENTRY(struct tap_net_dev) next;
	/* Mac address of the device */
	struct uk_hwaddr hw_addr;
	/* Tap Device identifier */
	__u16 tid;
	/* UK Netdevice identifier */
	__u16 id;
	/* File Descriptor for the tap device */
	int tap_fd;
	/* Control socket descriptor */
	int ctrl_sock;
	/* Name of the character device */
	char name[IFNAMSIZ];
	/* MTU of the device */
	__u16  mtu;
	/* RX promiscuous mode */
	__u8 promisc : 1;
	/* State of the net device */
	__u8 state;
};

struct tap_net_drv {
	/* allocator to initialize the driver data structure */
	struct uk_alloc *a;
	/* list of tap device */
	UK_TAILQ_HEAD(tdev_list, struct tap_net_dev) tap_dev_list;
	/* Number of tap devices */
	__u16 tap_dev_cnt;
	/* A list of bridges associated with the bridge */
	char **bridge_ifs;
};

/**
 * Module level variables
 */
static struct tap_net_drv tap_drv = {0};
static const char *drv_name = DRIVER_NAME;
static int tap_dev_cnt;
static char *bridgenames;

/**
 * Module Parameters.
 */
/**
 * tap.tap_dev_cnt=<# of tap device>
 */
UK_LIB_PARAM(tap_dev_cnt, __u32);
/**
 * tap.bridgenames="br0 br1 ... brn"
 */
UK_LIB_PARAM_STR(bridgenames);

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
static int tap_device_create(struct tap_net_dev *tdev, __u32 feature_flags);
static int tap_mac_generate(__u8 *addr, __u8 dev_id);

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
	dev_info->max_rx_queues = 1;
	dev_info->max_tx_queues = 1;
	dev_info->nb_encap_tx = 0;
	dev_info->nb_encap_rx = 0;
	dev_info->features = 0;
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
	struct tap_net_dev *tdev;

	UK_ASSERT(n);
	tdev = to_tapnetdev(n);
	return &tdev->hw_addr;
}

static int tap_netdev_mac_set(struct uk_netdev *n,
			      const struct uk_hwaddr *hwaddr)
{
	int rc = 0;
	struct tap_net_dev *tdev;
	struct uk_ifreq ifrq = {0};

	UK_ASSERT(n && hwaddr);
	tdev = to_tapnetdev(n);

	snprintf(ifrq.ifr_name, sizeof(ifrq.ifr_name), "%s", tdev->name);
	uk_pr_info("Setting mac address on tap device %s\n", tdev->name);

#ifdef CONFIG_TAP_DEV_DEBUG
	int  i;

	for (i = 0; i < UK_NETDEV_HWADDR_LEN; i++) {
		uk_pr_debug("hw_address: %d - %d\n", i,
			    hwaddr->addr_bytes[i] & 0xFF);
	}
#endif /* CONFIG_TAP_DEV_DEBUG */

	ifrq.ifr_hwaddr.sa_family = AF_LOCAL;
	memcpy(&ifrq.ifr_hwaddr.sa_data[0], &hwaddr->addr_bytes[0],
		UK_NETDEV_HWADDR_LEN);
	rc = tap_netif_configure(tdev->ctrl_sock, UK_SIOCSIFHWADDR, &ifrq);
	if (rc < 0) {
		uk_pr_err(DRIVER_NAME": Failed(%d) to set the hardware address\n",
			  rc);
		goto exit;
	}
	memcpy(&tdev->hw_addr, hwaddr, sizeof(*hwaddr));

exit:
	return rc;
}

static int tap_mac_generate(__u8 *addr, __u8 dev_id)
{
	const char fmt[] = {0x2, 0x0, 0x0, 0x0, 0x0, 0x0};

	UK_ASSERT(addr);

	memcpy(addr, fmt, UK_NETDEV_HWADDR_LEN - 1);
	*(addr + UK_NETDEV_HWADDR_LEN - 1) = (__u8) (dev_id + 1);
	return 0;
}

static inline int tapdev_ctrlsock_create(struct tap_net_dev *tdev)
{
	int rc = 0;

	rc = tap_netif_create();
	if (rc < 0) {
		uk_pr_err(DRIVER_NAME":Failed(%d) to create a control socket\n",
			  rc);
		goto exit;
	}
	tdev->ctrl_sock = rc;
	rc = 0;
exit:
	return rc;
}

static int tap_netdev_configure(struct uk_netdev *n,
				const struct uk_netdev_conf *conf)
{
	int rc = 0;
	struct tap_net_dev *tdev = NULL;
	__u32 feature_flag = 0;

	UK_ASSERT(n && conf);
	tdev = to_tapnetdev(n);

	if (conf->nb_rx_queues > tdev->max_qpairs
	    || conf->nb_tx_queues > tdev->max_qpairs) {
		uk_pr_err(DRIVER_NAME": rx-queue:%d, tx-queue:%d not supported",
			  conf->nb_rx_queues, conf->nb_tx_queues);
		return -ENOTSUP;
	} else if (conf->nb_rx_queues > 1 || conf->nb_tx_queues > 1)
		/**
		 * TODO:
		 * We don't support multi-queues on the uknetdev. Might need to
		 * revisit this when implementing multi-queue support on
		 * uknetdev
		 */
		feature_flag |= UK_IFF_MULTI_QUEUE;

	/* Open the device and configure the tap interface */
	rc = tap_device_create(tdev, feature_flag);
	if (rc < 0) {
		uk_pr_err(DRIVER_NAME": Failed to configure the tap device\n");
		goto exit;
	}

	/* Create a control socket for the network interface */
	rc = tapdev_ctrlsock_create(tdev);
	if (rc != 0) {
		uk_pr_err(DRIVER_NAME": Failed to create a control socket\n");
		goto close_tap_dev;
	}

	/* Generate MAC address */
	tap_mac_generate(&tdev->hw_addr.addr_bytes[0],
			 tdev->id);

	/* MAC Address configuration */
	rc = tap_netdev_mac_set(n, &tdev->hw_addr);
	if (rc < 0) {
		uk_pr_err(DRIVER_NAME": Failed to set the mac address\n");
		goto close_ctrl_sock;
	}

	/* Initialize the tx/rx queues */
	UK_TAILQ_INIT(&tdev->rxqs);
	tdev->rxq_cnt = 0;
	UK_TAILQ_INIT(&tdev->txqs);
	tdev->txq_cnt = 0;
exit:
	return rc;

close_ctrl_sock:
	tap_close(tdev->ctrl_sock);
close_tap_dev:
	tap_close(tdev->tap_fd);
	goto exit;
}

static int tap_device_create(struct tap_net_dev *tdev, __u32 feature_flags)
{
	int rc = 0;
	struct uk_ifreq ifreq = {0};

	/* Open the tap device */
	rc = tap_open(O_RDWR | O_NONBLOCK);
	if (rc < 0) {
		uk_pr_err(DRIVER_NAME": Failed(%d) to open the tap device\n",
			  rc);
		return rc;
	}

	tdev->tap_fd = rc;

	rc = tap_dev_configure(tdev->tap_fd, feature_flags, &ifreq);
	if (rc < 0) {
		uk_pr_err(DRIVER_NAME": Failed to setup the tap device\n");
		goto close_tap;
	}

	snprintf(tdev->name, sizeof(tdev->name), "%s", ifreq.ifr_name);
	uk_pr_info(DRIVER_NAME": Configured tap device %s\n", tdev->name);

exit:
	return rc;
close_tap:
	tap_close(tdev->tap_fd);
	tdev->tap_fd = -1;
	goto exit;
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
 * Registering the network device.
 */
static int tap_dev_init(int id)
{
	struct tap_net_dev *tdev;
	int rc = 0;

	tdev = uk_zalloc(tap_drv.a, sizeof(*tdev));
	if (!tdev) {
		uk_pr_err(DRIVER_NAME": Failed to allocate tap_device\n");
		rc = -ENOMEM;
		goto exit;
	}
	tdev->ndev.rx_one = tap_netdev_recv;
	tdev->ndev.tx_one = tap_netdev_xmit;
	tdev->ndev.ops = &tap_netdev_ops;
	tdev->tid = id;
	/**
	 * TODO:
	 * As an initial implementation we have limit on the number of queues.
	 */
	tdev->max_qpairs = 1;

	/* Registering the tap device with libuknet*/
	rc = uk_netdev_drv_register(&tdev->ndev, tap_drv.a, drv_name);
	if (rc < 0) {
		uk_pr_err(DRIVER_NAME": Failed to register the network device\n");
		goto free_tdev;
	}
	tdev->id = rc;
	rc = 0;
	tdev->mtu = ETH_PKT_PAYLOAD_LEN;
	tdev->promisc = 0;
	uk_pr_info(DRIVER_NAME": device(%d) registered with the libuknet\n",
		   tdev->id);

	/* Adding the list of devices maintained by this driver */
	UK_TAILQ_INSERT_TAIL(&tap_drv.tap_dev_list, tdev, next);
exit:
	return rc;
free_tdev:
	uk_free(tap_drv.a, tdev);
	goto exit;
}

/**
 * Register a tap driver as bus. Currently in Unikraft, the uk_bus interface
 * provides the necessary to provide callbacks for bring a pseudo device. In the
 * future we might provide interface to support the pseudo device.
 */
static int tap_drv_probe(void)
{
	int i;
	int rc = 0;
	char *idx = NULL, *prev_idx;

	if (tap_dev_cnt > 0) {
		tap_drv.bridge_ifs = uk_calloc(tap_drv.a, tap_dev_cnt,
					       sizeof(char *));
		if (!tap_drv.bridge_ifs) {
			uk_pr_err(DRIVER_NAME": Failed to allocate brigde_ifs\n");
			return -ENOMEM;
		}
	}

	idx = bridgenames;
	for (i = 0; i < tap_dev_cnt; i++) {
		if (idx) {
			prev_idx = idx;
			idx = strchr(idx, ' ');
			if (idx) {
				*idx = '\0';
				idx++;
			}
			tap_drv.bridge_ifs[i] = prev_idx;
			uk_pr_debug(DRIVER_NAME": Adding bridge %s\n",
				    prev_idx);
		} else {
			uk_pr_warn(DRIVER_NAME": Adding tap device %d without bridge\n",
				   i);
			tap_drv.bridge_ifs[i] = NULL;
		}

		rc = tap_dev_init(i);
		if (rc < 0) {
			uk_pr_err(DRIVER_NAME": Failed to initialize the tap dev id: %d\n",
				  i);
			return rc;
		}
		tap_drv.tap_dev_cnt++;
	}
	return 0;
}

static int tap_drv_init(struct uk_alloc *_a)
{
	tap_drv.a = _a;
	UK_TAILQ_INIT(&tap_drv.tap_dev_list);
	return 0;
}

static struct uk_bus tap_bus = {
	.init = tap_drv_init,
	.probe = tap_drv_probe,
};
UK_BUS_REGISTER(&tap_bus);
