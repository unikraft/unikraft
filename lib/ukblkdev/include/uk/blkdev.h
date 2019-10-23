/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Roxana Nicolescu <nicolescu.roxana1996@gmail.com>
 *
 * Copyright (c) 2019, University Politehnica of Bucharest
 * All rights reserved.
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
/* This is derived from uknetdev because of consistency reasons */
#ifndef __UK_BLKDEV__
#define __UK_BLKDEV__

/**
 * Unikraft Block API
 *
 * The Unikraft BLK API provides a generalized interface between Unikraft
 * drivers and low-level application which needs communication with
 * a block device.
 *
 * Most BLK API functions take as parameter a reference to the corresponding
 * Unikraft Block Device (struct uk_blkdev) which can be obtained with a call
 * to uk_blkdev_get(). The block app should store this reference and
 * use it for all subsequent API calls.
 *
 * The functions exported by the Unikraft BLK API to setup a device
 * designated by its ID must be invoked in the following order:
 *      - uk_blkdev_configure()
 *      - uk_blkdev_queue_setup()
 *      - uk_blkdev_start()
 *
 * There are 4 states in which a block device can be found:
 *      - UK_BLKDEV_UNREGISTERED
 *      - UK_BLKDEV_UNCONFIGURED
 *      - UK_BLKDEV_CONFIGURED
 *      - UK_BLKDEV_RUNNING
 */

#include <sys/types.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <limits.h>
#include <uk/list.h>
#include <uk/errptr.h>

#include "blkdev_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Get the number of available Unikraft Block devices.
 *
 * @return
 *	- (unsigned int): number of block devices.
 */
unsigned int uk_blkdev_count(void);

/**
 * Get a reference to a Unikraft Block Device, based on its ID.
 * This reference should be saved by the application and used for subsequent
 * API calls.
 *
 * @param id
 *	The identifier of the Unikraft block device to configure.
 * @return
 *	- NULL: device not found in list
 *	- (struct uk_blkdev *): reference to be passed to API calls
 */
struct uk_blkdev *uk_blkdev_get(unsigned int id);

/**
 * Returns the id of a block device
 *
 * @param dev
 *	The Unikraft Block Device.
 * @return
 *	- (>=0): Device ID
 */
uint16_t uk_blkdev_id_get(struct uk_blkdev *dev);

/**
 * Returns the driver name of a blkdev device.
 * The name might be set to NULL.
 *
 * @param dev
 *	The Unikraft Block Device.
 * @return
 *	- (NULL): if no name is defined.
 *	- (const char *): Reference to string if name is available.
 */
const char *uk_blkdev_drv_name_get(struct uk_blkdev *dev);

/**
 * Returns the current state of a blkdev device.
 *
 * @param dev
 *	The Unikraft Block Device.
 * @return
 *	- (enum uk_blkdev_state): current device state
 */
enum uk_blkdev_state uk_blkdev_state_get(struct uk_blkdev *dev);

/**
 * Query device capabilities.
 * Information that is useful for device initialization (e.g.,
 * maximum number of supported queues).
 *
 * @param dev
 *	The Unikraft Block Device.
 * @param dev_info
 *	A pointer to a structure of type *uk_blkdev_info* to be filled with
 *	the contextual information of a block device.
 * @return
 *	- 0: Success
 *	- <0: Error in driver
 */
int uk_blkdev_get_info(struct uk_blkdev *dev,
		struct uk_blkdev_info *dev_info);

/**
 * Configure an Unikraft block device.
 * This function must be invoked first before any other function in the
 * Unikraft BLK API. This function can also be re-invoked when a device is
 * in the stopped state.
 *
 * @param dev
 *	The Unikraft Block Device.

 * @return
 *	- 0: Success, device configured.
 *	- <0: Error code returned by the driver configuration function.
 */
int uk_blkdev_configure(struct uk_blkdev *dev,
		const struct uk_blkdev_conf *conf);

