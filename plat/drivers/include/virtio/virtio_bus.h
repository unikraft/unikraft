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
 * The structure define a list of configuration operation on a virtio device.
 */
struct virtio_config_ops {
	/** Resetting the device */
	void (*device_reset)(struct virtio_dev *vdev);
	/** Set configuration option */
	int (*config_set)(struct virtio_dev *vdev, __u16 offset,
			  const void *buf, __u32 len);
	/** Get configuration option */
	int (*config_get)(struct virtio_dev *vdev, __u16 offset, void *buf,
			  __u32 len, __u8 type_len);
	/** Get the feature */
	__u64 (*features_get)(struct virtio_dev *vdev);
	/** Set the feature */
	void (*features_set)(struct virtio_dev *vdev, __u64 features);
	/** Get and Set Status */
	__u8 (*status_get)(struct virtio_dev *vdev);
	void (*status_set)(struct virtio_dev *vdev, __u8 status);
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
	/* List of the config operations */
	struct virtio_config_ops *cops;
	/* Reference to the virtio driver for the device */
	struct virtio_driver *vdrv;
	/* Status of the device */
	enum virtio_dev_status status;
};

/**
 * Operation exported by the virtio device.
 */
int virtio_bus_register_device(struct virtio_dev *vdev);
void _virtio_bus_register_driver(struct virtio_driver *vdrv);

/**
 * The function updates the status of the virtio device
 * @param vdev
 *      Reference to the virtio device.
 * @param status
 *      The status to update.
 * @return
 *      0 on successful updating the status.
 *      -ENOTSUP, if the operation is not supported on the virtio device.
 */
static inline int virtio_dev_status_update(struct virtio_dev *vdev, __u8 status)
{
	int rc = -ENOTSUP;

	UK_ASSERT(vdev);

	if (likely(vdev->cops->status_set)) {
		vdev->cops->status_set(vdev, status);
		rc = 0;
	}
	return rc;
}

/**
 * The function to get the feature supported by the device.
 * @param vdev
 *	Reference to the virtio device.
 *
 * @return __u64
 *	A bit map of the feature supported by the device.
 */
static inline __u64 virtio_feature_get(struct virtio_dev *vdev)
{
	__u64 features = 0;

	UK_ASSERT(vdev);

	if (likely(vdev->cops->features_get))
		features = vdev->cops->features_get(vdev);
	return features;
}

/**
 * The function to set the negotiated features.
 * @param vdev
 *	Reference to the virtio device.
 * @param feature
 *	A bit map of the feature negotiated.
 */
static inline void virtio_feature_set(struct virtio_dev *vdev, __u32 feature)
{
	UK_ASSERT(vdev);

	if (likely(vdev->cops->features_set))
		vdev->cops->features_set(vdev, feature);
}

/**
 * Get the configuration information from the virtio device.
 * @param vdev
 *	Reference to the virtio device.
 * @param offset
 *	Offset into the virtio device configuration space.
 * @param buf
 *	A buffer to store the configuration information.
 * @param len
 *	The length of the buffer.
 * @param type_len
 *	The data type of the configuration data.
 * @return int
 *	0, on successful reading the configuration space.
 *	< 0, on error.
 */
static inline int virtio_config_get(struct virtio_dev *vdev, __u16 offset,
				    void *buf, __u32 len, __u8 type_len)
{
	int rc = -ENOTSUP;

	UK_ASSERT(vdev);

	if (likely(vdev->cops->config_get))
		rc = vdev->cops->config_get(vdev, offset, buf, len, type_len);

	return rc;
}

static inline int virtio_has_features(__u64 features, __u8 bpos)
{
	__u64 tmp_feature = 0;

	VIRTIO_FEATURES_UPDATE(tmp_feature, bpos);
	tmp_feature &= features;

	return tmp_feature ? 1 : 0;
}

static inline void virtio_dev_drv_up(struct virtio_dev *vdev)
{
	virtio_dev_status_update(vdev, VIRTIO_CONFIG_STATUS_DRIVER_OK);
}

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
