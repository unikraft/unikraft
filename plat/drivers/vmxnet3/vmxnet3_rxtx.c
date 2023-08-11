/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2010-2015 Intel Corporation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <stdarg.h>
#include <unistd.h>
#include <inttypes.h>

#include <vmxnet3/base/vmxnet3_defs.h>
#include <vmxnet3/vmxnet3_ring.h>
#include <vmxnet3/vmxnet3_ethdev.h>

#define to_vmxnet3dev(ndev) \
	__containerof(ndev, struct vmxnet3_hw, netdev)


static const uint32_t rxprod_reg[2] = {VMXNET3_REG_RXPROD, VMXNET3_REG_RXPROD2};

static int vmxnet3_post_rx_bufs(struct uk_netdev_rx_queue *, uint8_t);
static void vmxnet3_tq_tx_complete(struct uk_netdev_tx_queue *);

static void
vmxnet3_tx_cmd_ring_release_mbufs(vmxnet3_cmd_ring_t *ring)
{
	debug_uk_pr_info("vmxnet3_tx_cmd_ring_release_mbufs\n");

	while (ring->next2comp != ring->next2fill) {
		/* No need to worry about desc ownership, device is quiesced by now. */
		vmxnet3_buf_info_t *buf_info = ring->buf_info + ring->next2comp;

		if (buf_info->m) {
			uk_netbuf_free(buf_info->m);
			buf_info->m = NULL;
			buf_info->bufPA = 0;
			buf_info->len = 0;
		}
		vmxnet3_cmd_ring_adv_next2comp(ring);
	}
}

static void
vmxnet3_rx_cmd_ring_release_mbufs(vmxnet3_cmd_ring_t *ring)
{
	uint32_t i;

	debug_uk_pr_info("vmxnet3_rx_cmd_ring_release_mbufs\n");

	for (i = 0; i < ring->size; i++) {
		/* No need to worry about desc ownership, device is quiesced by now. */
		vmxnet3_buf_info_t *buf_info = &ring->buf_info[i];

		if (buf_info->m) {
			uk_netbuf_free(buf_info->m);
			buf_info->m = NULL;
			buf_info->bufPA = 0;
			buf_info->len = 0;
		}
		vmxnet3_cmd_ring_adv_next2comp(ring);
	}
}

static void
vmxnet3_cmd_ring_release(vmxnet3_cmd_ring_t *ring)
{
	debug_uk_pr_info("vmxnet3_cmd_ring_release\n");

	uk_free(ring->a, ring->buf_info);
	ring->buf_info = NULL;
}

void
vmxnet3_dev_tx_queue_release(struct uk_netdev *dev, uint16_t qid)
{
	struct uk_netdev_tx_queue *tq = dev->_tx_queue[qid];

	debug_uk_pr_info("vmxnet3_dev_tx_queue_release\n");

	if (tq != NULL) {
		/* Release mbufs */
		vmxnet3_tx_cmd_ring_release_mbufs(&tq->cmd_ring);
		/* Release the cmd_ring */
		vmxnet3_cmd_ring_release(&tq->cmd_ring);
		/* Release the memzone */
		uk_free(tq->a, tq->mz);
		/* Release the queue */
		uk_free(tq->a, tq);
	}
}

void
vmxnet3_dev_rx_queue_release(struct uk_netdev *dev, uint16_t qid)
{
	int i;
	struct uk_netdev_rx_queue *rq = dev->_rx_queue[qid];

	debug_uk_pr_info("vmxnet3_dev_rx_queue_release\n");

	if (rq != NULL) {
		/* Release mbufs */
		for (i = 0; i < VMXNET3_RX_CMDRING_SIZE; i++)
			vmxnet3_rx_cmd_ring_release_mbufs(&rq->cmd_ring[i]);

		/* Release both the cmd_rings */
		for (i = 0; i < VMXNET3_RX_CMDRING_SIZE; i++)
			vmxnet3_cmd_ring_release(&rq->cmd_ring[i]);

		/* Release the memzone */
		uk_free(rq->a, rq->mz);

		/* Release the queue */
		uk_free(rq->a, rq);
	}
}

static void
vmxnet3_dev_tx_queue_reset(void *txq)
{
	struct uk_netdev_tx_queue *tq = txq;
	struct vmxnet3_cmd_ring *ring = &tq->cmd_ring;
	struct vmxnet3_comp_ring *comp_ring = &tq->comp_ring;
	struct vmxnet3_data_ring *data_ring = &tq->data_ring;
	int size;

	debug_uk_pr_info("vmxnet3_dev_tx_queue_reset\n");

	if (tq != NULL) {
		/* Release the cmd_ring mbufs */
		vmxnet3_tx_cmd_ring_release_mbufs(&tq->cmd_ring);
	}

	/* Tx vmxnet rings structure initialization*/
	ring->next2fill = 0;
	ring->next2comp = 0;
	ring->gen = VMXNET3_INIT_GEN;
	comp_ring->next2proc = 0;
	comp_ring->gen = VMXNET3_INIT_GEN;

	size = sizeof(struct Vmxnet3_TxDesc) * ring->size;
	size += sizeof(struct Vmxnet3_TxCompDesc) * comp_ring->size;
	size += tq->txdata_desc_size * data_ring->size;

	memset(ring->base, 0, size);
}

