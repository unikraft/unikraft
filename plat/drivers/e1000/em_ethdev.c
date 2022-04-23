/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2010-2016 Intel Corporation
 */

#include <stdio.h>
#include <errno.h>
#include <stdint.h>
#include <stdarg.h>
#include <uk/list.h>
#include <pci/pci_bus.h>
#include <uk/config.h>
#include <uk/arch/types.h>
#include <uk/plat/lcpu.h>
#include <uk/plat/irq.h>
#include <uk/netdev.h>
#include <uk/netdev_core.h>
#include <uk/netdev_driver.h>

#include <e1000/e1000_api.h>
#include <e1000/e1000_ethdev.h>

#define EM_EIAC			0x000DC

/* Utility constants */
#define ETH_LINK_HALF_DUPLEX 0 /**< Half-duplex connection (see link_duplex). */
#define ETH_LINK_FULL_DUPLEX 1 /**< Full-duplex connection (see link_duplex). */
#define ETH_LINK_DOWN        0 /**< Link is down (see link_status). */
#define ETH_LINK_UP          1 /**< Link is up (see link_status). */
#define ETH_LINK_FIXED       0 /**< No autonegotiation (see link_autoneg). */
#define ETH_LINK_AUTONEG     1 /**< Autonegotiated (see link_autoneg). */

#define ETH_SPEED_NUM_NONE         0 /**< Not defined */
#define ETH_LINK_SPEED_FIXED    (1 <<  0)  /**< Disable autoneg (fixed speed) */

#define RTE_ETHER_MAX_LEN   1518  /**< Maximum frame len, including CRC. */

#define PMD_ROUNDUP(x,y)	(((x) + (y) - 1)/(y) * (y))
#define DRIVER_NAME           "e1000"

static struct uk_alloc *a;

#define to_e1000dev(ndev) \
	__containerof(ndev, struct e1000_hw, netdev)

static const char *drv_name = DRIVER_NAME;

static int eth_em_configure(struct uk_netdev *dev, const struct uk_netdev_conf *conf);
static int eth_em_start(struct uk_netdev *dev);
static void eth_em_stop(struct uk_netdev *dev);
// static void eth_em_close(struct pci_device *dev);
static void eth_em_promiscuous_set(struct uk_netdev *dev, unsigned mode);
static unsigned eth_em_promiscuous_get(struct uk_netdev *dev);
static int eth_em_link_update(struct uk_netdev *dev,
				int wait_to_complete);
static void eth_em_infos_get(struct uk_netdev *dev,
				struct uk_netdev_info *dev_info);
// TODO: uncomment
// static int eth_em_interrupt_setup(struct pci_device *dev);
// TODO: uncomment
// static int eth_em_rxq_interrupt_setup(struct pci_device *dev);
// static int eth_em_interrupt_get_status(struct pci_device *dev);
// static int eth_em_interrupt_action(struct e1000_hw *dev,
// 				   struct rte_intr_handle *handle);
static int eth_em_interrupt_handler(void *param);

static int em_hw_init(struct e1000_hw *hw);
static int em_hardware_init(struct e1000_hw *hw);
static void em_hw_control_acquire(struct e1000_hw *hw);
static void em_hw_control_release(struct e1000_hw *hw);

static uint16_t eth_em_mtu_get(struct uk_netdev *dev);
static int eth_em_mtu_set(struct uk_netdev *dev, uint16_t mtu);

static int eth_em_rx_queue_intr_enable(struct uk_netdev *dev, struct uk_netdev_rx_queue *queue);
static int eth_em_rx_queue_intr_disable(struct uk_netdev *dev, struct uk_netdev_rx_queue *queue);
// static void em_lsc_intr_disable(struct e1000_hw *hw);
// TODO: uncomment
// static void em_rxq_intr_enable(struct e1000_hw *hw);
// TODO: uncomment
// static void em_rxq_intr_disable(struct e1000_hw *hw);

static int em_get_rx_buffer_size(struct e1000_hw *hw);
// static int eth_em_rar_set(struct pci_device *dev,
// 			struct rte_ether_addr *mac_addr,
// 			uint32_t index, uint32_t pool);
// static void eth_em_rar_clear(struct pci_device *dev, uint32_t index);
static const struct uk_hwaddr * eth_em_default_mac_addr_get(struct uk_netdev *n);
static int eth_em_default_mac_addr_set(struct uk_netdev *dev,
					const struct uk_hwaddr *addr);

// static int eth_em_set_mc_addr_list(struct pci_device *dev,
// 				   struct rte_ether_addr *mc_addr_set,
// 				   uint32_t nb_mc_addr);

#define EM_FC_PAUSE_TIME 0x0680
#define EM_LINK_UPDATE_CHECK_TIMEOUT  90  /* 9s */
#define EM_LINK_UPDATE_CHECK_INTERVAL 100 /* ms */

static enum e1000_fc_mode em_fc_setting = e1000_fc_full;

