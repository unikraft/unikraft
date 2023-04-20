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

#ifndef _DEVICE_H
#define _DEVICE_H

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
	unsigned long		cookie;		/* index cookie */
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
 * flags for the driver.
 */

typedef enum device_state {
	DS_INACTIVE	    = 0x00,		/* driver is inactive */
	DS_ALIVE	    = 0x01,		/* probe succeded */
	DS_ACTIVE	    = 0x02,		/* intialized */
	DS_DEBUG	    = 0x04,		/* debug */
	DS_NOTPRESENT   = 0x08,     /* not probed or probe failed */
	DS_ATTACHING    = 0x10,     /* currently attaching */
	DS_ATTACHED     = 0x20,     /*attach method called */
} device_state_t;

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
	off_t		size;		/* device size */
	off_t		offset; /* 0 for the main drive, if we have a
				 *  partition, this is the start address
				 */
	size_t		max_io_size;
	void		*private_data;	/* private storage */

	void *softc;
	void *ivars;
	device_state_t state;
	const char *desc;
	int unit;
	int irq;
	int vector;
};

typedef struct device *device_t;

static inline int
device_set_unit(device_t dev, int unit)
{
	dev->unit = unit;
	return 0;
}

static inline int
device_get_unit(device_t dev)
{
	return dev->unit;
}

static inline const char *
device_get_desc(device_t dev)
{
	return dev->desc;
}

static inline void
device_set_desc(device_t dev, const char *desc)
{
	dev->desc = desc;
}

static inline void
device_set_softc(device_t dev, void *softc)
{
	dev->softc = softc;
}

static inline void *
device_get_softc(device_t dev)
{
	return dev->softc;
}

static inline void device_quiet(device_t dev __unused)
{
}

static inline const char *
devtoname(struct device *dev)
{
	return dev->name;
}

int device_open(const char *name, int mode, struct device **devp);
int device_close(struct device *dev);
int device_read(struct device *dev, struct uio *uio, int ioflags);
int device_write(struct device *dev, struct uio *uio, int ioflags);
int device_ioctl(struct device *dev, unsigned long cmd, void *arg);
int device_info(struct devinfo *info);

int bdev_read(struct device *dev, struct uio *uio, int ioflags);
int bdev_write(struct device *dev, struct uio *uio, int ioflags);

int devop_noop();
int devop_eperm();

struct device *device_create(struct driver *drv, const char *name, int flags);
int device_destroy(struct device *dev);
int device_destroy_locked(struct device *dev);
void device_register(struct device *device, const char *name, int flags);
void read_partition_table(struct device *device);

/*
 * Ideally, any dev node registration should happen before we mount devfs.
 * vfscore mounts '/' at priority 4, '/dev' is mounted at priority 5.
 * To be on the safe side, we do the registration to devfs before both,
 * at priority '3'.
 */
#define devfs_initcall(fn) uk_rootfs_initcall_prio(fn, 3)

#endif /* !_DEVICE_H */