static void
vmxnet3_dev_rx_queue_reset(void *rxq)
{
	int i;
	struct uk_netdev_rx_queue *rq = rxq;
	struct vmxnet3_hw *hw = rq->hw;
	struct vmxnet3_cmd_ring *ring0, *ring1;
	struct vmxnet3_comp_ring *comp_ring;
	struct vmxnet3_rx_data_ring *data_ring = &rq->data_ring;
	int size;

	debug_uk_pr_info("vmxnet3_dev_rx_queue_reset\n");

	/* Release both the cmd_rings mbufs */
	for (i = 0; i < VMXNET3_RX_CMDRING_SIZE; i++)
		vmxnet3_rx_cmd_ring_release_mbufs(&rq->cmd_ring[i]);

	ring0 = &rq->cmd_ring[0];
	ring1 = &rq->cmd_ring[1];
	comp_ring = &rq->comp_ring;

	/* Rx vmxnet rings structure initialization */
	ring0->next2fill = 0;
	ring1->next2fill = 0;
	ring0->next2comp = 0;
	ring1->next2comp = 0;
	ring0->gen = VMXNET3_INIT_GEN;
	ring1->gen = VMXNET3_INIT_GEN;
	comp_ring->next2proc = 0;
	comp_ring->gen = VMXNET3_INIT_GEN;

	size = sizeof(struct Vmxnet3_RxDesc) * (ring0->size + ring1->size);
	size += sizeof(struct Vmxnet3_RxCompDesc) * comp_ring->size;
	if (VMXNET3_VERSION_GE_3(hw) && rq->data_desc_size)
		size += rq->data_desc_size * data_ring->size;

	memset(ring0->base, 0, size);
}

void
vmxnet3_dev_clear_queues(struct uk_netdev *dev)
{
	struct vmxnet3_hw *hw = to_vmxnet3dev(dev);
	unsigned i;

	debug_uk_pr_info("vmxnet3_dev_clear_queues\n");

	for (i = 0; i < hw->num_tx_queues; i++) {
		struct uk_netdev_tx_queue *txq = dev->_tx_queue[i];

		if (txq != NULL) {
			txq->stopped = TRUE;
			vmxnet3_dev_tx_queue_reset(txq);
		}
	}

	for (i = 0; i < hw->num_rx_queues; i++) {
		struct uk_netdev_rx_queue *rxq = dev->_rx_queue[i];

		if (rxq != NULL) {
			rxq->stopped = TRUE;
			vmxnet3_dev_rx_queue_reset(rxq);
		}
	}
}

static int
vmxnet3_unmap_pkt(uint16_t eop_idx, struct uk_netdev_tx_queue *txq)
{
	int completed = 0;
	struct uk_netbuf *mbuf;

	debug_uk_pr_info("vmxnet3_unmap_pkt\n");

	/* Release cmd_ring descriptor and free mbuf */
	UK_ASSERT(txq->cmd_ring.base[eop_idx].txd.eop == 1);

	mbuf = txq->cmd_ring.buf_info[eop_idx].m;
	if (mbuf == NULL)
		UK_CRASH("EOP desc does not point to a valid mbuf");
	uk_netbuf_free(mbuf);

	txq->cmd_ring.buf_info[eop_idx].m = NULL;

	while (txq->cmd_ring.next2comp != eop_idx) {
		/* no out-of-order completion */
		UK_ASSERT(txq->cmd_ring.base[txq->cmd_ring.next2comp].txd.cq == 0);
		vmxnet3_cmd_ring_adv_next2comp(&txq->cmd_ring);
		completed++;
	}

	/* Mark the txd for which tcd was generated as completed */
	vmxnet3_cmd_ring_adv_next2comp(&txq->cmd_ring);

	return completed + 1;
}

static void
vmxnet3_tq_tx_complete(struct uk_netdev_tx_queue *txq)
{
	int completed = 0;
	vmxnet3_comp_ring_t *comp_ring = &txq->comp_ring;
	struct Vmxnet3_TxCompDesc *tcd = (struct Vmxnet3_TxCompDesc *)
		(comp_ring->base + comp_ring->next2proc);

	debug_uk_pr_info("vmxnet3_tq_tx_complete\n");

	while (tcd->gen == comp_ring->gen) {
		completed += vmxnet3_unmap_pkt(tcd->txdIdx, txq);

		vmxnet3_comp_ring_adv_next2proc(comp_ring);
		tcd = (struct Vmxnet3_TxCompDesc *)(comp_ring->base +
						    comp_ring->next2proc);
	}

	debug_uk_pr_info("Processed %d tx comps & command descs.\n", completed);
}

int vmxnet3_xmit_pkts(__unused struct uk_netdev *dev,
	struct uk_netdev_tx_queue *tx_queue,
	struct uk_netbuf *pkt)
{
	int ret = 0;
	uint16_t nb_tx;
	struct uk_netdev_tx_queue *txq = tx_queue;
	struct vmxnet3_hw *hw = txq->hw;
	Vmxnet3_TxQueueCtrl *txq_ctrl = &txq->shared->ctrl;
	uint32_t deferred = txq_ctrl->txNumDeferred;

	debug_uk_pr_info("vmxnet3_xmit_pkts\n");

	if (unlikely(txq->stopped)) {
		uk_pr_err("Tx queue is stopped.\n");
		return 0;
	}

