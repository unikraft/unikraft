/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2010-2015 Intel Corporation
 */

#ifndef _E1000_ETHDEV_H_
#define _E1000_ETHDEV_H_

#include <stdint.h>

#define E1000_INTEL_VENDOR_ID 0x8086

/* need update link, bit flag */
#define E1000_FLAG_NEED_LINK_UPDATE (uint32_t)(1 << 0)
#define E1000_FLAG_MAILBOX          (uint32_t)(1 << 1)

/*
 * Defines that were not part of e1000_hw.h as they are not used by the FreeBSD
 * driver.
 */
#define E1000_ADVTXD_POPTS_TXSM     0x00000200 /* L4 Checksum offload request */
#define E1000_ADVTXD_POPTS_IXSM     0x00000100 /* IP Checksum offload request */
#define E1000_ADVTXD_TUCMD_L4T_RSV  0x00001800 /* L4 Packet TYPE of Reserved */
#define E1000_RXD_STAT_TMST         0x10000    /* Timestamped Packet indication */
#define E1000_RXD_ERR_CKSUM_BIT     29
#define E1000_RXD_ERR_CKSUM_MSK     3
#define E1000_ADVTXD_MACLEN_SHIFT   9          /* Bit shift for l2_len */
#define E1000_CTRL_EXT_EXTEND_VLAN  (1<<26)    /* EXTENDED VLAN */

#define E1000_SYN_FILTER_ENABLE        0x00000001 /* syn filter enable field */
#define E1000_SYN_FILTER_QUEUE         0x0000000E /* syn filter queue field */
#define E1000_SYN_FILTER_QUEUE_SHIFT   1          /* syn filter queue field */
#define E1000_RFCTL_SYNQFP             0x00080000 /* SYNQFP in RFCTL register */

#define E1000_ETQF_ETHERTYPE           0x0000FFFF
#define E1000_ETQF_QUEUE               0x00070000
#define E1000_ETQF_QUEUE_SHIFT         16
#define E1000_MAX_ETQF_FILTERS         8

#define E1000_IMIR_DSTPORT             0x0000FFFF
#define E1000_IMIR_PRIORITY            0xE0000000
#define E1000_MAX_TTQF_FILTERS         8
#define E1000_2TUPLE_MAX_PRI           7

#define E1000_MAX_FLEX_FILTERS           8
#define E1000_MAX_FHFT                   4
#define E1000_MAX_FHFT_EXT               4
#define E1000_FHFT_SIZE_IN_DWD           64
#define E1000_MAX_FLEX_FILTER_PRI        7
#define E1000_MAX_FLEX_FILTER_LEN        128
#define E1000_MAX_FLEX_FILTER_DWDS \
	(E1000_MAX_FLEX_FILTER_LEN / sizeof(uint32_t))
#define E1000_FLEX_FILTERS_MASK_SIZE \
	(E1000_MAX_FLEX_FILTER_DWDS / 2)
#define E1000_FHFT_QUEUEING_LEN          0x0000007F
#define E1000_FHFT_QUEUEING_QUEUE        0x00000700
#define E1000_FHFT_QUEUEING_PRIO         0x00070000
#define E1000_FHFT_QUEUEING_OFFSET       0xFC
#define E1000_FHFT_QUEUEING_QUEUE_SHIFT  8
#define E1000_FHFT_QUEUEING_PRIO_SHIFT   16
#define E1000_WUFC_FLEX_HQ               0x00004000

#define E1000_SPQF_SRCPORT               0x0000FFFF

#define E1000_MAX_FTQF_FILTERS           8
#define E1000_FTQF_PROTOCOL_MASK         0x000000FF
#define E1000_FTQF_5TUPLE_MASK_SHIFT     28
#define E1000_FTQF_QUEUE_MASK            0x03ff0000
#define E1000_FTQF_QUEUE_SHIFT           16
#define E1000_FTQF_QUEUE_ENABLE          0x00000100

/*
 * The overhead from MTU to max frame size.
 * Considering VLAN so a tag needs to be counted.
 */
#define E1000_ETH_OVERHEAD (RTE_ETHER_HDR_LEN + RTE_ETHER_CRC_LEN + \
				VLAN_TAG_SIZE)

/*
 * Maximum number of Ring Descriptors.
 *
 * Since RDLEN/TDLEN should be multiple of 128 bytes, the number of ring
 * desscriptors should meet the following condition:
 * (num_ring_desc * sizeof(struct e1000_rx/tx_desc)) % 128 == 0
 */
#define	E1000_MIN_RING_DESC	32
#define	E1000_MAX_RING_DESC	4096

/*
 * TDBA/RDBA should be aligned on 16 byte boundary. But TDLEN/RDLEN should be
 * multiple of 128 bytes. So we align TDBA/RDBA on 128 byte boundary.
 * This will also optimize cache line size effect.
 * H/W supports up to cache line size 128.
 */
#define	E1000_ALIGN	128

#define	EM_RXD_ALIGN	(E1000_ALIGN / sizeof(struct e1000_rx_desc))
#define	EM_TXD_ALIGN	(E1000_ALIGN / sizeof(struct e1000_data_desc))

