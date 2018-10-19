/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Simon Kuenzer <simon.kuenzer@neclab.eu>
 *          Razvan Cojocaru <razvan.cojocaru93@gmail.com>
 *
 * Copyright (c) 2017-2018, NEC Europe Ltd., NEC Corporation.
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
#include <string.h>
#include <stdio.h>
#include <uk/netdev.h>
#include <uk/print.h>

struct uk_netdev_list uk_netdev_list =
	UK_TAILQ_HEAD_INITIALIZER(uk_netdev_list);
static uint16_t netdev_count;

static struct uk_netdev_data *_alloc_data(struct uk_alloc *a,
					  uint16_t netdev_id,
					  const char *drv_name)
{
	struct uk_netdev_data *data;

	data = uk_malloc(a, sizeof(*data));
	if (!data)
		return NULL;

	data->drv_name = drv_name;
	data->state    = UK_NETDEV_UNCONFIGURED;

	/* This is the only place where we set the device ID;
	 * during the rest of the device's life time this ID is read-only
	 */
	*(DECONST(uint16_t *, &data->id)) = netdev_id;

	return data;
}

int uk_netdev_drv_register(struct uk_netdev *dev, struct uk_alloc *a,
			   const char *drv_name)
{
	UK_ASSERT(dev);
	UK_ASSERT(!dev->_data);

	dev->_data = _alloc_data(a, netdev_count,  drv_name);
	if (!dev->_data)
		return -ENOMEM;

	UK_TAILQ_INSERT_TAIL(&uk_netdev_list, dev, _list);
	uk_pr_info("Registered netdev%"PRIu16": %p (%s)\n",
		   netdev_count, dev, drv_name);

	return netdev_count++;
}

unsigned int uk_netdev_count(void)
{
	return (unsigned int) netdev_count;
}

struct uk_netdev *uk_netdev_get(unsigned int id)
{
	struct uk_netdev *dev;

	UK_TAILQ_FOREACH(dev, &uk_netdev_list, _list) {
		UK_ASSERT(dev->_data);

		if (dev->_data->id == id)
			return dev;
	}
	return NULL;
}

uint16_t uk_netdev_id_get(struct uk_netdev *dev)
{
	UK_ASSERT(dev);
	UK_ASSERT(dev->_data);

	return dev->_data->id;
}

const char *uk_netdev_drv_name_get(struct uk_netdev *dev)
{
	UK_ASSERT(dev);
	UK_ASSERT(dev->_data);

	return dev->_data->drv_name;
}

enum uk_netdev_state uk_netdev_state_get(struct uk_netdev *dev)
{
	UK_ASSERT(dev);
	UK_ASSERT(dev->_data);

	return dev->_data->state;
}