	/* Free up the comp_descriptors aggressively */
	vmxnet3_tq_tx_complete(txq);

	nb_tx = 0;
	while (nb_tx < 1) {
		Vmxnet3_GenericDesc *gdesc;
		vmxnet3_buf_info_t *tbi;
		uint32_t first2fill, avail, dw2;
		struct uk_netbuf *txm = pkt;
		struct uk_netbuf *m_seg = txm;
		int copy_size = 0;
		/* # of descriptors needed for a packet. */
		unsigned count = 1; //txm->nb_segs;

		avail = vmxnet3_cmd_ring_desc_avail(&txq->cmd_ring);
		debug_uk_pr_info("vmxnet3_cmd_ring_desc_avail = %d\n", avail);
		if (count > avail) {
			/* Is command ring full? */
			if (unlikely(avail == 0)) {
				uk_pr_err("No free ring descriptors\n");
				break;
			}

			/* Command ring is not full but cannot handle the
			 * multi-segmented packet. Let's try the next packet
			 * in this case.
			 */
			uk_pr_err("Running out of ring descriptors "
				   "(avail %d needed %d)\n", avail, count);
			uk_netbuf_free(txm);
			nb_tx++;
			continue;
		}

		/* Drop non-TSO packet that is excessively fragmented */
		if (unlikely(count > VMXNET3_MAX_TXD_PER_PKT)) {
			uk_netbuf_free(txm);
			nb_tx++;
			continue;
		}

		/* Skip empty packets */
		if (unlikely(txm->len == 0)) {
			debug_uk_pr_info("Empty packet\n");
			uk_netbuf_free(txm);
			nb_tx++;
			continue;
		}

		debug_uk_pr_info("txm->len = %d, txq->txdata_desc_size = %d\n", txm->len, txq->txdata_desc_size);
		if (txm->len <= txq->txdata_desc_size) {
			struct Vmxnet3_TxDataDesc *tdd;

			debug_uk_pr_info("copy size %d\n", copy_size);
			tdd = (struct Vmxnet3_TxDataDesc *)
				((uint8 *)txq->data_ring.base +
				 txq->cmd_ring.next2fill *
				 txq->txdata_desc_size);
			copy_size = txm->len;
			memcpy(tdd->data, txm->buf, copy_size);
		}

		/* use the previous gen bit for the SOP desc */
		dw2 = (txq->cmd_ring.gen ^ 0x1) << VMXNET3_TXD_GEN_SHIFT;
		first2fill = txq->cmd_ring.next2fill;
		do {
			/* Remember the transmit buffer for cleanup */
			tbi = txq->cmd_ring.buf_info + txq->cmd_ring.next2fill;

			/* NB: the following assumes that VMXNET3 maximum
			 * transmit buffer size (16K) is greater than
			 * maximum size of mbuf segment size.
			 */
			gdesc = txq->cmd_ring.base + txq->cmd_ring.next2fill;

			/* Skip empty segments */
			if (unlikely(m_seg->len == 0)) {
				debug_uk_pr_info("Skipping empty segment");
				continue;
			}

			if (copy_size) {
				uint64 offset =
					(uint64)txq->cmd_ring.next2fill *
							txq->txdata_desc_size;
				gdesc->txd.addr =
					txq->data_ring.basePA +
							 offset;
			} else {
				gdesc->txd.addr = (uint64) m_seg->buf;
				// gdesc->txd.addr = rte_mbuf_data_iova(m_seg);
				ret |= UK_NETDEV_STATUS_MORE;
			}

			gdesc->dword[2] = dw2 | m_seg->len;
			gdesc->dword[3] = 0;

			/* move to the next2fill descriptor */
			vmxnet3_cmd_ring_adv_next2fill(&txq->cmd_ring);

			/* use the right gen for non-SOP desc */
			dw2 = txq->cmd_ring.gen << VMXNET3_TXD_GEN_SHIFT;
		} while ((m_seg = m_seg->next) != NULL);

		/* set the last buf_info for the pkt */
		tbi->m = txm;
		/* Update the EOP descriptor */
		gdesc->dword[3] |= VMXNET3_TXD_EOP | VMXNET3_TXD_CQ;

		/* Add VLAN tag if present */
		gdesc = txq->cmd_ring.base + first2fill;


		gdesc->txd.hlen = 0;
		gdesc->txd.om = VMXNET3_OM_NONE;
		gdesc->txd.msscof = 0;
		deferred++;

		/* flip the GEN bit on the SOP */
		barrier();
		gdesc->dword[2] ^= VMXNET3_TXD_GEN;

		txq_ctrl->txNumDeferred = deferred;
		nb_tx++;
	}

	debug_uk_pr_info("vmxnet3 txThreshold: %u\n", txq_ctrl->txThreshold);

	// if (deferred >= txq_ctrl->txThreshold) {
		debug_uk_pr_info("deferred (%u) >= txq_ctrl->txThreshold (%u)\n", deferred, txq_ctrl->txThreshold);
		txq_ctrl->txNumDeferred = 0;
		/* Notify vSwitch that packets are available. */
		VMXNET3_WRITE_BAR0_REG(hw, (VMXNET3_REG_TXPROD + txq->queue_id * VMXNET3_REG_ALIGN),
				       txq->cmd_ring.next2fill);
	// }

