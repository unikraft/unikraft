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
#include <uk/bitops.h>
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

#define DRIVER_NAME	"virtio-net"

/* VIRTIO_PKT_BUFFER_LEN = VIRTIO_NET_HDR + ETH_HDR + ETH_PKT_PAYLOAD_LEN */
#define VIRTIO_PKT_BUFFER_LEN(_vndev)				\
	((UK_ETH_PAYLOAD_MAXLEN) + (UK_ETH_HDR_UNTAGGED_LEN) +	\
	 (__sz)virtio_net_hdr_size(_vndev))

/**
 * Currently there is a bug where if the net buffer (of size 2048) is split
 * into more than one descriptor the VMM reports the vring as full and leading
 * to us returning an error and wasting IRQs. The buffer would be split in
 * multiple virtio descriptors if the scatter-gather list notices its span
 * crosses more than one page (i.e. the buffer is not entirely contained in
 * one page) since it is not guaranteed that the two virtually contiguous pages
 * are also contiguous physically. To avoid having this happen make sure that
 * the buffer is always contained in one page by having it aligned to half a
 * page: if the buffer is half a page in size and it is also half a page aligned
 * it is mathematically guaranteed to be entirely contained in one single
 * page.
 */
#define VIRTIO_PKT_BUFFER_ALIGN			2048

/**
 * When mergeable buffers are not negotiated, the virtio_net_hdr_padded struct
 * below is placed at the beginning of the netbuf data. Use 4 bytes of pad to
 * both keep the VirtIO header and the data non-contiguous and to keep the
 * frame's payload 4 byte aligned.
 */
#define VTNET_HDR_SIZE_PADDED(_vndev)			\
	(ALIGN_UP((__sz)virtio_net_hdr_size(_vndev), 4) + 4)

#define  VTNET_INTR_EN				UK_BIT(0)
#define  VTNET_INTR_EN_MASK			0x01
#define  VTNET_INTR_USR_EN			UK_BIT(1)
#define  VTNET_INTR_USR_EN_MASK			0x02

/**
 * Define max possible fragments for the network packets.
 */
#define NET_MAX_FRAGMENTS    ((__U16_MAX >> __PAGE_SHIFT) + 2)

#define to_virtionetdev(ndev) \
	__containerof(ndev, struct virtio_net_device, netdev)

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
	__u16 hwvq_id;
	/* The user queue identifier */
	__u16 lqueue_id;
	/* The nr. of descriptor limit */
	__u16 max_nb_desc;
	/* The nr. of descriptor user configured */
	__u16 nb_desc;
	/* The flag to interrupt on the transmit queue */
	__u8 intr_enabled;
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
	__u16 hwvq_id;
	/* The libuknet queue identifier */
	__u16 lqueue_id;
	/* The nr. of descriptor limit */
	__u16 max_nb_desc;
	/* The nr. of descriptor user configured */
	__u16 nb_desc;
	/* The flag to interrupt on the transmit queue */
	__u8 intr_enabled;
	/* User-provided receive buffer allocation function */
	uk_netdev_alloc_rxpkts alloc_rxpkts;
	void *alloc_rxpkts_argp;
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
	/* Expected amount of descriptors per buffer */
#define VIRTIO_NET_BUF_DESCR_COUNT_INLINE		1
#define VIRTIO_NET_BUF_DESCR_COUNT_SEPARATE		2
	__u8 buf_descr_count: 2;
};

/**
 * Static function declarations.
 */
static int virtio_net_drv_init(struct uk_alloc *drv_allocator);
static int virtio_net_add_dev(struct virtio_dev *vdev);
static void virtio_net_info_get(struct uk_netdev *dev,
				struct uk_netdev_info *dev_info);
static int virtio_netdev_configure(struct uk_netdev *n,
				   const struct uk_netdev_conf *conf);
static int virtio_netdev_rxtx_alloc(struct virtio_net_device *vndev,
				    const struct uk_netdev_conf *conf);
static int virtio_netdev_probe(struct uk_netdev *n);
static int virtio_netdev_feature_negotiate(struct uk_netdev *n,
					   const struct uk_netdev_conf *conf);
static struct uk_netdev_tx_queue *virtio_netdev_tx_queue_setup(
					struct uk_netdev *n, __u16 queue_id,
					__u16 nb_desc,
					struct uk_netdev_txqueue_conf *conf);
static int virtio_netdev_vqueue_setup(struct virtio_net_device *vndev,
				      __u16 queue_id, __u16 nr_desc,
				      virtq_type_t queue_type,
				      struct uk_alloc *a);
static struct uk_netdev_rx_queue *virtio_netdev_rx_queue_setup(
					struct uk_netdev *n,
					__u16 queue_id, __u16 nb_desc,
					struct uk_netdev_rxqueue_conf *conf);
static int virtio_net_rx_intr_disable(struct uk_netdev *n,
				      struct uk_netdev_rx_queue *queue);
static int virtio_net_rx_intr_enable(struct uk_netdev *n,
				     struct uk_netdev_rx_queue *queue);
static void virtio_netdev_xmit_free(struct uk_netdev_tx_queue *txq);
static int virtio_netdev_xmit(struct uk_netdev *dev,
			      struct uk_netdev_tx_queue *queue,
			      struct uk_netbuf *pkt);
static int virtio_netdev_recv(struct uk_netdev *dev,
			      struct uk_netdev_rx_queue *queue,
			      struct uk_netbuf **pkt);
static const struct uk_hwaddr *virtio_net_mac_get(struct uk_netdev *n);
static __u16 virtio_net_mtu_get(struct uk_netdev *n);
static unsigned virtio_net_promisc_get(struct uk_netdev *n);
static int virtio_netdev_rxq_info_get(struct uk_netdev *dev, __u16 queue_id,
				      struct uk_netdev_queue_info *qinfo);
static int virtio_netdev_txq_info_get(struct uk_netdev *dev, __u16 queue_id,
				      struct uk_netdev_queue_info *qinfo);
static int virtio_netdev_rxq_dequeue(struct virtio_net_device *vndev,
				     struct uk_netdev_rx_queue *rxq,
				     struct uk_netbuf **netbuf);
static int virtio_netdev_rxq_enqueue(struct virtio_net_device *vndev,
				     struct uk_netdev_rx_queue *rxq,
				     struct uk_netbuf *netbuf);
