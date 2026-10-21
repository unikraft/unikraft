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
/* Whether @stream (UK_CONSOLE_STREAM_*) of the device reaches a terminal */
typedef __bool (*uk_console_terminal_func)(struct uk_console *dev, int stream);

/* A kernel message, whole, as uk_console_log() hands it to a device */
struct uk_console_log_msg {
	int level;		/* UK_PRINT_KLVL_* */
	const char *libname;	/* the library that printed it, or NULL */
	const char *srcname;	/* its source file, or NULL */
	unsigned int srcline;
	const char *text;	/* the message, without its newline */
	__sz len;
};

/* 0 if the device took the message (or chose to drop it); otherwise it
 * gets its text, as a device without `log` does
 */
typedef int (*uk_console_log_func)(struct uk_console *dev,
				   const struct uk_console_log_msg *msg);

struct uk_console_ops {
	uk_console_out_func out;
	uk_console_in_func in;
	/* Optional: standard error.  A device without it prints standard
	 * error through `out`, like standard output.
	 */
	uk_console_out_func err;
	/* Optional: kernel messages, a whole one at a time, for a device
	 * that keeps a log of its own (a host's).  A device without it, or
	 * that declines one, gets them as text, through `err`.
	 */
	uk_console_log_func log;
	/* Optional: whether a stream reaches a terminal.  A device without
	 * it is a terminal, as a serial line is; one whose other end may be
	 * a pipe or a file says.
	 */
	uk_console_terminal_func terminal;
};

#define UK_CONSOLE_FLAG_STDOUT		UK_BIT(0)
#define UK_CONSOLE_FLAG_STDIN		UK_BIT(1)

struct uk_console {
	__u16 id;
	const struct uk_console_ops *ops;
	const char *name; /* Optional */
	int flags;
	struct uk_list_head _list;
	/* Its `log` declined the last kernel message: it gets the text */
	__bool _log_declined;
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