static const struct uk_netdev_ops eth_em_dev_ops = {
    /** RX queue interrupts. */
	.rxq_intr_enable                = eth_em_rx_queue_intr_enable,  /* optional */
	.rxq_intr_disable               = eth_em_rx_queue_intr_disable, /* optional */

	/** Set/Get hardware address. */
	.hwaddr_get                     = eth_em_default_mac_addr_get,       /* recommended */
	.hwaddr_set                     = eth_em_default_mac_addr_set,       /* optional */

	/** Set/Get MTU. */
	.mtu_get                        = eth_em_mtu_get,
	.mtu_set                        = eth_em_mtu_set,          /* optional */

	/** Promiscuous mode. */
	.promiscuous_set                = eth_em_promiscuous_set,  /* optional */
	.promiscuous_get                = eth_em_promiscuous_get,

	/** Device/driver capabilities and info. */
	.info_get                       = eth_em_infos_get,
	.txq_info_get                   = em_txq_info_get,
	.rxq_info_get                   = em_rxq_info_get,
	.einfo_get                      = NULL,        /* optional */

	/** Device life cycle. */
	.configure                      = eth_em_configure,
	.txq_configure                  = eth_em_tx_queue_setup,
	.rxq_configure                  = eth_em_rx_queue_setup,
	.start                          = eth_em_start,
};

static int
eth_em_dev_init(struct pci_device * pci_dev)
{
	int rc = 0;
	struct e1000_hw *hw = NULL;

    UK_ASSERT(pci_dev != NULL);

	hw = uk_malloc(a, sizeof(*hw));
	hw->a = a;
	uk_pr_info("hw->a %p\n", hw->a);
	if (!hw) {
		uk_pr_err("Failed to allocate e1000 device\n");
		return -ENOMEM;
	}

	hw->pdev = pci_dev;
	// e1000_dev->adapter = uk_malloc(a, sizeof(struct e1000_adapter));
	// struct e1000_adapter *adapter =
		// E1000_DEV_PRIVATE(eth_dev->data->dev_private);
	// struct e1000_hw *hw =
	// 	E1000_DEV_PRIVATE_TO_HW(eth_dev->data->dev_private);

	hw->netdev.ops = &eth_em_dev_ops;
	hw->netdev.rx_one = &eth_em_recv_pkts;
	hw->netdev.tx_one = &eth_em_xmit_pkts;
	// eth_dev->tx_pkt_prepare = (eth_tx_prep_t)&eth_em_prep_pkts;

	// rte_eth_copy_pci_info(eth_dev, pci_dev);

	// hw->hw_addr = (void *)pci_dev->mem_resource[0].addr;
	// hw->hw_addr = (void *)pci_dev->base;
	hw->hw_addr = (void *)(hw->pdev->bar0 & 0xFFFFFFF0);
	uk_pr_info("hw->hw_addr %p, hw->pdev->bar0 %p\n", hw->hw_addr, hw->pdev->bar0);
	hw->device_id = pci_dev->id.device_id;
	// adapter->stopped = 0;

	rc = uk_netdev_drv_register(&hw->netdev, a, drv_name);
	if (rc < 0) {
		uk_pr_err("Failed to register virtio-net device with libuknet\n");
		return 0;
	}

	if (e1000_setup_init_funcs(hw, TRUE) != E1000_SUCCESS ||
			em_hw_init(hw) != 0) {
		uk_pr_crit("addr %X %X %X %X %X %X ", hw->mac.addr[0],
			hw->mac.addr[1], hw->mac.addr[2], hw->mac.addr[3],
			hw->mac.addr[4], hw->mac.addr[5]);
		uk_pr_crit("perm_addr %X %X %X %X %X %X ", hw->mac.perm_addr[0],
			hw->mac.perm_addr[1], hw->mac.perm_addr[2], hw->mac.perm_addr[3],
			hw->mac.perm_addr[4], hw->mac.perm_addr[5]);
		// uk_pr_err("port_id %d vendorID=0x%x deviceID=0x%x: "
		// 	"failed to init HW", hw->data->port_id, pci_dev->id.vendor_id,
		// 	pci_dev->id.device_id);
		uk_pr_err("vendorID=0x%x deviceID=0x%x: "
			"failed to init HW", pci_dev->id.vendor_id,
			pci_dev->id.device_id);
		return -ENODEV;
	}

	// /* Allocate memory for storing MAC addresses */
	// hw->data->mac_addrs = uk_malloc(a, 6 * hw->mac.rar_entry_count);
	// if (hw->data->mac_addrs == NULL) {
	// 	uk_pr_err("Failed to allocate %d bytes needed to "
	// 		"store MAC addresses", 6 * hw->mac.rar_entry_count);
	// 	return -ENOMEM;
	// }

	// /* Copy the permanent MAC address */
	// memcpy(hw->mac.addr, hw->data->mac_addrs);

	// uk_pr_crit("port_id %d vendorID=0x%x deviceID=0x%x", hw->data->port_id,
	// 	pci_dev->id.vendor_id, pci_dev->id.device_id);
	uk_pr_crit("vendorID=0x%x deviceID=0x%x",
		pci_dev->id.vendor_id, pci_dev->id.device_id);

	rc = ukplat_irq_register(pci_dev->irq, eth_em_interrupt_handler, hw);
	if (rc != 0) {
		uk_pr_err("Failed to register the interrupt\n");
		return rc;
	}

	return 0;
}