static int virtio_netdev_recv_done(struct virtqueue *vq, void *priv);
static int virtio_netdev_rx_fillup(struct virtio_net_device *vndev,
				   struct uk_netdev_rx_queue *rxq,
				   __u16 num, int notify);

/**
 * Static global constants
 */
static const char *drv_name = DRIVER_NAME;
static struct uk_alloc *a;

/* This provides the size of the virtio-net header depending on the
 * virito version and features selected. Use this instead of
 * sizeof(struct virtio_net_hdr).
 *
 * Notice: Legacy drivers only provided `num_buffers` when
 * `VIRTIO_NET_F_MRG_RBUF` was negotiated. Without that feature,
 * the header is 2 bytes shorter (VIRTIO v1.2 Sect. 5.1.6.1).
 * When communicating with legacy devices this becomes part of the
 * padding (see `VIRTIO_HDR_SIZE_PADDED`).
 */
static inline __u16 virtio_net_hdr_size(struct virtio_net_device *vndev)
{
	__u16 hdr_size;

	/* Legacy */
	if (!(vndev->vdev->features & (1ULL << VIRTIO_F_VERSION_1))) {
		hdr_size = 10;
		if (vndev->vdev->features & (1ULL << VIRTIO_NET_F_MRG_RXBUF))
			hdr_size += 2;
		return hdr_size;
	}

	/* Modern */
	hdr_size = sizeof(struct virtio_net_hdr);
	if (!(vndev->vdev->features & (1ULL << VIRTIO_NET_F_HASH_REPORT)))
		hdr_size -= 8;

	return hdr_size;
}

/**
 * The Driver method implementation.
 */
static int virtio_netdev_recv_done(struct virtqueue *vq, void *priv)
{
	struct uk_netdev_rx_queue *rxq = NULL;

	UK_ASSERT(vq && priv);

	rxq = (struct uk_netdev_rx_queue *) priv;

	/* Disable the interrupt for the ring */
	virtqueue_intr_disable(vq);
	rxq->intr_enabled &= ~(VTNET_INTR_EN);

	/* Indicate to the network stack about an event */
	uk_netdev_drv_rx_event(rxq->ndev, rxq->lqueue_id);
	return 1;
}

static void virtio_netdev_xmit_free(struct uk_netdev_tx_queue *txq)
{
	struct uk_netbuf *pkt = NULL;
	int cnt = 0;
	int rc;

	for (;;) {
		rc = virtqueue_buffer_dequeue(txq->vq, (void **) &pkt, NULL);
		if (rc < 0)
			break;

		UK_ASSERT(pkt);

		/**
		 * Releasing the free buffer back to netbuf. The netbuf could
		 * use the destructor to inform the stack regarding the free up
		 * of memory.
		 */
		uk_netbuf_free(pkt);
		cnt++;
	}
	uk_pr_debug("Free %"__PRIu16" descriptors\n", cnt);
}

#define RX_FILLUP_BATCHLEN 64

static int virtio_netdev_rx_fillup(struct virtio_net_device *vndev,
				   struct uk_netdev_rx_queue *rxq,
				   __u16 nb_desc, int notify)
{
	struct uk_netbuf *netbuf[RX_FILLUP_BATCHLEN];
	int rc = 0;
	int status = 0x0;
	__u16 i, j;
	__u16 req;
	__u16 cnt = 0;
	__u16 filled = 0;

	/**
	 * The effective queue size depends on how we have to add the buffers
	 * into the ring. For example modern virtio devices only need a single
	 * descriptor for the virtio header + payload. However, depending on the
	 * physical layout of the buffer, we can end up using more descriptors.
	 * In this case we batch allocate too many buffers and have to free the
	 * superfluous ones.
	 */
	UK_ASSERT(POWER_OF_2(vndev->buf_descr_count));
	nb_desc = ALIGN_DOWN(nb_desc, vndev->buf_descr_count);
	while (filled < nb_desc) {
		req = MIN(nb_desc / vndev->buf_descr_count, RX_FILLUP_BATCHLEN);
		cnt = rxq->alloc_rxpkts(rxq->alloc_rxpkts_argp, netbuf, req);
		for (i = 0; i < cnt; i++) {
			uk_pr_debug("Enqueue netbuf %"PRIu16"/%"PRIu16" (%p) to virtqueue %p...\n",
				    i + 1, cnt, netbuf[i], rxq);
			rc = virtio_netdev_rxq_enqueue(vndev, rxq, netbuf[i]);
			if (unlikely(rc < 0)) {
				uk_pr_err("Failed to add a buffer to receive virtqueue %p: %d\n",
					  rxq, rc);

				/*
				 * Release netbufs that we are not going
				 * to use anymore
				 */
				for (j = i; j < cnt; j++)
					uk_netbuf_free(netbuf[j]);
				status |= UK_NETDEV_STATUS_UNDERRUN;
				goto out;
			}
			/* This count is not necessarily correct in the
			 * presence of paging, but should be close enough as an
			 * approximation.
			 * TODO: Use indirect descriptors to ensure we are
			 *       always below the specified count.
			 */
			filled += vndev->buf_descr_count;
		}

		if (unlikely(cnt < req)) {
			uk_pr_debug("Incomplete fill-up of netbufs on receive virtqueue %p: Out of memory",
				    rxq);
			status |= UK_NETDEV_STATUS_UNDERRUN;
			goto out;
		}
	}

out:
	uk_pr_debug("Programmed %"PRIu16" receive netbufs to receive virtqueue %p (status %x)\n",
		    filled / vndev->buf_descr_count, rxq, status);

	/**
	 * Notify the host, when we submit new descriptor(s).
	 */
	if (notify && filled)
		virtqueue_host_notify(rxq->vq);

	return status;
}

static int virtio_netdev_xmit(struct uk_netdev *dev,
			      struct uk_netdev_tx_queue *queue,
			      struct uk_netbuf *pkt)
{
	struct virtio_net_device *vndev;
	struct virtio_net_hdr *vhdr;
	int rc = 0;
	int status = 0x0;
	__sz total_len = 0;
	__u8  *buf_start;
	__sz buf_len;

	UK_ASSERT(dev);
	UK_ASSERT(pkt && queue);

	vndev = to_virtionetdev(dev);

	/**
	 * We are reclaiming the free descriptors from buffers. The function is
	 * not protected by means of locks. We need to be careful if there are
	 * multiple context through which we free the tx descriptors.
	 */
	virtio_netdev_xmit_free(queue);

