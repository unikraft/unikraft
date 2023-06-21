/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2010-2016 Intel Corporation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <stdarg.h>
#include <inttypes.h>
#include <uk/alloc.h>

#include <e1000/e1000_api.h>
#include <e1000/e1000_ethdev.h>
#include <e1000/e1000_osdep.h>

#define	E1000_TXD_VLAN_SHIFT	16

#define E1000_RXDCTL_GRAN	0x01000000 /* RXDCTL Granularity */
#define RTE_CACHE_LINE_SIZE 128
#define E1000_TX_OFFLOAD_MASK ( \
		PKT_TX_IPV6 |           \
		PKT_TX_IPV4 |           \
		PKT_TX_IP_CKSUM |       \
		PKT_TX_L4_MASK |        \
		PKT_TX_VLAN_PKT)

/**
 * Define max possible fragments for the network packets.
 */
#define NET_MAX_FRAGMENTS    ((__U16_MAX >> __PAGE_SHIFT) + 2)


#define E1000_TX_OFFLOAD_NOTSUP_MASK \
		(PKT_TX_OFFLOAD_MASK ^ E1000_TX_OFFLOAD_MASK)


#define to_e1000dev(ndev) \
	__containerof(ndev, struct e1000_hw, netdev)

/**
 * Structure associated with each descriptor of the RX ring of a RX queue.
 */
struct em_rx_entry {
	struct uk_netbuf *mbuf; /**< mbuf associated with RX descriptor. */
};

/**
 * Structure associated with each descriptor of the TX ring of a TX queue.
 */
struct em_tx_entry {
	struct uk_netbuf *mbuf; /**< mbuf associated with TX desc, if any. */
	uint16_t next_id; /**< Index of next descriptor in ring. */
	uint16_t last_id; /**< Index of last scattered descriptor. */
};

/**
 * Structure associated with each RX queue.
 */
struct em_rx_queue {
	struct uk_alloc *a;
	volatile struct e1000_rx_desc *rx_ring; /**< RX ring virtual address. */
	uint64_t            rx_ring_phys_addr; /**< RX ring DMA address. */
	volatile uint32_t   *rdt_reg_addr; /**< RDT register address. */
	volatile uint32_t   *rdh_reg_addr; /**< RDH register address. */
	struct em_rx_entry *sw_ring;   /**< address of RX software ring. */
	struct uk_netbuf *pkt_first_seg; /**< First segment of current packet. */
	struct uk_netbuf *pkt_last_seg;  /**< Last segment of current packet. */
	/* User-provided receive buffer allocation function */
	uk_netdev_alloc_rxpkts alloc_rxpkts;
	void *alloc_rxpkts_argp;
	// struct uk_sglist sg;
	// struct uk_sglist_seg sgsegs[NET_MAX_FRAGMENTS];
	uint64_t	    offloads;   /**< Offloads of DEV_RX_OFFLOAD_* */
	uint16_t            nb_rx_desc; /**< number of RX descriptors. */
	uint16_t            rx_tail;    /**< current value of RDT register. */
	uint16_t            nb_rx_hold; /**< number of held free RX desc. */
	uint16_t            rx_free_thresh; /**< max free RX desc to hold. */
	uint16_t            queue_id;   /**< RX queue index. */
	uint16_t            port_id;    /**< Device port identifier. */
	uint8_t             pthresh;    /**< Prefetch threshold register. */
	uint8_t             hthresh;    /**< Host threshold register. */
	uint8_t             wthresh;    /**< Write-back threshold register. */
	uint8_t             crc_len;    /**< 0 if CRC stripped, 4 otherwise. */
};
typedef struct em_rx_queue uk_netdev_rx_queue;

/**
 * Hardware context number
 */
enum {
	EM_CTX_0    = 0, /**< CTX0 */
	EM_CTX_NUM  = 1, /**< CTX NUM */
};

/** Offload features */
union em_vlan_macip {
	uint32_t data;
	struct {
		uint16_t l3_len:9; /**< L3 (IP) Header Length. */
		uint16_t l2_len:7; /**< L2 (MAC) Header Length. */
		uint16_t vlan_tci;
		/**< VLAN Tag Control Identifier (CPU order). */
	} f;
};

/*
 * Compare mask for vlan_macip_len.data,
 * should be in sync with em_vlan_macip.f layout.
 * */
#define TX_VLAN_CMP_MASK        0xFFFF0000  /**< VLAN length - 16-bits. */
#define TX_MAC_LEN_CMP_MASK     0x0000FE00  /**< MAC length - 7-bits. */
#define TX_IP_LEN_CMP_MASK      0x000001FF  /**< IP  length - 9-bits. */
/** MAC+IP  length. */
#define TX_MACIP_LEN_CMP_MASK   (TX_MAC_LEN_CMP_MASK | TX_IP_LEN_CMP_MASK)

/**
 * Structure to check if new context need be built
 */
struct em_ctx_info {
	uint64_t flags;              /**< ol_flags related to context build. */
	uint32_t cmp_mask;           /**< compare mask */
	union em_vlan_macip hdrlen;  /**< L2 and L3 header lenghts */
};

/**
 * Structure associated with each TX queue.
 */
struct em_tx_queue {
	volatile struct e1000_data_desc *tx_ring; /**< TX ring address */
	uint64_t               tx_ring_phys_addr; /**< TX ring DMA address. */
	struct em_tx_entry    *sw_ring; /**< virtual address of SW ring. */
	volatile uint32_t      *tdt_reg_addr; /**< Address of TDT register. */
	uint16_t               nb_tx_desc;    /**< number of TX descriptors. */
	uint16_t               tx_tail;  /**< Current value of TDT register. */
	/**< Start freeing TX buffers if there are less free descriptors than
	     this value. */
	uint16_t               tx_free_thresh;
	/**< Number of TX descriptors to use before RS bit is set. */
	uint16_t               tx_rs_thresh;
	/** Number of TX descriptors used since RS bit was set. */
	uint16_t               nb_tx_used;
	/** Index to last TX descriptor to have been cleaned. */
	uint16_t	       last_desc_cleaned;
	/** Total number of TX descriptors ready to be allocated. */
	uint16_t               nb_tx_free;
	uint16_t               queue_id; /**< TX queue index. */
	uint16_t               port_id;  /**< Device port identifier. */
	uint8_t                pthresh;  /**< Prefetch threshold register. */
	uint8_t                hthresh;  /**< Host threshold register. */
	uint8_t                wthresh;  /**< Write-back threshold register. */
	struct em_ctx_info ctx_cache;
	/**< Hardware context history.*/
	uint64_t	       offloads; /**< offloads of DEV_TX_OFFLOAD_* */
	struct e1000_hw		*hw;
};

typedef struct em_tx_queue uk_netdev_tx_queue;

#if 1
#define RTE_PMD_USE_PREFETCH
#endif

#ifdef RTE_PMD_USE_PREFETCH
#define rte_em_prefetch(p)	rte_prefetch0(p)
#else
#define rte_em_prefetch(p)	do {} while(0)
#endif

#ifdef RTE_PMD_PACKET_PREFETCH
#define rte_packet_prefetch(p) rte_prefetch1(p)
#else
#define rte_packet_prefetch(p)	do {} while(0)
#endif

#ifndef DEFAULT_TX_FREE_THRESH
#define DEFAULT_TX_FREE_THRESH  16
#endif /* DEFAULT_TX_FREE_THRESH */

#ifndef DEFAULT_TX_RS_THRESH
#define DEFAULT_TX_RS_THRESH  16
#endif /* DEFAULT_TX_RS_THRESH */


/*********************************************************************
 *
 *  TX function
 *
 **********************************************************************/

/*
 * Populates TX context descriptor.
 */
