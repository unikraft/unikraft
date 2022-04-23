/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2010-2015 Intel Corporation
 */

// #include <sys/queue.h>
#include <stdio.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <fcntl.h>
#include <inttypes.h>
#include <pci/pci_bus.h>

#include <uk/list.h>

#include <vmxnet3/base/vmxnet3_defs.h>
#include <vmxnet3/vmxnet3_ring.h>
#include <vmxnet3/vmxnet3_ethdev.h>


#define	VMXNET3_TX_MAX_SEG	UINT8_MAX

#define to_vmxnet3dev(ndev) \
	__containerof(ndev, struct vmxnet3_hw, netdev)

static struct uk_alloc *a;

static int vmxnet3_add_dev(struct pci_device *eth_dev);
static int vmxnet3_drv_init(struct uk_alloc *allocator);
static int eth_vmxnet3_dev_uninit(struct uk_netdev *eth_dev);
static int vmxnet3_dev_configure(struct uk_netdev *dev, const struct uk_netdev_conf *conf);
static int vmxnet3_dev_start(struct uk_netdev *dev);
static int vmxnet3_dev_stop(struct uk_netdev *dev);
static int vmxnet3_dev_close(struct uk_netdev *dev);
static void vmxnet3_dev_set_rxmode(struct vmxnet3_hw *hw, uint32_t feature, int set);
static int vmxnet3_dev_promiscuous_enable(struct uk_netdev *dev, unsigned mode);
static int vmxnet3_dev_promiscuous_disable(struct uk_netdev *dev);
static void vmxnet3_dev_info_get(struct uk_netdev *dev,
				struct uk_netdev_info *dev_info);
static int vmxnet3_dev_mtu_set(struct uk_netdev *dev, uint16_t mtu);
static int vmxnet3_mac_addr_set(struct uk_netdev *dev,
				 const struct uk_hwaddr *mac_addr);
static void vmxnet3_process_events(struct uk_netdev *dev);
static int vmxnet3_dev_rx_queue_intr_enable(struct uk_netdev *dev,
						struct uk_netdev_rx_queue *queue);
static int vmxnet3_dev_rx_queue_intr_disable(struct uk_netdev *dev,
						struct uk_netdev_rx_queue *queue);

/*
 * The set of PCI devices this driver supports
 */
#define VMWARE_PCI_VENDOR_ID 0x15AD
#define VMWARE_DEV_ID_VMXNET3 0x07B0

static const struct pci_device_id pci_vmxnet3_ids[] = {
	{PCI_DEVICE_ID(VMWARE_PCI_VENDOR_ID, VMWARE_DEV_ID_VMXNET3)},
	/* End of Driver List */
	{ PCI_ANY_DEVICE_ID },
};

static const struct uk_netdev_ops vmxnet3_dev_ops = {
	.rxq_intr_enable = vmxnet3_dev_rx_queue_intr_enable,
	.rxq_intr_disable = vmxnet3_dev_rx_queue_intr_disable,
	// .hwaddr_get recommended
	.hwaddr_set = vmxnet3_mac_addr_set,
	// .mtu_get
	.mtu_set = vmxnet3_dev_mtu_set,
	// .promiscuous_get 
	.promiscuous_set = vmxnet3_dev_promiscuous_enable,
	// vmxnet3_dev_promiscuous_disable
	.info_get = vmxnet3_dev_info_get,
	// .txq_info_get
	// .rxq_info_get
	// .einfo_get optional
	.configure = vmxnet3_dev_configure,
	// .txq_configure
	// .rxq_configure
	.start = vmxnet3_dev_start,
};


static int
is_power_of_2(int n)
{
	int ret = 0;

	while (n) {
		ret += n & 1;
		n = n >> 1;
	}

	return ret == 1;
}

/*
 * Enable the given interrupt
 */
static void
vmxnet3_enable_intr(struct vmxnet3_hw *hw, unsigned int intr_idx)
{
	VMXNET3_WRITE_BAR0_REG(hw, VMXNET3_REG_IMR + intr_idx * 8, 0);
}

/*
 * Disable the given interrupt
 */
static void
vmxnet3_disable_intr(struct vmxnet3_hw *hw, unsigned int intr_idx)
{
	VMXNET3_WRITE_BAR0_REG(hw, VMXNET3_REG_IMR + intr_idx * 8, 1);
}

/*
 * Enable all intrs used by the device
 */
static void
vmxnet3_enable_all_intrs(struct vmxnet3_hw *hw)
{
	Vmxnet3_DSDevRead *devRead = &hw->shared->devRead;


	devRead->intrConf.intrCtrl &= ~VMXNET3_IC_DISABLE_ALL;

	if (hw->intr.lsc_only) {
		vmxnet3_enable_intr(hw, devRead->intrConf.eventIntrIdx);
	} else {
		int i;

		for (i = 0; i < hw->intr.num_intrs; i++)
			vmxnet3_enable_intr(hw, i);
	}
}