	buf_start = pkt->data;
	buf_len = pkt->len;

	/**
	 * Use the preallocated header space for the virtio header.
	 */
	rc = uk_netbuf_header(pkt, VTNET_HDR_SIZE_PADDED(vndev));
	if (unlikely(rc != 1)) {
		uk_pr_err("Failed to prepend virtio header\n");
		rc = -ENOSPC;
		goto err_exit;
	}
	vhdr = pkt->data;

	/**
	 * Fill the virtio-net-header with the necessary information.
	 * Zero explicitly set.
	 * NOTE: We do not set VIRTIO_NET_HDR_F_DATA_VALID and
	 *       VIRTIO_NET_HDR_F_RSC_INFO because we aren't allowed
	 *       according to virtio specification during sending.
	 * TODO: We assume that the PARTIAL_CSUM flag is only set with
	 *       the first netbuf of a queue. If this is not the case,
	 *       (e.g., due to encapsulation of protocol headers with
	 *        prepending netbufs) we need to replace the call
	 *       to `uk_netbuf_sglist_append()`. However, a netbuf
	 *       chain can only once have set the PARTIAL_CSUM flag.
	 */
	memset(vhdr, 0, virtio_net_hdr_size(vndev));
	if (pkt->flags & UK_NETBUF_F_PARTIAL_CSUM) {
		vhdr->flags       |= VIRTIO_NET_HDR_F_NEEDS_CSUM;
		/* `csum_start` is without header size */
		vhdr->csum_start   = pkt->csum_start - VTNET_HDR_SIZE_PADDED(vndev);
		vhdr->csum_offset  = pkt->csum_offset;
	}
	if (pkt->flags & UK_NETBUF_F_GSO_TCPV4) {
		vhdr->gso_type     = VIRTIO_NET_HDR_GSO_TCPV4;
		vhdr->hdr_len      = pkt->header_len;
		vhdr->gso_size     = pkt->gso_size;
	}

	/**
	 * Prepare the sglist and enqueue the buffer to the virtio-ring.
	 */
	uk_sglist_reset(&queue->sg);

	/**
	 * According the specification 5.1.6.6, we need to explicitly use
	 * 2 descriptor for each transmit and receive network packet since we
	 * do not negotiate for the VIRTIO_F_ANY_LAYOUT.
	 *
	 * 1 for the virtio header and the other for the actual network packet.
	 */
	/* Appending the data to the list. */
	rc = uk_sglist_append(&queue->sg, vhdr, virtio_net_hdr_size(vndev));
	if (unlikely(rc != 0)) {
		uk_pr_err("Failed to append to the sg list\n");
		goto err_remove_vhdr;
	}
	rc = uk_sglist_append(&queue->sg, buf_start, buf_len);
	if (unlikely(rc != 0)) {
		uk_pr_err("Failed to append to the sg list\n");
		goto err_remove_vhdr;
	}
	if (pkt->next) {
		rc = uk_netbuf_sglist_append(&queue->sg, pkt->next);
		if (unlikely(rc != 0)) {
			uk_pr_err("Failed to append to the sg list: %d\n", rc);
			goto err_remove_vhdr;
		}
	}

	if (!(pkt->flags & UK_NETBUF_F_GSO_TCPV4)) {
		total_len = uk_sglist_length(&queue->sg);
		if (unlikely(total_len > VIRTIO_PKT_BUFFER_LEN(vndev))) {
			uk_pr_err("Packet size too big: %lu, max:%lu\n",
				  total_len, VIRTIO_PKT_BUFFER_LEN(vndev));
			rc = -ENOTSUP;
			goto err_remove_vhdr;
		}
	}

	/**
	 * Adding the descriptors to the virtqueue.
	 */
	rc = virtqueue_buffer_enqueue(queue->vq, pkt, &queue->sg,
				      queue->sg.sg_nseg, 0);
	if (likely(rc >= 0)) {
		status |= UK_NETDEV_STATUS_SUCCESS;
		/**
		 * Notify the host the new buffer.
		 */
		virtqueue_host_notify(queue->vq);
		/**
		 * When there is further space available in the ring
		 * return UK_NETDEV_STATUS_MORE.
		 */
		status |= likely(rc > 0) ? UK_NETDEV_STATUS_MORE : 0x0;
	} else if (rc == -ENOSPC) {
		uk_pr_debug("No more descriptor available\n");
		/**
		 * Remove header before exiting because we could not send
		 */
		uk_netbuf_header(pkt, -((__s16)VTNET_HDR_SIZE_PADDED(vndev)));
	} else {
		uk_pr_err("Failed to enqueue descriptors into the ring: %d\n",
			  rc);
		goto err_remove_vhdr;
	}
	return status;

err_remove_vhdr:
	uk_netbuf_header(pkt, -((__s16)VTNET_HDR_SIZE_PADDED(vndev)));
err_exit:
	UK_ASSERT(rc < 0);
	return rc;
}

static int virtio_netdev_rxq_enqueue(struct virtio_net_device *vndev,
				     struct uk_netdev_rx_queue *rxq,
				     struct uk_netbuf *netbuf)
{
	int rc = 0;
	struct virtio_net_hdr *rxhdr;
	__u8 *buf_start;
	__sz buf_len = 0;
	struct uk_sglist *sg;

	if (virtqueue_is_full(rxq->vq)) {
		uk_pr_debug("The virtqueue is full\n");
		return -ENOSPC;
	}

	/* Reset the scatter gather list */
	sg = &rxq->sg;
	uk_sglist_reset(sg);

	if (vndev->buf_descr_count == VIRTIO_NET_BUF_DESCR_COUNT_INLINE) {
		rc = uk_netbuf_header(netbuf, virtio_net_hdr_size(vndev));
		if (unlikely(rc != 1)) {
			uk_pr_err("Failed to allocate space to prepend virtio header\n");
			return -EINVAL;
		}
		uk_sglist_append(sg, netbuf->data, netbuf->len);
	} else {
		/**
		 * Saving the buffer information before reserving the header
		 * space.
		 */
		buf_start = netbuf->data;
		buf_len = netbuf->len;

		/**
		 * Retrieve the buffer header length.
		 */
		rc = uk_netbuf_header(netbuf, VTNET_HDR_SIZE_PADDED(vndev));
		if (unlikely(rc != 1)) {
			uk_pr_err("Failed to allocate space to prepend virtio header\n");
			return -EINVAL;
		}
		rxhdr = netbuf->data;

		/* Appending the header buffer to the sglist */
		uk_sglist_append(sg, rxhdr, virtio_net_hdr_size(vndev));

		/* Appending the data buffer to the sglist */
		uk_sglist_append(sg, buf_start, buf_len);
	}