// static inline void
// em_set_xmit_ctx(struct em_tx_queue* txq,
// 		volatile struct e1000_context_desc *ctx_txd,
// 		uint64_t flags,
// 		union em_vlan_macip hdrlen)
// {
// 	uint32_t cmp_mask, cmd_len;
// 	uint16_t ipcse, l2len;
// 	struct e1000_context_desc ctx;

// 	cmp_mask = 0;
// 	cmd_len = E1000_TXD_CMD_DEXT | E1000_TXD_DTYP_C;

// 	l2len = hdrlen.f.l2_len;
// 	ipcse = (uint16_t)(l2len + hdrlen.f.l3_len);

// 	/* setup IPCS* fields */
// 	ctx.lower_setup.ip_fields.ipcss = (uint8_t)l2len;
// 	ctx.lower_setup.ip_fields.ipcso = (uint8_t)(l2len +
// 			offsetof(struct rte_ipv4_hdr, hdr_checksum));

// 	/*
// 	 * When doing checksum or TCP segmentation with IPv6 headers,
// 	 * IPCSE field should be set t0 0.
// 	 */
// 	if (flags & PKT_TX_IP_CKSUM) {
// 		ctx.lower_setup.ip_fields.ipcse =
// 			(uint16_t)rte_cpu_to_le_16(ipcse - 1);
// 		cmd_len |= E1000_TXD_CMD_IP;
// 		cmp_mask |= TX_MACIP_LEN_CMP_MASK;
// 	} else {
// 		ctx.lower_setup.ip_fields.ipcse = 0;
// 	}

// 	/* setup TUCS* fields */
// 	ctx.upper_setup.tcp_fields.tucss = (uint8_t)ipcse;
// 	ctx.upper_setup.tcp_fields.tucse = 0;

// 	switch (flags & PKT_TX_L4_MASK) {
// 	case PKT_TX_UDP_CKSUM:
// 		ctx.upper_setup.tcp_fields.tucso = (uint8_t)(ipcse +
// 				offsetof(struct rte_udp_hdr, dgram_cksum));
// 		cmp_mask |= TX_MACIP_LEN_CMP_MASK;
// 		break;
// 	case PKT_TX_TCP_CKSUM:
// 		ctx.upper_setup.tcp_fields.tucso = (uint8_t)(ipcse +
// 				offsetof(struct rte_tcp_hdr, cksum));
// 		cmd_len |= E1000_TXD_CMD_TCP;
// 		cmp_mask |= TX_MACIP_LEN_CMP_MASK;
// 		break;
// 	default:
// 		ctx.upper_setup.tcp_fields.tucso = 0;
// 	}

// 	ctx.cmd_and_length = rte_cpu_to_le_32(cmd_len);
// 	ctx.tcp_seg_setup.data = 0;

// 	*ctx_txd = ctx;

// 	txq->ctx_cache.flags = flags;
// 	txq->ctx_cache.cmp_mask = cmp_mask;
// 	txq->ctx_cache.hdrlen = hdrlen;
// }

// /*
//  * Check which hardware context can be used. Use the existing match
//  * or create a new context descriptor.
//  */
// static inline uint32_t
// what_ctx_update(struct em_tx_queue *txq, uint64_t flags,
// 		union em_vlan_macip hdrlen)
// {
// 	/* If match with the current context */
// 	if (likely (txq->ctx_cache.flags == flags &&
// 			((txq->ctx_cache.hdrlen.data ^ hdrlen.data) &
// 			txq->ctx_cache.cmp_mask) == 0))
// 		return EM_CTX_0;

// 	/* Mismatch */
// 	return EM_CTX_NUM;
// }

// /* Reset transmit descriptors after they have been used */
static inline int
em_xmit_cleanup(struct em_tx_queue *txq)
{
	struct em_tx_entry *sw_ring = txq->sw_ring;
	volatile struct e1000_data_desc *txr = txq->tx_ring;
	uint16_t last_desc_cleaned = txq->last_desc_cleaned;
	uint16_t nb_tx_desc = txq->nb_tx_desc;
	uint16_t desc_to_clean_to;
	uint16_t nb_tx_to_clean;

	/* Determine the last descriptor needing to be cleaned */
	desc_to_clean_to = (uint16_t)(last_desc_cleaned + txq->tx_rs_thresh);
	if (desc_to_clean_to >= nb_tx_desc)
		desc_to_clean_to = (uint16_t)(desc_to_clean_to - nb_tx_desc);

	/* Check to make sure the last descriptor to clean is done */
	desc_to_clean_to = sw_ring[desc_to_clean_to].last_id;
	if (! (txr[desc_to_clean_to].upper.fields.status & E1000_TXD_STAT_DD))
	{
		// uk_pr_info(
		// 		"TX descriptor %4u is not done"
		// 		"(queue=%d)", desc_to_clean_to,
		// 		txq->queue_id);
		/* Failed to clean any descriptors, better luck next time */
		return -(1);
	}

	/* Figure out how many descriptors will be cleaned */
	if (last_desc_cleaned > desc_to_clean_to)
		nb_tx_to_clean = (uint16_t)((nb_tx_desc - last_desc_cleaned) +
							desc_to_clean_to);
	else
		nb_tx_to_clean = (uint16_t)(desc_to_clean_to -
						last_desc_cleaned);

	debug_uk_pr_info(
			"Cleaning %4u TX descriptors: %4u to %4u "
			"(queue=%d)", nb_tx_to_clean,
			last_desc_cleaned, desc_to_clean_to,
			txq->queue_id);

	/*
	 * The last descriptor to clean is done, so that means all the
	 * descriptors from the last descriptor that was cleaned
	 * up to the last descriptor with the RS bit set
	 * are done. Only reset the threshold descriptor.
	 */
	txr[desc_to_clean_to].upper.fields.status = 0;

	/* Update the txq to reflect the last descriptor that was cleaned */
	txq->last_desc_cleaned = desc_to_clean_to;
	txq->nb_tx_free = (uint16_t)(txq->nb_tx_free + nb_tx_to_clean);

	/* No Error */
	return 0;
}

// static inline uint32_t
// tx_desc_cksum_flags_to_upper(uint64_t ol_flags)
// {
// 	static const uint32_t l4_olinfo[2] = {0, E1000_TXD_POPTS_TXSM << 8};
// 	static const uint32_t l3_olinfo[2] = {0, E1000_TXD_POPTS_IXSM << 8};
// 	uint32_t tmp;

// 	tmp = l4_olinfo[(ol_flags & PKT_TX_L4_MASK) != PKT_TX_L4_NO_CKSUM];
// 	tmp |= l3_olinfo[(ol_flags & PKT_TX_IP_CKSUM) != 0];
// 	return tmp;
// }

// uint16_t
// eth_em_xmit_pkts(void *tx_queue, struct rte_mbuf **tx_pkts,
// 		uint16_t nb_pkts)
// {
int eth_em_xmit_pkts(__unused struct uk_netdev *dev,
	struct uk_netdev_tx_queue *queue,
	struct uk_netbuf *pkt)
{
	struct em_tx_queue *txq;
	struct em_tx_entry *sw_ring;
	struct em_tx_entry *txe, *txn;
	volatile struct e1000_data_desc *txr;
	volatile struct e1000_data_desc *txd;
	struct uk_netbuf     *tx_pkt;
	struct uk_netbuf     *m_seg;
	uint64_t buf_dma_addr;
	uint32_t popts_spec;
	uint32_t cmd_type_len;
	uint16_t slen;
	uint16_t tx_id;
	uint16_t tx_last;
	uint16_t nb_tx;
	uint16_t nb_used;
	uint32_t new_ctx;
	// uk_pr_info("eth_em_xmit_pkts\n");

	txq = queue;
	sw_ring = txq->sw_ring;
	txr     = txq->tx_ring;
	tx_id   = txq->tx_tail;
	txe = &sw_ring[tx_id];