/*
 * Disable all intrs used by the device
 */
static void
vmxnet3_disable_all_intrs(struct vmxnet3_hw *hw)
{
	int i;


	hw->shared->devRead.intrConf.intrCtrl |= VMXNET3_IC_DISABLE_ALL;
	for (i = 0; i < hw->num_intrs; i++)
		vmxnet3_disable_intr(hw, i);
}

/*
 * Gets tx data ring descriptor size.
 */
static uint16_t
eth_vmxnet3_txdata_get(struct vmxnet3_hw *hw)
{
	uint16 txdata_desc_size;

	VMXNET3_WRITE_BAR1_REG(hw, VMXNET3_REG_CMD,
			       VMXNET3_CMD_GET_TXDATA_DESC_SIZE);
	txdata_desc_size = VMXNET3_READ_BAR1_REG(hw, VMXNET3_REG_CMD);

	return (txdata_desc_size < VMXNET3_TXDATA_DESC_MIN_SIZE ||
		txdata_desc_size > VMXNET3_TXDATA_DESC_MAX_SIZE ||
		txdata_desc_size & VMXNET3_TXDATA_DESC_SIZE_MASK) ?
		sizeof(struct Vmxnet3_TxDataDesc) : txdata_desc_size;
}


#define PCI_CONF_READ(type, ret, reg, val)					\
	do {								\
		uint32_t _conf_data;					\
		outl(PCI_CONFIG_ADDR, (reg) | val);		\
		_conf_data = ((inl(PCI_CONFIG_DATA) >> 0) \
			      & (-1));			\
		*(ret) = (type) _conf_data;				\
	} while (0)


#define PCI_CONF_WRITE(type, reg, val)					\
	do {								\
		outl((reg), val);		\
	} while (0)


void adjust_pci_device(struct pci_device *pci __unused) {
	unsigned short new_command, pci_command;
	unsigned char pci_latency;

	PCI_CONF_READ(int, &pci_command, PCI_COMMAND, PCI_COMMAND);
	new_command = ( pci_command | PCI_COMMAND_MASTER |
			PCI_COMMAND_MEMORY | PCI_COMMAND_IO );
	if (pci_command != new_command) {
		PCI_CONF_WRITE(int, PCI_COMMAND, new_command);
	}

	// pci_read_config_byte ( pci, PCI_LATENCY_TIMER, &pci_latency);
	// if ( pci_latency < 32 ) {
	// 	DBGC ( pci, PCI_FMT " latency timer is unreasonably low at "
	// 	       "%d. Setting to 32.\n", PCI_ARGS ( pci ), pci_latency );
	// 	pci_write_config_byte ( pci, PCI_LATENCY_TIMER, 32);
	// }
}

/*
 * It returns 0 on success.
 */
