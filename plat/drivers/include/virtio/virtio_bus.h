/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Sharan Santhanam <sharan.santhanam@neclab.eu>
 *
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

#ifndef __PLAT_DRV_VIRTIO_H
#define __PLAT_DRV_VIRTIO_H

#include <uk/config.h>
#include <errno.h>
#include <uk/errptr.h>
#include <uk/arch/types.h>
#include <uk/arch/lcpu.h>
#include <uk/alloc.h>
#include <virtio/virtio_config.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define VIRTIO_FEATURES_UPDATE(features, bpos)	\
	(features |= (1ULL << bpos))

struct virtio_dev;
typedef int (*virtio_driver_init_func_t)(struct uk_alloc *);
typedef int (*virtio_driver_add_func_t)(struct virtio_dev *);

enum virtio_dev_status {
	/** Device reset */
	VIRTIO_DEV_RESET,
	/** Device Acknowledge and ready to be configured */
	VIRTIO_DEV_INITIALIZED,
	/** Device feature negotiated and ready to the started */
	VIRTIO_DEV_CONFIGURED,
	/** Device Running */
	VIRTIO_DEV_RUNNING,
	/** Device Stopped */
	VIRTIO_DEV_STOPPED,
};

/**
 * The structure define a virtio device.
 */
struct virtio_dev_id {
	/** Device identifier of the virtio device */
	__u16  virtio_device_id;
};

/**
 * The structure define the virtio driver.
 */
struct virtio_driver {
	/** Next entry of the driver list */
	UK_TAILQ_ENTRY(struct virtio_driver) next;
	/** The id map for the virtio device */
	const struct virtio_dev_id *dev_ids;
	/** The init function for the driver */
	virtio_driver_init_func_t init;
	/** Adding the virtio device */
	virtio_driver_add_func_t add_dev;
};

/**
 * The structure defines the virtio device.
 */
struct virtio_dev {
	/* Feature bit describing the virtio device */
	__u64 features;
	/* Private data of the driver */
	void *priv;
	/* Virtio device identifier */
	struct virtio_dev_id id;
	/* Reference to the virtio driver for the device */
	struct virtio_driver *vdrv;
	/* Status of the device */
	enum virtio_dev_status status;
};

/**
 * Operation exported by the virtio device.
 */
void _virtio_bus_register_driver(struct virtio_driver *vdrv);

#define VIRTIO_BUS_REGISTER_DRIVER(b)			\
	_VIRTIO_BUS_REGISTER_DRIVER(__LIBNAME__, b)

#define _VIRTIO_BUS_REGFNAME(x, y)       x##y

#define _VIRTIO_BUS_REGISTER_DRIVER(libname, b)				\
	    static void __constructor_prio(104)				\
	_VIRTIO_BUS_REGFNAME(libname, _virtio_register_driver)(void)	\
	{								\
		_virtio_bus_register_driver((b));			\
	}


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __PLAT_DRV_VIRTIO_H */