	ret |= UK_NETDEV_STATUS_SUCCESS;
	return ret;
}

static inline void
vmxnet3_renew_desc(struct uk_netdev_rx_queue *rxq, uint8_t ring_id,
		   struct uk_netbuf *mbuf)
{
	uint32_t val;
	struct vmxnet3_cmd_ring *ring = &rxq->cmd_ring[ring_id];
	struct Vmxnet3_RxDesc *rxd =
		(struct Vmxnet3_RxDesc *)(ring->base + ring->next2fill);
	vmxnet3_buf_info_t *buf_info = &ring->buf_info[ring->next2fill];

	debug_uk_pr_info("vmxnet3_renew_desc\n");

	if (ring_id == 0) {
		/* Usually: One HEAD type buf per packet
		 * val = (ring->next2fill % rxq->hw->bufs_per_pkt) ?
		 * VMXNET3_RXD_BTYPE_BODY : VMXNET3_RXD_BTYPE_HEAD;
		 */

		/* We use single packet buffer so all heads here */
		val = VMXNET3_RXD_BTYPE_HEAD;
	} else {
		/* All BODY type buffers for 2nd ring */
		val = VMXNET3_RXD_BTYPE_BODY;
	}

	/*
	 * Load mbuf pointer into buf_info[ring_size]
	 * buf_info structure is equivalent to cookie for virtio-virtqueue
	 */
	buf_info->m = mbuf;
	// buf_info->len = (uint16_t)(mbuf->buf_len - RTE_PKTMBUF_HEADROOM);
	buf_info->len = (uint16_t)(mbuf->buflen);
	buf_info->bufPA = (uint64) mbuf->buf;
	// buf_info->bufPA = rte_mbuf_data_iova_default(mbuf);

	/* Load Rx Descriptor with the buffer's GPA */
	rxd->addr = buf_info->bufPA;

	/* After this point rxd->addr MUST not be NULL */
	rxd->btype = val;
	rxd->len = buf_info->len;
	/* Flip gen bit at the end to change ownership */
	rxd->gen = ring->gen;

	vmxnet3_cmd_ring_adv_next2fill(ring);
}
/*
 *  Allocates mbufs and clusters. Post rx descriptors with buffer details
 *  so that device can receive packets in those buffers.
 *  Ring layout:
 *      Among the two rings, 1st ring contains buffers of type 0 and type 1.
 *      bufs_per_pkt is set such that for non-LRO cases all the buffers required
 *      by a frame will fit in 1st ring (1st buf of type0 and rest of type1).
 *      2nd ring contains buffers of type 1 alone. Second ring mostly be used
 *      only for LRO.
 */
static int
vmxnet3_post_rx_bufs(struct uk_netdev_rx_queue *rxq, uint8_t ring_id)
{
	int err = 0;
	uint32_t i = 0;
	struct vmxnet3_cmd_ring *ring = &rxq->cmd_ring[ring_id];

	debug_uk_pr_info("vmxnet3_post_rx_bufs\n");

	while (vmxnet3_cmd_ring_desc_avail(ring) > 0) {
		/* Allocate blank mbuf for the current Rx Descriptor */
		struct uk_netbuf *_pkt = NULL;
		int rc;

		rc = rxq->alloc_rxpkts(rxq->alloc_rxpkts_argp, &_pkt, 1);
		if (unlikely(rc == 0)) {
			uk_pr_err("Error allocating mbuf\n");
			err = ENOMEM;
			break;
		}

		vmxnet3_renew_desc(rxq, ring_id, _pkt);
		i++;
	}

	/* Return error only if no buffers are posted at present */
	if (vmxnet3_cmd_ring_desc_avail(ring) >= (ring->size - 1)) {
		uk_pr_err("Returning err!\n");
		return -err;
	}
	else {
		return i;
	}
}

/*
 * Process the Rx Completion Ring of given vmxnet3_rx_queue
 * for nb_pkts burst and return the number of packets received
 */
int
vmxnet3_recv_pkts(struct uk_netdev *dev __unused,
				  struct uk_netdev_rx_queue *rx_queue,
				  struct uk_netbuf **pkt)
{
	uint32_t idx;
	uint8_t ring_idx;
	struct uk_netdev_rx_queue *rxq;
	Vmxnet3_RxCompDesc *rcd;
	vmxnet3_buf_info_t *rbi;
	Vmxnet3_RxDesc *rxd;
	struct uk_netbuf *rxm = NULL;
	struct vmxnet3_hw *hw;
	int ret_status = 0;

	debug_uk_pr_info("vmxnet3_recv_pkts\n");

	ring_idx = 0;
	idx = 0;

	rxq = rx_queue;
	hw = rxq->hw;

	rcd = &rxq->comp_ring.base[rxq->comp_ring.next2proc].rcd;

	if (unlikely(rxq->stopped)) {
		uk_pr_err("Rx queue is stopped.\n");
		return 0;
	}