static int
vmxnet3_add_dev(struct pci_device * eth_dev)
{
	struct vmxnet3_hw *hw = to_vmxnet3dev(eth_dev);
	uint32_t mac_hi, mac_lo, ver;

	uk_pr_debug("vmxnet3_add_dev\n");
	uk_pr_debug("vmxnet3_add_dev\n");
	uk_pr_debug("vmxnet3_add_dev\n");
	uk_pr_debug("vmxnet3_add_dev\n");
	uk_pr_debug("vmxnet3_add_dev\n");
	uk_pr_debug("vmxnet3_add_dev\n");
	

	hw->netdev.ops = &vmxnet3_dev_ops;
	hw->netdev.rx_one = vmxnet3_recv_pkts;
	hw->netdev.tx_one = vmxnet3_xmit_pkts;

	/* Vendor and Device ID need to be set before init of shared code */
	hw->device_id = VMWARE_DEV_ID_VMXNET3;
	hw->vendor_id = VMWARE_PCI_VENDOR_ID;

	hw->num_rx_queues = 1;
	hw->num_tx_queues = 1;
	hw->bufs_per_pkt = 1;
	hw->hw_addr0 = eth_dev->base;
	hw->hw_addr1 = eth_dev->irq;
	uk_pr_info("hw->hw_addr0 = %p, hw->hw_addr1 = %p\n", hw->hw_addr0, hw->hw_addr1);
	uk_pr_info("eth_dev->base = %p, eth_dev->irq = %p\n", eth_dev->base, eth_dev->irq);


	// adjust_pci_device(eth_dev);
	// for (int i = 0; i < 20480; i++) {
	// 	printf("%d: %x\n", i, *((char *) eth_dev->base + i));
	// }

	// 1st 00:0C:29:E9:74:B5
	// 2nd 00:50:56:3A:1B:2E

	for (int i = 0; i < 0x100000; i++) {
		printf("%x: %x\n", i, vmxnet3_read_addr(i));
	}

	/* Check h/w version compatibility with driver. */
	ver = VMXNET3_READ_BAR1_REG(hw, VMXNET3_REG_VRRS);
	uk_pr_debug("Hardware version: %d\n", ver);

	if (ver & (1 << VMXNET3_REV_4)) {
		VMXNET3_WRITE_BAR1_REG(hw, VMXNET3_REG_VRRS,
				       1 << VMXNET3_REV_4);
		hw->version = VMXNET3_REV_4 + 1;
	} else if (ver & (1 << VMXNET3_REV_3)) {
		VMXNET3_WRITE_BAR1_REG(hw, VMXNET3_REG_VRRS,
				       1 << VMXNET3_REV_3);
		hw->version = VMXNET3_REV_3 + 1;
	} else if (ver & (1 << VMXNET3_REV_2)) {
		VMXNET3_WRITE_BAR1_REG(hw, VMXNET3_REG_VRRS,
				       1 << VMXNET3_REV_2);
		hw->version = VMXNET3_REV_2 + 1;
	} else if (ver & (1 << VMXNET3_REV_1)) {
		VMXNET3_WRITE_BAR1_REG(hw, VMXNET3_REG_VRRS,
				       1 << VMXNET3_REV_1);
		hw->version = VMXNET3_REV_1 + 1;
	} else {
		uk_pr_err("Incompatible hardware version: %d\n", ver);
		// PMD_INIT_LOG(ERR, "Incompatible hardware version: %d", ver);
		return -EIO;
	}

	uk_pr_info("Using device v%d\n", hw->version);

	/* Check UPT version compatibility with driver. */
	ver = VMXNET3_READ_BAR1_REG(hw, VMXNET3_REG_UVRS);
	uk_pr_info("UPT hardware version: %d\n", ver);
	if (ver & 0x1)
		VMXNET3_WRITE_BAR1_REG(hw, VMXNET3_REG_UVRS, 1);
	else {
		uk_pr_err("Incompatible UPT version.\n");
		return -EIO;
	}

	VMXNET3_WRITE_BAR1_REG(hw, VMXNET3_REG_CMD, VMXNET3_CMD_RESET_DEV);


	/* Getting MAC Address */
	mac_lo = VMXNET3_READ_BAR1_REG(hw, VMXNET3_REG_MACL);
	mac_hi = VMXNET3_READ_BAR1_REG(hw, VMXNET3_REG_MACH);
	uk_pr_info("mac_hi %08x\n", mac_hi);
	// uk_pr_debug("mac_lo mac_hi: %08x %08x\n", mac_lo, mac_hi);
	// uk_pr_debug("mac_lo mac_hi: %d %d\n", mac_lo, mac_hi);

	// for (int i = -2048 * 32; i < 2048 * 32; i++) {
	// 	int r = VMXNET3_READ_BAR0_REG(hw, i * 4);

	// 	if (r != -1)
	// 		uk_pr_debug("%d: %08x\n", i * 4, r);
	// }

	memcpy(hw->perm_addr, &mac_lo, 4);
	memcpy(hw->perm_addr + 4, &mac_hi, 2);

	/* Copy the permanent MAC address */
	memcpy((struct uk_hwaddr *)hw->perm_addr,
			&hw->hw_addr0,
			UK_NETDEV_HWADDR_LEN * sizeof(uint8_t));

	uk_pr_debug("MAC Address: %hhX:%hhX:%hhX:%hhX:%hhX:%hhX\n",
		     hw->perm_addr[0], hw->perm_addr[1], hw->perm_addr[2],
		     hw->perm_addr[3], hw->perm_addr[4], hw->perm_addr[5]);

	/* Put device in Quiesce Mode */
	VMXNET3_WRITE_BAR1_REG(hw, VMXNET3_REG_CMD, VMXNET3_CMD_QUIESCE_DEV);

	/* allow untagged pkts */
	VMXNET3_SET_VFTABLE_ENTRY(hw->shadow_vfta, 0);

	hw->txdata_desc_size = VMXNET3_VERSION_GE_3(hw) ?
		eth_vmxnet3_txdata_get(hw) : sizeof(struct Vmxnet3_TxDataDesc);

	hw->rxdata_desc_size = VMXNET3_VERSION_GE_3(hw) ?
		VMXNET3_DEF_RXDATA_DESC_SIZE : 0;
	// RTE_ASSERT((hw->rxdata_desc_size & ~VMXNET3_RXDATA_DESC_SIZE_MASK) ==
		//    hw->rxdata_desc_size);

	return 0;
}

struct uk_alloc *allocator;

