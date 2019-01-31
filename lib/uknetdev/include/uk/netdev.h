/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Simon Kuenzer <simon.kuenzer@neclab.eu>
 *          Razvan Cojocaru <razvan.cojocaru93@gmail.com>
 *
 * Copyright (c) 2010-2017 Intel Corporation
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
#ifndef __UK_NETDEV__
#define __UK_NETDEV__

#include <uk/netdev_core.h>
#include <uk/assert.h>
#include <uk/errptr.h>

/**
 * Unikraft Network API
 *
 * The Unikraft netdev API provides a generalized interface between network
 * device drivers and network stack implementations or low-level network
 * applications.
 *
 * Most API interfaces take as parameter a reference to the corresponding
 * Unikraft Network Device (struct uk_netdev) which can be initially obtained
 * by its ID by calling uk_netdev_get(). The network application should store
 * this reference and use it for subsequent API calls.
 *
 * The functions exported by the Unikraft NET API to setup a device
 * must be invoked in the following order:
 *     - uk_netdev
 *     - uk_netdev_configure()
 *     - uk_netdev_txq_configure()
 *     - uk_netdev_rxq_configure()
 *     - uk_netdev_start()
 * The transmit and receive functions should not be invoked when the
 * device is not started.
 *
 * There are 4 states in which a network device can be found:
 *     - UK_NETDEV_UNREGISTERED
 *     - UK_NETDEV_UNCONFIGURED
 *     - UK_NETDEV_CONFIGURED
 *     - UK_NETDEV_RUNNING
 */