	/* Determine if the descriptor ring needs to be cleaned. */
	 if (txq->nb_tx_free < txq->tx_free_thresh) {
		em_xmit_cleanup(txq);
	 }

	for (nb_tx = 0; nb_tx < 1; nb_tx++) {
		new_ctx = 0;
		tx_pkt = pkt;

		/*
		 * Keep track of how many descriptors are used this loop
		 * This will always be the number of segments + the number of
		 * Context descriptors required to transmit the packet
		 */
		nb_used = (uint16_t)(1 + new_ctx);
		// nb_used = (uint16_t)(tx_pkt->buflen + new_ctx);
		// nb_used = (uint16_t)(tx_pkt->nb_segs + new_ctx);

		/*
		 * The number of descriptors that must be allocated for a
		 * packet is the number of segments of that packet, plus 1
		 * Context Descriptor for the hardware offload, if any.
		 * Determine the last TX descriptor to allocate in the TX ring
		 * for the packet, starting from the current position (tx_id)
		 * in the ring.
		 */
		tx_last = (uint16_t) (tx_id + nb_used - 1);

		/* Circular ring */
		if (tx_last >= txq->nb_tx_desc)
			tx_last = (uint16_t) (tx_last - txq->nb_tx_desc);

		// uk_pr_info("queue_id=%u pktlen=%u"
		// 	   " tx_first=%u tx_last=%u\n",
		// 	   (unsigned) txq->queue_id,
		// 	//    (unsigned) tx_pkt->pkt_len,
		// 	   (unsigned) tx_pkt->buflen,
		// 	   (unsigned) tx_id,
		// 	   (unsigned) tx_last);

		/*
		 * Make sure there are enough TX descriptors available to
		 * transmit the entire packet.
		 * nb_used better be less than or equal to txq->tx_rs_thresh
		 */
		while (unlikely (nb_used > txq->nb_tx_free)) {
			// uk_pr_info("Not enough free TX descriptors "
			// 		"nb_used=%4u nb_free=%4u "
			// 		"(queue=%d)\n",
			// 		nb_used, txq->nb_tx_free,
			// 		txq->queue_id);

			// TODO
			if (em_xmit_cleanup(txq) != 0) {
				/* Could not clean any descriptors */
				if (nb_tx == 0)
					return 0;
				goto end_of_tx;
			}
		}

		/*
		 * By now there are enough free TX descriptors to transmit
		 * the packet.
		 */

		/*
		 * Set common flags of all TX Data Descriptors.
		 *
		 * The following bits must be set in all Data Descriptors:
		 *    - E1000_TXD_DTYP_DATA
		 *    - E1000_TXD_DTYP_DEXT
		 *
		 * The following bits must be set in the first Data Descriptor
		 * and are ignored in the other ones:
		 *    - E1000_TXD_POPTS_IXSM
		 *    - E1000_TXD_POPTS_TXSM
		 *
		 * The following bits must be set in the last Data Descriptor
		 * and are ignored in the other ones:
		 *    - E1000_TXD_CMD_VLE
		 *    - E1000_TXD_CMD_IFCS
		 *
		 * The following bits must only be set in the last Data
		 * Descriptor:
		 *   - E1000_TXD_CMD_EOP
		 *
		 * The following bits can be set in any Data Descriptor, but
		 * are only set in the last Data Descriptor:
		 *   - E1000_TXD_CMD_RS
		 */
		cmd_type_len = E1000_TXD_CMD_DEXT | E1000_TXD_DTYP_D |
			E1000_TXD_CMD_IFCS;
		popts_spec = 0;

		m_seg = tx_pkt;
		do {
			txd = &txr[tx_id];
			txn = &sw_ring[txe->next_id];

			if (txe->mbuf != NULL) {
				uk_netbuf_free_single(txe->mbuf); // TODO double-check
			}
			txe->mbuf = m_seg;

			/*
			 * Set up Transmit Data Descriptor.
			 */
			slen = m_seg->buflen;
			buf_dma_addr = m_seg->buf;

			txd->buffer_addr = buf_dma_addr;
			txd->lower.data = cmd_type_len | slen;
			txd->upper.data = popts_spec;

			txe->last_id = tx_last;
			tx_id = txe->next_id;
			txe = txn;
			m_seg = m_seg->next;
		} while (m_seg != NULL);

		/*
		 * The last packet data descriptor needs End Of Packet (EOP)
		 */
		cmd_type_len |= E1000_TXD_CMD_EOP;
		txq->nb_tx_used = (uint16_t)(txq->nb_tx_used + nb_used);
		txq->nb_tx_free = (uint16_t)(txq->nb_tx_free - nb_used);

		/* Set RS bit only on threshold packets' last descriptor */
		if (txq->nb_tx_used >= txq->tx_rs_thresh) {
			// uk_pr_info("Setting RS bit on TXD id=%4u "
			// 		"(queue=%d)\n",
			// 		tx_last, txq->queue_id);

			cmd_type_len |= E1000_TXD_CMD_RS;

			/* Update txq RS bit counters */
			txq->nb_tx_used = 0;
		}
		txd->lower.data |= cmd_type_len;
	}
end_of_tx:
	wmb();

	/*
	 * Set the Transmit Descriptor Tail (TDT)
	 */
	// uk_pr_info("queue_id=%u tx_tail=%u nb_tx=%u\n",
	// 	(unsigned) txq->queue_id,
	// 	(unsigned) tx_id, (unsigned) nb_tx);
	// uk_pr_info("tdt_reg_addr = %p\n", txq->tdt_reg_addr);
	E1000_PCI_REG_WRITE_RELAXED(txq->tdt_reg_addr, tx_id);
	txq->tx_tail = tx_id;

	return UK_NETDEV_STATUS_SUCCESS;
	// return UK_NETDEV_STATUS_SUCCESS & UK_NETDEV_STATUS_MORE;
}

/*********************************************************************
 *
 *  TX prep functions
 *
 **********************************************************************/
// uint16_t
// eth_em_prep_pkts(__rte_unused void *tx_queue, struct rte_mbuf **tx_pkts,
// 		uint16_t nb_pkts)
// {
// 	int i, ret;
// 	struct rte_mbuf *m;

// 	for (i = 0; i < nb_pkts; i++) {
// 		m = tx_pkts[i];

// 		if (m->ol_flags & E1000_TX_OFFLOAD_NOTSUP_MASK) {
// 			rte_errno = ENOTSUP;
// 			return i;
// 		}

// #ifdef RTE_LIBRTE_ETHDEV_DEBUG
// 		ret = rte_validate_tx_offload(m);
// 		if (ret != 0) {
// 			rte_errno = -ret;
// 			return i;
// 		}
// #endif
// 		ret = rte_net_intel_cksum_prepare(m);
// 		if (ret != 0) {
// 			rte_errno = -ret;
// 			return i;
// 		}
// 	}

// 	return i;
// }

/*********************************************************************
 *
 *  RX functions
 *
 **********************************************************************/

// static inline uint64_t
// rx_desc_status_to_pkt_flags(uint32_t rx_status)
// {
// 	uint64_t pkt_flags;

// 	/* Check if VLAN present */
// 	pkt_flags = ((rx_status & E1000_RXD_STAT_VP) ?
// 		PKT_RX_VLAN | PKT_RX_VLAN_STRIPPED : 0);

// 	return pkt_flags;
// }

// static inline uint64_t
// rx_desc_error_to_pkt_flags(uint32_t rx_error)
// {
// 	uint64_t pkt_flags = 0;

// 	if (rx_error & E1000_RXD_ERR_IPE)
// 		pkt_flags |= PKT_RX_IP_CKSUM_BAD;
// 	if (rx_error & E1000_RXD_ERR_TCPE)
// 		pkt_flags |= PKT_RX_L4_CKSUM_BAD;
// 	return pkt_flags;
// }