static int vmxnet3_drv_init(struct uk_alloc *allocator)
{
	/* driver initialization */
	if (!allocator)
		return -EINVAL;

	uk_pr_debug("vmxnet3_drv_init\n");
	uk_pr_debug("vmxnet3_drv_init\n");
	uk_pr_debug("vmxnet3_drv_init\n");
	uk_pr_debug("vmxnet3_drv_init\n");
	uk_pr_debug("vmxnet3_drv_init\n");


	a = allocator;
	return 0;
}

static struct pci_driver vmxnet3_pci_drv = {
	.device_ids = pci_vmxnet3_ids,
	.init = vmxnet3_drv_init,
	.add_dev = vmxnet3_add_dev,
};
PCI_REGISTER_DRIVER(&vmxnet3_pci_drv);

static void
vmxnet3_alloc_intr_resources(struct uk_netdev *dev)
{
	struct vmxnet3_hw *hw = to_vmxnet3dev(dev);
	uint32_t cfg;
	int nvec = 1; /* for link event */

	/* intr settings */
	VMXNET3_WRITE_BAR1_REG(hw, VMXNET3_REG_CMD,
			       VMXNET3_CMD_GET_CONF_INTR);
	cfg = VMXNET3_READ_BAR1_REG(hw, VMXNET3_REG_CMD);
	hw->intr.type = cfg & 0x3;
	hw->intr.mask_mode = (cfg >> 2) & 0x3;

	if (hw->intr.type == VMXNET3_IT_AUTO)
		hw->intr.type = VMXNET3_IT_MSIX;

	if (hw->intr.type == VMXNET3_IT_MSIX) {
		/* only support shared tx/rx intr */
		if (hw->num_tx_queues != hw->num_rx_queues)
			goto msix_err;

		nvec += hw->num_rx_queues;
		hw->intr.num_intrs = nvec;
		return;
	}

msix_err:
	/* the tx/rx queue interrupt will be disabled */
	hw->intr.num_intrs = 2;
	hw->intr.lsc_only = TRUE;
	// PMD_INIT_LOG(INFO, "Enabled MSI-X with %d vectors", hw->intr.num_intrs);
}

static int
vmxnet3_dev_configure(struct uk_netdev *dev, 
	const struct uk_netdev_conf *conf)
{
	const void *mz;
	struct vmxnet3_hw *hw = to_vmxnet3dev(dev);
	size_t size;

	uk_pr_debug("Configure vmxnet3\n");
	uk_pr_debug("Configure vmxnet3\n");
	uk_pr_debug("Configure vmxnet3\n");
	uk_pr_debug("Configure vmxnet3\n");
	uk_pr_debug("Configure vmxnet3\n");
	uk_pr_debug("Configure vmxnet3\n");
	uk_pr_debug("Configure vmxnet3\n");
	uk_pr_debug("Configure vmxnet3\n");
	uk_pr_debug("Configure vmxnet3\n");



	if (conf->nb_tx_queues > VMXNET3_MAX_TX_QUEUES ||
	    conf->nb_rx_queues > VMXNET3_MAX_RX_QUEUES) {
		// PMD_INIT_LOG(ERR, "ERROR: Number of queues not supported");
		return -EINVAL;
	}

	if (!is_power_of_2(conf->nb_rx_queues)) {
		// PMD_INIT_LOG(ERR, "ERROR: Number of rx queues not power of 2");
		return -EINVAL;
	}

	size = conf->nb_rx_queues * sizeof(struct Vmxnet3_TxQueueDesc) +
		conf->nb_tx_queues * sizeof(struct Vmxnet3_RxQueueDesc);

	if (size > UINT16_MAX)
		return -EINVAL;

	hw->num_rx_queues = (uint8_t)conf->nb_rx_queues;
	hw->num_tx_queues = (uint8_t)conf->nb_tx_queues;

	/*
	 * Allocate a memzone for Vmxnet3_DriverShared - Vmxnet3_DSDevRead
	 * on current socket
	 */
	mz = uk_calloc(a, 1, sizeof(struct Vmxnet3_DriverShared));

	if (mz == NULL) {
		// PMD_INIT_LOG(ERR, "ERROR: Creating shared zone");
		return -ENOMEM;
	}

	hw->shared = mz;
	hw->sharedPA = mz;

	/*
	 * Allocate a memzone for Vmxnet3_RxQueueDesc - Vmxnet3_TxQueueDesc
	 * on current socket.
	 *
	 * We cannot reuse this memzone from previous allocation as its size
	 * depends on the number of tx and rx queues, which could be different
	 * from one config to another.
	 */
	mz = uk_calloc(a, 1, size);
	if (mz == NULL) {
		// PMD_INIT_LOG(ERR, "ERROR: Creating queue descriptors zone");
		return -ENOMEM;
	}

	hw->tqd_start = (Vmxnet3_TxQueueDesc *)mz;
	hw->rqd_start = (Vmxnet3_RxQueueDesc *)(hw->tqd_start + hw->num_tx_queues);

	hw->queueDescPA = mz;
	hw->queue_desc_len = (uint16_t)size;

	vmxnet3_alloc_intr_resources(dev);

	return 0;
}