	debug_uk_pr_info("initial rcd->gen %d, rxq->comp_ring.gen = %d\n", rcd->gen, rxq->comp_ring.gen);
	if (rcd->gen == rxq->comp_ring.gen) {
		struct uk_netbuf *newm = NULL;
		int rc = 0;

		ret_status |= UK_NETDEV_STATUS_SUCCESS;

		rc = rxq->alloc_rxpkts(rxq->alloc_rxpkts_argp, &newm, 1);
		if (unlikely(rc == 0)) {
			uk_pr_err("Error allocating mbuf\n");
			return UK_NETDEV_STATUS_MORE;
			// break;
		}

		idx = rcd->rxdIdx;
		ring_idx = vmxnet3_get_ring_idx(hw, rcd->rqID);
		rxd = rxq->cmd_ring[ring_idx].base + idx;
		rbi = rxq->cmd_ring[ring_idx].buf_info + idx;

		debug_uk_pr_info("rxd idx: %d ring idx: %d.\n", idx, ring_idx);

		UK_ASSERT(rcd->len <= rxd->len);
		UK_ASSERT(rbi->m);

		/* Get the packet buffer pointer from buf_info */
		rxm = rbi->m;

		/* Clear descriptor associated buf_info to be reused */
		rbi->m = NULL;
		rbi->bufPA = 0;

		/* Update the index that we received a packet */
		rxq->cmd_ring[ring_idx].next2comp = idx;

		/* For RCD with EOP set, check if there is frame error */
		if (unlikely(rcd->eop && rcd->err)) {
			if (!rcd->fcs) {
				uk_pr_err("Recv packet dropped due to frame err.\n");
			}
			uk_pr_err("Error in received packet rcd#:%d rxd:%d\n",
					(int)(rcd - (struct Vmxnet3_RxCompDesc *)
						rxq->comp_ring.base), rcd->rxdIdx);
			uk_netbuf_free(rxm);
			if (rxq->start_seg) {
				struct uk_netbuf *start = rxq->start_seg;

				rxq->start_seg = NULL;
				uk_netbuf_free(start);
			}
			goto rcd_done;
		}

		/* Initialize newly received packet buffer */
		// TODO; should be put in priv
		// rxm->port = rxq->port_id;
		// rxm->nb_segs = 1;
		// rxm->next = NULL;
		// rxm->pkt_len = (uint16_t)rcd->len;
		// rxm->data_len = (uint16_t)rcd->len;
		// rxm->data_off = RTE_PKTMBUF_HEADROOM;
		rxm->buflen = (uint16_t)rcd->len;
		rxm->len = (uint16_t)rcd->len;
		rxm->next = NULL;

		/*
			* If this is the first buffer of the received packet,
			* set the pointer to the first mbuf of the packet
			* Otherwise, update the total length and the number of segments
			* of the current scattered packet, and update the pointer to
			* the last mbuf of the current packet.
			*/
		if (rcd->sop) {
			debug_uk_pr_info("rcd->sop set\n");
			UK_ASSERT(rxd->btype == VMXNET3_RXD_BTYPE_HEAD);

			debug_uk_pr_info("rcd->len = %d\n", rcd->len);
			if (unlikely(rcd->len == 0)) {
				UK_ASSERT(rcd->eop);

				uk_pr_err("Rx buf was skipped. rxring[%d][%d])\n",
						ring_idx, idx);
				uk_netbuf_free(rxm);
				goto rcd_done;
			}

			debug_uk_pr_info("hw->num_rx_queues = %d, rcd->rqID = %d\n", hw->num_rx_queues, rcd->rqID);
			if (vmxnet3_rx_data_ring(hw, rcd->rqID)) {
				debug_uk_pr_info("copying\n");
				uint8_t *rdd = rxq->data_ring.base +
					idx * rxq->data_desc_size;

				UK_ASSERT(VMXNET3_VERSION_GE_3(hw));
				memcpy(rxm->buf, rdd, rcd->len);
			}

			rxq->start_seg = rxm;
			rxq->last_seg = rxm;
		} else {
			struct uk_netbuf *start = rxq->start_seg;

			debug_uk_pr_info("rcd->sop not set\n");
			UK_ASSERT(rxd->btype == VMXNET3_RXD_BTYPE_BODY);

			if (likely(start && rxm->len > 0)) {
				debug_uk_pr_info("not empty\n");
				// start->pkt_len += rxm->data_len;
				// start->nb_segs++;

				rxq->last_seg->next = rxm;
				rxq->last_seg = rxm;
			} else {
				uk_pr_err("Error received empty or out of order frame.\n");

				uk_netbuf_free(rxm);
			}
		}

		if (rcd->eop) {
			// struct uk_netbuf *start = rxq->start_seg;

			debug_uk_pr_info("rcd->eop set\n");
			*pkt = rxq->start_seg;
			// rx_pkts[nb_rx++] = start;

			// debug_uk_pr_info("Buff->next: %p\n", (*pkt)->next);
			// debug_uk_pr_info("Buff->prev: %p\n", (*pkt)->prev);
			// debug_uk_pr_info("Buff->flags: %d\n", (*pkt)->flags);
			// debug_uk_pr_info("Buff->data: %p\n", (*pkt)->data);
			// debug_uk_pr_info("Buff->len: %d\n", (*pkt)->len);
			// debug_uk_pr_info("Buff->priv: %p\n", (*pkt)->priv);
			// debug_uk_pr_info("Buff->buf: %p\n", (*pkt)->buf);
			// debug_uk_pr_info("Buff->buflen: %lu\n", (*pkt)->buflen);
		}
rcd_done:
		rxq->cmd_ring[ring_idx].next2comp = idx;
		VMXNET3_INC_RING_IDX_ONLY(rxq->cmd_ring[ring_idx].next2comp,
						rxq->cmd_ring[ring_idx].size);

		/* It's time to renew descriptors */
		vmxnet3_renew_desc(rxq, ring_idx, newm);
		if (unlikely(rxq->shared->ctrl.updateRxProd)) {
			debug_uk_pr_info("Unlikely rxq->shared->ctrl.updateRxProd\n");
			VMXNET3_WRITE_BAR0_REG(hw, rxprod_reg[ring_idx] + (rxq->queue_id * VMXNET3_REG_ALIGN),
							rxq->cmd_ring[ring_idx].next2fill);
		}

		/* Advance to the next descriptor in comp_ring */
		vmxnet3_comp_ring_adv_next2proc(&rxq->comp_ring);

		rcd = &rxq->comp_ring.base[rxq->comp_ring.next2proc].rcd;
	} else {
		debug_uk_pr_info("nb_rxd = 0\n");
		uint32_t avail;
		for (ring_idx = 0; ring_idx < VMXNET3_RX_CMDRING_SIZE; ring_idx++) {
			avail = vmxnet3_cmd_ring_desc_avail(&rxq->cmd_ring[ring_idx]);
			if (unlikely(avail > 0)) {
				uk_pr_info("Unlikely avail %d > 0\n", avail);
				/* try to alloc new buf and renew descriptors */
				vmxnet3_post_rx_bufs(rxq, ring_idx);
			}
		}
		if (unlikely(rxq->shared->ctrl.updateRxProd)) {
			debug_uk_pr_info("Unlikely rxq->shared->ctrl.updateRxProd\n");
			for (ring_idx = 0; ring_idx < VMXNET3_RX_CMDRING_SIZE; ring_idx++) {
				VMXNET3_WRITE_BAR0_REG(hw, rxprod_reg[ring_idx] + (rxq->queue_id * VMXNET3_REG_ALIGN),
						       rxq->cmd_ring[ring_idx].next2fill);
			}
		}
	}