/**
 * Query device queue capabilities.
 * Information that is useful for device queue initialization (e.g.,
 * maximum number of supported descriptors on queues).
 *
 * @param dev
 *   The Unikraft Block Device in configured state.
 * @param queue_id
 *   The index of the queue to set up.
 *   The value must be in the range [0, nb_queue - 1] previously supplied
 *   to uk_blkdev_configure().
 * @param queue_info
 *   A pointer to a structure of type *uk_blkdev_queue_info* to be filled out.
 * @return
 *   - (0): Success, queue_info is filled out.
 *   - (<0): Error code of the drivers function.
 */
int uk_blkdev_queue_get_info(struct uk_blkdev *dev, uint16_t queue_id,
		struct uk_blkdev_queue_info *q_info);

/**
 * Allocate and set up a queue for an Unikraft block device.
 * The queue is responsible for both requests and responses.
 *
 * @param dev
 *	The Unikraft Block Device.
 * @param queue_id
 *	The index of the queue to set up.
 *	The value must be in range [0, nb_queue -1] previously supplied
 *	to uk_blkdev_configure()
 * @param nb_desc
 *	Number of descriptors for the queue. Inspect uk_blkdev_queue_get_info()
 *	to retrieve limitations.
 * @param queue_conf
 *	The pointer to the configuration data to be used for the queue.
 *	This can be shared across multiple queue setups.
 * @return
 *	- 0: Success, receive queue correctly set up.
 *	- <0: Unable to allocate and set up the ring descriptors.
 */
int uk_blkdev_queue_configure(struct uk_blkdev *dev, uint16_t queue_id,
		uint16_t nb_desc,
		const struct uk_blkdev_queue_conf *queue_conf);

/**
 * Start a Block device.
 *
 * The device start step is the last one and consists of setting the configured
 * offload features and in starting the transmit and the receive units of the
 * device.
 * On success, all basic functions exported by the Unikraft BLK API
 * can be invoked.
 *
 * @param dev
 *	The Unikraft Block Device.
 * @return
 *	- 0: Success, Unikraft block device started.
 *	- <0: Error code of the driver device start function.
 */
int uk_blkdev_start(struct uk_blkdev *dev);

/**
 * Get the capabilities info which stores info about the device,
 * like nb_of_sectors, sector_size etc
 * The device state has to be UK_BLKDEV_RUNNING.
 *
 * @param dev
 *	The Unikraft Block Device.
 *
 * @return
 *	A pointer to a structure of type *uk_blkdev_capabilities*.
 **/
static inline const struct uk_blkdev_cap *uk_blkdev_capabilities(
		struct uk_blkdev *blkdev)
{
	UK_ASSERT(blkdev);
	UK_ASSERT(blkdev->_data->state >= UK_BLKDEV_RUNNING);

	return &blkdev->capabilities;
}

#define uk_blkdev_ssize(blkdev) \
	(uk_blkdev_capabilities(blkdev)->ssize)

#define uk_blkdev_max_sec_per_req(blkdev) \
	(uk_blkdev_capabilities(blkdev)->max_sectors_per_req)

#define uk_blkdev_mode(blkdev) \
	(uk_blkdev_capabilities(blkdev)->mode)

#define uk_blkdev_sectors(blkdev) \
	(uk_blkdev_capabilities(blkdev)->sectors)

#define uk_blkdev_ioalign(blkdev) \
	(uk_blkdev_capabilities(blkdev)->ioalign)
/**
 * Enable interrupts for a queue.
 *
 * @param dev
 *	The Unikraft Block Device in running state.
 * @param queue_id
 *	The index of the queue to set up.
 *	The value must be in the range [0, nb_queue - 1] previously supplied
 *	to uk_blkdev_configure().
 * @return
 *	- (0): Success, interrupts enabled.
 *	- (-ENOTSUP): Driver does not support interrupts.
 */
static inline int uk_blkdev_queue_intr_enable(struct uk_blkdev *dev,
		uint16_t queue_id)
{
	UK_ASSERT(dev);
	UK_ASSERT(dev->dev_ops);
	UK_ASSERT(dev->_data);
	UK_ASSERT(queue_id < CONFIG_LIBUKBLKDEV_MAXNBQUEUES);
	UK_ASSERT(!PTRISERR(dev->_queue[queue_id]));

	if (unlikely(!dev->dev_ops->queue_intr_enable))
		return -ENOTSUP;

	return dev->dev_ops->queue_intr_enable(dev, dev->_queue[queue_id]);
}

