/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2005-2009, Kohsuke Ohtani
 * Copyright (C) 2014 Cloudius Systems, Ltd.
 * Copyright (c) 2019, University Politehnica of Bucharest.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * device.c - device I/O support routines
 */

/**
 * The device_* system calls are interfaces to access the specific
 * device object which is handled by the related device driver.
 *
 * The routines in this moduile have the following role:
 *  - Manage the name space for device objects.
 *  - Forward user I/O requests to the drivers with minimum check.
 *  - Provide the table for the Driver-Kernel Interface.
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <vfscore/prex.h>
#include <vfscore/uio.h>
#include <uk/essentials.h>
#include <uk/mutex.h>
#include <uk/assert.h>

#include <devfs/device.h>

static struct uk_mutex devfs_lock = UK_MUTEX_INITIALIZER(devfs_lock);

/* List head of the devices */
static struct device *device_list;

/*
 * Look up a device object by device name.
 */
static struct device *
device_lookup(const char *name)
{
	struct device *dev;

	for (dev = device_list; dev != NULL; dev = dev->next) {
		if (!strncmp(dev->name, name, MAXDEVNAME))
			return dev;
	}
	return NULL;
}

/*
 * device_create - create new device object.
 *
 * A device object is created by the device driver to provide
 * I/O services to applications. Returns 0 on success, an errno
 * number otherwise.
 */
int
device_create(struct driver *drv, const char *name, int flags,
	      struct device **devp)
{
	struct device *dev;
	size_t len;

	UK_ASSERT(drv);

	UK_ASSERT(drv->devops->open);
	UK_ASSERT(drv->devops->close);
	UK_ASSERT(drv->devops->read);
	UK_ASSERT(drv->devops->write);
	UK_ASSERT(drv->devops->ioctl);

	/* Check the length of the name */
	len = strnlen(name, MAXDEVNAME);
	if (unlikely(len == 0 || len >= MAXDEVNAME))
		return EINVAL;

	/* Allocate a device and device memory if needed */
	dev = malloc(sizeof(*dev) + drv->devsz);
	if (unlikely(!dev))
		return ENOMEM;

	/* Allocate device private data if needed */
	if (drv->devsz != 0) {
		dev->private_data = (void *)(dev + 1);
		memset(dev->private_data, 0, drv->devsz);
	} else {
		dev->private_data = NULL;
	}

	memcpy(dev->name, name, len + 1);
	dev->driver = drv;
	dev->flags = flags;
	dev->active = 1;
	dev->refcnt = 1;

	/* Insert device into device list. This must be done while holding
	 * the devfs lock. We optimize for success and reduce lock holder time
	 * by doing the check for name collision only here, instead of keeping
	 * the lock held the whole time.
	 */
	uk_mutex_lock(&devfs_lock);

	if (unlikely(device_lookup(name) != NULL)) {
		uk_mutex_unlock(&devfs_lock);
		free(dev);

		return EEXIST;
	}

	dev->next   = device_list;
	device_list = dev;

	uk_mutex_unlock(&devfs_lock);

	if (devp)
		*devp = dev;
	return 0;
}

/*
 * Return true if specified device is valid.
 */
static int
device_valid(struct device *dev)
{
	struct device *tmp;
	int found = 0;

	for (tmp = device_list; tmp != NULL; tmp = tmp->next) {
		if (tmp == dev) {
			found = 1;
			break;
		}
	}
	if (found && dev->active)
		return 1;
	return 0;
}

/*
 * Increment the reference count on an active device.
 */
static int
device_reference_locked(struct device *dev)
{
	UK_ASSERT(uk_mutex_is_locked(&devfs_lock));

	if (!device_valid(dev)) {
		uk_mutex_unlock(&devfs_lock);
		return ENODEV;
	}
	dev->refcnt++;
	return 0;
}

/*
 * Increment the reference count on an active device.
 */
static int
device_reference(struct device *dev)
{
	int ret;

	uk_mutex_lock(&devfs_lock);
	ret = device_reference_locked(dev);
	if (!ret) {
		uk_mutex_unlock(&devfs_lock);
		return ret;
	}
	uk_mutex_unlock(&devfs_lock);
	return 0;
}

/*
 * Decrement the reference count on a device. If the reference
 * count becomes zero, we can release the resource for the device.
 */
