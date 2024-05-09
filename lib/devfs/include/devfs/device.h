/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2005-2008, Kohsuke Ohtani
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

#ifndef __DEVFS_DEVICE_H__
#define __DEVFS_DEVICE_H__

#include <sys/types.h>
#include <uk/init.h>
#include <uk/posix-fdtab.h>

#define MAXDEVNAME	12
#define DO_RWMASK	0x3

struct uio;
struct device;

/*
 * Device information
 */
struct devinfo {
	/* Index cookie */
	unsigned long cookie;
	/* Device id */
	struct device *id;
	/* Device characteristics flags */
	int flags;
	/* Device name */
	char name[MAXDEVNAME];
};

/*
 * Device flags
 */

/* Character device */
#define D_CHR		0x00000001
/* Block device */
#define D_BLK		0x00000002
/* Removable device */
#define D_REM		0x00000004
/* tty device */
#define D_TTY		0x00000010

typedef int (*devop_open_t)(struct device *, int);
typedef int (*devop_close_t)(struct device *);
typedef int (*devop_read_t)(struct device *, struct uio *, int);
typedef int (*devop_write_t)(struct device *, struct uio *, int);
typedef int (*devop_ioctl_t)(struct device *, unsigned long, void *);

/*
 * Device operations
 *
 * Holds pointers to the device-specific implementations
 * of the I/O operations listed below
 */
struct devops {
	devop_open_t	open;
	devop_close_t	close;
	devop_read_t	read;
	devop_write_t	write;
	devop_ioctl_t	ioctl;
};

#define	dev_noop_open	((devop_open_t)devop_noop)
#define	dev_noop_close	((devop_close_t)devop_noop)
#define dev_noop_ioctl	((devop_ioctl_t)devop_eperm)

/*
 * Driver object
 */
struct driver {
	/* Name of device driver */
	const char *name;
	/* Device operations */
	struct devops *devops;
	/* Size of private data */
	size_t devsz;
	/* State of driver */
	int flags;
};

/*
 * Device object
 */
struct device {
	/* Linkage on list of all devices */
	struct device *next;
	/* Pointer to the driver object */
	struct driver *driver;
	/* Name of device */
	char name[MAXDEVNAME];
	/* D_* flags defined above */
	int flags;
	/* Status of the device (1 if active, 0 if inactive) */
	int active;
	/* Reference count */
	int refcnt;
	/* Private storage */
	void *private_data;
};

/**
 * Opens the device specified by name.
 * This may be used to check whether a device exists or not.
 *
 * @param name
 *   The name of the device
 * @param mode
 *   The open mode (should be handled by the device driver if needed)
 * @param[out] devp
 *   The opened device will be reachable in devp after
 *   the function returns.
 * @return
 *   - (0):  Completed successfully
 *   - (<0): Negative value with error code
 */
int device_open(const char *name, int mode, struct device **devp);

/**
 * Closes the device specified by dev.
 * The function does not return an error if the target driver
 * does not have a close routine.
 *
 * @param dev
 *   The device to be closed
 * @return
 *   - (0):  Completed successfully
 *   - (<0): Negative value with error code
 */
int device_close(struct device *dev);

/**
 * Performs a scatter/gather read on the device pointed to by dev.
 *
 * @param dev
 *   The device to read from
 * @param uio
 *   The structure containing the scatter-gather list
 * @param ioflags
 *   The flags to be sent to the read operation
 * @return
 *  - (0):  Completed successfully
 *  - (<0): Negative value with error code
 */
int device_read(struct device *dev, struct uio *uio, int ioflags);

/**
 * Performs a scatter/gather write on the device pointed to by dev.
 *
 * @param dev
 *   The device to write to
 * @param uio
 *   The structure containing the scatter-gather list
 * @param ioflags
 *   The flags to be sent to the write operation
 * @return
 *  - (0):  Completed successfully
 *  - (<0): Negative value with error code
 */
int device_write(struct device *dev, struct uio *uio, int ioflags);

/**
 * Performs an I/O control request.
 *
 * @param dev
 *   The device to perform the request on
 * @param cmd
 *   The device dependent command to be executed
 * @param arg
 *   The user buffer sent as argument to the command. The buffer
 *   must be validated by the ioctl routine of the driver.
 * @return
 *   - (0):  Completed successfully
 *   - (<0): Negative value with error code
 */
int device_ioctl(struct device *dev, unsigned long cmd, void *arg);

/**
 * Returns information about a device.
 * The target of the query should be placed in the cookie field
 * of the parameter.
 *
 * @param[out] info
 *   The device must be specified in the cookie field and
 *   on return the id, flags and name fields will be filled
 *   with the requested information.
 * @return
 *   - (0): The information was successfully retrieved
 *   - (ESRCH): The operation failed
 */
int device_info(struct devinfo *info);

/**
 * Always returns 0.
 */
int devop_noop();

/**
 * Always returns EPERM.
 */
int devop_eperm();

/**
 * Creates a new device object.
 *
 * @param drv
 *   The driver to create the device
 * @param name
 *   The name of the new device
 * @param flags
 *   The flags used for the creation of the device (D_* flags: D_CHR, D_BLK,
 *   D_REM, D_TTY)
 * @param[out] devp
 *   The created device will be reachable in devp after
 *   the function returns.
 * @return
 *   A pointer to a device structure corresponding to the newly created
 *   device on success or NULL otherwise
 */
int device_create(struct driver *drv, const char *name, int flags,
		  struct device **devp);

/**
 * Destroys a device.
 *
 * @param dev
 *   The device to be destroyed
 * @return
 *   - (0): The device was destroyed successfully
 *   - (ENODEV): The operation failed
 */
int device_destroy(struct device *dev);

/**
 * Registers a Unikraft init function that is called during bootstrap.
 *
 * Ideally, any dev node registration should happen before we mount devfs.
 * vfscore mounts '/' at priority 4, '/dev' is mounted at priority 5.
 * To be on the safe side, we do the registration to devfs before both,
 * at priority '3'.
 *
 * @param fn
 *   The initilization function to be called
 *   Sample functions: `devfs_register()`, `devfs_register_null()`,
 *   `devfs_register_stdout()`.
 */
#define devfs_initcall(fn)						\
	uk_rootfs_initcall_prio(fn, 0x0, UK_PRIO_AFTER(UK_LIBPOSIX_FDTAB_INIT_PRIO))

#endif /* !__DEVFS_DEVICE_H__ */
