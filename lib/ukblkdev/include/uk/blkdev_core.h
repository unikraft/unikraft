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
 * @internal
 * libukblkdev internal data associated with each block device.
 */
struct uk_blkdev_data {
	 /* Device id identifier */
	const uint16_t id;
	/* Device state */
	enum uk_blkdev_state state;
	/* Name of device*/
	const char *drv_name;
	/* Allocator */
	struct uk_alloc *a;
};

struct uk_blkdev {
	/* Pointer to API-internal state data. */
	struct uk_blkdev_data *_data;
	/* Entry for list of block devices */
	UK_TAILQ_ENTRY(struct uk_blkdev) _list;
};

#ifdef __cplusplus
}
#endif

#endif /* __UK_BLKDEV_CORE__ */