// uint16_t
// eth_em_recv_pkts(void *rx_queue, struct rte_mbuf **rx_pkts,
// 		uint16_t nb_pkts)
// {
int
eth_em_recv_pkts(__unused struct uk_netdev *dev, struct uk_netdev_rx_queue *rx_queue, 
	struct uk_netbuf **pkt)
{
	volatile struct e1000_rx_desc *rx_ring;
	volatile struct e1000_rx_desc *rxdp;
	struct em_rx_entry *sw_ring;
	struct em_rx_entry *rxe;
	struct em_rx_queue *rxq;
	struct uk_netbuf *rxm;
	struct uk_netbuf *nmb;
	struct e1000_rx_desc rxd;
	uint64_t dma_addr;
	uint16_t pkt_len;
	uint16_t rx_id;
	uint16_t nb_rx;
	uint16_t nb_hold;
	uint8_t status;
	int ret_status = 0;

	// uk_pr_info("eth_em_recv_pkts\n");

	rxq = (struct em_rx_queue *) rx_queue;
	nb_rx = 0;
	nb_hold = 0;
	rx_id = rxq->rx_tail;
	rx_ring = rxq->rx_ring;
	sw_ring = rxq->sw_ring;
	// uk_pr_info("rx_id = %d\n", rx_id);
	while (nb_rx < 1) {
		/*
		 * The order of operations here is important as the DD status
		 * bit must not be read after any other descriptor fields.
		 * rx_ring and rxdp are pointing to volatile data so the order
		 * of accesses cannot be reordered by the compiler. If they were
		 * not volatile, they could be reordered which could lead to
		 * using invalid descriptor fields when read from rxd.
		 */
		rxdp = &rx_ring[rx_id];
		status = rxdp->status;
		if (! (status & E1000_RXD_STAT_DD)) {
			// uk_pr_info("Brreaking; E1000_RXD_STAT_DD not set; status is %X\n", status);
			// uk_pr_info("buffer_addr = %d\n", rxdp->buffer_addr);
			// uk_pr_info("length = %d\n", rxdp->length);
			// uk_pr_info("csum = %d\n", rxdp->csum);
			// uk_pr_info("status = %d\n", rxdp->status);
			// uk_pr_info("errors = %d\n", rxdp->errors);
			// uk_pr_info("special = %d\n", rxdp->special);
			return 0; // TODO
			break;
		}
		rxd = *rxdp;

		/*
		 * End of packet.
		 *
		 * If the E1000_RXD_STAT_EOP flag is not set, the RX packet is
		 * likely to be invalid and to be dropped by the various
		 * validation checks performed by the network stack.
		 *
		 * Allocate a new mbuf to replenish the RX ring descriptor.
		 * If the allocation fails:
		 *    - arrange for that RX descriptor to be the first one
		 *      being parsed the next time the receive function is
		 *      invoked [on the same queue].
		 *
		 *    - Stop parsing the RX ring and return immediately.
		 *
		 * This policy do not drop the packet received in the RX
		 * descriptor for which the allocation of a new mbuf failed.
		 * Thus, it allows that packet to be later retrieved if
		 * mbuf have been freed in the mean time.
		 * As a side effect, holding RX descriptors instead of
		 * systematically giving them back to the NIC may lead to
		 * RX ring exhaustion situations.
		 * However, the NIC can gracefully prevent such situations
		 * to happen by sending specific "back-pressure" flow control
		 * frames to its peer(s).
		 */
		// uk_pr_info("queue_id=%u rx_id=%u "
		// 	   "status=0x%x pkt_len=%u\n",
		// 	   (unsigned) rxq->queue_id,
		// 	   (unsigned) rx_id, (unsigned) status,
		// 	   (unsigned) rxd.length);


		struct uk_netbuf *mbufs[1];
		rxq->alloc_rxpkts(rxq->alloc_rxpkts_argp, mbufs, 1);
		nmb = mbufs[0];
		// nmb = uk_netbuf_alloc_buf(rxq->a, 2048, 128, 128, 0, NULL);
		if (nmb == NULL) {
			// uk_pr_info("RX mbuf alloc failed queue_id=%u",
			// 	   (unsigned) rxq->queue_id);
			break;
		}

		nb_hold++;
		rxe = &sw_ring[rx_id];
		rx_id++;
		if (rx_id == rxq->nb_rx_desc)
			rx_id = 0;

		/* Rearm RXD: attach new mbuf and reset status to zero. */
		rxm = rxe->mbuf;
		rxe->mbuf = nmb;
		dma_addr = nmb->buf;
		// uk_pr_info("Descriptor has buff address set to %X\n", rxdp->buffer_addr);

		rxdp->buffer_addr = dma_addr;
		rxdp->status = 0;

		/*
		 * Initialize the returned mbuf.
		 * 1) setup generic mbuf fields:
		 *    - number of segments,
		 *    - next segment,
		 *    - packet length,
		 *    - RX port identifier.
		 * 2) integrate hardware offload data, if any:
		 *    - RSS flag & hash,
		 *    - IP checksum flag,
		 *    - VLAN TCI, if any,
		 *    - error flags.
		 */
		pkt_len = (uint16_t) rxd.length - rxq->crc_len;
		rxm->buflen = pkt_len;
		rxm->len = pkt_len;
		rxm->next = NULL;
		// rxm->nb_segs = 1;
		ret_status |= UK_NETDEV_STATUS_SUCCESS;

		/*
		 * Store the mbuf address into the next entry of the array
		 * of returned packets.
		 */
		*pkt = rxm;
		// uk_pr_info("Buff content (of len %d) at %p: ", rxm->buflen, rxm->buf);
		// for (int i = 0; i < rxm->buflen; i++) {
		// 	uk_pr_info("%x", ((char *) rxm->buf)[i]);
		// }
		// uk_pr_info("\n");
		nb_rx++;
	}
	rxq->rx_tail = rx_id;

	/*
	 * If the number of free RX descriptors is greater than the RX free
	 * threshold of the queue, advance the Receive Descriptor Tail (RDT)
	 * register.
	 * Update the RDT with the value of the last processed RX descriptor
	 * minus 1, to guarantee that the RDT register is never equal to the
	 * RDH register, which creates a "full" ring situtation from the
	 * hardware point of view...
	 */
	nb_hold = (uint16_t) (nb_hold + rxq->nb_rx_hold);
	if (nb_hold > rxq->rx_free_thresh) {
		debug_uk_pr_info("queue_id=%u rx_tail=%u "
			   "nb_hold=%u nb_rx=%u\n",
			   (unsigned) rxq->queue_id, (unsigned) rx_id,
			   (unsigned) nb_hold, (unsigned) nb_rx);
		rx_id = (uint16_t) ((rx_id == 0) ?
			(rxq->nb_rx_desc - 1) : (rx_id - 1));
		E1000_PCI_REG_WRITE(rxq->rdt_reg_addr, rx_id);
		nb_hold = 0;
	}
	rxq->nb_rx_hold = nb_hold;
	ret_status |= UK_NETDEV_STATUS_MORE;
	return ret_status;
}

#define	EM_MAX_BUF_SIZE     16384
#define EM_RCTL_FLXBUF_STEP 1024

static void
em_tx_queue_release_mbufs(struct em_tx_queue *txq)
{
	unsigned i;

	if (txq->sw_ring != NULL) {
		for (i = 0; i != txq->nb_tx_desc; i++) {
			if (txq->sw_ring[i].mbuf != NULL) {
				uk_netbuf_free_single(txq->sw_ring[i].mbuf); // TODO double-check
				txq->sw_ring[i].mbuf = NULL;
			}
		}
	}
}