// TODO: uncomment
// static int
// eth_em_dev_uninit(struct uk_netdev *eth_dev)
// {
// 	struct rte_pci_device *pci_dev = RTE_ETH_DEV_TO_PCI(eth_dev);
// 	struct e1000_adapter *adapter =
// 		E1000_DEV_PRIVATE(eth_dev->data->dev_private);
// 	struct rte_intr_handle *intr_handle = &pci_dev->intr_handle;


// 	if (adapter->stopped == 0)
// 		eth_em_close(eth_dev);

// 	eth_dev->dev_ops = NULL;
// 	eth_dev->rx_pkt_burst = NULL;
// 	eth_dev->tx_pkt_burst = NULL;

// 	/* disable uio intr before callback unregister */
// 	rte_intr_disable(intr_handle);
// 	rte_intr_callback_unregister(intr_handle,
// 				     eth_em_interrupt_handler, eth_dev);

// 	return 0;
// }

static int
em_hw_init(struct e1000_hw *hw)
{
	int diag;

	diag = hw->mac.ops.init_params(hw);
	if (diag != 0) {
		uk_pr_err("MAC Initialization Error\n");
		return diag;
	}
	diag = hw->nvm.ops.init_params(hw);
	if (diag != 0) {
		uk_pr_err("NVM Initialization Error\n");
		return diag;
	}
	diag = hw->phy.ops.init_params(hw);
	if (diag != 0) {
		uk_pr_err("PHY Initialization Error\n");
		return diag;
	}
	(void) e1000_get_bus_info(hw);

	hw->mac.autoneg = 1;
	hw->phy.autoneg_wait_to_complete = 0;
	hw->phy.autoneg_advertised = E1000_ALL_SPEED_DUPLEX;

	/* Copper options */
	if (hw->phy.media_type == e1000_media_type_copper) {
		hw->phy.mdix = 0; /* AUTO_ALL_MODES */
		hw->phy.disable_polarity_correction = 0;
		hw->phy.ms_type = e1000_ms_hw_default;
	}

	/*
	 * Start from a known state, this is important in reading the nvm
	 * and mac from that.
	 */
	uk_pr_crit("before e1000_reset_hw\n");
	e1000_reset_hw(hw);
	uk_pr_crit("after e1000_reset_hw\n");

	/* Make sure we have a good EEPROM before we read from it */
	if (e1000_validate_nvm_checksum(hw) < 0) {
		/*
		 * Some PCI-E parts fail the first check due to
		 * the link being in sleep state, call it again,
		 * if it fails a second time its a real issue.
		 */
		diag = e1000_validate_nvm_checksum(hw);
		if (diag < 0) {
			// PMD_INIT_LOG(ERR, "EEPROM checksum invalid");
			goto error;
		}
	}

	/* Read the permanent MAC address out of the EEPROM */
	uk_pr_crit("before e1000_read_mac_addr\n");
	diag = e1000_read_mac_addr(hw);

	uk_pr_crit("after e1000_read_mac_addr\n");
	if (diag != 0) {
		uk_pr_err("EEPROM error while reading MAC address\n");
		goto error;
	}

	/* Now initialize the hardware */
	diag = em_hardware_init(hw);
	uk_pr_info("after em_hardware_init\n");
	if (diag != 0) {
		uk_pr_err("Hardware initialization failed\n");
		goto error;
	}

	hw->mac.get_link_status = 1;
	uk_pr_info("after get_link_status\n");

	/* Indicate SOL/IDER usage */
	diag = e1000_check_reset_block(hw);
	if (diag < 0) {
		// PMD_INIT_LOG(ERR, "PHY reset is blocked due to "
			// "SOL/IDER session");
	}
	uk_pr_info("after e1000_check_reset_block\n");
	return 0;

error:
	em_hw_control_release(hw);
	return diag;
}

static int
eth_em_configure(struct uk_netdev *dev, const struct uk_netdev_conf *conf)
{
	struct e1000_hw *hw = to_e1000dev(dev);

	uk_pr_info("eth_em_configure\n");
	// struct e1000_interrupt *intr =
	// 	E1000_DEV_PRIVATE_TO_INTR(dev->data->dev_private);

	// intr->flags |= E1000_FLAG_NEED_LINK_UPDATE;

	return 0;
}

static void
em_set_pba(struct e1000_hw *hw)
{
	uint32_t pba;

	/*
	 * Packet Buffer Allocation (PBA)
	 * Writing PBA sets the receive portion of the buffer
	 * the remainder is used for the transmit buffer.
	 * Devices before the 82547 had a Packet Buffer of 64K.
	 * After the 82547 the buffer was reduced to 40K.
	 */
    pba = E1000_PBA_40K; /* 40K for Rx, 24K for Tx */
	
	E1000_WRITE_REG(hw, E1000_PBA, pba);
}

static void
eth_em_rxtx_control(struct uk_netdev *dev,
		    bool enable)
{
	struct e1000_hw *hw =
		to_e1000dev(dev);
	uint32_t tctl, rctl;

	tctl = E1000_READ_REG(hw, E1000_TCTL);
	rctl = E1000_READ_REG(hw, E1000_RCTL);
	if (enable) {
		/* enable Tx/Rx */
		tctl |= E1000_TCTL_EN;
		rctl |= E1000_RCTL_EN;
	} else {
		/* disable Tx/Rx */
		tctl &= ~E1000_TCTL_EN;
		rctl &= ~E1000_RCTL_EN;
	}
	E1000_WRITE_REG(hw, E1000_TCTL, tctl);
	E1000_WRITE_REG(hw, E1000_RCTL, rctl);
	E1000_WRITE_FLUSH(hw);
}