/**
 * Disable interrupts for a queue.
 *
 * @param dev
 *	The Unikraft Block Device in running state.
 * @param queue_id
 *	The index of the queue to set up.
 *	The value must be in the range [0, nb_queue - 1] previously supplied
 *	to uk_blkdev_configure().
 * @return
 *	- (0): Success, interrupts disabled.
 *	- (-ENOTSUP): Driver does not support interrupts.
 */
static inline int uk_blkdev_queue_intr_disable(struct uk_blkdev *dev,
		uint16_t queue_id)
{
	UK_ASSERT(dev);
	UK_ASSERT(dev->dev_ops);
	UK_ASSERT(dev->_data);
	UK_ASSERT(queue_id < CONFIG_LIBUKBLKDEV_MAXNBQUEUES);
	UK_ASSERT(!PTRISERR(dev->_queue[queue_id]));

	if (unlikely(!dev->dev_ops->queue_intr_disable))
		return -ENOTSUP;

	return dev->dev_ops->queue_intr_disable(dev, dev->_queue[queue_id]);
}

/**
 * Make an aio request to the device
 *
 * @param dev
 *	The Unikraft Block Device
 * @param queue_id
 *	The index of the receive queue to receive from.
 *	The value must be in the range [0, nb_queue - 1] previously supplied
 *	to uk_blkdev_configure().
 * @param req
 *	Request structure
 * @return
 *	- (>=0): Positive value with status flags
 *		- UK_BLKDEV_STATUS_SUCCESS: `req` was successfully put to the
 *		queue.
 *		- UK_BLKDEV_STATUS_MORE: Indicates there is still at least
 *		one descriptor available for a subsequent transmission.
 *		If the flag is unset means that the queue is full.
 *		This may only be set together with UK_BLKDEV_STATUS_SUCCESS.
 *	- (<0): Negative value with error code from driver, no request was sent.
 */
int uk_blkdev_queue_submit_one(struct uk_blkdev *dev, uint16_t queue_id,
		struct uk_blkreq *req);

/**
 * Tests for status flags returned by `uk_blkdev_submit_one`
 * When the function returned an error code or one of the selected flags is
 * unset, this macro returns False.
 *
 * @param status
 *	Return status (int)
 * @param flag
 *	Flag(s) to test
 * @return
 *	- (True):  All flags are set and status is not negative
 *	- (False): At least one flag is not set or status is negative
 */
#define uk_blkdev_status_test_set(status, flag)			\
	(((int)(status) & ((int)(flag) | INT_MIN)) == (flag))

/**
 * Tests for unset status flags returned by `uk_blkdev_submit_one`
 * When the function returned an error code or one of the
 * selected flags is set, this macro returns False.
 *
 * @param status
 *	Return status (int)
 * @param flag
 *	Flag(s) to test
 * @return
 *	- (True):  Flags are not set and status is not negative
 *	- (False): At least one flag is set or status is negative
 */
#define uk_blkdev_status_test_unset(status, flag)			\
	(((int)(status) & ((int)(flag) | INT_MIN)) == (0x0))

/**
 * Tests if the return status of `uk_blkdev_submit_one`
 * indicates a successful operation.
 *
 * @param status
 *	Return status (int)
 * @return
 *	- (True):  Operation was successful
 *	- (False): Operation was unsuccessful or error happened
 */
#define uk_blkdev_status_successful(status)			\
	uk_blkdev_status_test_set((status), UK_BLKDEV_STATUS_SUCCESS)

/**
 * Tests if the return status of `uk_blkdev_submit_one`
 * indicates that the operation should be retried/
 *
 * @param status
 *	Return status (int)
 * @return
 *	- (True):  Operation should be retried
 *	- (False): Operation was successful or error happened
 */