static void
em_tx_queue_release(struct em_tx_queue *txq)
{
	if (txq != NULL) {
		em_tx_queue_release_mbufs(txq);
		// TODO free
		// uk_free(a, txq->sw_ring);
		// uk_free(a, txq);
	}
}

// void
// eth_em_tx_queue_release(void *txq)
// {
// 	em_tx_queue_release(txq);
// }

/* (Re)set dynamic em_tx_queue fields to defaults */
static void
em_reset_tx_queue(struct em_tx_queue *txq)
{
	uint16_t i, nb_desc, prev;
	static const struct e1000_data_desc txd_init = {
		.upper.fields = {.status = E1000_TXD_STAT_DD},
	};

	nb_desc = txq->nb_tx_desc;

	/* Initialize ring entries */

	prev = (uint16_t) (nb_desc - 1);

	for (i = 0; i < nb_desc; i++) {
		txq->tx_ring[i] = txd_init;
		txq->sw_ring[i].mbuf = NULL;
		txq->sw_ring[i].last_id = i;
		txq->sw_ring[prev].next_id = i;
		prev = i;
	}

	/*
	 * Always allow 1 descriptor to be un-allocated to avoid
	 * a H/W race condition
	 */
	txq->nb_tx_free = (uint16_t)(nb_desc - 1);
	txq->last_desc_cleaned = (uint16_t)(nb_desc - 1);
	txq->nb_tx_used = 0;
	txq->tx_tail = 0;

	memset((void*)&txq->ctx_cache, 0, sizeof (txq->ctx_cache));
}


struct uk_netdev_tx_queue *
eth_em_tx_queue_setup(struct uk_netdev *dev,
			 uint16_t queue_idx,
			 uint16_t nb_desc,
			 __unused struct uk_netdev_txqueue_conf *tx_conf)
{
	debug_uk_pr_info("eth_em_tx_queue_setup\n");

	void *mem;
	struct em_tx_queue *txq;
	struct e1000_hw     *hw;
	uint16_t tx_rs_thresh, tx_free_thresh;
	
	hw = to_e1000dev(dev);

	debug_uk_pr_info("dev->_rx_queue[0] = %p\n", dev->_rx_queue[0]);
	/*
	 * Validate number of transmit descriptors.
	 * It must not exceed hardware maximum, and must be multiple
	 * of E1000_ALIGN.
	 */
	nb_desc = 32;
	if (nb_desc % EM_TXD_ALIGN != 0 ||
			(nb_desc > E1000_MAX_RING_DESC) ||
			(nb_desc < E1000_MIN_RING_DESC)) {
		return NULL;
		// return -(EINVAL);
	}

	tx_free_thresh = DEFAULT_TX_FREE_THRESH;
	tx_rs_thresh = DEFAULT_TX_RS_THRESH;

	if (tx_free_thresh >= (nb_desc - 3)) {
		uk_pr_err("tx_free_thresh must be less than the "
			     "number of TX descriptors minus 3. "
			     "(tx_free_thresh=%u queue=%d)",
			     (unsigned int)tx_free_thresh, (int)queue_idx);
		return NULL;
		// return -(EINVAL);
	}
	if (tx_rs_thresh > tx_free_thresh) {
		uk_pr_err("tx_rs_thresh must be less than or equal to "
			     "tx_free_thresh. (tx_free_thresh=%u "
			     "tx_rs_thresh=%u queue=%d)",
			     (unsigned int)tx_free_thresh,
			     (unsigned int)tx_rs_thresh,
			     (int)queue_idx);
		return NULL;
		// return -(EINVAL);
	}

	// /*
	//  * If rs_bit_thresh is greater than 1, then TX WTHRESH should be
	//  * set to 0. If WTHRESH is greater than zero, the RS bit is ignored
	//  * by the NIC and all descriptors are written back after the NIC
	//  * accumulates WTHRESH descriptors.
	//  */
	// if (tx_conf->tx_thresh.wthresh != 0 && tx_rs_thresh != 1) {
	// 	uk_pr_err("TX WTHRESH must be set to 0 if "
	// 		     "tx_rs_thresh is greater than 1. (tx_rs_thresh=%u "
	// 		     "queue=%d)", (unsigned int)tx_rs_thresh,
	// 			 (int)queue_idx);
	// 	return -(EINVAL);
	// }

	/* Free memory prior to re-allocation if needed... */
	if (dev->_tx_queue[queue_idx] != NULL) {
		em_tx_queue_release(dev->_tx_queue[queue_idx]);
		dev->_tx_queue[queue_idx] = NULL;
	}

	/*
	 * Allocate TX ring hardware descriptors. A memzone large enough to
	 * handle the maximum ring size is allocated in order to allow for
	 * resizing in later calls to the queue setup function.
	 */
	// mem = uk_calloc(hw->a, E1000_MAX_RING_DESC, sizeof(txq->tx_ring[0]));
	mem = uk_memalign(hw->a, 16, E1000_MAX_RING_DESC * sizeof(txq->tx_ring[0]));
	if (mem == NULL) {
		return NULL;
		// return -ENOMEM;
	}

	/* Allocate the tx queue data structure. */
	if ((txq = uk_calloc(hw->a, 1, sizeof(*txq))) == NULL) {
		return NULL;
		// return -ENOMEM;
	}

	// /* Allocate software ring */
	if ((txq->sw_ring = uk_calloc(hw->a, nb_desc,
			sizeof(txq->sw_ring[0]))) == NULL) {
		em_tx_queue_release(txq);
		return NULL;
		// return -ENOMEM;
	}

	txq->nb_tx_desc = nb_desc;
	txq->tx_free_thresh = tx_free_thresh;
	txq->tx_rs_thresh = tx_rs_thresh;
	txq->pthresh = 4;
	txq->hthresh = 0;
	txq->wthresh = 0;
	txq->queue_id = queue_idx;
	txq->hw = hw;
	// txq->port_id = dev->data->port_id;

	txq->tdt_reg_addr = E1000_PCI_REG_ADDR(hw, E1000_TDT(queue_idx));
	txq->tx_ring_phys_addr = mem;
	txq->tx_ring = (struct e1000_data_desc *) mem;

	// PMD_INIT_LOG(DEBUG, "sw_ring=%p hw_ring=%p dma_addr=0x%"PRIx64,
	// 	     txq->sw_ring, txq->tx_ring, txq->tx_ring_phys_addr);

	em_reset_tx_queue(txq);

	dev->_tx_queue[queue_idx] = txq;
	hw->nb_tx_queues++;

	return txq;
}

static void
em_rx_queue_release_mbufs(struct em_rx_queue *rxq)
{
	unsigned i;

	debug_uk_pr_info("em_rx_queue_release_mbufs\n");
	if (rxq->sw_ring != NULL) {
		for (i = 0; i != rxq->nb_rx_desc; i++) {
			if (rxq->sw_ring[i].mbuf != NULL) {
				// rte_pktmbuf_free_seg(rxq->sw_ring[i].mbuf);
				uk_netbuf_free_single(rxq->sw_ring[i].mbuf); // TODO double-check
				rxq->sw_ring[i].mbuf = NULL;
			}
		}
	}
}

static void
em_rx_queue_release(struct em_rx_queue *rxq)
{
	debug_uk_pr_info("em_rx_queue_release\n");

	if (rxq != NULL) {
		em_rx_queue_release_mbufs(rxq);
		// TODO free
		// uk_free(a, rxq->sw_ring);
		// uk_free(a, rxq);
	}
}

// void
// eth_em_rx_queue_release(void *rxq)
// {
// 	em_rx_queue_release(rxq);
// }

