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

#ifdef __cplusplus
extern "C" {
#endif

struct uk_console;

typedef __ssz (*uk_console_out_func)(struct uk_console *dev, const char *buf,
				     __sz len);
typedef __ssz (*uk_console_in_func)(struct uk_console *dev, char *buf,
				    __sz len);

struct uk_console_ops {
	uk_console_out_func out;
	uk_console_in_func in;
};

#define UK_CONSOLE_FLAG_STDOUT		UK_BIT(0)
#define UK_CONSOLE_FLAG_STDIN		UK_BIT(1)

struct uk_console {
	__u16 id;
	const struct uk_console_ops *ops;
	const char *name; /* Optional */
	int flags;
	struct uk_list_head _list;
};

/**
 * Initialize a `struct uk_console`
 *
 * @param name
 *  Optional name of the device
 * @param ops
 *   Operations of the device
 * @param flags
 *   Requested flags for the device
 */
static inline void uk_console_init(struct uk_console *dev, const char *name,
				   const struct uk_console_ops *ops, int flags)
{
	UK_ASSERT(dev);
	UK_ASSERT(ops);

	*dev = (struct uk_console) {
		.name = name,
		.ops = ops,
		.flags = flags,
		.id = __U16_MAX
	};
	UK_INIT_LIST_HEAD(&dev->_list);
}

/**
 * Register a console device driver with `ukconsole`.
 *
 * The device driver must be initialized and ready to be used. This
 * function will assign a unique ID to the device. The ID can be found
 * in the `id` field of the `struct uk_console` structure after this
 * function has returned. There is no way to unregister a device so
 * assume that `dev` and all pointers inside `dev` must live forever.
 *
 * NOTE: Drivers are only allowed to register a single `struct uk_console`
 * for each underlying device. E.g., it's forbidden to register two distinct
 * structures that lead to reads from or writes to the same device.
 *
 * @param dev
 *   The device driver to register
 */
void uk_console_register(struct uk_console *dev);

#ifdef __cplusplus
}
#endif

#endif /* __UK_CONSOLE_DRIVER_H__ */