static void
vmxnet3_write_mac(struct vmxnet3_hw *hw, const uint8_t *addr)
{
	uint32_t val;

	// PMD_INIT_LOG(DEBUG,
		    //  "Writing MAC Address : " RTE_ETHER_ADDR_PRT_FMT,
		    //  addr[0], addr[1], addr[2],
		    //  addr[3], addr[4], addr[5]);

	memcpy(&val, addr, 4);
	VMXNET3_WRITE_BAR1_REG(hw, VMXNET3_REG_MACL, val);

	memcpy(&val, addr + 4, 2);
	VMXNET3_WRITE_BAR1_REG(hw, VMXNET3_REG_MACH, val);
}

static int
vmxnet3_setup_driver_shared(struct uk_netdev *dev)
{
	struct vmxnet3_hw *hw = to_vmxnet3dev(dev);
	uint32_t mtu = hw->mtu;
	Vmxnet3_DriverShared *shared = hw->shared;
	Vmxnet3_DSDevRead *devRead = &shared->devRead;
	uint32_t i;
	int ret;

	shared->magic = VMXNET3_REV1_MAGIC;
	devRead->misc.driverInfo.version = VMXNET3_DRIVER_VERSION_NUM;

	/* Setting up Guest OS information */
	devRead->misc.driverInfo.gos.gosBits   = sizeof(void *) == 4 ?
		VMXNET3_GOS_BITS_32 : VMXNET3_GOS_BITS_64;
	devRead->misc.driverInfo.gos.gosType   = VMXNET3_GOS_TYPE_LINUX;
	devRead->misc.driverInfo.vmxnet3RevSpt = 1;
	devRead->misc.driverInfo.uptVerSpt     = 1;

	devRead->misc.mtu = mtu;
	devRead->misc.queueDescPA  = hw->queueDescPA;
	devRead->misc.queueDescLen = hw->queue_desc_len;
	devRead->misc.numTxQueues  = hw->num_tx_queues;
	devRead->misc.numRxQueues  = hw->num_rx_queues;

	/*
	 * Set number of interrupts to 1
	 * PMD disables all the interrupts but this is MUST to activate device
	 * It needs at least one interrupt for link events to handle
	 * So we'll disable it later after device activation if needed
	 */
	devRead->intrConf.numIntrs = 1;
	devRead->intrConf.intrCtrl |= VMXNET3_IC_DISABLE_ALL;

	for (i = 0; i < hw->num_tx_queues; i++) {
		Vmxnet3_TxQueueDesc *tqd = &hw->tqd_start[i];
		vmxnet3_tx_queue_t *txq  = dev->_tx_queue[i];

		tqd->ctrl.txNumDeferred  = 0;
		tqd->ctrl.txThreshold    = 1;
		tqd->conf.txRingBasePA   = txq->cmd_ring.basePA;
		tqd->conf.compRingBasePA = txq->comp_ring.basePA;
		tqd->conf.dataRingBasePA = txq->data_ring.basePA;

		tqd->conf.txRingSize   = txq->cmd_ring.size;
		tqd->conf.compRingSize = txq->comp_ring.size;
		tqd->conf.dataRingSize = txq->data_ring.size;
		tqd->conf.intrIdx      = txq->comp_ring.intr_idx;
		tqd->status.stopped    = TRUE;
		tqd->status.error      = 0;
		memset(&tqd->stats, 0, sizeof(tqd->stats));
	}

	for (i = 0; i < hw->num_rx_queues; i++) {
		Vmxnet3_RxQueueDesc *rqd  = &hw->rqd_start[i];
		vmxnet3_rx_queue_t *rxq   = dev->_rx_queue[i];

		rqd->conf.rxRingBasePA[0] = rxq->cmd_ring[0].basePA;
		rqd->conf.rxRingBasePA[1] = rxq->cmd_ring[1].basePA;
		rqd->conf.compRingBasePA  = rxq->comp_ring.basePA;

		rqd->conf.rxRingSize[0]   = rxq->cmd_ring[0].size;
		rqd->conf.rxRingSize[1]   = rxq->cmd_ring[1].size;
		rqd->conf.compRingSize    = rxq->comp_ring.size;
		rqd->conf.intrIdx         = rxq->comp_ring.intr_idx;
		rqd->status.stopped       = TRUE;
		rqd->status.error         = 0;
		memset(&rqd->stats, 0, sizeof(rqd->stats));
	}

	/* RxMode set to 0 of VMXNET3_RXM_xxx */
	devRead->rxFilterConf.rxMode = 0;

	vmxnet3_write_mac(hw, hw->perm_addr);

	return VMXNET3_SUCCESS;
}

