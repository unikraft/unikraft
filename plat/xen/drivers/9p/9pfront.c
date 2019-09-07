/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Cristian Banu <cristb@gmail.com>
 *
 * Copyright (c) 2019, University Politehnica of Bucharest. All rights reserved.
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
#include <uk/alloc.h>
#include <uk/assert.h>
#include <uk/essentials.h>
#include <uk/list.h>
#include <uk/plat/spinlock.h>
#include <xenbus/xenbus.h>

#include "9pfront_xb.h"

#define DRIVER_NAME	"xen-9pfront"

static struct uk_alloc *a;
static UK_LIST_HEAD(p9front_device_list);
static DEFINE_SPINLOCK(p9front_device_list_lock);

static int p9front_drv_init(struct uk_alloc *drv_allocator)
{
	if (!drv_allocator)
		return -EINVAL;

	a = drv_allocator;

	return 0;
}

static int p9front_add_dev(struct xenbus_device *xendev)
{
	struct p9front_dev *p9fdev;
	int rc;
	unsigned long flags;

	p9fdev = uk_calloc(a, 1, sizeof(*p9fdev));
	if (!p9fdev) {
		rc = -ENOMEM;
		goto out;
	}

	p9fdev->xendev = xendev;
	rc = p9front_xb_init(p9fdev);
	if (rc)
		goto out_free;

	rc = 0;
	ukplat_spin_lock_irqsave(&p9front_device_list_lock, flags);
	uk_list_add(&p9fdev->_list, &p9front_device_list);
	ukplat_spin_unlock_irqrestore(&p9front_device_list_lock, flags);

out_free:
	uk_free(a, p9fdev);
out:
	return rc;
}

static const xenbus_dev_type_t p9front_devtypes[] = {
	xenbus_dev_9pfs,
};

static struct xenbus_driver p9front_driver = {
	.device_types   = p9front_devtypes,
	.init           = p9front_drv_init,
	.add_dev        = p9front_add_dev
};

XENBUS_REGISTER_DRIVER(&p9front_driver);
