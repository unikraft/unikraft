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

#ifdef __cplusplus
}
#endif

#endif /* __UK_BLKDEV__ */