	rc = virtqueue_buffer_enqueue(rxq->vq, netbuf, sg, 0,
				      sg->sg_nseg);
	return rc;
}

static int virtio_netdev_rxq_dequeue(struct virtio_net_device *vndev,
				     struct uk_netdev_rx_queue *rxq,
				     struct uk_netbuf **netbuf)
{
	int ret;
	int rc __maybe_unused = 0;
	struct uk_netbuf *buf = NULL, *chain;
	struct virtio_net_hdr *vhdr;
	__u32 num_buffers = 1;
	__u32 len;

	UK_ASSERT(netbuf);

	ret = virtqueue_buffer_dequeue(rxq->vq, (void **) &buf, &len);
	if (ret < 0) {
		uk_pr_debug("No data available in the queue\n");
		*netbuf = NULL;
		return rxq->nb_desc;
	}
	if (unlikely(
		(len < (__u32)virtio_net_hdr_size(vndev) +
			UK_ETH_HDR_UNTAGGED_LEN) ||
		(!(vndev->vdev->features & (1ULL << VIRTIO_NET_F_GUEST_TSO4) ||
		   vndev->vdev->features & (1ULL << VIRTIO_NET_F_GUEST_TSO6))
		  && (len > VIRTIO_PKT_BUFFER_LEN(vndev))))) {
		uk_pr_err("Received invalid packet size: %"__PRIu32"\n", len);
		return -EINVAL;
	}

	/**
	 * Copy virtio header flags to netbuf
	 */
	vhdr = (struct virtio_net_hdr *) buf->data;
	buf->flags  = ((vhdr->flags & VIRTIO_NET_HDR_F_DATA_VALID)
		       ? UK_NETBUF_F_DATA_VALID   : 0x0);
	if (vhdr->flags & VIRTIO_NET_HDR_F_NEEDS_CSUM) {
		buf->flags |= UK_NETBUF_F_PARTIAL_CSUM;
		buf->csum_offset = vhdr->csum_offset;
		buf->csum_start = vhdr->csum_start;
		/* NOTE: csum_start is without virtio header
		 *       (uk_netbuf_header() will remove it again)
		 */
		if (vndev->buf_descr_count == VIRTIO_NET_BUF_DESCR_COUNT_INLINE)
			buf->csum_start += virtio_net_hdr_size(vndev);
		else
			buf->csum_start += VTNET_HDR_SIZE_PADDED(vndev);
	}
	if (vndev->vdev->features & (1ULL << VIRTIO_NET_F_MRG_RXBUF))
		num_buffers = vhdr->num_buffers;

	/**
	 * Removing the virtio header from the buffer and adjusting length.
	 */
	if (vndev->buf_descr_count == VIRTIO_NET_BUF_DESCR_COUNT_INLINE) {
		buf->len = len;
		rc = uk_netbuf_header(buf,
				      -((__s16)virtio_net_hdr_size(vndev)));
	} else {
		/* The length tracked by the virtio driver does include the
		 * header but *not the padding*, therefore adjust the netbuf
		 * size and add the padding.
		 */
		buf->len = len + (VTNET_HDR_SIZE_PADDED(vndev) -
			   virtio_net_hdr_size(vndev));
		/* Shift the position forward to remove the header */
		rc = uk_netbuf_header(buf,
				      -((__s16)VTNET_HDR_SIZE_PADDED(vndev)));
	}
	UK_ASSERT(rc == 1);

	while (num_buffers > 1) {
		ret = virtqueue_buffer_dequeue(rxq->vq, (void **)&chain, &len);
		if (unlikely(ret < 0)) {
			uk_pr_err("mergeable buffer indicated more buffers\n");
			*netbuf = NULL;
			return rxq->nb_desc;
		}
		UK_ASSERT(len <= chain->buflen);
		chain->len = len;
		uk_netbuf_append(buf, chain);
		num_buffers--;
	}

	*netbuf = buf;

	return ret;
}

static int virtio_netdev_recv(struct uk_netdev *dev,
			      struct uk_netdev_rx_queue *queue,
			      struct uk_netbuf **pkt)
{
	struct virtio_net_device *vndev;
	int status = 0x0;
	int rc = 0;

	UK_ASSERT(dev && queue);
	UK_ASSERT(pkt);

	vndev = to_virtionetdev(dev);

	/* Queue interrupts have to be off when calling receive */
	UK_ASSERT(!(queue->intr_enabled & VTNET_INTR_EN));

	rc = virtio_netdev_rxq_dequeue(vndev, queue, pkt);
	if (unlikely(rc < 0)) {
		uk_pr_err("Failed to dequeue the packet: %d\n", rc);
		goto err_exit;
	}
	status |= (*pkt) ? UK_NETDEV_STATUS_SUCCESS : 0x0;
	status |= virtio_netdev_rx_fillup(vndev, queue, (queue->nb_desc - rc),
					  1);

	/* Enable interrupt only when user had previously enabled it */
	if (queue->intr_enabled & VTNET_INTR_USR_EN_MASK) {
		/* Need to enable the interrupt on the last packet */
		rc = virtqueue_intr_enable(queue->vq);
		if (rc == 1 && !(*pkt)) {
			/**
			 * Packet arrive after reading the queue and before
			 * enabling the interrupt
			 */
			rc = virtio_netdev_rxq_dequeue(vndev, queue, pkt);
			if (unlikely(rc < 0)) {
				uk_pr_err("Failed to dequeue the packet: %d\n",
					  rc);
				goto err_exit;
			}
			status |= UK_NETDEV_STATUS_SUCCESS;

			/*
			 * Since we received something, we need to fillup
			 * and notify
			 */
			status |= virtio_netdev_rx_fillup(vndev, queue,
							  (queue->nb_desc - rc),
							  1);

			/* Need to enable the interrupt on the last packet */
			rc = virtqueue_intr_enable(queue->vq);
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
		status |= UK_NETDEV_STATUS_MORE;
	}
	return status;

err_exit:
	UK_ASSERT(rc < 0);
	return rc;
}

static struct uk_netdev_rx_queue *virtio_netdev_rx_queue_setup(
				struct uk_netdev *n, __u16 queue_id,
				__u16 nb_desc,
				struct uk_netdev_rxqueue_conf *conf)
{
	struct virtio_net_device *vndev;
	struct uk_netdev_rx_queue *rxq = NULL;
	int rc;

