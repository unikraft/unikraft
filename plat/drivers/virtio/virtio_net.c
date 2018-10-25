/*
 * Authors: Dan Williams
 *          Martin Lucina
 *          Ricardo Koller
 *          Razvan Cojocaru <razvan.cojocaru93@gmail.com>
 *          Sharan Santhanam
 *
 * Copyright (c) 2015-2017 IBM
 * Copyright (c) 2016-2017 Docker, Inc.
 * Copyright (c) 2018, NEC Europe Ltd., NEC Corporation
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
#include <uk/assert.h>
#include <uk/essentials.h>
#include <uk/sglist.h>
#include <uk/arch/types.h>
#include <uk/arch/limits.h>
#include <uk/netbuf.h>
#include <uk/netdev.h>
#include <uk/netdev_core.h>
#include <uk/netdev_driver.h>
#include <virtio/virtio_bus.h>
#include <virtio/virtqueue.h>
#include <virtio/virtio_net.h>

/**
 * VIRTIO_PKT_BUFFER_LEN = VIRTIO_NET_HDR + ETH_HDR + ETH_PKT_PAYLOAD_LEN
 * VIRTIO_NET_HDR: 10 bytes in length in legacy mode + 2 byte of padded data.
 *		   12 bytes in length in modern mode.
 */
#define VIRTIO_HDR_LEN          12
#define ETH_HDR_LEN             14
#define ETH_PKT_PAYLOAD_LEN   1500
#define VIRTIO_PKT_BUFFER_LEN ((ETH_PKT_PAYLOAD_LEN)	\
			       + (ETH_HDR_LEN)		\
			       + (VIRTIO_HDR_LEN))

#define DRIVER_NAME           "virtio-net"


#define  VTNET_RX_HEADER_PAD (4)
#define  VTNET_INTR_EN   (1 << 0)
#define  VTNET_INTR_EN_MASK   (1)
#define  VTNET_INTR_USR_EN   (1 << 1)
#define  VTNET_INTR_USR_EN_MASK   (2)

/**
 * Define max possible fragments for the network packets.
 */
#define NET_MAX_FRAGMENTS    ((__U16_MAX >> __PAGE_SHIFT) + 2)

#define to_virtionetdev(ndev) \
	__containerof(ndev, struct virtio_net_device, netdev)

#define VIRTIO_NET_DRV_FEATURES(features)           \
	(VIRTIO_FEATURES_UPDATE(features, VIRTIO_NET_F_MAC))

typedef enum {
	VNET_RX,
	VNET_TX,
} virtq_type_t;

/**
 * @internal structure to represent the transmit queue.
 */
struct uk_netdev_tx_queue {
	/* The virtqueue reference */
	struct virtqueue *vq;
	/* The hw queue identifier */
	uint16_t hwvq_id;
	/* The user queue identifier */
	uint16_t lqueue_id;
	/* The nr. of descriptor limit */
	uint16_t max_nb_desc;
	/* The nr. of descriptor user configured */
	uint16_t nb_desc;
	/* The flag to interrupt on the transmit queue */
	uint8_t intr_enabled;
	/* Reference to the uk_netdev */
	struct uk_netdev *ndev;
	/* The scatter list and its associated fragements */
	struct uk_sglist sg;
	struct uk_sglist_seg sgsegs[NET_MAX_FRAGMENTS];
};

/**
 * @internal structure to represent the receive queue.
 */
struct uk_netdev_rx_queue {
	/* The virtqueue reference */
	struct virtqueue *vq;
	/* The virtqueue hw identifier */
	uint16_t hwvq_id;
	/* The libuknet queue identifier */
	uint16_t lqueue_id;
	/* The nr. of descriptor limit */
	uint16_t max_nb_desc;
	/* The nr. of descriptor user configured */
	uint16_t nb_desc;
	/* The flag to interrupt on the transmit queue */
	uint8_t intr_enabled;
	/* Reference to the uk_netdev */
	struct uk_netdev *ndev;
	/* The scatter list and its associated fragements */
	struct uk_sglist sg;
	struct uk_sglist_seg sgsegs[NET_MAX_FRAGMENTS];
};

struct virtio_net_device {
	/* Virtio Device */
	struct virtio_dev *vdev;
	/* List of all the virtqueue in the pci device */
	struct virtqueue *vq;
	struct uk_netdev netdev;
	/* Count of the number of the virtqueues */
	__u16 max_vqueue_pairs;
	/* List of the Rx/Tx queue */
	__u16    rx_vqueue_cnt;
	struct   uk_netdev_rx_queue *rxqs;
	__u16    tx_vqueue_cnt;
	struct   uk_netdev_tx_queue *txqs;
	/* The netdevice identifier */
	__u16 uid;
	/* The max mtu */
	__u16 max_mtu;
	/* The mtu */
	__u16 mtu;
	/* The hw address of the netdevice */
	struct uk_hwaddr hw_addr;
	/*  Netdev state */
	__u8 state;
	/* RX promiscuous mode. */
	__u8 promisc : 1;
};

