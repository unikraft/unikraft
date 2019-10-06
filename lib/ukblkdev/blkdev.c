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
#include <string.h>
#include <stdio.h>
#include <inttypes.h>
#include <uk/alloc.h>
#include <uk/assert.h>
#include <uk/bitops.h>
#include <uk/print.h>
#include <uk/ctors.h>
#include <uk/arch/atomic.h>
#include <uk/blkdev.h>

struct uk_blkdev_list uk_blkdev_list =
UK_TAILQ_HEAD_INITIALIZER(uk_blkdev_list);

static uint16_t blkdev_count;

static struct uk_blkdev_data *_alloc_data(struct uk_alloc *a,
		uint16_t blkdev_id,
		const char *drv_name)
{
	struct uk_blkdev_data *data;

	data = uk_calloc(a, 1, sizeof(*data));
	if (!data)
		return NULL;

	data->drv_name = drv_name;
	data->state    = UK_BLKDEV_UNCONFIGURED;
	data->a = a;
	/* This is the only place where we set the device ID;
	 * during the rest of the device's life time this ID is read-only
	 */
	*(DECONST(uint16_t *, &data->id)) = blkdev_id;

	return data;
}

int uk_blkdev_drv_register(struct uk_blkdev *dev, struct uk_alloc *a,
		const char *drv_name)
{
	UK_ASSERT(dev);

	/* Data must be unallocated. */
	UK_ASSERT(PTRISERR(dev->_data));

	dev->_data = _alloc_data(a, blkdev_count,  drv_name);
	if (!dev->_data)
		return -ENOMEM;

	UK_TAILQ_INSERT_TAIL(&uk_blkdev_list, dev, _list);
	uk_pr_info("Registered blkdev%"PRIu16": %p (%s)\n",
			blkdev_count, dev, drv_name);
	dev->_data->state = UK_BLKDEV_UNCONFIGURED;

	return blkdev_count++;
}

unsigned int uk_blkdev_count(void)
{
	return (unsigned int) blkdev_count;
}

struct uk_blkdev *uk_blkdev_get(unsigned int id)
{
	struct uk_blkdev *blkdev;

	UK_TAILQ_FOREACH(blkdev, &uk_blkdev_list, _list) {
		UK_ASSERT(blkdev->_data);
		if (blkdev->_data->id == id)
			return blkdev;
	}

	return NULL;
}

uint16_t uk_blkdev_id_get(struct uk_blkdev *dev)
{
	UK_ASSERT(dev);
	UK_ASSERT(dev->_data);

	return dev->_data->id;
}

const char *uk_blkdev_drv_name_get(struct uk_blkdev *dev)
{
	UK_ASSERT(dev);
	UK_ASSERT(dev->_data);

	return dev->_data->drv_name;
}

enum uk_blkdev_state uk_blkdev_state_get(struct uk_blkdev *dev)
{
	UK_ASSERT(dev);
	UK_ASSERT(dev->_data);

	return dev->_data->state;
}