	ret_status |= UK_NETDEV_STATUS_MORE;
	return ret_status;
}

struct uk_netdev_tx_queue *
vmxnet3_dev_tx_queue_setup(struct uk_netdev *dev,
			   uint16_t queue_idx,
			   uint16_t nb_desc,
			   struct uk_netdev_txqueue_conf *tx_conf __unused)
{
	struct vmxnet3_hw *hw = to_vmxnet3dev(dev);
	// const struct void *mz;
	struct uk_netdev_tx_queue *txq;
	struct vmxnet3_cmd_ring *ring;
	struct vmxnet3_comp_ring *comp_ring;
	struct vmxnet3_data_ring *data_ring;
	int size;

	debug_uk_pr_info("vmxnet3_dev_tx_queue_setup\n");

	txq = uk_calloc(hw->a, 1, sizeof(struct uk_netdev_tx_queue));
	if (txq == NULL) {
		uk_pr_err("Can not allocate tx queue structure\n");
		return NULL;
		// return -ENOMEM;
	}

	txq->a = hw->a;
	txq->queue_id = queue_idx;
	// txq->port_id = dev->data->port_id;
	txq->shared = NULL; /* set in vmxnet3_setup_driver_shared() */
	txq->hw = hw;
	txq->qid = queue_idx;
	txq->stopped = TRUE;
	txq->txdata_desc_size = hw->txdata_desc_size;

	ring = &txq->cmd_ring;
	ring->a = hw->a;
	comp_ring = &txq->comp_ring;
	comp_ring->a = hw->a;
	data_ring = &txq->data_ring;
	data_ring->a = hw->a;

	/* Tx vmxnet ring length should be between 512-4096 */
	nb_desc = VMXNET3_TX_RING_MAX_SIZE;
	if (nb_desc < VMXNET3_DEF_TX_RING_SIZE) {
		uk_pr_err("VMXNET3 Tx Ring Size Min: %u\n",
			     VMXNET3_DEF_TX_RING_SIZE);
		return NULL;
	} else if (nb_desc > VMXNET3_TX_RING_MAX_SIZE) {
		uk_pr_err("VMXNET3 Tx Ring Size Max: %u\n",
			     VMXNET3_TX_RING_MAX_SIZE);
		return NULL;
	} else {
		ring->size = nb_desc;
		ring->size &= ~VMXNET3_RING_SIZE_MASK;
	}
	comp_ring->size = data_ring->size = ring->size;

	/* Tx vmxnet rings structure initialization*/
	ring->next2fill = 0;
	ring->next2comp = 0;
	ring->gen = VMXNET3_INIT_GEN;
	comp_ring->next2proc = 0;
	comp_ring->gen = VMXNET3_INIT_GEN;

	size = sizeof(struct Vmxnet3_TxDesc) * ring->size;
	size += sizeof(struct Vmxnet3_TxCompDesc) * comp_ring->size;
	size += txq->txdata_desc_size * data_ring->size;