#ifdef __cplusplus
extern "C" {
#endif

/**
 * Get the number of registered network devices.
 *
 * @return
 *   - (unsigned int): number of network devices.
 */
unsigned int uk_netdev_count(void);

/**
 * Get the reference to a network device based on its ID.
 * Valid IDs are in the range of 0 to (n-1) where n is
 * the number of available network devices.
 *
 * @param id
 *   The identifier of the network device to configure.
 * @return
 *   - (NULL): device not available
 *   - (struct uk_netdev *): reference to network device
 */
struct uk_netdev *uk_netdev_get(unsigned int id);

/**
 * Returns the id of a network device
 *
 * @param dev
 *   The Unikraft Network Device.
 * @return
 *   - (>=0): Device ID
 */
uint16_t uk_netdev_id_get(struct uk_netdev *dev);

/**
 * Returns the driver name of a network device.
 * The name might be set to NULL
 *
 * @param dev
 *   The Unikraft Network Device.
 * @return
 *   - (NULL): if no name is defined.
 *   - (const char *): Reference to string if name is available.
 */
const char *uk_netdev_drv_name_get(struct uk_netdev *dev);

/**
 * Returns the current state of a network device.
 *
 * @param dev
 *   The Unikraft Network Device.
 * @return
 *   - (enum uk_netdev_state): current device state
 */
enum uk_netdev_state uk_netdev_state_get(struct uk_netdev *dev);

/**
 * Query device capabilities.
 * Information that is useful for device initialization (e.g.,
 * maximum number of supported RX/TX queues).
 *
 * @param dev
 *   The Unikraft Network Device.
 * @param dev_info
 *   A pointer to a structure of type *uk_netdev_info* to be filled with
 *   the contextual information of the network device.
 */
void uk_netdev_info_get(struct uk_netdev *dev,
			struct uk_netdev_info *dev_info);

/**
 * Extra information query interface.
 * The user can query the driver for any additional information (e.g,
 * IP address configuration preferred by hypervisor), using a number of
 * pre-defined configuration types.
 *
 * If the driver doesn't support the provided data type, it must return NULL.
 *
 * This allows the driver to provide configuration data without the need of
 * parsing it in a pre-determined way, eliminating the need for utility
 * functions in the API, or parsing the data multiple times both by driver
 * and user.
 *
 * @param dev
 *   The Unikraft Network Device.
 * @param einfo
 *   Extra configuration type selection (see enum definition).
 * @return
 *   - (NULL): if configuration unavailable or data type unsupported
 *   - (void *): Reference to configuration, format specified by *einfo*
 */
const void *uk_netdev_einfo_get(struct uk_netdev *dev,
				enum uk_netdev_einfo_type einfo);

/**
 * Configures an Unikraft network device.
 *
 * @param dev
 *   The Unikraft Network Device in unconfigured state.
 * @param conf
 *   The pointer to the configuration data to be used for the Unikraft
 *   network device.
 * @return
 *   - (0): Success, device is in configured state.
 *   - (<0): Error code returned by the driver.
 */
int uk_netdev_configure(struct uk_netdev *dev,
			const struct uk_netdev_conf *dev_conf);


/**
 * Query receive device queue capabilities.
 * Information that is useful for device queue initialization (e.g.,
 * maximum number of supported descriptors on RX queues).
 *
 * @param dev
 *   The Unikraft Network Device in configured state.
 * @param queue_id
 *   The index of the receive queue to set up.
 *   The value must be in the range [0, nb_rx_queue - 1] previously supplied
 *   to uk_netdev_configure().
 * @param queue_info
 *   A pointer to a structure of type *uk_netdev_queues_info* to be filled out
 * @return
 *   - (0): Success, queue_info is filled out.
 *   - (<0): Error code of the drivers function.
 */
int uk_netdev_rxq_info_get(struct uk_netdev *dev, uint16_t queue_id,
			   struct uk_netdev_queue_info *queue_info);

/**
 * Sets up one receive queue for an Unikraft network device.
 *
 * @param dev
 *   The Unikraft Network Device in configured state.
 * @param queue_id
 *   The index of the receive queue to set up.
 *   The value must be in the range [0, nb_rx_queue - 1] previously supplied
 *   to uk_netdev_configure().
 * @param nb_desc
 *   Number of descriptors for the queue. Inspect uk_netdev_rxq_info_get() to
 *   retrieve limitations. If nb_desc is set to 0, the driver chooses a default
 *   value.
 * @param rx_conf
 *   The pointer to the configuration data to be used for the receive queue.
 *   Its memory can be released after invoking this function. Please note that
 *   the receive buffer allocator (`rx_conf->alloc_rxpkts`) has to be
 *   interrupt-context-safe when `uk_netdev_rx_one` is going to be called from
 *   interrupt context.
 * @return
 *   - (0): Success, receive queue correctly set up.
 *   - (-ENOMEM): Unable to allocate the receive ring descriptors.
 */
int uk_netdev_rxq_configure(struct uk_netdev *dev, uint16_t queue_id,
			    uint16_t nb_desc,
			    struct uk_netdev_rxqueue_conf *rx_conf);

/**
 * Query device transmit queue capabilities.
 * Information that is useful for device queue initialization (e.g.,
 * maximum number of supported descriptors on TX queues).
 *
 * @param dev
 *   The Unikraft Network Device in configured state.
 * @param queue_id
 *   The index of the receive queue to set up.
 *   The value must be in the range [0, nb_tx_queue - 1] previously supplied
 *   to uk_netdev_configure().
 * @param queue_info
 *   A pointer to a structure of type *uk_netdev_queues_info* to be filled out
 * @return
 *   - (0): Success, queue_info is filled out.
 *   - (<0): Error code of the drivers function.
 */
int uk_netdev_txq_info_get(struct uk_netdev *dev, uint16_t queue_id,
			   struct uk_netdev_queue_info *queue_info);

/**
 * Sets up one transmit queue for an Unikraft network device.
 *
 * @param dev
 *   The Unikraft Network Device in configured state.
 * @param queue_id
 *   The index of the transmit queue to set up.
 *   The value must be in the range [0, nb_tx_queue - 1] previously supplied
 *   to uk_netdev_configure().
 * @param nb_desc
 *   Number of descriptors for the queue. Inspect uk_netdev_txq_info_get() to
 *   retrieve limitations. If nb_desc is set to 0, the driver chooses a default
 *   value.
 * @param tx_conf
 *   The pointer to the configuration data to be used for the transmit queue.
 *   Its memory can be released after invoking this function.
 * @return
 *   - (0): Success, the transmit queue is correctly set up.
 *   - (-ENOMEM): Unable to allocate the transmit ring descriptors.
 */
int uk_netdev_txq_configure(struct uk_netdev *dev, uint16_t queue_id,
			    uint16_t nb_desc,
			    struct uk_netdev_txqueue_conf *tx_conf);

/**
 * Start a Network device.
 *
 * After a network device was configured and its queues are set up the device
 * can be started. On success, all operational functions exported by the
 * Unikraft netdev API (interrupts, receive/transmit, and so on) can be invoked
 * afterwards.
 *
 * @param dev
 *   The Unikraft Network Device in configured state.
 * @return
 *   - (0): Success, Unikraft network device started.
 *   - (<0): Error code of the driver device start function.
 */
int uk_netdev_start(struct uk_netdev *dev);

/**
 * Returns the hardware address of the Unikraft network device.
 *
 * @param dev
 *   The Unikraft Network Device.
 * @return
 *   - (NULL): no hardware address available
 *   - (struct uk_hwaddr *): Reference to hardware address
 */
const struct uk_hwaddr *uk_netdev_hwaddr_get(struct uk_netdev *dev);

/**
 * Set the hardware address.
 *
 * @param dev
 *   The Unikraft Network Device.
 * @param hwaddr
 *   Reference to new hardware address.
 * @return
 *   - (0): if successful.
 *   - (-ENOTSUP): if hardware doesn't support.
 *   - (-EINVAL): if address is invalid.
 */
int uk_netdev_hwaddr_set(struct uk_netdev *dev, const struct uk_hwaddr *hwaddr);

/**
 * Returns if promiscuous mode is enabled for an Unikraft network device.
 *
 * @param dev
 *   The Unikraft Network Device.
 * @return
 *   - (>0): if promiscuous is enabled
 *   - (0): if promiscuous is disabled.
 */
unsigned uk_netdev_promiscuous_get(struct uk_netdev *dev);

/**
 * Enables or disables promiscuous mode for an Unikraft network device.
 *
 * @param dev
 *   The Unikraft Network Device.
 * @param mode
 *   - (0): disable promiscuous mode
 *   - (1): enable promiscuous mode
 * @return
 *   - (0): if successful.
 *   - (-ENOTSUP): if operation is not supported.
 *   - (<0): on error returned by driver
 */
int uk_netdev_promiscuous_set(struct uk_netdev *dev, unsigned mode);

/**
 * Returns the current MTU of an Unikraft network device.
 *
 * @param dev
 *   The Unikraft Network Device.
 * @return
 *   - MTU of the uk_netdev
 */
uint16_t uk_netdev_mtu_get(struct uk_netdev *dev);

/**
 * Change the MTU of an Unikraft network device.
 *
 * @param dev
 *   The Unikraft Network Device.
 * @param mtu
 *   A uint16_t for the MTU to be applied.
 * @return
 *   - (0): if successful.
 *   - (-ENOTSUP): if operation is not supported.
 *   - (<0): on error returned by driver
 */
int uk_netdev_mtu_set(struct uk_netdev *dev, uint16_t mtu);

/**
 * Enable interrupts for an RX queue.
 *
 * @param dev
 *   The Unikraft Network Device in running state.
 * @param queue_id
 *   The index of the receive queue to set up.
 *   The value must be in the range [0, nb_rx_queue - 1] previously supplied
 *   to uk_netdev_configure().
 * @return
 *   - (0): Success, interrupts enabled.
 *   - (1): More packets are left on the queue, interrupts are NOT enabled yet.
 *          Interrupts will be automatically enabled when all received packets
 *          on the queue are consumed and the receive queue is empty.
 *   - (-ENOTSUP): Driver does not support interrupts.
 */
static inline int uk_netdev_rxq_intr_enable(struct uk_netdev *dev,
					    uint16_t queue_id)
{
	UK_ASSERT(dev);
	UK_ASSERT(dev->ops);
	UK_ASSERT(dev->_data);
	UK_ASSERT(dev->_data->state == UK_NETDEV_RUNNING);
	UK_ASSERT(queue_id < CONFIG_LIBUKNETDEV_MAXNBQUEUES);
	UK_ASSERT(!PTRISERR(dev->_rx_queue[queue_id]));

	if (unlikely(!dev->ops->rxq_intr_enable))
		return -ENOTSUP;
	return dev->ops->rxq_intr_enable(dev, dev->_rx_queue[queue_id]);
}

/**
 * Disable interrupts for an RX queue.
 *
 * @param dev
 *   The Unikraft Network Device in running state.
 * @param queue_id
 *   The index of the receive queue to set up.
 *   The value must be in the range [0, nb_tx_queue - 1] previously supplied
 *   to uk_netdev_configure().
 * @return
 *   - (0): Success, interrupts disabled.
 *   - (-ENOTSUP): Driver does not support interrupts.
 */
static inline int uk_netdev_rxq_intr_disable(struct uk_netdev *dev,
					     uint16_t queue_id)
{
	UK_ASSERT(dev);
	UK_ASSERT(dev->ops);
	UK_ASSERT(dev->_data);
	UK_ASSERT(dev->_data->state == UK_NETDEV_RUNNING);
	UK_ASSERT(queue_id < CONFIG_LIBUKNETDEV_MAXNBQUEUES);
	UK_ASSERT(!PTRISERR(dev->_rx_queue[queue_id]));

	if (unlikely(!dev->ops->rxq_intr_disable))
		return -ENOTSUP;
	return dev->ops->rxq_intr_disable(dev, dev->_rx_queue[queue_id]);
}

/**
 * Receive one packet and re-program used receive descriptors. In order to avoid
 * race conditions, queue interrupts have to be off while executing this
 * function. When operating the queue in interrupt mode, this is automatically
 * the case as soon as an interrupt arrived or the return code of
 * uk_netdev_rxq_intr_enable() indicated that packets are left on the queue.
 * In both cases, uk_netdev_rx_one() is going to enable interrupts again as soon
 * as the last packet was received from the queue.
 * If this function is called from interrupt context (e.g., within receive event
 * handler when no dispatcher threads are configured) make sure that the
 * provided receive buffer allocator function is interrupt-context-safe
 * (see: `uk_netdev_rxq_configure`).
 *
 * @param dev
 *   The Unikraft Network Device.
 * @param queue_id
 *   The index of the receive queue to receive from.
 *   The value must be in the range [0, nb_rx_queue - 1] previously supplied
 *   to uk_netdev_configure().
 * @param pkt
 *   Reference to netbuf pointer which will be point to the received packet
 *   after the function call. `pkt` has never to be `NULL`.
 * @return
 *   - (>=0): Positive value with status flags
 *     - UK_NETDEV_STATUS_SUCCESS: `pkt` points to received netbuf. Whenever
 *        this flag is not set, there was no packet received.
 *     - UK_NETDEV_STATUS_MORE: Indicates that more received packets are
 *        available on the receive queue. When interrupts are used, they are
 *        disabled until this flag is unset by a subsequent call.
 *        This flag may only be set together with UK_NETDEV_STATUS_SUCCESS.
 *     - UK_NETDEV_STATUS_UNDERRUN: Informs that some available slots of the
 *        receive queue could not be programmed with a receive buffer. The
 *        user-provided receive buffer allocator function returned with an error
 *        (e.g., out of memory).
 *   - (<0): Negative value with error code from driver, no packet is returned.
 */
static inline int uk_netdev_rx_one(struct uk_netdev *dev, uint16_t queue_id,
				   struct uk_netbuf **pkt)
{
	UK_ASSERT(dev);
	UK_ASSERT(dev->rx_one);
	UK_ASSERT(queue_id < CONFIG_LIBUKNETDEV_MAXNBQUEUES);
	UK_ASSERT(dev->_data->state == UK_NETDEV_RUNNING);
	UK_ASSERT(!PTRISERR(dev->_rx_queue[queue_id]));
	UK_ASSERT(pkt);

	return dev->rx_one(dev, dev->_rx_queue[queue_id], pkt);
}

/**
 * Transmit one packet
 *
 * @param dev
 *   The Unikraft Network Device.
 * @param queue_id
 *   The index of the receive queue to receive from.
 *   The value must be in the range [0, nb_tx_queue - 1] previously supplied
 *   to uk_netdev_configure().
 * @param pkt
 *   Reference to netbuf to sent. Packet is free'd by the driver after sending
 *   was successfully finished by the device.
 *   Please note that some drivers may require available headroom on the netbuf
 *   for doing a transmission - inspect `nb_encap` with uk_netdev_info_get().
 *   `pkt` has never to be `NULL`.
 * @return
 *   - (>=0): Positive value with status flags
 *     - UK_NETDEV_STATUS_SUCCESS: `pkt` was successfully put to the transmit
 *        queue. Whenever this flag is not set, there was no space left on the
 *        transmit queue to send `pkt`.
 *     - UK_NETDEV_STATUS_MORE: Indicates there is still at least one descriptor
 *         available for a subsequent transmission. If the flag is unset means
 *         that the transmit queue is full.
 *         This flag may only be set together with UK_NETDEV_STATUS_SUCCESS.
 *   - (<0): Negative value with error code from driver, no packet was sent.
 */
static inline int uk_netdev_tx_one(struct uk_netdev *dev, uint16_t queue_id,
				   struct uk_netbuf *pkt)
{
	UK_ASSERT(dev);
	UK_ASSERT(dev->tx_one);
	UK_ASSERT(queue_id < CONFIG_LIBUKNETDEV_MAXNBQUEUES);
	UK_ASSERT(dev->_data->state == UK_NETDEV_RUNNING);
	UK_ASSERT(!PTRISERR(dev->_tx_queue[queue_id]));
	UK_ASSERT(pkt);

	return dev->tx_one(dev, dev->_tx_queue[queue_id], pkt);
}

/**
 * Tests for status flags returned by `uk_netdev_rx_one` or `uk_netdev_tx_one`.
 * When the functions returned an error code or one of the selected flags is
 * unset, this macro returns False.
 *
 * @param status
 *   Return status (int)
 * @param flag
 *   Flag(s) to test
 * @return
 *   - (True):  All flags are set and status is not negative
 *   - (False): At least one flag is not set or status is negative
 */
#define uk_netdev_status_test_set(status, flag)			\
	(((int)(status) & ((int)(flag) | INT_MIN)) == (flag))

/**
 * Tests for unset status flags returned by `uk_netdev_rx_one` or
 * `uk_netdev_tx_one`. When the functions returned an error code or one of the
 * selected flags is set, this macro returns False.
 *
 * @param status
 *   Return status (int)
 * @param flag
 *   Flag(s) to test
 * @return
 *   - (True):  Flags are not set and status is not negative
 *   - (False): At least one flag is set or status is negative
 */
#define uk_netdev_status_test_unset(status, flag)			\
	(((int)(status) & ((int)(flag) | INT_MIN)) == (0x0))

/**
 * Tests if the return status of `uk_netdev_rx_one` or `uk_netdev_tx_one`
 * indicates a successful operation (e.g., packet sent or received).
 *
 * @param status
 *   Return status (int)
 * @return
 *   - (True):  Operation was successful
 *   - (False): Operation was unsuccessful or error happened
 */
#define uk_netdev_status_successful(status)			\
	uk_netdev_status_test_set((status), UK_NETDEV_STATUS_SUCCESS)

/**
 * Tests if the return status of `uk_netdev_rx_one` or `uk_netdev_tx_one`
 * indicates that the operation should be retried (e.g., packet sent or
 * received).
 *
 * @param status
 *   Return status (int)
 * @return
 *   - (True):  Operation should be retried
 *   - (False): Operation was successful or error happened
 */
#define uk_netdev_status_notready(status)				\
	uk_netdev_status_test_unset((status), UK_NETDEV_STATUS_SUCCESS)

/**
 * Tests if the return status of `uk_netdev_rx_one` or `uk_netdev_tx_one`
 * indicates that the last operation can be successfully repeatet again.
 *
 * @param status
 *   Return status (int)
 * @return
 *   - (True):  Flag UK_NETDEV_STATUS_MORE is set
 *   - (False): Operation was successful or error happened
 */
#define uk_netdev_status_more(status)					\
	uk_netdev_status_test_set((status), (UK_NETDEV_STATUS_SUCCESS	\
					     | UK_NETDEV_STATUS_MORE))

#ifdef __cplusplus
}
#endif

#endif /* __UK_NETDEV__ */