static int
vmxnet3_dev_setup_memreg(struct uk_netdev *dev)
{
	struct vmxnet3_hw *hw = to_vmxnet3dev(dev);
	Vmxnet3_DriverShared *shared = hw->shared;
	Vmxnet3_CmdInfo *cmdInfo;
	void **mp[VMXNET3_MAX_RX_QUEUES];
	uint8_t index[VMXNET3_MAX_RX_QUEUES + VMXNET3_MAX_TX_QUEUES];
	uint32_t num, i, j, size;

	if (hw->memRegsPA == 0) {
		const void *mz;

		size = sizeof(Vmxnet3_MemRegs) +
			(VMXNET3_MAX_RX_QUEUES + VMXNET3_MAX_TX_QUEUES) *
			sizeof(Vmxnet3_MemoryRegion);

		mz = uk_calloc(a, 1, size);
		if (mz == NULL) {
			// PMD_INIT_LOG(ERR, "ERROR: Creating memRegs zone");
			return -ENOMEM;
		}
		hw->memRegs = mz;
		hw->memRegsPA = mz;
	}

	num = hw->num_rx_queues;

	for (i = 0; i < num; i++) {
		struct uk_netdev_rx_queue *rxq = dev->_rx_queue[i];

		mp[i] = rxq->mp;
		index[i] = 1 << i;
	}

	/*
	 * The same mempool could be used by multiple queues. In such a case,
	 * remove duplicate mempool entries. Only one entry is kept with
	 * bitmask indicating queues that are using this mempool.
	 */
	for (i = 1; i < num; i++) {
		for (j = 0; j < i; j++) {
			if (mp[i] == mp[j]) {
				mp[i] = NULL;
				index[j] |= 1 << i;
				break;
			}
		}
	}

	j = 0;
	for (i = 0; i < num; i++) {
		if (mp[i] == NULL)
			continue;

		Vmxnet3_MemoryRegion *mr = &hw->memRegs->memRegs[j];

		// mr->startPA =
		// 	(uintptr_t)UK_STAILQ_FIRST(&mp[i]->mem_list)->iova;
		// mr->length = UK_STAILQ_FIRST(&mp[i]->mem_list)->len <= INT32_MAX ?
		// 	UK_STAILQ_FIRST(&mp[i]->mem_list)->len : INT32_MAX;
		mr->txQueueBits = index[i];
		mr->rxQueueBits = index[i];

		// PMD_INIT_LOG(INFO,
			    //  "index: %u startPA: %" PRIu64 " length: %u, "
			    //  "rxBits: %x",
			    //  j, mr->startPA, mr->length, mr->rxQueueBits);
		j++;
	}
	hw->memRegs->numRegs = j;
	// PMD_INIT_LOG(INFO, "numRegs: %u", j);

	size = sizeof(Vmxnet3_MemRegs) +
		(j - 1) * sizeof(Vmxnet3_MemoryRegion);

	cmdInfo = &shared->cu.cmdInfo;
	cmdInfo->varConf.confVer = 1;
	cmdInfo->varConf.confLen = size;

	return 0;
}

/*
 * Must be called after vmxnet3_add_dev. Other wise it might fail
 * It returns 0 on success.
 */