#define E1000_MISC_VEC_ID               RTE   _INTR_VEC_ZERO_OFFSET
#define E1000_RX_VEC_START              RTE_INTR_VEC_RXTX_OFFSET

#define E1000_MTU		   1500

#define EM_TX_MAX_SEG      UINT8_MAX
#define EM_TX_MAX_MTU_SEG  UINT8_MAX

#define MAC_TYPE_FILTER_SUP(type)    do {\
	if ((type) != e1000_82580 && (type) != e1000_i350 &&\
		(type) != e1000_82576 && (type) != e1000_i210 &&\
		(type) != e1000_i211)\
		return -ENOTSUP;\
} while (0)

#define MAC_TYPE_FILTER_SUP_EXT(type)    do {\
	if ((type) != e1000_82580 && (type) != e1000_i350 &&\
		(type) != e1000_i210 && (type) != e1000_i211)\
		return -ENOTSUP; \
} while (0)

struct rte_eth_link {
	uint32_t link_speed;        /**< ETH_SPEED_NUM_ */
	uint16_t link_duplex  : 1;  /**< ETH_LINK_[HALF/FULL]_DUPLEX */
	uint16_t link_autoneg : 1;  /**< ETH_LINK_SPEED_[AUTONEG/FIXED] */
	uint16_t link_status  : 1;  /**< ETH_LINK_[DOWN/UP] */
} __attribute__((aligned(8)));      /**< aligned for atomic64 read/write */

/* structure for interrupt relative data */
struct e1000_interrupt {
	uint32_t flags;
	uint32_t mask;
};

/*
 * Structure to store private data for each driver instance (for each port).
 */
struct e1000_adapter {
	struct e1000_hw         hw;
	struct e1000_interrupt  intr;
	bool stopped;
};

#define E1000_DEV_PRIVATE(adapter) \
	((struct e1000_adapter *)adapter)

#define E1000_DEV_PRIVATE_TO_HW(adapter) \
	(&((struct e1000_adapter *)adapter)->hw)

#define E1000_DEV_PRIVATE_TO_STATS(adapter) \
	(&((struct e1000_adapter *)adapter)->stats)

#define E1000_DEV_PRIVATE_TO_INTR(adapter) \
	(&((struct e1000_adapter *)adapter)->intr)

#define E1000_DEV_PRIVATE_TO_P_VFDATA(adapter) \
        (&((struct e1000_adapter *)adapter)->vfdata)

#define E1000_DEV_PRIVATE_TO_FILTER_INFO(adapter) \
	(&((struct e1000_adapter *)adapter)->filter)

// uint32_t em_get_max_pktlen(struct rte_eth_dev *dev);

/*
 * RX/TX EM function prototypes
 */
void eth_em_tx_queue_release(void *txq);
void eth_em_rx_queue_release(void *rxq);

void em_dev_clear_queues(struct uk_netdev *dev);
// void em_dev_free_queues(struct rte_eth_dev *dev);

struct uk_netdev_rx_queue * eth_em_rx_queue_setup(struct uk_netdev *dev, uint16_t rx_queue_id,
		uint16_t nb_rx_desc, struct uk_netdev_rxqueue_conf *rx_conf);

// uint32_t eth_em_rx_queue_count(struct rte_eth_dev *dev,
// 		uint16_t rx_queue_id);

int eth_em_rx_descriptor_done(void *rx_queue, uint16_t offset);

int eth_em_rx_descriptor_status(void *rx_queue, uint16_t offset);
int eth_em_tx_descriptor_status(void *tx_queue, uint16_t offset);

struct uk_netdev_tx_queue * eth_em_tx_queue_setup(struct uk_netdev *dev, uint16_t tx_queue_id,
		uint16_t nb_tx_desc, struct uk_netdev_txqueue_conf *tx_conf);

int eth_em_rx_init(struct e1000_hw *hw);

void eth_em_tx_init(struct e1000_hw *hw);

// uint16_t eth_em_xmit_pkts(void *tx_queue, struct rte_mbuf **tx_pkts,
// 		uint16_t nb_pkts);

// uint16_t eth_em_prep_pkts(void *txq, struct rte_mbuf **tx_pkts,
// 		uint16_t nb_pkts);

// uint16_t eth_em_recv_pkts(void *rx_queue, struct rte_mbuf **rx_pkts,
// 		uint16_t nb_pkts);

// uint16_t eth_em_recv_scattered_pkts(void *rx_queue, struct rte_mbuf **rx_pkts,
// 		uint16_t nb_pkts);

int eth_em_recv_pkts(struct uk_netdev *dev, struct uk_netdev_rx_queue *queue,
	struct uk_netbuf **pkt);

int eth_em_xmit_pkts(struct uk_netdev *dev, struct uk_netdev_tx_queue *queue,
	struct uk_netbuf *pkt);

int em_rxq_info_get(struct uk_netdev *dev, uint16_t queue_id,
	struct uk_netdev_queue_info *qinfo);

int em_txq_info_get(struct uk_netdev *dev, uint16_t queue_id,
	struct uk_netdev_queue_info *qinfo);

#endif /* _E1000_ETHDEV_H_ */