#define uk_blkdev_status_notready(status)				\
	uk_blkdev_status_test_unset((status), UK_BLKDEV_STATUS_SUCCESS)

/**
 * Tests if the return status of `uk_blkdev_submit_one`
 * indicates that the last operation can be successfully repeated again.
 *
 * @param status
 *	Return status (int)
 * @return
 *	- (True):  Flag UK_BLKDEV_STATUS_MORE is set
 *	- (False): Operation was successful or error happened
 */
#define uk_blkdev_status_more(status)					\
	uk_blkdev_status_test_set((status), (UK_BLKDEV_STATUS_SUCCESS	\
					     | UK_BLKDEV_STATUS_MORE))

/**
 * Get responses from the queue
 *
 * @param dev
 *	The Unikraft Block Device
 * @param queue_id
 *	queue id
 * @return
 *	- 0: Success
 *	- (<0): on error returned by driver
 */
int uk_blkdev_queue_finish_reqs(struct uk_blkdev *dev, uint16_t queue_id);

#if CONFIG_LIBUKBLKDEV_SYNC_IO_BLOCKED_WAITING
/**
 * Make a sync io request on a specific queue.
 * `uk_blkdev_queue_finish_reqs()` must be called in queue interrupt context
 * or another thread context in order to avoid blocking of the thread forever.
 *
 * @param dev
 *	The Unikraft Block Device
 * @param queue_id
 *	queue_id
 * @param op
 *	Type of operation
 * @param sector
 *	Start Sector
 * @param nb_sectors
 *	Number of sectors
 * @param buf
 *	Buffer where data is found
 * @return
 *	- 0: Success
 *	- (<0): on error returned by driver
 */
int uk_blkdev_sync_io(struct uk_blkdev *dev,
		uint16_t queue_id,
		enum uk_blkreq_op op,
		__sector sector,
		__sector nb_sectors,
		void *buf);

/*
 * Wrappers for uk_blkdev_sync_io
 */
#define uk_blkdev_sync_write(blkdev,\
		queue_id,	\
		sector,		\
		nb_sectors,	\
		buf)		\
	uk_blkdev_sync_io(blkdev, queue_id, UK_BLKDEV_WRITE, sector, \
			nb_sectors, buf) \

#define uk_blkdev_sync_read(blkdev,\
		queue_id,	\
		sector,		\
		nb_sectors,	\
		buf)		\
	uk_blkdev_sync_io(blkdev, queue_id, UK_BLKDEV_READ, sector, \
			nb_sectors, buf) \

#endif

/**
 * Stop a Unikraft block device, and set its state to UK_BLKDEV_CONFIGURED
 * state. From now on, users cannot send any requests.
 * If there are pending requests, this function will return -EBUSY because
 * the queues are not empty. If polling is used instead of interrupts,
 * make sure to clean the queue and process all the responses before this
 * operation is called.
 * The device can be restarted with a call to uk_blkdev_start().
 *
 * @param dev
 *	The Unikraft Block Device.
 * @return
 *	- 0: Success
 *	- (<0): on error returned by driver
 */
int uk_blkdev_stop(struct uk_blkdev *dev);

/**
 * Free a queue and its descriptors for an Unikraft block device.
 * @param dev
 *	The Unikraft Block Device.
 * @param queue_id
 *	The index of the queue to release.
 *	The value must be in range [0, nb_queue -1] previously supplied
 *	to uk_blkdev_configure()
 *	@return
 *	- 0: Success
 *	- (<0): on error returned by driver
 */
int uk_blkdev_queue_release(struct uk_blkdev *dev, uint16_t queue_id);

/**
 * Close a stopped Unikraft block device.
 * The function frees all resources except for
 * the ones needed by the UK_BLKDEV_UNCONFIGURED state.
 * The device can be reconfigured with a call to uk_blkdev_configure().
 * @param dev
 *	The Unikraft Block Device.
 * @return
 *	- 0: Success
 *	- (<0): on error returned by driver
 */
int uk_blkdev_unconfigure(struct uk_blkdev *dev);

#ifdef __cplusplus
}
#endif

#endif /* __UK_BLKDEV__ */