static int
eth_em_start(struct uk_netdev *dev)
{
	uk_pr_info("eth_em_start\n");

	struct e1000_hw *hw = to_e1000dev(dev);
	// struct rte_pci_device *pci_dev = RTE_ETH_DEV_TO_PCI(dev);
	// struct rte_intr_handle *intr_handle = &pci_dev->intr_handle;
	int ret, mask;
	uint32_t intr_vector = 0;
	uint32_t *speeds;
	int num_speeds;
	bool autoneg;

	uk_pr_info("dev->_rx_queue[0] = %p\n", dev->_rx_queue[0]);
	eth_em_stop(dev);
	uk_pr_info("dev->_rx_queue[0] = %p\n", dev->_rx_queue[0]);
	e1000_power_up_phy(hw);
	uk_pr_info("dev->_rx_queue[0] = %p\n", dev->_rx_queue[0]);
	/* Set default PBA value */
	em_set_pba(hw);
	uk_pr_info("dev->_rx_queue[0] = %p\n", dev->_rx_queue[0]);
	/* Put the address into the Receive Address Array */
	e1000_rar_set(hw, hw->mac.addr, 0);
	uk_pr_info("dev->_rx_queue[0] = %p\n", dev->_rx_queue[0]);
	/* Initialize the hardware */
	if (em_hardware_init(hw)) {
		uk_pr_err("Unable to initialize the hardware\n");
		return -EIO;
	}
	uk_pr_info("dev->_rx_queue[0] = %p\n", dev->_rx_queue[0]);
	uk_pr_info("after em_hardware_init\n");

	E1000_WRITE_REG(hw, E1000_VET, 0x8100);

// 	if (dev->data->dev_conf.intr_conf.rxq != 0) {
// 		intr_vector = dev->data->nb_rx_queues;
// 		if (rte_intr_efd_enable(intr_handle, intr_vector))
// 			return -1;
// 	}

// 	if (rte_intr_dp_is_en(intr_handle)) {
// 		intr_handle->intr_vec =
// 			rte_zmalloc("intr_vec",
// 					dev->data->nb_rx_queues * sizeof(int), 0);
// 		if (intr_handle->intr_vec == NULL) {
// 			PMD_INIT_LOG(ERR, "Failed to allocate %d rx_queues"
// 						" intr_vec", dev->data->nb_rx_queues);
// 			return -ENOMEM;
// 		}

// 		/* enable rx interrupt */
// 		em_rxq_intr_enable(hw);
// 	}

	eth_em_tx_init(hw);

	uk_pr_info("before eth_em_rx_init\n");
	uk_pr_info("dev->_rx_queue[0] = %p\n", dev->_rx_queue[0]);
	ret = eth_em_rx_init(hw);
	uk_pr_info("after eth_em_rx_init\n");
	if (ret) {
		uk_pr_err("Unable to initialize RX hardware\n");
		em_dev_clear_queues(dev);
		return ret;
	}

	e1000_clear_hw_cntrs_base_generic(hw);

// 	mask = ETH_VLAN_STRIP_MASK | ETH_VLAN_FILTER_MASK | ETH_VLAN_EXTEND_MASK;
// 	ret = eth_em_vlan_offload_set(dev, mask);
// 	if (ret) {
// 		PMD_INIT_LOG(ERR, "Unable to update vlan offload");
// 		em_dev_clear_queues(dev);
// 		return ret;
// 	}

	/* Set Interrupt Throttling Rate to maximum allowed value. */
	E1000_WRITE_REG(hw, E1000_ITR, UINT16_MAX);

	/* Setup link speed and duplex */
	hw->phy.autoneg_advertised = E1000_ALL_SPEED_DUPLEX;
	hw->mac.autoneg = 1;

	uk_pr_info("before e1000_setup_link\n");
	e1000_setup_link(hw);
	uk_pr_info("after e1000_setup_link\n");

// 	if (rte_intr_allow_others(intr_handle)) {
// 		/* check if lsc interrupt is enabled */
// 		if (dev->data->dev_conf.intr_conf.lsc != 0) {
// 			ret = eth_em_interrupt_setup(dev);
// 			if (ret) {
// 				PMD_INIT_LOG(ERR, "Unable to setup interrupts");
// 				em_dev_clear_queues(dev);
// 				return ret;
// 			}
// 		}
// 	} else {
// 		rte_intr_callback_unregister(intr_handle,
// 						eth_em_interrupt_handler,
// 						(void *)dev);
// 		if (dev->data->dev_conf.intr_conf.lsc != 0)
// 			PMD_INIT_LOG(INFO, "lsc won't enable because of"
// 				     " no intr multiplexn");
// 	}
// 	/* check if rxq interrupt is enabled */
// 	if (dev->data->dev_conf.intr_conf.rxq != 0)
// 		eth_em_rxq_interrupt_setup(dev);

// 	rte_intr_enable(intr_handle);

	// hw->adapter->stopped = 0;

	uk_pr_info("before eth_em_rxtx_control\n");
	eth_em_rxtx_control(dev, true);
	uk_pr_info("before eth_em_link_update\n");
	eth_em_link_update(dev, 0);

// 	PMD_INIT_LOG(DEBUG, "<<");

// error_invalid_config:
// 	PMD_INIT_LOG(ERR, "Invalid advertised speeds (%u) for port %u",
// 		     dev->data->dev_conf.link_speeds, dev->data->port_id);
// 	em_dev_clear_queues(dev);
// 	return -EINVAL;

	return 0;
}