// /* Reset dynamic em_rx_queue fields back to defaults */
static void
em_reset_rx_queue(struct em_rx_queue *rxq)
{
	rxq->rx_tail = 0;
	rxq->nb_rx_hold = 0;
	rxq->pkt_first_seg = NULL;
	rxq->pkt_last_seg = NULL;
}

struct uk_netdev_rx_queue *
eth_em_rx_queue_setup(struct uk_netdev *dev,
		uint16_t queue_idx,
		uint16_t nb_desc,
		struct uk_netdev_rxqueue_conf *rx_conf)
{
	debug_uk_pr_info("eth_em_rx_queue_setup\n");

	UK_ASSERT(rx_conf->alloc_rxpkts);

	void *mem;
	struct em_rx_queue *rxq;
	struct e1000_hw     *hw;

	hw = to_e1000dev(dev);

	/*
	 * Validate number of receive descriptors.
	 * It must not exceed hardware maximum, and must be multiple
	 * of E1000_ALIGN.
	 */
	nb_desc = 32;
	if (nb_desc % EM_RXD_ALIGN != 0 ||
			(nb_desc > E1000_MAX_RING_DESC) ||
			(nb_desc < E1000_MIN_RING_DESC)) {
		uk_pr_err("Invalid number of received descriptors %d\n", nb_desc);
		return NULL;
		// return -EINVAL;
	}

	/* Free memory prior to re-allocation if needed. */
	if (dev->_rx_queue[queue_idx] != NULL) {
		// em_rx_queue_release(hw->a, dev->_rx_queue[queue_idx]);
		em_rx_queue_release(dev->_rx_queue[queue_idx]);
		debug_uk_pr_info("Setting dev->_rx_queue[queue_idx] to null\n");
		dev->_rx_queue[queue_idx] = NULL;
	}

	/* Allocate RX ring for max possible mumber of hardware descriptors. */
	// mem = uk_calloc(hw->a, E1000_MAX_RING_DESC, sizeof(rxq->rx_ring[0]));
	// mem = uk_malloc(hw->a, ALIGN_UP(E1000_MAX_RING_DESC * sizeof(rxq->rx_ring[0]), 16));
	mem = uk_memalign(hw->a, 16, E1000_MAX_RING_DESC * sizeof(rxq->rx_ring[0]));
	if (mem == NULL) {
		uk_pr_err("Failed to allocate RX ring\n");
		return NULL;
		// return -ENOMEM;
	}

	/* Allocate the RX queue data structure. */
	if ((rxq = uk_calloc(hw->a, 1, sizeof(*rxq))) == NULL) {
		uk_pr_err("Failed to allocate RX queue data\n");
		return NULL;
		// return -ENOMEM;
	}

	/* Allocate software ring. */
	if ((rxq->sw_ring = uk_calloc(hw->a, nb_desc, sizeof (rxq->sw_ring[0]))) == NULL) {
		uk_pr_err("Failed to allocate software ring\n");
		em_rx_queue_release(rxq);
		return NULL;
		// return -ENOMEM;
	}

	// uk_sglist_init(&rxq->sg, (sizeof(rxq.sgsegs) / sizeof(rxq.sgsegs[0])), &rxq.sgsegs[0]);

	rxq->nb_rx_desc = nb_desc;
	rxq->pthresh = 8;
	rxq->hthresh = 8;
	rxq->wthresh = 36;
	rxq->rx_free_thresh = 0;
	rxq->queue_id = queue_idx;
	rxq->crc_len = 0;
	rxq->a = hw->a;

	rxq->rdt_reg_addr = E1000_PCI_REG_ADDR(hw, E1000_RDT(queue_idx));
	rxq->rdh_reg_addr = E1000_PCI_REG_ADDR(hw, E1000_RDH(queue_idx));
	rxq->rx_ring_phys_addr = mem;
	rxq->rx_ring = (struct e1000_rx_desc *) mem;

	dev->_rx_queue[queue_idx] = rxq;
	hw->nb_rx_queues++;
	em_reset_rx_queue(rxq);

	rxq->alloc_rxpkts = rx_conf->alloc_rxpkts;
	rxq->alloc_rxpkts_argp = rx_conf->alloc_rxpkts_argp;

	return rxq;
}

// uint32_t
// eth_em_rx_queue_count(struct rte_eth_dev *dev, uint16_t rx_queue_id)
// {
// #define EM_RXQ_SCAN_INTERVAL 4
// 	volatile struct e1000_rx_desc *rxdp;
// 	struct em_rx_queue *rxq;
// 	uint32_t desc = 0;

// 	rxq = dev->data->rx_queues[rx_queue_id];
// 	rxdp = &(rxq->rx_ring[rxq->rx_tail]);

// 	while ((desc < rxq->nb_rx_desc) &&
// 		(rxdp->status & E1000_RXD_STAT_DD)) {
// 		desc += EM_RXQ_SCAN_INTERVAL;
// 		rxdp += EM_RXQ_SCAN_INTERVAL;
// 		if (rxq->rx_tail + desc >= rxq->nb_rx_desc)
// 			rxdp = &(rxq->rx_ring[rxq->rx_tail +
// 				desc - rxq->nb_rx_desc]);
// 	}

// 	return desc;
// }

// int
// eth_em_rx_descriptor_done(void *rx_queue, uint16_t offset)
// {
// 	volatile struct e1000_rx_desc *rxdp;
// 	struct em_rx_queue *rxq = rx_queue;
// 	uint32_t desc;

// 	if (unlikely(offset >= rxq->nb_rx_desc))
// 		return 0;
// 	desc = rxq->rx_tail + offset;
// 	if (desc >= rxq->nb_rx_desc)
// 		desc -= rxq->nb_rx_desc;

// 	rxdp = &rxq->rx_ring[desc];
// 	return !!(rxdp->status & E1000_RXD_STAT_DD);
// }

// int
// eth_em_rx_descriptor_status(void *rx_queue, uint16_t offset)
// {
// 	struct em_rx_queue *rxq = rx_queue;
// 	volatile uint8_t *status;
// 	uint32_t desc;

// 	if (unlikely(offset >= rxq->nb_rx_desc))
// 		return -EINVAL;

// 	if (offset >= rxq->nb_rx_desc - rxq->nb_rx_hold)
// 		return RTE_ETH_RX_DESC_UNAVAIL;

// 	desc = rxq->rx_tail + offset;
// 	if (desc >= rxq->nb_rx_desc)
// 		desc -= rxq->nb_rx_desc;

// 	status = &rxq->rx_ring[desc].status;
// 	if (*status & E1000_RXD_STAT_DD)
// 		return RTE_ETH_RX_DESC_DONE;

// 	return RTE_ETH_RX_DESC_AVAIL;
// }

// int
// eth_em_tx_descriptor_status(void *tx_queue, uint16_t offset)
// {
// 	struct em_tx_queue *txq = tx_queue;
// 	volatile uint8_t *status;
// 	uint32_t desc;

// 	if (unlikely(offset >= txq->nb_tx_desc))
// 		return -EINVAL;

// 	desc = txq->tx_tail + offset;
// 	/* go to next desc that has the RS bit */
// 	desc = ((desc + txq->tx_rs_thresh - 1) / txq->tx_rs_thresh) *
// 		txq->tx_rs_thresh;
// 	if (desc >= txq->nb_tx_desc) {
// 		desc -= txq->nb_tx_desc;
// 		if (desc >= txq->nb_tx_desc)
// 			desc -= txq->nb_tx_desc;
// 	}

// 	status = &txq->tx_ring[desc].upper.fields.status;
// 	if (*status & E1000_TXD_STAT_DD)
// 		return RTE_ETH_TX_DESC_DONE;

// 	return RTE_ETH_TX_DESC_FULL;
// }

