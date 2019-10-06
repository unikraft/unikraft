/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Roxana Nicolescu <nicolescu.roxana1996@gmail.com>
 *
 * Copyright (c) 2019, University Politehnica of Bucharest.
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
#ifndef __UK_BLKDEV_CORE__
#define __UK_BLKDEV_CORE__

#include <uk/list.h>
#include <uk/config.h>
#if defined(CONFIG_LIBUKBLKDEV_DISPATCHERTHREADS)
#include <uk/sched.h>
#include <uk/semaphore.h>
#endif

/**
 * Unikraft block API common declarations.
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

struct uk_blkdev;

/**
 * List with devices
 */
UK_TAILQ_HEAD(uk_blkdev_list, struct uk_blkdev);

/**
 * Enum to describe the possible states of an block device.
 */
enum uk_blkdev_state {
	UK_BLKDEV_INVALID = 0,
	UK_BLKDEV_UNCONFIGURED,
	UK_BLKDEV_CONFIGURED,
	UK_BLKDEV_RUNNING,
};

/**
 * Structure used to configure an Unikraft block device.
 */
struct uk_blkdev_conf {
	uint16_t nb_queues;
};

/**
 * Structure used to  describe block device capabilities
 * before negotiation
 */
struct uk_blkdev_info {
	/* Max nb of supported queues by device. */
	uint16_t max_queues;
};

/**
 * Structure used to describe device descriptor ring limitations.
 */
struct uk_blkdev_queue_info {
	/* Max allowed number of descriptors. */
	uint16_t nb_max;
	/* Min allowed number of descriptors. */
	uint16_t nb_min;
	/* Number should be a multiple of nb_align. */
	uint16_t nb_align;
	/* Number should be a power of two. */
	int nb_is_power_of_two;
};

/**
 * Queue Structure used for both requests and responses.
 * This is private to the drivers.
 * In the API, this structure is used only for type checking.
 */
struct uk_blkdev_queue;

/**
 * Function type used for queue event callbacks.
 *
 * @param dev
 *   The Unikraft Block Device.
 * @param queue
 *   The queue on the Unikraft block device on which the event happened.
 * @param argp
 *   Extra argument that can be defined on callback registration.
 */
typedef void (*uk_blkdev_queue_event_t)(struct uk_blkdev *dev,
		uint16_t queue_id, void *argp);

/**
 * Structure used to configure an Unikraft block device queue.
 *
 */
struct uk_blkdev_queue_conf {
	/* Allocator used for descriptor rings */
	struct uk_alloc *a;
	/* Event callback function */
	uk_blkdev_queue_event_t callback;
	/* Argument pointer for callback*/
	void *callback_cookie;

#if CONFIG_LIBUKBLKDEV_DISPATCHERTHREADS
	/* Scheduler for dispatcher. */
	struct uk_sched *s;
#endif
};

/** Driver callback type to get initial device capabilities */
typedef void (*uk_blkdev_get_info_t)(struct uk_blkdev *dev,
		struct uk_blkdev_info *dev_info);

/** Driver callback type to configure a block device. */
typedef int (*uk_blkdev_configure_t)(struct uk_blkdev *dev,
		const struct uk_blkdev_conf *conf);

/* Driver callback type to get info about a device queue */
typedef int (*uk_blkdev_queue_get_info_t)(struct uk_blkdev *dev,
		uint16_t queue_id, struct uk_blkdev_queue_info *q_info);

/** Driver callback type to set up a queue of an Unikraft block device. */
typedef struct uk_blkdev_queue * (*uk_blkdev_queue_configure_t)(
		struct uk_blkdev *dev, uint16_t queue_id, uint16_t nb_desc,
		const struct uk_blkdev_queue_conf *queue_conf);

/** Driver callback type to start a configured Unikraft block device. */
typedef int (*uk_blkdev_start_t)(struct uk_blkdev *dev);

struct uk_blkdev_ops {
	uk_blkdev_get_info_t				get_info;
	uk_blkdev_configure_t				dev_configure;
	uk_blkdev_queue_get_info_t			queue_get_info;
	uk_blkdev_queue_configure_t			queue_setup;
	uk_blkdev_start_t				dev_start;
};

/**
 * @internal
 * Event handler configuration (internal to libukblkdev)
 */
struct uk_blkdev_event_handler {
	/* Callback */
	uk_blkdev_queue_event_t callback;
	/* Parameter for callback */
	void *cookie;

#if CONFIG_LIBUKBLKDEV_DISPATCHERTHREADS
	/* Semaphore to trigger events. */
	struct uk_semaphore events;
	/* Reference to blk device. */
	struct uk_blkdev    *dev;
	/* Queue id which caused event. */
	uint16_t            queue_id;
	/* Dispatcher thread. */
	struct uk_thread    *dispatcher;
	/* Reference to thread name. */
	char	*dispatcher_name;
	/* Scheduler for dispatcher. */
	struct uk_sched     *dispatcher_s;
#endif
};

/**
 * @internal
 * libukblkdev internal data associated with each block device.
 */
struct uk_blkdev_data {
	 /* Device id identifier */
	const uint16_t id;
	/* Device state */
	enum uk_blkdev_state state;
	/* Event handler for each queue */
	struct uk_blkdev_event_handler
		queue_handler[CONFIG_LIBUKBLKDEV_MAXNBQUEUES];
	/* Name of device*/
	const char *drv_name;
	/* Allocator */
	struct uk_alloc *a;
};

struct uk_blkdev {
	/* Pointer to API-internal state data. */
	struct uk_blkdev_data *_data;
	/* Functions callbacks by driver. */
	const struct uk_blkdev_ops *dev_ops;
	/* Pointers to queues (API-private) */
	struct uk_blkdev_queue *_queue[CONFIG_LIBUKBLKDEV_MAXNBQUEUES];
	/* Entry for list of block devices */
	UK_TAILQ_ENTRY(struct uk_blkdev) _list;
};

#ifdef __cplusplus
}
#endif

#endif /* __UK_BLKDEV_CORE__ */