/*********************************************************************
 *
 *  This routine disables all traffic on the adapter by issuing a
 *  global reset on the MAC.
 *
 **********************************************************************/
static void
eth_em_stop(struct uk_netdev *dev)
{
	// struct rte_eth_link link;
	// struct e1000_hw *hw = to_e1000dev(dev);
	// struct pci_device *pci_dev = hw->pdev;
	// struct rte_intr_handle *intr_handle = &pci_dev->intr_handle;

	// eth_em_rxtx_control(dev, false);
	// em_rxq_intr_disable(hw);
	// em_lsc_intr_disable(hw);

	// e1000_reset_hw(hw);

	/* Power down the phy. Needed to make the link go down */
	// e1000_power_down_phy(hw);

	// em_dev_clear_queues(dev);

	/* clear the recorded link status */
	// memset(&link, 0, sizeof(link));
	// rte_eth_linkstatus_set(dev, &link);

	// if (!rte_intr_allow_others(intr_handle))
	// 	/* resume to the default handler */
	// 	rte_intr_callback_register(intr_handle,
	// 				   eth_em_interrupt_handler,
	// 				   (void *)dev);

	/* Clean datapath event and queue/vec mapping */
	// rte_intr_efd_disable(intr_handle);
	// if (intr_handle->intr_vec != NULL) {
	// 	rte_free(intr_handle->intr_vec);
	// 	intr_handle->intr_vec = NULL;
	// }
}

// static void
// eth_em_close(struct uk_netdev *dev)
// {
// 	struct e1000_hw *hw = E1000_DEV_PRIVATE_TO_HW(dev->data->dev_private);
// 	struct e1000_adapter *adapter =
// 		E1000_DEV_PRIVATE(dev->data->dev_private);

// 	eth_em_stop(dev);
// 	adapter->stopped = 1;
// 	em_dev_free_queues(dev);
// 	e1000_phy_hw_reset(hw);
// 	em_hw_control_release(hw);
// }

static int
em_get_rx_buffer_size(struct e1000_hw *hw)
{
	uint32_t rx_buf_size;

	uk_pr_info("em_get_rx_buffer_size\n");

	rx_buf_size = ((E1000_READ_REG(hw, E1000_PBA) & UINT16_MAX) << 10);

	uk_pr_info("em_get_rx_buffer_size %d\n", rx_buf_size);

	return rx_buf_size;
}

/*********************************************************************
 *
 *  Initialize the hardware
 *
 **********************************************************************/
static int
em_hardware_init(struct e1000_hw *hw)
{
	uint32_t rx_buf_size;
	int diag;

	uk_pr_info("em_hardware_init\n");

	/* Issue a global reset */
	e1000_reset_hw(hw);

	/* Let the firmware know the OS is in control */
	em_hw_control_acquire(hw);

	/*
	 * These parameters control the automatic generation (Tx) and
	 * response (Rx) to Ethernet PAUSE frames.
	 * - High water mark should allow for at least two standard size (1518)
	 *   frames to be received after sending an XOFF.
	 * - Low water mark works best when it is very near the high water mark.
	 *   This allows the receiver to restart by sending XON when it has
	 *   drained a bit. Here we use an arbitrary value of 1500 which will
	 *   restart after one full frame is pulled from the buffer. There
	 *   could be several smaller frames in the buffer and if so they will
	 *   not trigger the XON until their total number reduces the buffer
	 *   by 1500.
	 * - The pause time is fairly large at 1000 x 512ns = 512 usec.
	 */
	rx_buf_size = em_get_rx_buffer_size(hw);

	hw->fc.high_water = rx_buf_size -
		PMD_ROUNDUP(RTE_ETHER_MAX_LEN * 2, 1024);
	hw->fc.low_water = hw->fc.high_water - 1500;

	hw->fc.pause_time = EM_FC_PAUSE_TIME;

	hw->fc.send_xon = 1;

	/* Set Flow control, use the tunable location if sane */
	if (em_fc_setting <= e1000_fc_full)
		hw->fc.requested_mode = em_fc_setting;
	else
		hw->fc.requested_mode = e1000_fc_none;

	diag = e1000_init_hw(hw);
	if (diag < 0)
		return diag;
	e1000_check_for_link(hw);
	uk_pr_info("e1000_check_for_link\n");
	return 0;
}

static int
eth_em_rx_queue_intr_enable(struct uk_netdev *dev, struct uk_netdev_rx_queue *queue)
{
	uk_pr_info("eth_em_rx_queue_intr_enable\n");

	// TODO: uncomment
	// struct e1000_hw *hw = E1000_DEV_PRIVATE_TO_HW(dev->_data->dev_private);
	// struct rte_pci_device *pci_dev = RTE_ETH_DEV_TO_PCI(dev);
	// struct rte_intr_handle *intr_handle = &pci_dev->intr_handle;

	// em_rxq_intr_enable(hw);
	// rte_intr_enable(intr_handle);

	return 0;
}

