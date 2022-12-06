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

#define MAXDEVNAME	12
#define DO_RWMASK	0x3

struct uio;
struct device;

/*
 * Device information
 */
struct devinfo {
	unsigned long	cookie;		/* index cookie */
	struct device	*id;		/* device id */
	int		flags;		/* device characteristics flags */
	char		name[MAXDEVNAME]; /* device name */
};

/*
 * Device flags
 */
#define D_CHR		0x00000001	/* character device */
#define D_BLK		0x00000002	/* block device */
#define D_REM		0x00000004	/* removable device */
#define D_TTY		0x00000010	/* tty device */

typedef int (*devop_open_t)(struct device *, int);
typedef int (*devop_close_t)(struct device *);
typedef int (*devop_read_t)(struct device *, struct uio *, int);
typedef int (*devop_write_t)(struct device *, struct uio *, int);
typedef int (*devop_ioctl_t)(struct device *, unsigned long, void *);

/*
 * Device operations
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
	const char	*name;		/* name of device driver */
	struct devops	*devops;	/* device operations */
	size_t		devsz;		/* size of private data */
	int		flags;		/* state of driver */
};

/*
 * Device object
 */
struct device {
	struct device	*next;		/* linkage on list of all devices */
	struct driver	*driver;	/* pointer to the driver object */
	char		name[MAXDEVNAME]; /* name of device */
	int		flags;		/* D_* flags defined above */
	int		active;		/* device has not been destroyed */
	int		refcnt;		/* reference count */
	void		*private_data;	/* private storage */
};

int device_open(const char *name, int mode, struct device **devp);
int device_close(struct device *dev);
int device_read(struct device *dev, struct uio *uio, int ioflags);
int device_write(struct device *dev, struct uio *uio, int ioflags);
int device_ioctl(struct device *dev, unsigned long cmd, void *arg);
int device_info(struct devinfo *info);

int devop_noop();
int devop_eperm();

int device_create(struct driver *drv, const char *name, int flags,
		  struct device **devp);
int device_destroy(struct device *dev);

/*
 * Ideally, any dev node registration should happen before we mount devfs.
 * vfscore mounts '/' at priority 4, '/dev' is mounted at priority 5.
 * To be on the safe side, we do the registration to devfs before both,
 * at priority '3'.
 */
#define devfs_initcall(fn) uk_rootfs_initcall_prio(fn, 0x0, 3)

#endif /* !__DEVFS_DEVICE_H__ */