static int
vmxnet3_dev_start(struct uk_netdev *dev)
{
	int ret;
	struct vmxnet3_hw *hw = to_vmxnet3dev(dev);

	uk_pr_debug("Starting vmxnet3\n");
	uk_pr_debug("Starting vmxnet3\n");
	uk_pr_debug("Starting vmxnet3\n");
	uk_pr_debug("Starting vmxnet3\n");
	uk_pr_debug("Starting vmxnet3\n");
	uk_pr_debug("Starting vmxnet3\n");
	uk_pr_debug("Starting vmxnet3\n");
	uk_pr_debug("Starting vmxnet3\n");
	uk_pr_debug("Starting vmxnet3\n");
	uk_pr_debug("Starting vmxnet3\n");
	uk_pr_debug("Starting vmxnet3\n");


	hw->intr.num_intrs = 2;
	hw->intr.lsc_only = TRUE;

	ret = vmxnet3_setup_driver_shared(dev);
	if (ret != VMXNET3_SUCCESS)
		return ret;

	/* Exchange shared data with device */
	VMXNET3_WRITE_BAR1_REG(hw, VMXNET3_REG_DSAL,
			       VMXNET3_GET_ADDR_LO(hw->sharedPA));
	VMXNET3_WRITE_BAR1_REG(hw, VMXNET3_REG_DSAH,
			       VMXNET3_GET_ADDR_HI(hw->sharedPA));

	/* Activate device by register write */
	VMXNET3_WRITE_BAR1_REG(hw, VMXNET3_REG_CMD, VMXNET3_CMD_ACTIVATE_DEV);
	ret = VMXNET3_READ_BAR1_REG(hw, VMXNET3_REG_CMD);

	if (ret != 0) {
		// PMD_INIT_LOG(ERR, "Device activation: UNSUCCESSFUL");
		return -EINVAL;
	}

	/* Setup memory region for rx buffers */
	ret = vmxnet3_dev_setup_memreg(dev);
	if (ret == 0) {
		VMXNET3_WRITE_BAR1_REG(hw, VMXNET3_REG_CMD,
				       VMXNET3_CMD_REGISTER_MEMREGS);
		ret = VMXNET3_READ_BAR1_REG(hw, VMXNET3_REG_CMD);
		if (ret != 0)
			// PMD_INIT_LOG(DEBUG,
				    //  "Failed in setup memory region cmd\n");
		ret = 0;
	} else {
		// PMD_INIT_LOG(DEBUG, "Failed to setup memory region\n");
	}

	/*
	 * Load RX queues with blank mbufs and update next2fill index for device
	 * Update RxMode of the device
	 */
	ret = vmxnet3_dev_rxtx_init(dev);
	if (ret != VMXNET3_SUCCESS) {
		// PMD_INIT_LOG(ERR, "Device queue init: UNSUCCESSFUL");
		return ret;
	}

	hw->adapter_stopped = FALSE;

	/* Setting proper Rx Mode and issue Rx Mode Update command */
	vmxnet3_dev_set_rxmode(hw, VMXNET3_RXM_UCAST | VMXNET3_RXM_BCAST, 1);

	/* enable all intrs */
	vmxnet3_enable_all_intrs(hw);

	vmxnet3_process_events(dev);

	return VMXNET3_SUCCESS;
}

/*
 * Stop device: disable rx and tx functions to allow for reconfiguring.
 */
static int
vmxnet3_dev_stop(struct uk_netdev *dev)
{
	struct vmxnet3_hw *hw = to_vmxnet3dev(dev);
	int ret;


	if (hw->adapter_stopped == 1) {
		// PMD_INIT_LOG(DEBUG, "Device already stopped.");
		return 0;
	}

	// PMD_INIT_LOG(DEBUG, "Disabled %d intr callbacks", ret);

	/* disable interrupts */
	vmxnet3_disable_all_intrs(hw);

	/* quiesce the device first */
	VMXNET3_WRITE_BAR1_REG(hw, VMXNET3_REG_CMD, VMXNET3_CMD_QUIESCE_DEV);
	VMXNET3_WRITE_BAR1_REG(hw, VMXNET3_REG_DSAL, 0);
	VMXNET3_WRITE_BAR1_REG(hw, VMXNET3_REG_DSAH, 0);

	/* reset the device */
	VMXNET3_WRITE_BAR1_REG(hw, VMXNET3_REG_CMD, VMXNET3_CMD_RESET_DEV);
	// PMD_INIT_LOG(DEBUG, "Device reset.");

	vmxnet3_dev_clear_queues(dev);

	hw->adapter_stopped = 1;
	// dev->data->dev_started = 0;

	return 0;
}

static void
vmxnet3_free_queues(struct uk_netdev *dev)
{
	int i;
	struct vmxnet3_hw *hw = to_vmxnet3dev(dev);

	for (i = 0; i < hw->num_rx_queues; i++)
		vmxnet3_dev_rx_queue_release(dev, i);
	hw->num_rx_queues = 0;

	for (i = 0; i < hw->num_tx_queues; i++)
		vmxnet3_dev_tx_queue_release(dev, i);
	hw->num_tx_queues = 0;
}

/*
 * Reset and stop device.
 */
static int
vmxnet3_dev_close(struct uk_netdev *dev)
{
	int ret;

	ret = vmxnet3_dev_stop(dev);
	vmxnet3_free_queues(dev);

	return ret;
}

static void
vmxnet3_dev_info_get(struct uk_netdev *dev,
		     struct uk_netdev_info *dev_info)
{
	struct vmxnet3_hw *hw = to_vmxnet3dev(dev);

	dev_info->max_rx_queues = VMXNET3_MAX_RX_QUEUES;
	dev_info->max_tx_queues = VMXNET3_MAX_TX_QUEUES;
	dev_info->max_mtu = VMXNET3_MAX_MTU;
}

static int
vmxnet3_dev_mtu_set(struct uk_netdev *dev, uint16_t mtu)
{
	return 0;
}