	UK_ASSERT(n);
	UK_ASSERT(conf);
	UK_ASSERT(conf->alloc_rxpkts);

	vndev = to_virtionetdev(n);
	if (queue_id >= vndev->max_vqueue_pairs) {
		uk_pr_err("Invalid virtqueue identifier: %"__PRIu16"\n",
			  queue_id);
		rc = -EINVAL;
		goto err_exit;
	}
	/* Setup the virtqueue with the descriptor */
	rc = virtio_netdev_vqueue_setup(vndev, queue_id, nb_desc, VNET_RX,
					conf->a);
	if (rc < 0) {
		uk_pr_err("Failed to set up virtqueue %"__PRIu16": %d\n",
			  queue_id, rc);
		goto err_exit;
	}
	rxq  = &vndev->rxqs[rc];
	rxq->alloc_rxpkts = conf->alloc_rxpkts;
	rxq->alloc_rxpkts_argp = conf->alloc_rxpkts_argp;

	/* Allocate receive buffers for this queue */
	virtio_netdev_rx_fillup(vndev, rxq, rxq->nb_desc, 0);

exit:
	return rxq;

err_exit:
	rxq = ERR2PTR(rc);
	goto exit;
}

/**
 * This function setup the vring infrastructure.
 * @param vndev
 *	Reference to the virtio net device.
 * @param queue_id
 *	User queue identifier
 * @param nr_desc
 *	User configured number of descriptors.
 * @param queue_type
 *	Queue type.
 * @param a
 *	Reference to the allocator.
 */
static int virtio_netdev_vqueue_setup(struct virtio_net_device *vndev,
		__u16 queue_id, __u16 nr_desc, virtq_type_t queue_type,
		struct uk_alloc *a)
{
	int rc = 0;
	int id = 0;
	virtqueue_callback_t callback;
	__u16 max_desc, hwvq_id;
	struct virtqueue *vq;

	if (queue_type == VNET_RX) {
		id = vndev->rx_vqueue_cnt;
		callback = virtio_netdev_recv_done;
		max_desc = vndev->rxqs[id].max_nb_desc;
		hwvq_id = vndev->rxqs[id].hwvq_id;
	} else {
		id = vndev->tx_vqueue_cnt;
		/* We don't support the callback from the txqueue yet */
		callback = NULL;
		max_desc = vndev->txqs[id].max_nb_desc;
		hwvq_id = vndev->txqs[id].hwvq_id;
	}

	if (unlikely(max_desc < nr_desc)) {
		uk_pr_err("Max allowed desc: %"__PRIu16" Requested desc:%"__PRIu16"\n",
			  max_desc, nr_desc);
		return -ENOBUFS;
	}

	nr_desc = (nr_desc != 0) ? nr_desc : max_desc;
	uk_pr_debug("Configuring the %d descriptors\n", nr_desc);

	/* Check if the descriptor is a power of 2 */
	if (unlikely(nr_desc & (nr_desc - 1))) {
		uk_pr_err("Expect descriptor count as a power 2\n");
		return -EINVAL;
	}
	vq = virtio_vqueue_setup(vndev->vdev, hwvq_id, nr_desc, callback, a);
	if (unlikely(PTRISERR(vq))) {
		uk_pr_err("Failed to set up virtqueue %"__PRIu16"\n",
			  queue_id);
		rc = PTR2ERR(vq);
		return rc;
	}

	if (queue_type == VNET_RX) {
		vq->priv = &vndev->rxqs[id];
		vndev->rxqs[id].ndev = &vndev->netdev;
		vndev->rxqs[id].vq = vq;
		vndev->rxqs[id].nb_desc = nr_desc;
		vndev->rxqs[id].lqueue_id = queue_id;
		vndev->rx_vqueue_cnt++;
	} else {
		vndev->txqs[id].vq = vq;
		vndev->txqs[id].ndev = &vndev->netdev;
		vndev->txqs[id].nb_desc = nr_desc;
		vndev->txqs[id].lqueue_id = queue_id;
		vndev->tx_vqueue_cnt++;
	}
	return id;
}

static struct uk_netdev_tx_queue *virtio_netdev_tx_queue_setup(
				struct uk_netdev *n, __u16 queue_id __unused,
				__u16 nb_desc __unused,
				struct uk_netdev_txqueue_conf *conf __unused)
{
	struct uk_netdev_tx_queue *txq = NULL;
	struct virtio_net_device *vndev;
	int rc = 0;

	UK_ASSERT(n);
	vndev = to_virtionetdev(n);
	if (queue_id >= vndev->max_vqueue_pairs) {
		uk_pr_err("Invalid virtqueue identifier: %"__PRIu16"\n",
			  queue_id);
		rc = -EINVAL;
		goto err_exit;
	}
	/* Setup the virtqueue */
	rc = virtio_netdev_vqueue_setup(vndev, queue_id, nb_desc, VNET_TX,
					conf->a);
	if (rc < 0) {
		uk_pr_err("Failed to set up virtqueue %"__PRIu16": %d\n",
			  queue_id, rc);
		goto err_exit;
	}
	txq = &vndev->txqs[rc];
exit:
	return txq;

err_exit:
	txq = ERR2PTR(rc);
	goto exit;
}

static int virtio_netdev_rxq_info_get(struct uk_netdev *dev,
				      __u16 queue_id,
				      struct uk_netdev_queue_info *qinfo)
{
	struct virtio_net_device *vndev;
	struct uk_netdev_rx_queue *rxq;
	int rc = 0;

	UK_ASSERT(dev);
	UK_ASSERT(qinfo);
	vndev = to_virtionetdev(dev);
	if (unlikely(queue_id >= vndev->max_vqueue_pairs)) {
		uk_pr_err("Invalid virtqueue id: %"__PRIu16"\n", queue_id);
		rc = -EINVAL;
		goto exit;
	}
	rxq = &vndev->rxqs[queue_id];
	qinfo->nb_min = 1;
	qinfo->nb_max = rxq->max_nb_desc;
	qinfo->nb_is_power_of_two = 1;

exit:
	return rc;

}

static int virtio_netdev_txq_info_get(struct uk_netdev *dev,
				      __u16 queue_id __unused,
				      struct uk_netdev_queue_info *qinfo)
{
	struct virtio_net_device *vndev;
	struct uk_netdev_tx_queue *txq;
	int rc = 0;