static int
eth_em_rx_queue_intr_disable(struct uk_netdev *dev, struct uk_netdev_rx_queue *queue)
{
	uk_pr_info("eth_em_rx_queue_intr_disable\n");
	// TODO: uncomment
	// struct e1000_hw *hw = E1000_DEV_PRIVATE_TO_HW(dev->data->dev_private);

	// em_rxq_intr_disable(hw);

	return 0;
}

static void
eth_em_infos_get(struct uk_netdev *dev, struct uk_netdev_info *dev_info)
{
	struct e1000_hw *hw = to_e1000dev(dev);

	uk_pr_info("eth_em_infos_get\n");

	dev_info->max_rx_queues = 1;
	dev_info->max_tx_queues = 1;

	dev_info->max_mtu = EM_TX_MAX_MTU_SEG; //E1000_MTU;
	dev_info->ioalign = EM_RXD_ALIGN;
}

/* return 0 means link status changed, -1 means not changed */
static int
eth_em_link_update(struct uk_netdev *dev, int wait_to_complete)
{
	struct e1000_hw *hw =
		to_e1000dev(dev);
	struct rte_eth_link link;
	int link_check, count;

	link_check = 0;
	hw->mac.get_link_status = 1;

	/* possible wait-to-complete in up to 9 seconds */
	for (count = 0; count < EM_LINK_UPDATE_CHECK_TIMEOUT; count ++) {
		/* Read the real link status */
		/* Do the work to read phy */
		e1000_check_for_link(hw);
		uk_pr_info("e1000_check_for_link\n");
		link_check = !hw->mac.get_link_status;

		if (link_check || wait_to_complete == 0)
			break;
		DELAY(EM_LINK_UPDATE_CHECK_INTERVAL);
	}
	memset(&link, 0, sizeof(link));

	/* Now we check if a transition has happened */
	if (link_check && (link.link_status == ETH_LINK_DOWN)) {
		uint16_t duplex, speed;
		hw->mac.ops.get_link_up_info(hw, &speed, &duplex);
		link.link_duplex = (duplex == FULL_DUPLEX) ?
				ETH_LINK_FULL_DUPLEX :
				ETH_LINK_HALF_DUPLEX;
		link.link_speed = speed;
		link.link_status = ETH_LINK_UP;
		link.link_autoneg = 0;
		// link.link_autoneg = !(dev->data->dev_conf.link_speeds &
		// 		ETH_LINK_SPEED_FIXED);
	} else if (!link_check && (link.link_status == ETH_LINK_UP)) {
		link.link_speed = ETH_SPEED_NUM_NONE;
		link.link_duplex = ETH_LINK_HALF_DUPLEX;
		link.link_status = ETH_LINK_DOWN;
		link.link_autoneg = ETH_LINK_FIXED;
	}

	return 0;
	// return rte_eth_linkstatus_set(dev, &link);
}

/*
 * em_hw_control_acquire sets {CTRL_EXT|FWSM}:DRV_LOAD bit.
 * For ASF and Pass Through versions of f/w this means
 * that the driver is loaded. For AMT version type f/w
 * this means that the network i/f is open.
 */
static void
em_hw_control_acquire(struct e1000_hw *hw)
{
	uint32_t ctrl_ext, swsm;

	uk_pr_info("em_hw_control_acquire\n");

    ctrl_ext = E1000_READ_REG(hw, E1000_CTRL_EXT);
    E1000_WRITE_REG(hw, E1000_CTRL_EXT,
        ctrl_ext | E1000_CTRL_EXT_DRV_LOAD);
}

/*
 * em_hw_control_release resets {CTRL_EXTT|FWSM}:DRV_LOAD bit.
 * For ASF and Pass Through versions of f/w this means that the
 * driver is no longer loaded. For AMT versions of the
 * f/w this means that the network i/f is closed.
 */
static void
em_hw_control_release(struct e1000_hw *hw)
{
	uint32_t ctrl_ext;

	/* Let firmware taken over control of h/w */
    ctrl_ext = E1000_READ_REG(hw, E1000_CTRL_EXT);
    E1000_WRITE_REG(hw, E1000_CTRL_EXT,
        ctrl_ext & ~E1000_CTRL_EXT_DRV_LOAD);
}

static void
eth_em_promiscuous_set(struct uk_netdev *dev, unsigned mode)
{
	uk_pr_info("eth_em_promiscuous_set\n");
	struct e1000_hw *hw = to_e1000dev(dev);

	uint32_t rctl;

    rctl = E1000_READ_REG(hw, E1000_RCTL);
    if (mode) {
        rctl |= (E1000_RCTL_UPE | E1000_RCTL_MPE);
    } else {
        rctl &= ~(E1000_RCTL_UPE | E1000_RCTL_SBP);
        rctl &= (~E1000_RCTL_MPE);
    }
    E1000_WRITE_REG(hw, E1000_RCTL, rctl);
}