	// mz = rte_eth_dma_zone_reserve(dev, "txdesc", queue_idx, size,
	// 			      VMXNET3_RING_BA_ALIGN, socket_id);
	// if (mz == NULL) {
	// 	// PMD_INIT_LOG(ERR, "ERROR: Creating queue descriptors zone");
	// 	return -ENOMEM;
	// }
	// txq->mz = mz;
	// memset(mz->addr, 0, mz->len);
	txq->mz = uk_memalign(hw->a, VMXNET3_RING_BA_ALIGN, size);
	if (txq->mz == NULL) {
		uk_pr_err("ERROR: Creating queue descriptors zone\n");
		return NULL;
	}
	memset(txq->mz, 0, size);

	// ring->base = uk_memalign(hw->a, VMXNET3_RING_BA_ALIGN, sizeof(struct Vmxnet3_TxDesc) * ring->size);
	ring->base = txq->mz;
	ring->basePA = (uint64_t) ring->base;
	/* cmd_ring initialization */
	// ring->base = mz->addr;
	// ring->basePA = mz->iova;

	/* comp_ring initialization */
	// comp_ring->base = uk_memalign(hw->a, VMXNET3_RING_BA_ALIGN, sizeof(struct Vmxnet3_TxCompDesc) * comp_ring->size);
	// memset(comp_ring->base, 0, );
	comp_ring->base = ring->base + ring->size;
	comp_ring->basePA = (uint64_t)comp_ring->base;
	// comp_ring->basePA = ring->basePA +
	// 	(sizeof(struct Vmxnet3_TxDesc) * ring->size);

	/* data_ring initialization */
	// data_ring->base = uk_memalign(hw->a, VMXNET3_RING_BA_ALIGN, sizeof(struct Vmxnet3_TxDataDesc) * data_ring->size);
	// memset(data_ring->base, 0, );
	data_ring->base = (Vmxnet3_TxDataDesc *)(comp_ring->base + comp_ring->size);
	data_ring->basePA = (uint64_t) data_ring->base;
	// data_ring->basePA = comp_ring->basePA +
	// 		(sizeof(struct Vmxnet3_TxCompDesc) * comp_ring->size);

	/* cmd_ring0 buf_info allocation */
	ring->buf_info = uk_calloc(hw->a, ring->size, sizeof(vmxnet3_buf_info_t));
	if (ring->buf_info == NULL) {
		uk_pr_err("ERROR: Creating tx_buf_info structure\n");
		return NULL;
	}

	/* Update the data portion with txq */
	dev->_tx_queue[queue_idx] = txq;

	return txq;
}

struct uk_netdev_rx_queue *
vmxnet3_dev_rx_queue_setup(struct uk_netdev *dev,
			   uint16_t queue_idx,
			   uint16_t nb_desc,
			   struct uk_netdev_rxqueue_conf *rx_conf)
{
	// const void *mz;
	struct uk_netdev_rx_queue *rxq;
	struct vmxnet3_hw *hw = to_vmxnet3dev(dev);
	struct vmxnet3_cmd_ring *ring0, *ring1, *ring;
	struct vmxnet3_comp_ring *comp_ring;
	struct vmxnet3_rx_data_ring *data_ring;
	int size;
	uint8_t i;
	char mem_name[32];

	debug_uk_pr_info("vmxnet3_dev_rx_queue_setup\n");

	rxq = uk_calloc(hw->a, 1, sizeof(struct uk_netdev_rx_queue));
	if (rxq == NULL) {
		uk_pr_err("Can not allocate rx queue structure\n");
		return NULL;
	}

	// rxq->mp = mp;
	rxq->queue_id = queue_idx;
	// rxq->port_id = dev->data->port_id;
	rxq->a = hw->a;
	rxq->shared = NULL; /* set in vmxnet3_setup_driver_shared() */
	rxq->hw = hw;
	rxq->qid1 = queue_idx;
	rxq->qid2 = queue_idx + hw->num_rx_queues;
	rxq->data_ring_qid = queue_idx + 2 * hw->num_rx_queues;
	rxq->data_desc_size = hw->rxdata_desc_size;
	rxq->stopped = TRUE;

	ring0 = &rxq->cmd_ring[0];
	ring0->a = hw->a;
	ring1 = &rxq->cmd_ring[1];
	ring1->a = hw->a;
	comp_ring = &rxq->comp_ring;
	comp_ring->a = hw->a;
	data_ring = &rxq->data_ring;
	data_ring->a = hw->a;

	/* Rx vmxnet rings length should be between 256-4096 */
	nb_desc = VMXNET3_RX_RING_MAX_SIZE;
	if (nb_desc < VMXNET3_DEF_RX_RING_SIZE) {
		uk_pr_err("VMXNET3 Rx Ring Size Min: 256\n");
		// return -EINVAL;
		return NULL;
	} else if (nb_desc > VMXNET3_RX_RING_MAX_SIZE) {
		uk_pr_err("VMXNET3 Rx Ring Size Max: 4096\n");
		// return -EINVAL;
		return NULL;
	} else {
		ring0->size = nb_desc;
		ring0->size &= ~VMXNET3_RING_SIZE_MASK;
		ring1->size = ring0->size;
	}

	comp_ring->size = ring0->size + ring1->size;
	data_ring->size = ring0->size;

	/* Rx vmxnet rings structure initialization */
	ring0->next2fill = 0;
	ring1->next2fill = 0;
	ring0->next2comp = 0;
	ring1->next2comp = 0;
	ring0->gen = VMXNET3_INIT_GEN;
	ring1->gen = VMXNET3_INIT_GEN;
	comp_ring->next2proc = 0;
	comp_ring->gen = VMXNET3_INIT_GEN;