/**
 * Static function declarations.
 */
static int virtio_net_drv_init(struct uk_alloc *drv_allocator);
static int virtio_net_add_dev(struct virtio_dev *vdev);
static void virtio_net_info_get(struct uk_netdev *dev,
				struct uk_netdev_info *dev_info);
static inline void virtio_netdev_feature_set(struct virtio_net_device *vndev);
static int virtio_netdev_configure(struct uk_netdev *n,
				   const struct uk_netdev_conf *conf);
static struct uk_netdev_tx_queue *virtio_netdev_tx_queue_setup(
					struct uk_netdev *n, uint16_t queue_id,
					uint16_t nb_desc,
					struct uk_netdev_txqueue_conf *conf);
static struct uk_netdev_rx_queue *virtio_netdev_rx_queue_setup(
					struct uk_netdev *n,
					uint16_t queue_id, uint16_t nb_desc,
					struct uk_netdev_rxqueue_conf *conf);
static int virtio_net_rx_intr_disable(struct uk_netdev *n,
				      struct uk_netdev_rx_queue *queue);
static int virtio_net_rx_intr_enable(struct uk_netdev *n,
				     struct uk_netdev_rx_queue *queue);
static int virtio_netdev_xmit(struct uk_netdev *dev,
			      struct uk_netdev_tx_queue *queue,
			      struct uk_netbuf *pkt);
static int virtio_netdev_recv(struct uk_netdev *dev,
			      struct uk_netdev_rx_queue *queue,
			      struct uk_netbuf **pkt,
			      struct uk_netbuf *fillup[],
			      uint16_t *fillup_count);
static const struct uk_hwaddr *virtio_net_mac_get(struct uk_netdev *n);
static __u16 virtio_net_mtu_get(struct uk_netdev *n);
static unsigned virtio_net_promisc_get(struct uk_netdev *n);
static int virtio_netdev_rxq_info_get(struct uk_netdev *dev, __u16 queue_id,
				      struct uk_netdev_queue_info *qinfo);
static int virtio_netdev_txq_info_get(struct uk_netdev *dev, __u16 queue_id,
				      struct uk_netdev_queue_info *qinfo);

/**
 * Static global constants
 */
static const char *drv_name = DRIVER_NAME;
static struct uk_alloc *a;

/**
 * The Driver method implementation.
 */
static int virtio_netdev_xmit(struct uk_netdev *dev,
			      struct uk_netdev_tx_queue *queue,
			      struct uk_netbuf *pkt)
{
	int rc = 0;

	UK_ASSERT(dev);
	UK_ASSERT(pkt && queue);

	return rc;
}

static int virtio_netdev_recv(struct uk_netdev *dev __unused,
			      struct uk_netdev_rx_queue *queue,
			      struct uk_netbuf **pkt __unused,
			      struct uk_netbuf *fillup[],
			      uint16_t *fillup_count)
{
	int rc = 0;

	UK_ASSERT(dev && queue);
	UK_ASSERT(!fillup || (fillup && *fillup_count > 0));

	return rc;
}

static struct uk_netdev_rx_queue *virtio_netdev_rx_queue_setup(
				struct uk_netdev *n, uint16_t queue_id __unused,
				uint16_t nb_desc __unused,
				struct uk_netdev_rxqueue_conf *conf __unused)
{
	struct uk_netdev_rx_queue *rxq = NULL;

	UK_ASSERT(n);

	return rxq;
}

static struct uk_netdev_tx_queue *virtio_netdev_tx_queue_setup(
				struct uk_netdev *n, uint16_t queue_id __unused,
				uint16_t nb_desc __unused,
				struct uk_netdev_txqueue_conf *conf __unused)
{
	struct uk_netdev_tx_queue *txq = NULL;

	UK_ASSERT(n);
	return txq;
}

static int virtio_netdev_rxq_info_get(struct uk_netdev *dev,
				      __u16 queue_id __unused,
				      struct uk_netdev_queue_info *qinfo)
{
	UK_ASSERT(dev);
	UK_ASSERT(qinfo);
	return 0;
}

static int virtio_netdev_txq_info_get(struct uk_netdev *dev,
				      __u16 queue_id __unused,
				      struct uk_netdev_queue_info *qinfo)
{
	UK_ASSERT(dev);
	UK_ASSERT(qinfo);
	return 0;
}