	UK_ASSERT(dev);
	UK_ASSERT(qinfo);

	vndev = to_virtionetdev(dev);
	if (unlikely(queue_id >= vndev->max_vqueue_pairs)) {
		uk_pr_err("Invalid queue_id %"__PRIu16"\n", queue_id);
		rc = -EINVAL;
		goto exit;
	}
	txq = &vndev->txqs[queue_id];
	qinfo->nb_min = 1;
	qinfo->nb_max = txq->max_nb_desc;
	qinfo->nb_is_power_of_two = 1;

exit:
	return rc;
}

static unsigned virtio_net_promisc_get(struct uk_netdev *n)
{
	struct virtio_net_device *d;

	UK_ASSERT(n);
	d = to_virtionetdev(n);
	return d->promisc;
}

static const struct uk_hwaddr *virtio_net_mac_get(struct uk_netdev *n)
{
	struct virtio_net_device *d;

	UK_ASSERT(n);
	d = to_virtionetdev(n);
	return &d->hw_addr;
}

static __u16 virtio_net_mtu_get(struct uk_netdev *n)
{
	struct virtio_net_device *d;

	UK_ASSERT(n);
	d = to_virtionetdev(n);
	return d->mtu;
}

static int virtio_netdev_probe(struct uk_netdev *n)
{
	struct virtio_net_device *vndev;
	__u64 drv_features = 0;
	__u64 host_features;
	int rc;

	UK_ASSERT(n);
	vndev = to_virtionetdev(n);

	/**
	 * Read device feature bits, and write the subset of feature bits
	 * understood by the OS and driver to the device. During this step the
	 * driver MAY read (but MUST NOT write) the device-specific
	 * configuration fields to check that it can support the device before
	 * accepting it.
	 */
	host_features = virtio_feature_get(vndev->vdev);

	/**
	 * Hardware address
	 * NOTE: For now, we require a provided hw address. In principle,
	 *       we could generate one if none was given.
	 */
	if (!VIRTIO_FEATURE_HAS(host_features, VIRTIO_NET_F_MAC)) {
		/**
		 * The feature that aren't supported are usually masked out and
		 * provided with default value. In this case we need to
		 * report an error as we don't support  generation of random
		 * MAC Address.
		 */
		uk_pr_err("%p: Host system does not offer MAC feature\n", n);
		rc = -EINVAL;
		goto err_negotiate_feature;
	}
	VIRTIO_FEATURE_SET(drv_features, VIRTIO_NET_F_MAC);

	/**
	 * MTU information
	 */
	if (!VIRTIO_FEATURE_HAS(host_features, VIRTIO_NET_F_MTU))
		uk_pr_debug("%p: Host system does not offer MTU feature\n", n);
	else
		VIRTIO_FEATURE_SET(drv_features, VIRTIO_NET_F_MTU);

	if (VIRTIO_FEATURE_HAS(host_features, VIRTIO_NET_F_STATUS))
		VIRTIO_FEATURE_SET(drv_features, VIRTIO_NET_F_STATUS);

	/**
	 * Gratuitous ARP
	 * NOTE: We tell that we will do gratuitous ARPs ourselves.
	 */
	VIRTIO_FEATURE_SET(drv_features, VIRTIO_NET_F_GUEST_ANNOUNCE);

	/**
	 * Partial checksumming
	 * NOTE: This enables sending and receiving of packets marked with
	 *       VIRTIO_NET_HDR_F_DATA_VALID and VIRTIO_NET_HDR_F_NEEDS_CSUM
	 */
	if (!VIRTIO_FEATURE_HAS(host_features, VIRTIO_NET_F_CSUM)) {
		uk_pr_debug("%p: Host does not offer partial checksumming feature: Checksum offloading disabled.\n",
			    n);
	} else {
		VIRTIO_FEATURE_SET(drv_features, VIRTIO_NET_F_CSUM);
		VIRTIO_FEATURE_SET(drv_features, VIRTIO_NET_F_GUEST_CSUM);
	}

	/* VirtIO modern */
	if (VIRTIO_FEATURE_HAS(host_features, VIRTIO_F_VERSION_1))
		VIRTIO_FEATURE_SET(drv_features, VIRTIO_F_VERSION_1);

	/**
	 * Mergeable receive buffers
	 */
	if (VIRTIO_FEATURE_HAS(host_features, VIRTIO_NET_F_MRG_RXBUF))
		VIRTIO_FEATURE_SET(drv_features, VIRTIO_NET_F_MRG_RXBUF);

	/**
	 * TCP Segmentation Offload
	 * NOTE: This enables sending and receiving of packets marked with
	 *       VIRTIO_NET_HDR_GSO_TCPV4
	 */
	if (VIRTIO_FEATURE_HAS(host_features, VIRTIO_NET_F_GSO))
		VIRTIO_FEATURE_SET(drv_features, VIRTIO_NET_F_GSO);
	if (VIRTIO_FEATURE_HAS(host_features, VIRTIO_NET_F_HOST_TSO4))
		VIRTIO_FEATURE_SET(drv_features, VIRTIO_NET_F_HOST_TSO4);

	/**
	 * Use index based event supression when it's available.
	 * This allows a more fine-grained control when the hypervisor should
	 * notify the guest. Some hypervisors such as firecracker also do not
	 * support the original flag.
	 */
	if (VIRTIO_FEATURE_HAS(host_features, VIRTIO_F_EVENT_IDX))
		VIRTIO_FEATURE_SET(drv_features, VIRTIO_F_EVENT_IDX);

	/* Store the preliminary result in the features field.
	 * virtio_netdev_feature_negotiate will take care of sending the result
	 * to the host depending on how the uknetdev client configured the
	 * device.
	 */
	vndev->vdev->features = drv_features;

	if ((vndev->vdev->features & (1ULL << VIRTIO_F_VERSION_1)) ||
	    (vndev->vdev->features & (1ULL << VIRTIO_NET_F_MRG_RXBUF))) {
		/* Do not use a separate (padded) header descriptor for modern
		 * devices or when we can use mergeable buffers
		 */
		vndev->buf_descr_count = 1;
	} else {
		vndev->buf_descr_count = 2;
	}

	return 0;
err_negotiate_feature:
	virtio_dev_status_update(vndev->vdev, VIRTIO_CONFIG_STATUS_FAIL);
	return rc;
}

static int virtio_netdev_feature_negotiate(struct uk_netdev *n,
					   const struct uk_netdev_conf *conf)
{
	struct virtio_net_device *vndev;
	__u64 host_features;
	int rc;

	UK_ASSERT(n);
	UK_ASSERT(conf);
	vndev = to_virtionetdev(n);

	host_features = virtio_feature_get(vndev->vdev);

	/**
	 * Large Receive Offload
	 * NOTE: This allows the host to send packets larger than MTU. The
	 *       network stack needs to be able to handle such packets.
	 *       We only enable this if we have also support for mergeable RX
	 *       buffers, otherwise we would have to either allocate huge
	 *       buffers (wasting space for smaller buffers) or use many small
	 *       descriptors (needing indirect descriptors and putting a large
	 *       burden on the allocator).
	 */
	if (conf->lro) {
		if (unlikely(!VIRTIO_FEATURE_HAS(vndev->vdev->features,
						 VIRTIO_NET_F_GUEST_CSUM) ||
			     !VIRTIO_FEATURE_HAS(vndev->vdev->features,
						 VIRTIO_NET_F_MRG_RXBUF))) {
			rc = -EINVAL;
			goto err_negotiate_feature;
		}
		if (VIRTIO_FEATURE_HAS(host_features, VIRTIO_NET_F_GUEST_TSO4))
			VIRTIO_FEATURE_SET(vndev->vdev->features,
					   VIRTIO_NET_F_GUEST_TSO4);
		if (VIRTIO_FEATURE_HAS(host_features, VIRTIO_NET_F_GUEST_TSO6))
			VIRTIO_FEATURE_SET(vndev->vdev->features,
					   VIRTIO_NET_F_GUEST_TSO6);
	}

	/**
	 * Announce our enabled driver features back to the backend device
	 */
	virtio_feature_set(vndev->vdev);

	/**
	 * According to Virtio specification, section 2.3.1. Config fields
	 * greater than 32-bits cannot be atomically read. We may need to
	 * reconsider providing generic read/write function for all these
	 * virtio device in a separate header file which could be reused across
	 * different virtio devices.
	 * Currently, unaligned read is supported in the underlying function.
	 */
	virtio_config_get(vndev->vdev,
			  __offsetof(struct virtio_net_config, mac),
			  &vndev->hw_addr.addr_bytes[0],
			  UK_NETDEV_HWADDR_LEN, 1);

	if (VIRTIO_FEATURE_HAS(vndev->vdev->features, VIRTIO_NET_F_MTU)) {
		virtio_config_get(vndev->vdev,
				  __offsetof(struct virtio_net_config, mac),
				  &vndev->mtu, sizeof(vndev->mtu), 1);
		vndev->max_mtu = vndev->mtu;
	} else {
		/**
		 * Report some reasonable defaults. This breaks down in cases
		 * where MTU is <UK_ETH_PAYLOAD_MAXLEN (e.g. encapsulation),
		 * but there's no way to signal the lack of MTU reporting in
		 * mtu_get as yet, so we're stuck with this for now.
		 */
		vndev->max_mtu = vndev->mtu = UK_ETH_PAYLOAD_MAXLEN;
	}

	virtio_dev_status_update(vndev->vdev,
				 (VIRTIO_CONFIG_STATUS_ACK |
				  VIRTIO_CONFIG_STATUS_DRIVER |
				  VIRTIO_CONFIG_STATUS_FEATURES_OK));

	return 0;

err_negotiate_feature:
	virtio_dev_status_update(vndev->vdev, VIRTIO_CONFIG_STATUS_FAIL);
	return rc;
}

static int virtio_netdev_rxtx_alloc(struct virtio_net_device *vndev,
				    const struct uk_netdev_conf *conf)
{
	int rc = 0;
	int i = 0;
	int vq_avail = 0;
	int total_vqs = conf->nb_rx_queues + conf->nb_tx_queues;
	__u16 qdesc_size[total_vqs];

	if (conf->nb_rx_queues != 1 || conf->nb_tx_queues != 1) {
		uk_pr_err("Queue combination not supported: %"__PRIu16"/%"__PRIu16" rx/tx\n",
			  conf->nb_rx_queues, conf->nb_tx_queues);

		return -ENOTSUP;
	}

	/**
	 * TODO:
	 * The virtio device management data structure are allocated using the
	 * allocator from the netdev configuration. In the future it might be
	 * wiser to move it to the allocator of each individual queue. This
	 * would better considering NUMA support.
	 */
	vndev->rxqs = uk_malloc(a, sizeof(*vndev->rxqs) * conf->nb_rx_queues);
	vndev->txqs = uk_malloc(a, sizeof(*vndev->txqs) * conf->nb_tx_queues);
	if (unlikely(!vndev->rxqs || !vndev->txqs)) {
		uk_pr_err("Failed to allocate memory for queue management\n");
		rc = -ENOMEM;
		goto err_free_txrx;
	}

	vq_avail = virtio_find_vqs(vndev->vdev, total_vqs, qdesc_size);
	if (unlikely(vq_avail != total_vqs)) {
		uk_pr_err("Expected: %d queues, Found: %d queues\n",
			  total_vqs, vq_avail);
		rc = -ENOMEM;
		goto err_free_txrx;
	}

	/**
	 * The virtqueue are organized as:
	 * Virtqueue-rx0
	 * Virtqueue-tx0
	 * Virtqueue-rx1
	 * Virtqueue-tx1
	 * ...
	 * Virtqueue-ctrlq
	 */
	for (i = 0; i < vndev->max_vqueue_pairs; i++) {
		/**
		 * Initialize the received queue with the information received
		 * from the device.
		 */
		vndev->rxqs[i].hwvq_id = 2 * i;
		vndev->rxqs[i].max_nb_desc = qdesc_size[vndev->rxqs[i].hwvq_id];
		uk_sglist_init(&vndev->rxqs[i].sg,
			       (sizeof(vndev->rxqs[i].sgsegs) /
				sizeof(vndev->rxqs[i].sgsegs[0])),
			       &vndev->rxqs[i].sgsegs[0]);

		/**
		 * Initialize the transmit queue with the information received
		 * from the device.
		 */
		vndev->txqs[i].hwvq_id = (2 * i) + 1;
		vndev->txqs[i].max_nb_desc = qdesc_size[vndev->txqs[i].hwvq_id];
		uk_sglist_init(&vndev->txqs[i].sg,
			       (sizeof(vndev->txqs[i].sgsegs) /
				sizeof(vndev->txqs[i].sgsegs[0])),
			       &vndev->txqs[i].sgsegs[0]);
	}
exit:
	return rc;

err_free_txrx:
	if (!vndev->rxqs)
		uk_free(a, vndev->rxqs);
	if (!vndev->txqs)
		uk_free(a, vndev->txqs);
	goto exit;
}

static int virtio_netdev_configure(struct uk_netdev *n,
				   const struct uk_netdev_conf *conf)
{
	int rc = 0;
	struct virtio_net_device *vndev;

	UK_ASSERT(n);
	UK_ASSERT(conf);
	vndev = to_virtionetdev(n);

	rc = virtio_netdev_feature_negotiate(n, conf);
	if (unlikely(rc < 0)) {
		uk_pr_err("%p: Failed to negotiate features: %d\n", n, rc);
		return rc;
	}

	rc = virtio_netdev_rxtx_alloc(vndev, conf);
	if (unlikely(rc < 0)) {
		uk_pr_err("%p: Failed to initialize rx and tx rings: %d\n",
			  n, rc);
	}

	/* Initialize the count of the virtio-net device */
	vndev->rx_vqueue_cnt = 0;
	vndev->tx_vqueue_cnt = 0;

	return rc;
}

static int virtio_net_rx_intr_enable(struct uk_netdev *n,
				     struct uk_netdev_rx_queue *queue)
{
	struct virtio_net_device *d __unused;
	int rc = 0;

