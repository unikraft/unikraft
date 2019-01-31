/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Simon Kuenzer <simon.kuenzer@neclab.eu>
 *          Razvan Cojocaru <razvan.cojocaru93@gmail.com>
 *
 * Copyright (c) 2017 Intel Corporation
 * Copyright (c) 2018, NEC Europe Ltd., NEC Corporation. All rights reserved.
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
 * THIS HEADER MAY NOT BE EXTRACTED OR MODIFIED IN ANY WAY.
 */
/* Derived from DPDK rte_ethdev_core.h - DPDK.org 18.02.2 */
#ifndef __UK_NETDEV_CORE__
#define __UK_NETDEV_CORE__

#include <sys/types.h>
#include <stdint.h>
#include <errno.h>
#include <uk/config.h>
#include <uk/netbuf.h>
#include <uk/list.h>
#include <uk/alloc.h>
#include <uk/essentials.h>
#ifdef CONFIG_LIBUKNETDEV_DISPATCHERTHREADS
#include <uk/sched.h>
#include <uk/semaphore.h>
#endif

/**
 * Unikraft network API common declarations.
 *
 * This header contains all API data types. Some of them are part of the
 * public API and some are part of the internal API.
 *
 * The device data and operations are separated. This split allows the
 * function pointer and driver data to be per-process, while the actual
 * configuration data for the device is shared.
 */