static unsigned virtio_net_promisc_get(struct uk_netdev *n)
{
	UK_ASSERT(n);
	return 0;
}

static const struct uk_hwaddr *virtio_net_mac_get(struct uk_netdev *n)
{
	UK_ASSERT(n);
	return NULL;
}

static __u16 virtio_net_mtu_get(struct uk_netdev *n)
{
	UK_ASSERT(n);
	return 0;
}

static int virtio_netdev_configure(struct uk_netdev *n,
				   const struct uk_netdev_conf *conf __unused)
{
	int rc = 0;

	UK_ASSERT(n);

	return rc;
}

static int virtio_net_rx_intr_enable(struct uk_netdev *n,
				     struct uk_netdev_rx_queue *queue __unused)
{
	int rc = 0;

	UK_ASSERT(n);

	return rc;
}

static int virtio_net_rx_intr_disable(struct uk_netdev *n,
				      struct uk_netdev_rx_queue *queue __unused)
{
	UK_ASSERT(n);
	return 0;
}

static void virtio_net_info_get(struct uk_netdev *dev,
				struct uk_netdev_info *dev_info)
{
	struct virtio_net_device *vndev;

	UK_ASSERT(dev && dev_info);
	vndev = to_virtionetdev(dev);

	dev_info->max_rx_queues = vndev->max_vqueue_pairs;
	dev_info->max_tx_queues = vndev->max_vqueue_pairs;
}

static int virtio_net_start(struct uk_netdev *n)
{
	UK_ASSERT(n != NULL);
	return 0;
}

static inline void virtio_netdev_feature_set(struct virtio_net_device *vndev)
{
	vndev->vdev->features = 0;
	/* Setting the feature the driver support */
	VIRTIO_NET_DRV_FEATURES(vndev->vdev->features);
	/**
	 * TODO:
	 * Adding multiqueue support for the virtio net driver.
	 */
	vndev->max_vqueue_pairs = 1;
}

static const struct uk_netdev_ops virtio_netdev_ops = {
	.configure = virtio_netdev_configure,
	.rxq_configure = virtio_netdev_rx_queue_setup,
	.txq_configure = virtio_netdev_tx_queue_setup,
	.start = virtio_net_start,
	.rxq_intr_enable = virtio_net_rx_intr_enable,
	.rxq_intr_disable = virtio_net_rx_intr_disable,
	.info_get = virtio_net_info_get,
	.promiscuous_get = virtio_net_promisc_get,
	.hwaddr_get = virtio_net_mac_get,
	.mtu_get = virtio_net_mtu_get,
	.txq_info_get = virtio_netdev_txq_info_get,
	.rxq_info_get = virtio_netdev_rxq_info_get,
};

static int virtio_net_add_dev(struct virtio_dev *vdev)
{
	struct virtio_net_device *vndev;
	int rc = 0;

	UK_ASSERT(vdev != NULL);

	vndev = uk_malloc(a, sizeof(*vndev));
	if (!vndev) {
		rc = -ENOMEM;
		goto err_out;
	}
	vndev->vdev = vdev;
	/* register netdev */
	vndev->netdev.rx_one = virtio_netdev_recv;
	vndev->netdev.tx_one = virtio_netdev_xmit;
	vndev->netdev.ops = &virtio_netdev_ops;

	rc = uk_netdev_drv_register(&vndev->netdev, a, drv_name);
	if (rc < 0) {
		uk_pr_err("Failed to register virtio-net device with libuknet\n");
		goto err_netdev_data;
	}
	vndev->uid = rc;
	rc = 0;
	vndev->max_mtu = ETH_PKT_PAYLOAD_LEN;
	vndev->mtu = vndev->max_mtu;
	vndev->promisc = 0;
	virtio_netdev_feature_set(vndev);
	uk_pr_info("virtio-net device registered with libuknet\n");

exit:
	return rc;
err_netdev_data:
	uk_free(a, vndev);
err_out:
	goto exit;
}

static int virtio_net_drv_init(struct uk_alloc *drv_allocator)
{
	/* driver initialization */
	if (!drv_allocator)
		return -EINVAL;

	a = drv_allocator;
	return 0;
}

static const struct virtio_dev_id vnet_dev_id[] = {
	{VIRTIO_ID_NET},
	{VIRTIO_ID_INVALID} /* List Terminator */
};

static struct virtio_driver vnet_drv = {
	.dev_ids = vnet_dev_id,
	.init    = virtio_net_drv_init,
	.add_dev = virtio_net_add_dev
};
VIRTIO_BUS_REGISTER_DRIVER(&vnet_drv);
