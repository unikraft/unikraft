/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Costin Lupu <costin.lupu@cs.pub.ro>
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

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <uk/essentials.h>
#include <uk/list.h>
#include <uk/bus.h>
#include <uk/print.h>
#include <uk/errptr.h>
#include <uk/assert.h>
#include <xenbus/xenbus.h>
#include <xenbus/xs.h>
#include <xenbus/client.h>
#include "xs_comms.h"

#define XS_DEV_PATH "device"

static struct xenbus_handler xbh;


/* Helper functions for Xenbus related allocations */
void *uk_xb_malloc(size_t size)
{
	UK_ASSERT(xbh.a != NULL);
	return uk_malloc(xbh.a, size);
}

void *uk_xb_calloc(size_t nmemb, size_t size)
{
	UK_ASSERT(xbh.a != NULL);
	return uk_calloc(xbh.a, nmemb, size);
}

void uk_xb_free(void *ptr)
{
	UK_ASSERT(xbh.a != NULL);
	uk_free(xbh.a, ptr);
}


static struct xenbus_driver *xenbus_find_driver(xenbus_dev_type_t devtype)
{
	struct xenbus_driver *drv;
	const xenbus_dev_type_t *pdevtype;

	UK_TAILQ_FOREACH(drv, &xbh.drv_list, next) {
		for (pdevtype = drv->device_types;
				*pdevtype != xenbus_dev_none; pdevtype++) {
			if (*pdevtype == devtype)
				return drv;
		}
	}

	return NULL; /* no driver found */
}

static int xenbus_probe_device(struct xenbus_driver *drv,
		xenbus_dev_type_t type, const char *name)
{
	int err;
	struct xenbus_device *dev;
	char *nodename = NULL;
	XenbusState state;

	/* device/type/name */
	err = asprintf(&nodename, "%s/%s/%s",
		XS_DEV_PATH, xenbus_devtype_to_str(type), name);
	if (err < 0)
		goto out;

	state = xenbus_read_driver_state(nodename);
	if (state != XenbusStateInitialising)
		return 0;

	uk_pr_info("Xenbus device: %s\n", nodename);

	dev = uk_xb_calloc(1, sizeof(*dev) + strlen(nodename) + 1);
	if (!dev) {
		uk_pr_err("Failed to initialize: Out of memory!\n");
		err = -ENOMEM;
		goto out;
	}

	dev->state = XenbusStateInitialising;
	dev->devtype = type;
	dev->nodename = (char *) (dev + 1);
	strcpy(dev->nodename, nodename);

	err = drv->add_dev(dev);
	if (err) {
		uk_pr_err("Failed to add device.\n");
		uk_xb_free(dev);
	}

out:
	if (nodename)
		free(nodename);

	return err;
}

static int xenbus_probe_device_type(const char *devtype_str)
{
	struct xenbus_driver *drv;
	xenbus_dev_type_t devtype;
	char dirname[sizeof(XS_DEV_PATH) + strlen(devtype_str)];
	char **devices = NULL;
	int err = 0;

	devtype = xenbus_str_to_devtype(devtype_str);
	if (!devtype) {
		uk_pr_warn("Unsupported device type: %s\n", devtype_str);
		goto out;
	}

	drv = xenbus_find_driver(devtype);
	if (!drv) {
		uk_pr_warn("No driver for device type: %s\n", devtype_str);
		goto out;
	}

	sprintf(dirname, "%s/%s", XS_DEV_PATH, devtype_str);

	/* Get device list */
	devices = xs_ls(XBT_NIL, dirname);
	if (PTRISERR(devices)) {
		err = PTR2ERR(devices);
		uk_pr_err("Error reading %s devices: %d\n", devtype_str, err);
		goto out;
	}

	for (int i = 0; devices[i] != NULL; i++) {
		/* Probe only if no prior error */
		if (err == 0)
			err = xenbus_probe_device(drv, devtype, devices[i]);
	}

out:
	if (!PTRISERR(devices))
		free(devices);

	return err;
}

static int xenbus_probe(void)
{
	char **devtypes;
	int err = 0;

	uk_pr_info("Probe Xenbus\n");

	/* Get device types list */
	devtypes = xs_ls(XBT_NIL, XS_DEV_PATH);
	if (PTRISERR(devtypes)) {
		err = PTR2ERR(devtypes);
		uk_pr_err("Error reading device types: %d\n", err);
		goto out;
	}

	for (int i = 0; devtypes[i] != NULL; i++) {
		/* Probe only if no previous error */
		if (err == 0)
			err = xenbus_probe_device_type(devtypes[i]);
	}

out:
	if (!PTRISERR(devtypes))
		free(devtypes);

	return err;
}

static int xenbus_init(struct uk_alloc *a)
{
	struct xenbus_driver *drv, *drv_next;
	int ret = 0;

	UK_ASSERT(a != NULL);

	xbh.a = a;

	ret = xs_comms_init();
	if (ret) {
		uk_pr_err("Error initializing Xenstore communication.");
		return ret;
	}

	UK_TAILQ_FOREACH_SAFE(drv, &xbh.drv_list, next, drv_next) {
		if (drv->init) {
			ret = drv->init(a);
			if (ret == 0)
				continue;
			uk_pr_err("Failed to initialize driver %p: %d\n",
				drv, ret);
			UK_TAILQ_REMOVE(&xbh.drv_list, drv, next);
		}
	}

	return 0;
}

void _xenbus_register_driver(struct xenbus_driver *drv)
{
	UK_ASSERT(drv != NULL);
	UK_TAILQ_INSERT_TAIL(&xbh.drv_list, drv, next);
}

/*
 * Register this bus driver to libukbus:
 */
static struct xenbus_handler xbh = {
	.b.init  = xenbus_init,
	.b.probe = xenbus_probe,
	.drv_list = UK_TAILQ_HEAD_INITIALIZER(xbh.drv_list),
	.dev_list = UK_TAILQ_HEAD_INITIALIZER(xbh.dev_list),
};

UK_BUS_REGISTER(&xbh.b);
