/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_CONSOLE_H__
#define __UK_CONSOLE_H__

#include <uk/arch/types.h>
#include <uk/assert.h>
#include <uk/list.h>
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
	struct uk_console_ops *ops;
	const char *name; /* Optional */
	int flags;
	struct uk_list_head _list;
};

#define UK_CONSOLE(_name, _ops, _flags)		\
	((struct uk_console) {			\
		.id = 0,			\
		.name = (_name),		\
		.flags = _flags,		\
		.ops = (_ops),			\
		._list = {0}			\
	})

/**
 * Retrieve a registered device by its ID.
 *
 * The range of valid IDs is from 0 (inclusive) to
 * uk_console_get_device_count() (exclusive).
 *
 * @return
 *   - (__NULL): No device with ID was found
 *   - (dev): The device with the ID
 */
struct uk_console *uk_console_get(__u16 id);

/**
 * Write bytes to the STDOUT default console(s).
 *
 * Write bytes to all devices that have the UK_CONSOLE_FLAG_STDOUT
 * flag set ("STDOUT devices").
 *
 * @param buf
 *   Source of bytes to write
 * @param len
 *   Number of bytes to write starting at buf
 * @return
 *   - (>=0): Number of bytes written
 *   - (<0): Error
 */
__ssz uk_console_out(const char *buf, __sz len);

/**
 * Read bytes from the STDIN console device(s).
 *
 * Read bytes from all devices that have the UK_CONSOLE_FLAG_STDIN
 * flag set ("STDIN devices"). Input from all of the devices is
 * concatenated and the total length is returned.
 *
 * @param buf
 *   Destination of the bytes that are read
 * @param len
 *   Maximum number of bytes to read
 * @return
 *   - (>=0): Number of bytes read. Might be less than len
 *   - (<0): Error
 */
__ssz uk_console_in(char *buf, __sz len);

 /**
  * Write bytes to the given console device.
  *
  * @param buf
  *   Source of bytes to write
  * @param len
  *   Number of bytes to write starting at buf
  * @return
  *   - (>=0): Number of bytes written
  *   - (<0): Error
  */
static inline __ssz uk_console_out_with(struct uk_console *dev,
					const char *buf, __sz len)
{
	if (unlikely(!len))
		return 0;

	if (unlikely(!buf || !dev || !dev->ops || !dev->ops->out))
		return -EINVAL;

	return dev->ops->out(dev, buf, len);
}

/**
 * Read bytes from the given console device.
 *
 * @param buf
 *   Destination of the bytes that are read
 * @param len
 *   Maximum number of bytes to read
 * @return
 *   - (>=0): Number of bytes read. Might be less than len
 *   - (<0): Error
 */
static inline __ssz uk_console_in_with(struct uk_console *dev,
				       char *buf, __sz len)
{
	if (unlikely(!len))
		return 0;

	if (unlikely(!buf || !dev || !dev->ops || !dev->ops->in))
		return -EINVAL;


	return dev->ops->in(dev, buf, len);
}

/**
 * Get the number of devices registered with `ukconsole`.
 *
 * This number is the upper bound for the ID of a device. It can
 * be used in combination with `uk_console_get` to iterate over
 * all devices.
 *
 * @return Number of devices registered with `ukconsole`
 */
__u16 uk_console_get_device_count(void);

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

#endif /* __UK_CONSOLE_H__ */