void
em_dev_clear_queues(struct uk_netdev *dev)
{
	uint16_t i;
	struct em_tx_queue *txq;
	struct em_rx_queue *rxq;
	struct e1000_hw *hw;

	hw = to_e1000dev(dev);

	for (i = 0; i < hw->nb_tx_queues; i++) {
		txq = dev->_tx_queue[i];
		if (txq != NULL) {
			em_tx_queue_release_mbufs(txq);
			em_reset_tx_queue(txq);
		}
	}

	for (i = 0; i < hw->nb_rx_queues; i++) {
		rxq = dev->_rx_queue[i];
		if (rxq != NULL) {
			em_rx_queue_release_mbufs(rxq);
			em_reset_rx_queue(rxq);
		}
	}
}

// void
// em_dev_free_queues(struct rte_eth_dev *dev)
// {
// 	uint16_t i;

// 	for (i = 0; i < dev->data->nb_rx_queues; i++) {
// 		eth_em_rx_queue_release(dev->data->rx_queues[i]);
// 		dev->data->rx_queues[i] = NULL;
// 	}
// 	dev->data->nb_rx_queues = 0;

// 	for (i = 0; i < dev->data->nb_tx_queues; i++) {
// 		eth_em_tx_queue_release(dev->data->tx_queues[i]);
// 		dev->data->tx_queues[i] = NULL;
// 	}
// 	dev->data->nb_tx_queues = 0;
// }

/*
 * Takes as input/output parameter RX buffer size.
 * Returns (BSIZE | BSEX | FLXBUF) fields of RCTL register.
 */
static uint32_t
em_rctl_bsize(__unused enum e1000_mac_type hwtyp, uint32_t *bufsz)
{
	/*
	 * For BSIZE & BSEX all configurable sizes are:
	 * 16384: rctl |= (E1000_RCTL_SZ_16384 | E1000_RCTL_BSEX);
	 *  8192: rctl |= (E1000_RCTL_SZ_8192  | E1000_RCTL_BSEX);
	 *  4096: rctl |= (E1000_RCTL_SZ_4096  | E1000_RCTL_BSEX);
	 *  2048: rctl |= E1000_RCTL_SZ_2048;
	 *  1024: rctl |= E1000_RCTL_SZ_1024;
	 *   512: rctl |= E1000_RCTL_SZ_512;
	 *   256: rctl |= E1000_RCTL_SZ_256;
	 */
	static const struct {
		uint32_t bufsz;
		uint32_t rctl;
	} bufsz_to_rctl[] = {
		{16384, (E1000_RCTL_SZ_16384 | E1000_RCTL_BSEX)},
		{8192,  (E1000_RCTL_SZ_8192  | E1000_RCTL_BSEX)},
		{4096,  (E1000_RCTL_SZ_4096  | E1000_RCTL_BSEX)},
		{2048,  E1000_RCTL_SZ_2048},
		{1024,  E1000_RCTL_SZ_1024},
		{512,   E1000_RCTL_SZ_512},
		{256,   E1000_RCTL_SZ_256},
	};

	int i;
	uint32_t rctl_bsize;

	debug_uk_pr_info("em_rctl_bsize\n");

	rctl_bsize = *bufsz;

	/*
	 * Starting from 82571 it is possible to specify RX buffer size
	 * by RCTL.FLXBUF. When this field is different from zero, the
	 * RX buffer size = RCTL.FLXBUF * 1K
	 * (e.g. t is possible to specify RX buffer size  1,2,...,15KB).
	 * It is working ok on real HW, but by some reason doesn't work
	 * on VMware emulated 82574L.
	 * So for now, always use BSIZE/BSEX to setup RX buffer size.
	 * If you don't plan to use it on VMware emulated 82574L and
	 * would like to specify RX buffer size in 1K granularity,
	 * uncomment the following lines:
	 * ***************************************************************
	 * if (hwtyp >= e1000_82571 && hwtyp <= e1000_82574 &&
	 *		rctl_bsize >= EM_RCTL_FLXBUF_STEP) {
	 *	rctl_bsize /= EM_RCTL_FLXBUF_STEP;
	 *	*bufsz = rctl_bsize;
	 *	return (rctl_bsize << E1000_RCTL_FLXBUF_SHIFT &
	 *		E1000_RCTL_FLXBUF_MASK);
	 * }
	 * ***************************************************************
	 */

	for (i = 0; i != sizeof(bufsz_to_rctl) / sizeof(bufsz_to_rctl[0]);
			i++) {
		if (rctl_bsize >= bufsz_to_rctl[i].bufsz) {
			*bufsz = bufsz_to_rctl[i].bufsz;
			return bufsz_to_rctl[i].rctl;
		}
	}

	/* Should never happen. */
	return -EINVAL;
}

static int
em_alloc_rx_queue_mbufs(struct em_rx_queue *rxq)
{
	struct em_rx_entry *rxe = rxq->sw_ring;
	uint64_t dma_addr;
	unsigned i;
	static const struct e1000_rx_desc rxd_init = {
		.buffer_addr = 0,
	};

	debug_uk_pr_info("em_alloc_rx_queue_mbufs\n");

	/* Initialize software ring entries */
	debug_uk_pr_info("rxq->nb_rx_desc %d\n", rxq->nb_rx_desc);
	for (i = 0; i < rxq->nb_rx_desc; i++) {
		volatile struct e1000_rx_desc *rxd;
		debug_uk_pr_info("before uk_netbuf_alloc_buf\n");
		debug_uk_pr_info("rxq->a = %p\n", rxq->a);
		
		// struct uk_netbuf *mbuf = uk_netbuf_alloc_buf(rxq->a, 2048, 128, 128, 0, NULL);
		struct uk_netbuf *mbufs[1];
		rxq->alloc_rxpkts(rxq->alloc_rxpkts_argp, mbufs, 1);
		struct uk_netbuf *mbuf = mbufs[0];

		debug_uk_pr_info("after uk_netbuf_alloc_buf\n");

		if (mbuf == NULL) {
			uk_pr_err("RX mbuf alloc failed "
				     "queue_id=%hu\n", rxq->queue_id);
			return -ENOMEM;
		}

		// TODO mbuf->buf or mbuf->data?
		dma_addr = mbuf->buf;

		/* Clear HW ring memory */
		rxq->rx_ring[i] = rxd_init;

		rxd = &rxq->rx_ring[i];
		rxd->buffer_addr = dma_addr;
		rxe[i].mbuf = mbuf;
	}

	return 0;
}

/*********************************************************************
 *
 *  Enable receive unit.
 *
 **********************************************************************/