static unsigned
eth_em_promiscuous_get(struct uk_netdev *dev) {
	uk_pr_info("eth_em_promiscuous_get\n");

	return 0;
}



/*
 * It enables the interrupt mask and then enable the interrupt.
 *
 * @param dev
 *  Pointer to struct uk_netdev.
 *
 * @return
 *  - On success, zero.
 *  - On failure, a negative value.
 */
// TODO: uncomment
// static int
// eth_em_interrupt_setup(struct uk_netdev *dev)
// {
// 	uint32_t regval;
// 	struct e1000_hw *hw =
// 		E1000_DEV_PRIVATE_TO_HW(dev->data->dev_private);

// 	/* clear interrupt */
// 	E1000_READ_REG(hw, E1000_ICR);
// 	regval = E1000_READ_REG(hw, E1000_IMS);
// 	E1000_WRITE_REG(hw, E1000_IMS,
// 			regval | E1000_ICR_LSC | E1000_ICR_OTHER);
// 	return 0;
// }

/*
 * It clears the interrupt causes and enables the interrupt.
 * It will be called once only during nic initialized.
 *
 * @param dev
 *  Pointer to struct uk_netdev.
 *
 * @return
 *  - On success, zero.
 *  - On failure, a negative value.
 */
// TODO: uncomment
// static int
// eth_em_rxq_interrupt_setup(struct uk_netdev *dev)
// {
// 	struct e1000_hw *hw =
// 	E1000_DEV_PRIVATE_TO_HW(dev->data->dev_private);

// 	E1000_READ_REG(hw, E1000_ICR);
// 	em_rxq_intr_enable(hw);
// 	return 0;
// }

/*
 * It enable receive packet interrupt.
 * @param hw
 * Pointer to struct e1000_hw
 *
 * @return
 */
// TODO: uncomment
// static void
// em_rxq_intr_enable(struct e1000_hw *hw)
// {
// 	E1000_WRITE_REG(hw, E1000_IMS, E1000_IMS_RXT0);
// 	E1000_WRITE_FLUSH(hw);
// }

/*
 * It disabled lsc interrupt.
 * @param hw
 * Pointer to struct e1000_hw
 *
 * @return
 */
// static void
// em_lsc_intr_disable(struct e1000_hw *hw)
// {
// 	E1000_WRITE_REG(hw, E1000_IMC, E1000_IMS_LSC | E1000_IMS_OTHER);
// 	E1000_WRITE_FLUSH(hw);
// }

/*
 * It disabled receive packet interrupt.
 * @param hw
 * Pointer to struct e1000_hw
 *
 * @return
 */
// TODO: uncomment
// static void
// em_rxq_intr_disable(struct e1000_hw *hw)
// {
// 	E1000_READ_REG(hw, E1000_ICR);
// 	E1000_WRITE_REG(hw, E1000_IMC, E1000_IMS_RXT0);
// 	E1000_WRITE_FLUSH(hw);
// }

/*
 * It reads ICR and gets interrupt causes, check it and set a bit flag
 * to update link status.
 *
 * @param dev
 *  Pointer to struct uk_netdev.
 *
 * @return
 *  - On success, zero.
 *  - On failure, a negative value.
 */
// static int
// eth_em_interrupt_get_status(struct e1000_hw *dev)
// {
// 	uint32_t icr;
// 	struct e1000_hw *hw =
// 		E1000_DEV_PRIVATE_TO_HW(dev->data->dev_private);
// 	struct e1000_interrupt *intr =
// 		E1000_DEV_PRIVATE_TO_INTR(dev->data->dev_private);

// 	/* read-on-clear nic registers here */
// 	icr = E1000_READ_REG(hw, E1000_ICR);
// 	if (icr & E1000_ICR_LSC) {
// 		intr->flags |= E1000_FLAG_NEED_LINK_UPDATE;
// 	}

// 	return 0;
// }

/*
 * It executes link_update after knowing an interrupt is prsent.
 *
 * @param dev
 *  Pointer to struct uk_netdev.
 *
 * @return
 *  - On success, zero.
 *  - On failure, a negative value.
 */
// static int
// eth_em_interrupt_action(struct e1000_hw *dev,
// 			struct rte_intr_handle *intr_handle)
// {
// 	struct rte_pci_device *pci_dev = RTE_ETH_DEV_TO_PCI(dev);
// 	struct e1000_hw *hw =
// 		E1000_DEV_PRIVATE_TO_HW(dev->data->dev_private);
// 	struct e1000_interrupt *intr =
// 		E1000_DEV_PRIVATE_TO_INTR(dev->data->dev_private);
// 	int ret;

// 	if (!(intr->flags & E1000_FLAG_NEED_LINK_UPDATE))
// 		return -1;

// 	intr->flags &= ~E1000_FLAG_NEED_LINK_UPDATE;
// 	rte_intr_enable(intr_handle);

// 	/* set get_link_status to check register later */
// 	hw->mac.get_link_status = 1;
// 	ret = eth_em_link_update(dev, 0);

// 	/* check if link has changed */
// 	if (ret < 0)
// 		return 0;

// 	return 0;
// }

/**
 * Interrupt handler which shall be registered at first.
 *
 * @param handle
 *  Pointer to interrupt handle.
 * @param param
 *  The address of parameter (struct uk_netdev *) regsitered before.
 *
 * @return
 *  void
 */