#ifdef __cplusplus
extern "C" {
#endif

struct uk_netdev;
UK_TAILQ_HEAD(uk_netdev_list, struct uk_netdev);

/**
 * A structure used for Ethernet hardware addresses
 */
#define UK_NETDEV_HWADDR_LEN 6 /**< Length of Ethernet address. */

struct uk_hwaddr {
	uint8_t addr_bytes[UK_NETDEV_HWADDR_LEN];
} __packed;

/**
 * A structure used to describe network device capabilities.
 */
struct uk_netdev_info {
	uint16_t max_rx_queues;
	uint16_t max_tx_queues;
	int in_queue_pairs; /**< If true, allocate queues in pairs. */
	uint16_t max_mtu;   /**< Maximum supported MTU size. */
	uint16_t nb_encap_tx;  /**< Number of bytes required as headroom for tx. */
	uint16_t nb_encap_rx;  /**< Number of bytes required as headroom for rx. */
};

/**
 * A structure used to describe device descriptor ring limitations.
 */
struct uk_netdev_queue_info {
	uint16_t nb_max;        /**< Max allowed number of descriptors. */
	uint16_t nb_min;        /**< Min allowed number of descriptors. */
	uint16_t nb_align;      /**< Number should be a multiple of nb_align. */
	int nb_is_power_of_two; /**< Number should be a power of two. */
};

/**
 * A structure used to configure a network device.
 */
struct uk_netdev_conf {
	uint16_t nb_rx_queues;
	uint16_t nb_tx_queues;
};

/**
 * @internal Queue structs that are defined internally by each driver
 * The datatype is introduced here for having type checking on the
 * API code
 */
struct uk_netdev_tx_queue;
struct uk_netdev_rx_queue;

/**
 * Enum to describe possible states of an Unikraft network device.
 */
enum uk_netdev_state {
	UK_NETDEV_INVALID = 0,
	UK_NETDEV_UNCONFIGURED,
	UK_NETDEV_CONFIGURED,
	UK_NETDEV_RUNNING,
};

/**
 * Enum used by the extra information query interface.
 *
 * The purpose of this type is to allow drivers to forward extra configurations
 * options such as IP information without parsing this data by themselves (e.g.,
 * strings of IP address and mask found on XenStore by netfront).
 * We do not want to introduce any additional parsing logic inside uknetdev API
 * because we assume that most network stacks provide this functionality
 * anyways. So one could forward this data within the glue code.
 *
 * This list is extensible in the future without needing the drivers to adopt
 * any or all of the data types.
 *
 * The extra information can available in one of the following formats:
 * - *_NINT16: Network-order raw int (4 bytes)
 * - *_STR: Null-terminated string
 */
enum uk_netdev_einfo_type {
	/* IPv4 address and mask */
	UK_NETDEV_IPV4_ADDR_NINT16,
	UK_NETDEV_IPV4_ADDR_STR,
	UK_NETDEV_IPV4_MASK_NINT16,
	UK_NETDEV_IPV4_MASK_STR,

	/* IPv4 gateway */
	UK_NETDEV_IPV4_GW_NINT16,
	UK_NETDEV_IPV4_GW_STR,

	/* IPv4 Primary DNS */
	UK_NETDEV_IPV4_DNS0_NINT16,
	UK_NETDEV_IPV4_DNS0_STR,

	/* IPv4 Secondary DNS */
	UK_NETDEV_IPV4_DNS1_NINT16,
	UK_NETDEV_IPV4_DNS1_STR,
};

/**
 * Function type used for queue event callbacks.
 *
 * @param dev
 *   The Unikraft Network Device.
 * @param queue
 *   The queue on the Unikraft network device on which the event happened.
 * @param argp
 *   Extra argument that can be defined on callback registration.
 */
typedef void (*uk_netdev_queue_event_t)(struct uk_netdev *dev,
					uint16_t queue_id, void *argp);

/**
 * User callback used by the driver to allocate netbufs
 * that are used to setup receive descriptors.
 *
 * @param argp
 *   User-provided argument.
 * @param pkts
 *   Array for netbuf pointers that the function should allocate.
 * @param count
 *   Number of netbufs requested (equal to length of pkts).
 * @return
 *   Number of successful allocated netbufs,
 *   has to be in range [0, count].
 *   References to allocated packets are placed to pkts[0]...pkts[count -1].
 */
typedef uint16_t (*uk_netdev_alloc_rxpkts)(void *argp,
					   struct uk_netbuf *pkts[],
					   uint16_t count);

/**
 * A structure used to configure an Unikraft network device RX queue.
 */
struct uk_netdev_rxqueue_conf {
	uk_netdev_queue_event_t callback; /**< Event callback function. */
	void *callback_cookie;            /**< Argument pointer for callback. */

	struct uk_alloc *a;               /**< Allocator for descriptors. */

	uk_netdev_alloc_rxpkts alloc_rxpkts; /**< Allocator for rx netbufs */
	void *alloc_rxpkts_argp;             /**< Argument for alloc_rxpkts */
#ifdef CONFIG_LIBUKNETDEV_DISPATCHERTHREADS
	struct uk_sched *s;               /**< Scheduler for dispatcher. */
#endif
};

/**
 * A structure used to configure an Unikraft network device TX queue.
 */
struct uk_netdev_txqueue_conf {
	struct uk_alloc *a;               /* Allocator for descriptors. */
};

/** Driver callback type to read device/driver capabilities,
 *  used for configuring the device
 */
typedef void (*uk_netdev_info_get_t)(struct uk_netdev *dev,
				     struct uk_netdev_info *dev_info);

/** Driver callback type to read any extra configuration. */
typedef const void *(*uk_netdev_einfo_get_t)(struct uk_netdev *dev,
					     enum uk_netdev_einfo_type econf);

/** Driver callback type to configure a network device. */
typedef int  (*uk_netdev_configure_t)(struct uk_netdev *dev,
				      const struct uk_netdev_conf *conf);

/** Driver callback type to retrieve RX queue limitations,
 *  used for configuring the RX queue later
 */
typedef int (*uk_netdev_rxq_info_get_t)(struct uk_netdev *dev,
	uint16_t queue_id, struct uk_netdev_queue_info *queue_info);

/** Driver callback type to retrieve TX queue limitations,
 *  used for configuring the TX queue later
 */
typedef int (*uk_netdev_txq_info_get_t)(struct uk_netdev *dev,
	uint16_t queue_id, struct uk_netdev_queue_info *queue_info);

/** Driver callback type to set up a RX queue of an Unikraft network device. */
typedef struct uk_netdev_tx_queue * (*uk_netdev_txq_configure_t)(
	struct uk_netdev *dev, uint16_t queue_id, uint16_t nb_desc,
	struct uk_netdev_txqueue_conf *tx_conf);

/** Driver callback type to set up a TX queue of an Unikraft network device. */
typedef struct uk_netdev_rx_queue * (*uk_netdev_rxq_configure_t)(
	struct uk_netdev *dev, uint16_t queue_id, uint16_t nb_desc,
	struct uk_netdev_rxqueue_conf *rx_conf);

/** Driver callback type to start a configured Unikraft network device. */
typedef int  (*uk_netdev_start_t)(struct uk_netdev *dev);

/** Driver callback type to get the hardware address. */
typedef const struct uk_hwaddr *(*uk_netdev_hwaddr_get_t)(
	struct uk_netdev *dev);

/** Driver callback type to set the hardware address. */
typedef int (*uk_netdev_hwaddr_set_t)(struct uk_netdev *dev,
				      const struct uk_hwaddr *hwaddr);

/** Driver callback type to get the current promiscuous mode */
typedef unsigned (*uk_netdev_promiscuous_get_t)(struct uk_netdev *dev);

/** Driver callback type to enable or disable promiscuous mode */
typedef int (*uk_netdev_promiscuous_set_t)(struct uk_netdev *dev,
					   unsigned mode);

/** Driver callback type to get the MTU. */
typedef uint16_t (*uk_netdev_mtu_get_t)(struct uk_netdev *dev);

/** Driver callback type to set the MTU */
typedef int (*uk_netdev_mtu_set_t)(struct uk_netdev *dev, uint16_t mtu);

/** Driver callback type to enable interrupts of a RX queue */
typedef int (*uk_netdev_rxq_intr_enable_t)(struct uk_netdev *dev,
					   struct uk_netdev_rx_queue *queue);

/** Driver callback type to disable interrupts of a RX queue */
typedef int (*uk_netdev_rxq_intr_disable_t)(struct uk_netdev *dev,
					    struct uk_netdev_rx_queue *queue);

/**
 * Status code flags returned by rx and tx functions
 */
/** Successful operation (packet received or transmitted). */
#define UK_NETDEV_STATUS_SUCCESS  (0x1)
/**
 * More room available for operation (e.g., still space on queue for sending
 * or more packets available on receive queue
 */
#define UK_NETDEV_STATUS_MORE     (0x2)
/** Queue underrun (e.g., out-of-memory when allocating new receive buffers). */
#define UK_NETDEV_STATUS_UNDERRUN (0x4)

/** Driver callback type to retrieve one packet from a RX queue. */
typedef int (*uk_netdev_rx_one_t)(struct uk_netdev *dev,
				  struct uk_netdev_rx_queue *queue,
				  struct uk_netbuf **pkt);

/** Driver callback type to submit one packet to a TX queue. */
typedef int (*uk_netdev_tx_one_t)(struct uk_netdev *dev,
				  struct uk_netdev_tx_queue *queue,
				  struct uk_netbuf *pkt);

/**
 * A structure containing the functions exported by a driver.
 */
struct uk_netdev_ops {
	/** RX queue interrupts. */
	uk_netdev_rxq_intr_enable_t     rxq_intr_enable;  /* optional */
	uk_netdev_rxq_intr_disable_t    rxq_intr_disable; /* optional */

	/** Set/Get hardware address. */
	uk_netdev_hwaddr_get_t          hwaddr_get;       /* recommended */
	uk_netdev_hwaddr_set_t          hwaddr_set;       /* optional */

	/** Set/Get MTU. */
	uk_netdev_mtu_get_t             mtu_get;
	uk_netdev_mtu_set_t             mtu_set;          /* optional */

	/** Promiscuous mode. */
	uk_netdev_promiscuous_set_t     promiscuous_set;  /* optional */
	uk_netdev_promiscuous_get_t     promiscuous_get;

	/** Device/driver capabilities and info. */
	uk_netdev_info_get_t            info_get;
	uk_netdev_txq_info_get_t        txq_info_get;
	uk_netdev_rxq_info_get_t        rxq_info_get;
	uk_netdev_einfo_get_t           einfo_get;        /* optional */

	/** Device life cycle. */
	uk_netdev_configure_t           configure;
	uk_netdev_txq_configure_t       txq_configure;
	uk_netdev_rxq_configure_t       rxq_configure;
	uk_netdev_start_t               start;
};

/**
 * @internal
 * Event handler configuration (internal to libuknetdev)
 */
struct uk_netdev_event_handler {
	uk_netdev_queue_event_t callback;
	void                    *cookie;

#ifdef CONFIG_LIBUKNETDEV_DISPATCHERTHREADS
	struct uk_semaphore events;      /**< semaphore to trigger events */
	struct uk_netdev    *dev;        /**< reference to net device */
	uint16_t            queue_id;    /**< queue id which caused event */
	struct uk_thread    *dispatcher; /**< dispatcher thread */
	char                *dispatcher_name; /**< reference to thread name */
	struct uk_sched     *dispatcher_s;    /**< Scheduler for dispatcher. */
#endif
};

/**
 * @internal
 * libuknetdev internal data associated with each network device.
 */
struct uk_netdev_data {
	enum uk_netdev_state state;

	struct uk_netdev_event_handler
			     rxq_handler[CONFIG_LIBUKNETDEV_MAXNBQUEUES];

	const uint16_t       id;    /**< ID is assigned during registration */
	const char           *drv_name;
};

/**
 * NETDEV
 * A structure used to interact with a network device.
 *
 * Function callbacks (tx_one, rx_one, ops) are registered by the driver before
 * registering the netdev. They change during device life time. Packet RX/TX
 * functions are added directly to this structure for performance reasons.
 * It prevents another indirection to ops.
 */
struct uk_netdev {
	/** Packet transmission. */
	uk_netdev_tx_one_t          tx_one; /* by driver */

	/** Packet reception. */
	uk_netdev_rx_one_t          rx_one; /* by driver */

	/** Pointer to API-internal state data. */
	struct uk_netdev_data       *_data;

	/** Functions callbacks by driver. */
	const struct uk_netdev_ops  *ops;   /* by driver */

	/** Pointers to queues (API-private) */
	struct uk_netdev_rx_queue   *_rx_queue[CONFIG_LIBUKNETDEV_MAXNBQUEUES];
	struct uk_netdev_tx_queue   *_tx_queue[CONFIG_LIBUKNETDEV_MAXNBQUEUES];

	UK_TAILQ_ENTRY(struct uk_netdev) _list;
};

#ifdef __cplusplus
}
#endif

#endif /* __UK_NETDEV_CORE__ */
