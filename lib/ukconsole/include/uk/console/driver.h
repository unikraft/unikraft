/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_CONSOLE_DRIVER_H__
#define __UK_CONSOLE_DRIVER_H__

#include <uk/arch/types.h>
#include <uk/list.h>
#include <uk/assert.h>
#include <uk/bitops.h>
#include <uk/spinlock.h>

#ifdef __cplusplus
extern "C" {
#endif

enum uk_console_devclass {
	_UK_CONSOLE_CLASS_MIN,
	/**
	 * Console registration that fits no device class, e.g. pseudo-devices
	 */
	UK_CONSOLE_CLASS_NONE,
	/* UART-based console devices */
	UK_CONSOLE_CLASS_UART,
	/* Hypervisor-defined console devices */
	UK_CONSOLE_CLASS_HVC,
	/* Framebuffer/video/screen-based console devices */
	UK_CONSOLE_CLASS_FB,
	_UK_CONSOLE_CLASS_MAX,
};

struct uk_console;

typedef __ssz (*uk_console_out_func)(struct uk_console *dev, const char *buf,
				     __sz len);
typedef __ssz (*uk_console_in_func)(struct uk_console *dev, char *buf,
				    __sz len);
typedef __ssz (*uk_console_emerg_out_func)(struct uk_console *dev,
					   const char *buf, __sz len);

struct uk_console_ops {
	uk_console_out_func out;
	uk_console_in_func in;
	uk_console_emerg_out_func emerg_out;
};

#define UK_CONSOLE_FLAG_STDOUT		UK_BIT(0)
#define UK_CONSOLE_FLAG_STDIN		UK_BIT(1)
#define UK_CONSOLE_FLAG_EMERG_STDOUT	UK_BIT(2)
#define UK_CONSOLE_FLAG_ASYNC_TX	UK_BIT(3)
#define UK_CONSOLE_FLAG_ASYNC_RX	UK_BIT(4)

struct uk_console {
	__u16 id;
	const struct uk_console_ops *ops;
	const char *name; /* Optional */
	int flags;
	enum uk_console_devclass dclass;
	struct uk_list_head _list;
};

struct uk_console_async {
	struct uk_console cons;
	struct uk_list_head _cb_list;
	struct uk_spinlock _cb_list_lock;
};

/**
 * Initialize a `struct uk_console`
 *
 * @param name
 *   Optional name of the device
 * @param ops
 *   Operations of the device
 * @param flags
 *   Requested flags for the device
 * @param dclass
 *   Console device class
 */
static inline void uk_console_init(struct uk_console *dev, const char *name,
				   const struct uk_console_ops *ops, int flags,
				   enum uk_console_devclass dclass)
{
	UK_ASSERT(dev);
	UK_ASSERT(ops);
	UK_ASSERT(dclass > _UK_CONSOLE_CLASS_MIN &&
		  dclass < _UK_CONSOLE_CLASS_MAX);

	*dev = (struct uk_console) {
		.name = name,
		.ops = ops,
		.flags = flags,
		.id = __U16_MAX,
		.dclass = dclass,
	};
	UK_INIT_LIST_HEAD(&dev->_list);
}

/**
 * Initialize a `struct uk_console_async`
 *
 * @param name
 *   Optional name of the device
 * @param ops
 *   Operations of the device
 * @param flags
 *   Requested flags for the device
 * @param dclass
 *   Console device class
 */
static inline void uk_console_async_init(struct uk_console_async *dev,
					 const char *name,
					 const struct uk_console_ops *ops,
					 int flags,
					 enum uk_console_devclass dclass)
{
	UK_ASSERT(dev);
	UK_ASSERT((flags & UK_CONSOLE_FLAG_ASYNC_TX) ||
		  (flags & UK_CONSOLE_FLAG_ASYNC_RX));

	uk_console_init(&dev->cons, name, ops, flags, dclass);
	UK_INIT_LIST_HEAD(&dev->_cb_list);
	uk_spin_init(&dev->_cb_list_lock);
}

/**
 * Register a console device driver with `ukconsole`.
 *
 * The device driver must be initialized and ready to be used. This
 * function will assign a unique ID to the device. The ID can be found
 * in the `id` field of the `struct uk_console` structure after this
 * function has returned.
 *
 * NOTE: Drivers are only allowed to register a single `struct uk_console`
 * for each underlying device. E.g., it's forbidden to register two distinct
 * structures that lead to reads from or writes to the same device.
 *
 * @param dev
 *   The device driver to register
 */
void uk_console_register(struct uk_console *dev);

/**
 * Unregister a console device driver with `ukconsole`.
 * After this operation, the console framework is no longer aware of the
 * unregistered device driver.
 *
 * @param dev
 *   The device driver to unregister
 */
void uk_console_unregister(struct uk_console *dev);

/**
 * Call registered interrupt-safe callbacks during RX interrupt.
 *
 * @param dev
 *   Console device to handle the event for
 */
void uk_console_async_in_handle(struct uk_console_async *dev);

/**
 * Call registered interrupt-safe callbacks during TX interrupt.
 *
 * @param dev
 *   Console device to handle the event for
 */
void uk_console_async_out_handle(struct uk_console_async *dev);

#ifdef __cplusplus
}
#endif

#endif /* __UK_CONSOLE_DRIVER_H__ */