static int
eth_em_interrupt_handler(void *param)
{
	// struct e1000_hw *dev = (struct e1000_hw *)param;

	// eth_em_interrupt_get_status(dev);
	// eth_em_interrupt_action(dev, dev->intr_handle);
}

// static int
// eth_em_rar_set(struct uk_netdev *dev, struct rte_ether_addr *mac_addr,
// 		uint32_t index, __rte_unused uint32_t pool)
// {
// 	struct e1000_hw *hw = E1000_DEV_PRIVATE_TO_HW(dev->data->dev_private);

// 	return e1000_rar_set(hw, mac_addr->addr_bytes, index);
// }

// static void
// eth_em_rar_clear(struct uk_netdev *dev, uint32_t index)
// {
// 	uint8_t addr[RTE_ETHER_ADDR_LEN];
// 	struct e1000_hw *hw = E1000_DEV_PRIVATE_TO_HW(dev->data->dev_private);

// 	memset(addr, 0, sizeof(addr));

// 	e1000_rar_set(hw, addr, index);
// }


static const struct uk_hwaddr *
eth_em_default_mac_addr_get(struct uk_netdev *n)
{
	struct e1000_hw *d;

	UK_ASSERT(n);
	d = to_e1000dev(n);

	return &d->mac.addr;
}

static int
eth_em_default_mac_addr_set(struct uk_netdev *dev,
			    const struct uk_hwaddr *hwaddr)
{
	uk_pr_info("eth_em_default_mac_addr_set\n");
	// TODO: uncomment
	// eth_em_rar_clear(dev, 0);

	// return eth_em_rar_set(dev, (void *)addr, 0, 0);
	return 0;
}

static uint16_t
eth_em_mtu_get(struct uk_netdev *dev)
{
	uk_pr_info("eth_em_mtu_get\n");

	 // TODO
	return EM_TX_MAX_MTU_SEG;
}


static int
eth_em_mtu_set(struct uk_netdev *dev, uint16_t mtu)
{
	uk_pr_info("eth_em_mtu_set\n");
	// TODO: uncomment
	// struct uk_netdev_info dev_info;
	// struct e1000_hw *hw;
	// uint32_t frame_size;
	// uint32_t rctl;

	// eth_em_infos_get(dev, &dev_info);
	// frame_size = mtu + RTE_ETHER_HDR_LEN + RTE_ETHER_CRC_LEN +
	// 	VLAN_TAG_SIZE;

	// /* check that mtu is within the allowed range */
	// if (mtu < RTE_ETHER_MIN_MTU || frame_size > dev_info.max_rx_pktlen)
	// 	return -EINVAL;

	// /* refuse mtu that requires the support of scattered packets when this
	//  * feature has not been enabled before. */
	// if (!dev->data->scattered_rx &&
	//     frame_size > dev->data->min_rx_buf_size - RTE_PKTMBUF_HEADROOM)
	// 	return -EINVAL;

	// hw = E1000_DEV_PRIVATE_TO_HW(dev->data->dev_private);
	// rctl = E1000_READ_REG(hw, E1000_RCTL);

	// /* switch to jumbo mode if needed */
	// if (frame_size > RTE_ETHER_MAX_LEN) {
	// 	dev->data->dev_conf.rxmode.offloads |=
	// 		DEV_RX_OFFLOAD_JUMBO_FRAME;
	// 	rctl |= E1000_RCTL_LPE;
	// } else {
	// 	dev->data->dev_conf.rxmode.offloads &=
	// 		~DEV_RX_OFFLOAD_JUMBO_FRAME;
	// 	rctl &= ~E1000_RCTL_LPE;
	// }
	// E1000_WRITE_REG(hw, E1000_RCTL, rctl);

	// /* update max frame size */
	// dev->data->dev_conf.rxmode.max_rx_pkt_len = frame_size;
	return 0;
}

// static int
// eth_em_set_mc_addr_list(struct uk_netdev *dev,
// 			struct rte_ether_addr *mc_addr_set,
// 			uint32_t nb_mc_addr)
// {
// 	struct e1000_hw *hw;

// 	hw = E1000_DEV_PRIVATE_TO_HW(dev->data->dev_private);
// 	e1000_update_mc_addr_list(hw, (u8 *)mc_addr_set, nb_mc_addr);
// 	return 0;
// }

/*
 * The set of PCI devices this driver supports
 */
static const struct pci_device_id e1000_pci_ids[] = {
	{ PCI_DEVICE_ID(E1000_INTEL_VENDOR_ID, E1000_DEV_ID_82545EM_COPPER) },
	{ PCI_DEVICE_ID(E1000_INTEL_VENDOR_ID, E1000_DEV_ID_82545EM_FIBER) },
	{ PCI_ANY_DEVICE_ID },
};


static int e1000_drv_init(struct uk_alloc *allocator)
{
	/* driver initialization */
	if (!allocator)
		return -EINVAL;

	a = allocator;
	return 0;
}

static struct pci_driver e1000_pci_drv = {
	.device_ids = e1000_pci_ids,
	.init = e1000_drv_init,
	.add_dev = eth_em_dev_init
};

PCI_REGISTER_DRIVER(&e1000_pci_drv);