static int
vmxnet3_mac_addr_set(struct uk_netdev *dev, const struct uk_hwaddr *mac_addr)
{
	struct vmxnet3_hw *hw = to_vmxnet3dev(dev);

	memcpy(mac_addr, (struct uk_hwaddr *)(hw->perm_addr), UK_NETDEV_HWADDR_LEN * sizeof(uint8_t));
	vmxnet3_write_mac(hw, mac_addr->addr_bytes);
	return 0;
}

/* Updating rxmode through Vmxnet3_DriverShared structure in adapter */
static void
vmxnet3_dev_set_rxmode(struct vmxnet3_hw *hw, uint32_t feature, int set)
{
	struct Vmxnet3_RxFilterConf *rxConf = &hw->shared->devRead.rxFilterConf;

	if (set)
		rxConf->rxMode = rxConf->rxMode | feature;
	else
		rxConf->rxMode = rxConf->rxMode & (~feature);

	VMXNET3_WRITE_BAR1_REG(hw, VMXNET3_REG_CMD, VMXNET3_CMD_UPDATE_RX_MODE);
}

/* Promiscuous supported only if Vmxnet3_DriverShared is initialized in adapter */
static int
vmxnet3_dev_promiscuous_enable(struct uk_netdev *dev, unsigned mode)
{
	struct vmxnet3_hw *hw = to_vmxnet3dev(dev);
	uint32_t *vf_table = hw->shared->devRead.rxFilterConf.vfTable;

	memset(vf_table, 0, VMXNET3_VFT_TABLE_SIZE);
	vmxnet3_dev_set_rxmode(hw, VMXNET3_RXM_PROMISC, 1);

	VMXNET3_WRITE_BAR1_REG(hw, VMXNET3_REG_CMD,
			       VMXNET3_CMD_UPDATE_VLAN_FILTERS);

	return 0;
}

/* Promiscuous supported only if Vmxnet3_DriverShared is initialized in adapter */
static int
vmxnet3_dev_promiscuous_disable(struct uk_netdev *dev)
{
	struct vmxnet3_hw *hw = to_vmxnet3dev(dev);
	uint32_t *vf_table = hw->shared->devRead.rxFilterConf.vfTable;

	memset(vf_table, 0xff, VMXNET3_VFT_TABLE_SIZE);
	vmxnet3_dev_set_rxmode(hw, VMXNET3_RXM_PROMISC, 0);
	VMXNET3_WRITE_BAR1_REG(hw, VMXNET3_REG_CMD,
			       VMXNET3_CMD_UPDATE_VLAN_FILTERS);

	return 0;
}

static void
vmxnet3_process_events(struct uk_netdev *dev)
{
	struct vmxnet3_hw *hw = to_vmxnet3dev(dev);
	uint32_t events = hw->shared->ecr;

	if (!events)
		return;

	/*
	 * ECR bits when written with 1b are cleared. Hence write
	 * events back to ECR so that the bits which were set will be reset.
	 */
	VMXNET3_WRITE_BAR1_REG(hw, VMXNET3_REG_ECR, events);

	// /* Check if there is an error on xmit/recv queues */
	// if (events & (VMXNET3_ECR_TQERR | VMXNET3_ECR_RQERR)) {
	// 	VMXNET3_WRITE_BAR1_REG(hw, VMXNET3_REG_CMD,
	// 			       VMXNET3_CMD_GET_QUEUE_STATUS);

	// 	if (hw->tqd_start->status.stopped)
	// 		PMD_DRV_LOG(ERR, "tq error 0x%x",
	// 			    hw->tqd_start->status.error);

	// 	if (hw->rqd_start->status.stopped)
	// 		PMD_DRV_LOG(ERR, "rq error 0x%x",
	// 			     hw->rqd_start->status.error);

	// 	/* Reset the device */
	// 	/* Have to reset the device */
	// }

	// if (events & VMXNET3_ECR_DIC)
	// 	PMD_DRV_LOG(DEBUG, "Device implementation change event.");

	// if (events & VMXNET3_ECR_DEBUG)
	// 	PMD_DRV_LOG(DEBUG, "Debug event generated by device.");
}

static int
vmxnet3_dev_rx_queue_intr_enable(struct uk_netdev *dev, struct uk_netdev_rx_queue *queue)
{
	struct vmxnet3_hw *hw = to_vmxnet3dev(dev);

	vmxnet3_enable_intr(hw, queue->queue_id);

	return 0;
}

static int
vmxnet3_dev_rx_queue_intr_disable(struct uk_netdev *dev, struct uk_netdev_rx_queue *queue)
{
	struct vmxnet3_hw *hw = to_vmxnet3dev(dev);

	vmxnet3_disable_intr(hw, queue->queue_id);

	return 0;
}