	UK_ASSERT(n);
	d = to_virtionetdev(n);
	/* If the interrupt is enabled */
	if (queue->intr_enabled & VTNET_INTR_EN)
		return 0;

	/**
	 * Enable the user configuration bit. This would cause the interrupt to
	 * be enabled automatically, if the interrupt could not be enabled now
	 * due to data in the queue.
	 */
	queue->intr_enabled = VTNET_INTR_USR_EN;
	rc = virtqueue_intr_enable(queue->vq);
	if (!rc)
		queue->intr_enabled |= VTNET_INTR_EN;

	return rc;
}

static int virtio_net_rx_intr_disable(struct uk_netdev *n,
				      struct uk_netdev_rx_queue *queue)
{
	struct virtio_net_device *vndev __unused;

	UK_ASSERT(n);
	vndev = to_virtionetdev(n);
	virtqueue_intr_disable(queue->vq);
	queue->intr_enabled &= ~(VTNET_INTR_USR_EN | VTNET_INTR_EN);
	return 0;
}

static void virtio_net_info_get(struct uk_netdev *dev,
				struct uk_netdev_info *dev_info)
{
	struct virtio_net_device *vndev;
	__u64 host_features;

	UK_ASSERT(dev && dev_info);
	vndev = to_virtionetdev(dev);

