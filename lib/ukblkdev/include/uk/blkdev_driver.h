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
#ifndef __UK_BLKDEV_DRIVER__
#define __UK_BLKDEV_DRIVER__

#include <uk/blkdev_core.h>
#include <uk/assert.h>

/**
 * Unikraft block driver API.
 *
 * This header contains all API functions that are supposed to be called
 * by a block device driver.
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Adds a Unikraft block device to the device list.
 * This should be called whenever a driver adds a new found device.
 *
 * @param dev
 *	Struct to unikraft block device that shall be registered
 * @param a
 *	Allocator to be use for libukblkdev private data (dev->_data)
 * @param drv_name
 *	(Optional) driver name
 *	The memory for this string has to stay available as long as the
 *	device is registered.
 * @return
 *	- (-ENOMEM): Allocation of private
 *	- (>=0): Block device ID on success
 */
int uk_blkdev_drv_register(struct uk_blkdev *dev, struct uk_alloc *a,
		const char *drv_name);

#ifdef __cplusplus
}
#endif

#endif /* __UK_BLKDEV_DRIVER__ */