static void
device_release(struct device *dev)
{
	struct device **tmp;

	uk_mutex_lock(&devfs_lock);

	if (--dev->refcnt > 0) {
		uk_mutex_unlock(&devfs_lock);
		return;
	}
	/*
	 * No more references - we can remove the device.
	 */
	for (tmp = &device_list; *tmp; tmp = &(*tmp)->next) {
		if (*tmp == dev) {
			*tmp = dev->next;
			break;
		}
	}
	free(dev);
	uk_mutex_unlock(&devfs_lock);
}

int
device_destroy_locked(struct device *dev)
{

	UK_ASSERT(uk_mutex_is_locked(&devfs_lock));
	if (!device_valid(dev)) {
		uk_mutex_unlock(&devfs_lock);
		return ENODEV;
	}
	dev->active = 0;
	device_release(dev);
	return 0;
}

int
device_destroy(struct device *dev)
{

	uk_mutex_lock(&devfs_lock);
	if (!device_valid(dev)) {
		uk_mutex_unlock(&devfs_lock);
		return ENODEV;
	}
	dev->active = 0;
	uk_mutex_unlock(&devfs_lock);
	device_release(dev);
	return 0;
}

/*
 * device_open - open the specified device.
 *
 * Even if the target driver does not have an open
 * routine, this function does not return an error. By
 * using this mechanism, an application can check whether
 * the specific device exists or not. The open mode
 * should be handled by an each device driver if it is
 * needed.
 */
int
device_open(const char *name, int mode, struct device **devp)
{
	struct devops *ops;
	struct device *dev;
	int error;

	uk_mutex_lock(&devfs_lock);
	if ((dev = device_lookup(name)) == NULL) {
		uk_mutex_unlock(&devfs_lock);
		return ENXIO;
	}

	error = device_reference_locked(dev);
	if (error) {
		uk_mutex_unlock(&devfs_lock);
		return error;
	}
	uk_mutex_unlock(&devfs_lock);

	ops = dev->driver->devops;
	UK_ASSERT(ops->open != NULL);
	error = (*ops->open)(dev, mode);
	*devp = dev;

	device_release(dev);
	return error;
}

/*
 * device_close - close a device.
 *
 * Even if the target driver does not have close routine,
 * this function does not return any errors.
 */
int
device_close(struct device *dev)
{
	struct devops *ops;
	int error;

	if ((error = device_reference(dev)) != 0)
		return error;

	ops = dev->driver->devops;
	UK_ASSERT(ops->close != NULL);
	error = (*ops->close)(dev);

	device_release(dev);
	return error;
}

int
device_read(struct device *dev, struct uio *uio, int ioflags)
{
	struct devops *ops;
	int error;

	if ((error = device_reference(dev)) != 0)
		return error;

	ops = dev->driver->devops;
	UK_ASSERT(ops->read != NULL);
	error = (*ops->read)(dev, uio, ioflags);

	device_release(dev);
	return error;
}

int
device_write(struct device *dev, struct uio *uio, int ioflags)
{
	struct devops *ops;
	int error;

	if ((error = device_reference(dev)) != 0)
		return error;

	ops = dev->driver->devops;
	UK_ASSERT(ops->write != NULL);
	error = (*ops->write)(dev, uio, ioflags);

	device_release(dev);
	return error;
}

/*
 * device_ioctl - I/O control request.
 *
 * A command and its argument are completely device dependent.
 * The ioctl routine of each driver must validate the user buffer
 * pointed by the arg value.
 */
int
device_ioctl(struct device *dev, unsigned long cmd, void *arg)
{
	struct devops *ops;
	int error;

	if ((error = device_reference(dev)) != 0)
		return error;

	ops = dev->driver->devops;
	UK_ASSERT(ops->ioctl != NULL);
	error = (*ops->ioctl)(dev, cmd, arg);

	device_release(dev);
	return error;
}

/*
 * Return device information.
 */
int
device_info(struct devinfo *info)
{
	unsigned long target = info->cookie;
	unsigned long i = 0;
	struct device *dev;
	int error = ESRCH;

	uk_mutex_lock(&devfs_lock);
	for (dev = device_list; dev != NULL; dev = dev->next) {
		if (i++ == target) {
			info->cookie = i;
			info->id = dev;
			info->flags = dev->flags;
			strlcpy(info->name, dev->name, MAXDEVNAME);
			error = 0;
			break;
		}
	}
	uk_mutex_unlock(&devfs_lock);
	return error;
}

int
devop_noop()
{
	return 0;
}

int
devop_eperm()
{
	return EPERM;
}