int
eth_em_rx_init(struct e1000_hw *hw)
{
	struct em_rx_queue *rxq;
	struct uk_netdev *dev;
	uint32_t rctl;
	uint32_t rfctl;
	uint32_t rxcsum;
	uint32_t rctl_bsize;
	uint16_t i;
	int ret;

	debug_uk_pr_info("eth_em_rx_init\n");

	dev = &hw->netdev;
	debug_uk_pr_info("hw = %p, dev = %p\n", hw, dev);

	// rxmode = &dev->data->dev_conf.rxmode;

	/*
	 * Make sure receives are disabled while setting
	 * up the descriptor ring.
	 */
	rctl = E1000_READ_REG(hw, E1000_RCTL);
	E1000_WRITE_REG(hw, E1000_RCTL, rctl & ~E1000_RCTL_EN);

	rfctl = E1000_READ_REG(hw, E1000_RFCTL);

	/* Disable extended descriptor type. */
	rfctl &= ~E1000_RFCTL_EXTEN;

	E1000_WRITE_REG(hw, E1000_RFCTL, rfctl);

	// dev->rx_pkt_burst = (eth_rx_burst_t)eth_em_recv_pkts;

	/* Determine RX bufsize. */
	rctl_bsize = EM_MAX_BUF_SIZE;
	// for (i = 0; i < hw->nb_rx_queues; i++) {
	// 	uint32_t buf_size;

	// 	rxq = dev->_rx_queue[i];
	// 	buf_size = rte_pktmbuf_data_room_size(rxq->mb_pool) -
	// 		RTE_PKTMBUF_HEADROOM;
	// 	rctl_bsize = RTE_MIN(rctl_bsize, buf_size);
	// }

	rctl |= em_rctl_bsize(hw->mac.type, &rctl_bsize);

	/* Configure and enable each RX queue. */
	for (i = 0; i < hw->nb_rx_queues; i++) {
		uint64_t bus_addr;
		uint32_t rxdctl;

		debug_uk_pr_info("dev->_rx_queue = %p\n", dev->_rx_queue);
		debug_uk_pr_info("i = %d\n", i);
		rxq = dev->_rx_queue[i];
		debug_uk_pr_info("dev->_rx_queue[%d] = %p\n", i, dev->_rx_queue[i]);

		/* Allocate buffers for descriptor rings and setup queue */
		debug_uk_pr_info("i = %d, rxq = %p, rxq->a = %p\n", i, rxq, rxq->a);
		ret = em_alloc_rx_queue_mbufs(rxq);
		if (ret)
			return ret;

		/*
		 * Reset crc_len in case it was changed after queue setup by a
		 *  call to configure
		 */
		rxq->crc_len = 0;

		bus_addr = rxq->rx_ring_phys_addr;
		E1000_WRITE_REG(hw, E1000_RDLEN(i),
				rxq->nb_rx_desc *
				sizeof(*rxq->rx_ring));
		E1000_WRITE_REG(hw, E1000_RDBAH(i),
				(uint32_t)(bus_addr >> 32));
		E1000_WRITE_REG(hw, E1000_RDBAL(i), (uint32_t)bus_addr);

		E1000_WRITE_REG(hw, E1000_RDH(i), 0);
		E1000_WRITE_REG(hw, E1000_RDT(i), rxq->nb_rx_desc - 1);

		rxdctl = E1000_READ_REG(hw, E1000_RXDCTL(0));
		rxdctl &= 0xFE000000;
		rxdctl |= rxq->pthresh & 0x3F;
		rxdctl |= (rxq->hthresh & 0x3F) << 8;
		rxdctl |= (rxq->wthresh & 0x3F) << 16;
		rxdctl |= E1000_RXDCTL_GRAN;
		E1000_WRITE_REG(hw, E1000_RXDCTL(i), rxdctl);
	}

	/*
	 * Setup the Checksum Register.
	 * Receive Full-Packet Checksum Offload is mutually exclusive with RSS.
	 */
	rxcsum = E1000_READ_REG(hw, E1000_RXCSUM);
	rxcsum &= ~E1000_RXCSUM_IPOFL;
	E1000_WRITE_REG(hw, E1000_RXCSUM, rxcsum);

	/* No MRQ or RSS support for now */

	rctl &= ~(3 << E1000_RCTL_MO_SHIFT);
	rctl |= E1000_RCTL_EN | E1000_RCTL_BAM | E1000_RCTL_LBM_NO |
		E1000_RCTL_RDMTS_HALF |
		(hw->mac.mc_filter_type << E1000_RCTL_MO_SHIFT);

	/* Make sure VLAN Filters are off. */
	rctl &= ~E1000_RCTL_VFE;
	/* Don't store bad packets. */
	rctl &= ~E1000_RCTL_SBP;
	/* Legacy descriptor type. */
	rctl &= ~E1000_RCTL_DTYP_MASK;

	/*
	 * Configure support of jumbo frames, if any.
	 */
	rctl &= ~E1000_RCTL_LPE;

	/* Enable Receives. */
	E1000_WRITE_REG(hw, E1000_RCTL, rctl);

	return 0;
}

/*********************************************************************
 *
 *  Enable transmit unit.
 *
 **********************************************************************/
void
eth_em_tx_init(struct e1000_hw *hw)
{
	struct uk_netdev *dev;
	struct em_tx_queue *txq;
	uint32_t tctl;
	uint32_t txdctl;
	uint16_t i;

	dev = &hw->netdev;

	debug_uk_pr_info("eth_em_tx_init\n");

	/* Setup the Base and Length of the Tx Descriptor Rings. */
	for (i = 0; i < hw->nb_tx_queues; i++) {
		uint64_t bus_addr;
		txq = dev->_tx_queue[i];
		bus_addr = txq->tx_ring_phys_addr;
		E1000_WRITE_REG(hw, E1000_TDLEN(i),
				txq->nb_tx_desc *
				sizeof(*txq->tx_ring));
		E1000_WRITE_REG(hw, E1000_TDBAH(i),
				(uint32_t)(bus_addr >> 32));
		E1000_WRITE_REG(hw, E1000_TDBAL(i), (uint32_t)bus_addr);

		/* Setup the HW Tx Head and Tail descriptor pointers. */
		E1000_WRITE_REG(hw, E1000_TDT(i), 0);
		E1000_WRITE_REG(hw, E1000_TDH(i), 0);

		/* Setup Transmit threshold registers. */
		txdctl = E1000_READ_REG(hw, E1000_TXDCTL(i));
		/*
		 * bit 22 is reserved, on some models should always be 0,
		 * on others  - always 1.
		 */
		txdctl &= E1000_TXDCTL_COUNT_DESC;
		txdctl |= txq->pthresh & 0x3F;
		txdctl |= (txq->hthresh & 0x3F) << 8;
		txdctl |= (txq->wthresh & 0x3F) << 16;
		txdctl |= E1000_TXDCTL_GRAN;
		E1000_WRITE_REG(hw, E1000_TXDCTL(i), txdctl);
	}

	/* Program the Transmit Control Register. */
	tctl = E1000_READ_REG(hw, E1000_TCTL);
	tctl &= ~E1000_TCTL_CT;
	tctl |= (E1000_TCTL_PSP | E1000_TCTL_RTLC | E1000_TCTL_EN |
		 (E1000_COLLISION_THRESHOLD << E1000_CT_SHIFT));

	/* This write will effectively turn on the transmit unit. */
	E1000_WRITE_REG(hw, E1000_TCTL, tctl);
}

int
em_rxq_info_get(__unused struct uk_netdev *dev, __unused uint16_t queue_id,
	__unused struct uk_netdev_queue_info *qinfo)
{
	// TODO: uncomment
	debug_uk_pr_info("em_rxq_info_get\n");

	// struct em_rx_queue *rxq;

	// rxq = dev->data->rx_queues[queue_id];

	// qinfo->mp = rxq->mb_pool;
	// qinfo->scattered_rx = dev->data->scattered_rx;
	// qinfo->nb_desc = rxq->nb_rx_desc;
	// qinfo->conf.rx_free_thresh = rxq->rx_free_thresh;
	// qinfo->conf.offloads = rxq->offloads;

	return 0;
}

int
em_txq_info_get(__unused struct uk_netdev *dev, __unused uint16_t queue_id,
	__unused struct uk_netdev_queue_info *qinfo)
{
	// TODO: uncomment
	debug_uk_pr_info("em_txq_info_get\n");

	// struct em_tx_queue *txq;

	// txq = dev->data->tx_queues[queue_id];

	// qinfo->nb_desc = txq->nb_tx_desc;

	// qinfo->conf.tx_thresh.pthresh = txq->pthresh;
	// qinfo->conf.tx_thresh.hthresh = txq->hthresh;
	// qinfo->conf.tx_thresh.wthresh = txq->wthresh;
	// qinfo->conf.tx_free_thresh = txq->tx_free_thresh;
	// qinfo->conf.tx_rs_thresh = txq->tx_rs_thresh;
	// qinfo->conf.offloads = txq->offloads;

	return 0;
}