	size = sizeof(struct Vmxnet3_RxDesc) * (ring0->size + ring1->size);
	size += sizeof(struct Vmxnet3_RxCompDesc) * comp_ring->size;
	if (VMXNET3_VERSION_GE_3(hw) && rxq->data_desc_size)
		size += rxq->data_desc_size * data_ring->size;

	// mz = rte_eth_dma_zone_reserve(dev, "rxdesc", queue_idx, size,
	// 			      VMXNET3_RING_BA_ALIGN, socket_id);
	// if (mz == NULL) {
	// 	// PMD_INIT_LOG(ERR, "ERROR: Creating queue descriptors zone");
	// 	return -ENOMEM;
	// }
	// rxq->mz = mz;
	// memset(mz->addr, 0, mz->len);

	/* cmd_ring0 initialization */
	rxq->mz = uk_memalign(hw->a, VMXNET3_RING_BA_ALIGN, size);
	if (rxq->mz == NULL) {
		uk_pr_err("ERROR: Creating queue descriptors zone\n");
		return NULL;
		// return -ENOMEM;
	}
	memset(rxq->mz, 0, size);

	ring0->base = rxq->mz;
	ring0->basePA = (uint64_t) ring0->base;

	/* cmd_ring1 initialization */
	// ring1->base = uk_memalign(hw->a, VMXNET3_RING_BA_ALIGN, sizeof(struct Vmxnet3_RxDesc) * ring1->size);
	// memset(ring1->base, 0, );
	ring1->base = ring0->base + ring0->size;
	ring1->basePA = (uint64_t) ring1->base;
	// ring1->basePA = ring0->basePA + sizeof(struct Vmxnet3_RxDesc) * ring0->size;

	/* comp_ring initialization */
	// comp_ring->base = uk_memalign(hw->a, VMXNET3_RING_BA_ALIGN, sizeof(struct Vmxnet3_RxCompDesc) * comp_ring->size);
	// memset(comp_ring->base, 0, );
	comp_ring->base = ring1->base + ring1->size;
	comp_ring->basePA = (uint64_t) comp_ring->base;
	// comp_ring->basePA = ring1->basePA + sizeof(struct Vmxnet3_RxDesc) * ring1->size;

	/* data_ring initialization */
	if (VMXNET3_VERSION_GE_3(hw) && rxq->data_desc_size) {
		data_ring->base =
			(uint8_t *)((uint64_t) comp_ring->base + comp_ring->size);
		// data_ring->basePA = (uint64_t) data_ring->base;
		data_ring->basePA = comp_ring->basePA +
			sizeof(struct Vmxnet3_RxCompDesc) * comp_ring->size;
	}

	/* cmd_ring0-cmd_ring1 buf_info allocation */
	for (i = 0; i < VMXNET3_RX_CMDRING_SIZE; i++) {
		ring = &rxq->cmd_ring[i];
		ring->rid = i;

		ring->buf_info = uk_calloc(hw->a, ring->size, sizeof(vmxnet3_buf_info_t));
		if (ring->buf_info == NULL) {
			uk_pr_err("ERROR: Creating rx_buf_info structure\n");
			return NULL;
			// return -ENOMEM;
		}
	}

	/* Update the data portion with rxq */
	dev->_rx_queue[queue_idx] = rxq;

	rxq->alloc_rxpkts = rx_conf->alloc_rxpkts;
	rxq->alloc_rxpkts_argp = rx_conf->alloc_rxpkts_argp;

	return rxq;
}

/*
 * Initializes Receive Unit
 * Load mbufs in rx queue in advance
 */
int
vmxnet3_dev_rxtx_init(struct uk_netdev *dev)
{
	struct vmxnet3_hw *hw = to_vmxnet3dev(dev);

	int i, ret;
	uint8_t j;

	debug_uk_pr_info("vmxnet3_dev_rxtx_init\n");

	for (i = 0; i < hw->num_rx_queues; i++) {
		struct uk_netdev_rx_queue *rxq = dev->_rx_queue[i];

		for (j = 0; j < VMXNET3_RX_CMDRING_SIZE; j++) {
			/* Passing 0 as alloc_num will allocate full ring */
			ret = vmxnet3_post_rx_bufs(rxq, j);
			if (ret <= 0) {
				uk_pr_err("ERROR: Posting Rxq: %d buffers ring: %d\n", i, j);
				return -ret;
			}
			/*
			 * Updating device with the index:next2fill to fill the
			 * mbufs for coming packets.
			 */
			if (unlikely(rxq->shared->ctrl.updateRxProd)) {
				debug_uk_pr_info("Unlikely rxq->shared->ctrl.updateRxProd\n");
				VMXNET3_WRITE_BAR0_REG(hw, rxprod_reg[j] + (rxq->queue_id * VMXNET3_REG_ALIGN),
						       rxq->cmd_ring[j].next2fill);
			}
		}
		rxq->stopped = FALSE;
		rxq->start_seg = NULL;
	}

	for (i = 0; i < hw->num_tx_queues; i++) {
		struct uk_netdev_tx_queue *txq = dev->_tx_queue[i];

		txq->stopped = FALSE;
	}

	return 0;
}