	host_features = virtio_feature_get(vndev->vdev);

	dev_info->max_rx_queues = vndev->max_vqueue_pairs;
	dev_info->max_tx_queues = vndev->max_vqueue_pairs;
	dev_info->max_mtu = vndev->max_mtu;
	dev_info->nb_encap_tx = VTNET_HDR_SIZE_PADDED(vndev);
	if (vndev->buf_descr_count == VIRTIO_NET_BUF_DESCR_COUNT_INLINE)
		dev_info->nb_encap_rx = virtio_net_hdr_size(vndev);
	else
		dev_info->nb_encap_rx = VTNET_HDR_SIZE_PADDED(vndev);
	dev_info->ioalign = VIRTIO_PKT_BUFFER_ALIGN;

	dev_info->features = UK_NETDEV_F_RXQ_INTR
		| (VIRTIO_FEATURE_HAS(host_features, VIRTIO_NET_F_CSUM)
		   ? UK_NETDEV_F_PARTIAL_CSUM : 0)
		| ((VIRTIO_FEATURE_HAS(host_features, VIRTIO_NET_F_HOST_TSO4)
		    || VIRTIO_FEATURE_HAS(host_features, VIRTIO_NET_F_GSO))
		   ? UK_NETDEV_F_TSO4 : 0)
		| ((VIRTIO_FEATURE_HAS(host_features, VIRTIO_NET_F_GUEST_TSO4)
		    || VIRTIO_FEATURE_HAS(host_features,
					  VIRTIO_NET_F_GUEST_TSO6)
		   ? UK_NETDEV_F_LRO : 0));
}

static int virtio_net_start(struct uk_netdev *n)
{
	struct virtio_net_device *d;
	int i = 0;

	UK_ASSERT(n != NULL);
	d = to_virtionetdev(n);

	/*
	 * By default, interrupts are disabled and it is up to the user or
	 * network stack to manually enable them with a call to
	 * enable_tx|rx_intr()
	 */
	for (i = 0; i < d->rx_vqueue_cnt; i++) {
		virtqueue_intr_disable(d->rxqs[i].vq);
		d->rxqs[i].intr_enabled = 0;
	}

	for (i = 0; i < d->tx_vqueue_cnt; i++) {
		virtqueue_intr_disable(d->txqs[i].vq);
		d->txqs[i].intr_enabled = 0;
	}

	/*
	 * Set the DRIVER_OK status bit. At this point the device is "live".
	 */
	virtio_dev_drv_up(d->vdev);
	uk_pr_info(DRIVER_NAME": %"__PRIu16" started\n", d->uid);

	for (i = 0; i < d->rx_vqueue_cnt; i++)
		virtqueue_host_notify(d->rxqs[i].vq);

	return 0;
}

static const struct uk_netdev_ops virtio_netdev_ops = {
	.probe = virtio_netdev_probe,
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

	vndev = uk_calloc(a, 1, sizeof(*vndev));
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
	vndev->promisc = 0;

	/**
	 * TODO:
	 * Adding multiqueue support for the virtio net driver.
	 */
	vndev->max_vqueue_pairs = 1;
	uk_pr_debug("virtio-net device registered with libuknet\n");

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
