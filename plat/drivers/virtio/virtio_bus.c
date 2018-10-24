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

#include <uk/config.h>
#include <uk/arch/types.h>
#include <uk/list.h>
#include <uk/alloc.h>
#include <uk/bus.h>
#include <virtio/virtio_ids.h>
#include <virtio/virtio_config.h>
#include <virtio/virtio_bus.h>

UK_TAILQ_HEAD(virtio_driver_list, struct virtio_driver);
/**
 * Module local data structure(s).
 */
static struct virtio_driver_list virtio_drvs =
			UK_TAILQ_HEAD_INITIALIZER(virtio_drvs);
static struct uk_alloc *a;

/**
 *   Driver module local function(s).
 */
static int virtio_bus_init(struct uk_alloc *mem_alloc);
static int virtio_bus_probe(void);

/**
 * Probe for the virtio device.
 */
static int virtio_bus_probe(void)
{
	return 0;
}

/**
 * Initialize the virtio bus driver(s).
 * @param mem_alloc
 *	Reference to the mem_allocator.
 * @return
 *	(int) On successful initialization return the count of device
 *	initialized.
 *	On error return -1.
 */
static int virtio_bus_init(struct uk_alloc *mem_alloc)
{
	struct virtio_driver *drv = NULL, *ndrv = NULL;
	int ret = 0, dev_count = 0;

	a = mem_alloc;
	UK_TAILQ_FOREACH_SAFE(drv, &virtio_drvs, next, ndrv) {
		if (drv->init) {
			ret = drv->init(a);
			if (unlikely(ret)) {
				uk_pr_err("Failed to initialize virtio driver %p: %d\n",
					  drv, ret);
				UK_TAILQ_REMOVE(&virtio_drvs, drv, next);
			} else
				dev_count++;
		}
	}
	return (dev_count > 0) ? dev_count : -1;
}

/**
 * Register the virtio driver(s).
 * @param vdrv
 *	Reference to the virtio_driver
 */
void _virtio_bus_register_driver(struct virtio_driver *vdrv)
{
	UK_TAILQ_INSERT_TAIL(&virtio_drvs, vdrv, next);
}

static struct uk_bus virtio_bus = {
	.init = virtio_bus_init,
	.probe = virtio_bus_probe,
};
UK_BUS_REGISTER(&virtio_bus);
