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

#ifndef __XENBUS_H__
#define __XENBUS_H__

#include <uk/arch/spinlock.h>
#include <uk/bus.h>
#include <uk/alloc.h>
#include <uk/wait.h>
#include <xen/xen.h>
#include <xen/io/xenbus.h>
#include <uk/ctors.h>


/*
 * Supported device types
 */
typedef enum xenbus_dev_type {
	xenbus_dev_none = 0,
	xenbus_dev_vbd,
	xenbus_dev_9pfs,
} xenbus_dev_type_t;

struct xenbus_device;

/*
 * Xenbus driver
 */

typedef int (*xenbus_driver_init_func_t)(struct uk_alloc *a);
typedef int (*xenbus_driver_add_func_t)(struct xenbus_device *dev);


struct xenbus_driver {
	UK_TAILQ_ENTRY(struct xenbus_driver) next;
	const xenbus_dev_type_t *device_types;

	xenbus_driver_init_func_t init;
	xenbus_driver_add_func_t add_dev;
};
UK_TAILQ_HEAD(xenbus_driver_list, struct xenbus_driver);


#define XENBUS_REGISTER_DRIVER(b) \
	_XENBUS_REGISTER_DRIVER(__LIBNAME__, (b))

#define _XENBUS_REGFNNAME(x, y)      x##y

#define _XENBUS_REGISTER_CTOR(ctor)  \
	UK_CTOR_PRIO(ctor, UK_PRIO_AFTER(UK_BUS_REGISTER_PRIO))

#define _XENBUS_REGISTER_DRIVER(libname, b)				\
	static void							\
	_XENBUS_REGFNNAME(libname, _xenbus_register_driver)(void)	\
	{								\
		_xenbus_register_driver((b));				\
	}								\
	_XENBUS_REGISTER_CTOR(						\
		_XENBUS_REGFNNAME(libname, _xenbus_register_driver))

/* Do not use this function directly: */
void _xenbus_register_driver(struct xenbus_driver *drv);

typedef unsigned long xenbus_transaction_t;
#define XBT_NIL ((xenbus_transaction_t) 0)

/*
 * Xenbus watch
 */

struct xenbus_watch {
	/**< in use internally */
	UK_TAILQ_ENTRY(struct xenbus_watch) watch_list;
	/**< Lock */
	spinlock_t lock;
	/**< Number of pending events */
	int pending_events;
	/**< Watch waiting queue */
	struct uk_waitq wq;
};
UK_TAILQ_HEAD(xenbus_watch_list, struct xenbus_watch);


/*
 * Xenbus device
 */

struct xenbus_device {
	/**< in use by Xenbus handler */
	UK_TAILQ_ENTRY(struct xenbus_device) next;
	/**< Device state */
	XenbusState state;
	/**< Device type */
	enum xenbus_dev_type devtype;
	/**< Xenstore path of the device */
	char *nodename;
	/**< Xenstore path of the device peer (e.g. backend for frontend) */
	char *otherend;
	/**< Domain id of the other end */
	domid_t otherend_id;
	/**< Watch for monitoring changes on other end */
	struct xenbus_watch *otherend_watch;
	/**< Xenbus driver */
	struct xenbus_driver *drv;
};
UK_TAILQ_HEAD(xenbus_device_list, struct xenbus_device);


/*
 * Xenbus handler
 */

struct xenbus_handler {
	struct uk_bus b;
	struct uk_alloc *a;
	struct xenbus_driver_list drv_list;  /**< List of Xenbus drivers */
	struct xenbus_device_list dev_list;  /**< List of Xenbus devices */
};

/* Helper functions for Xenbus related allocations */
void *uk_xb_malloc(size_t size);
void *uk_xb_calloc(size_t nmemb, size_t size);
void  uk_xb_free(void *ptr);

#endif /* __XENBUS_H__ */
