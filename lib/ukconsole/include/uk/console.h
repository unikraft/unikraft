/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_CONSOLE_H__
#define __UK_CONSOLE_H__

#include <uk/arch/types.h>
#include <uk/assert.h>
#include <uk/bitops.h>

#ifdef __cplusplus
extern "C" {
#endif

struct uk_console;

/**
 * Retrieve a registered device by its ID.
 *
 * The range of valid IDs is from 0 (inclusive) to
 * uk_console_count() (exclusive).
 *
 * @return
 *   - (__NULL): No device with ID was found
 *   - (dev): The device with the ID
 */
struct uk_console *uk_console_get(__u16 id);

/**
 * Get the number of devices registered with `ukconsole`.
 *
 * This number is the upper bound for the ID of a device. It can
 * be used in combination with `uk_console_get` to iterate over
 * all devices.
 *
 * @return Number of devices registered with `ukconsole`
 */
__u16 uk_console_count(void);

/**
 * Write bytes to the STDOUT default console(s).
 *
 * Write bytes to all devices that have the UK_CONSOLE_FLAG_STDOUT
 * flag set ("STDOUT devices"). Output is best-effort: the
 * function iterates all eligible devices regardless of individual
 * device errors.
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
 * concatenated and the total length is returned. Input is best-effort: the
 * function iterates all eligible devices regardless of individual
 * device errors.
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
 * Write bytes to the STDOUT default console(s) in emergency mode.
 *
 * Write bytes to all devices that have the UK_CONSOLE_FLAG_EMERG_STDOUT
 * flag set ("emergency STDOUT devices"). Output is best-effort: the
 * function iterates all eligible devices regardless of individual
 * device errors.
 *
 * @param buf
 *   Source of bytes to write
 * @param len
 *   Number of bytes to write starting at buf
 * @return
 *   - (>=0): Number of bytes written
 *   - (<0): Error
 */
__isr __ssz uk_console_emerg_out(const char *buf, __sz len);

/**
 * Write bytes to the given console device. If it's not possible to write
 * the entire buffer, the number of bytes that were managed to be written
 * are returned, leaving it to the caller to call this again for the
 * remaining bytes.
 *
 * @param dev
 *   Console device to write to
 * @param buf
 *   Source of bytes to write
 * @param len
 *   Number of bytes to write starting at buf
 * @return
 *   - (>=0): Number of bytes written
 *   - (<0): Error
 */
__ssz uk_console_out_direct(struct uk_console *dev, const char *buf, __sz len);

/**
 * Read bytes from the given console device. If it's not possible to read
 * into the entire buffer, the number of bytes that were managed to be read
 * are returned, leaving it to the caller to call this again for the
 * remaining bytes.
 *
 * @param dev
 *   Console device to read from
 * @param buf
 *   Destination of the bytes that are read
 * @param len
 *   Maximum number of bytes to read
 * @return
 *   - (>=0): Number of bytes read. Might be less than len
 *   - (<0): Error
 */
__ssz uk_console_in_direct(struct uk_console *dev, char *buf, __sz len);

/**
 * Write bytes to the given console device in emergency mode.
 * If it's not possible to write the entire buffer, the number of bytes that
 * were managed to be written are returned, leaving it to the caller to call
 * this again for the remaining bytes.
 *
 * @param dev
 *   Console device to write to
 * @param buf
 *   Source of bytes to write
 * @param len
 *   Number of bytes to write starting at buf
 * @return
 *   - (>=0): Number of bytes written
 *   - (<0): Error
 */
__isr __ssz uk_console_emerg_out_direct(struct uk_console *dev,
					const char *buf, __sz len);

/**
 * Tries to write all bytes to the given console device and blocks until
 * it achieves to do so.
 *
 * @param buf
 *   Source of bytes to write
 * @param len
 *   Number of bytes to write starting at buf
 * @return
 *   - 0 on success
 *   - (<0): Error
 */
int uk_console_out_direct_all(struct uk_console *dev,
			      const char *buf, __sz len);

/**
 * Tries to read all bytes from the given console device and blocks until it
 * achieves to do so.
 *
 * @param dev
 *   Console device to read from
 * @param buf
 *   Destination of the bytes that are read
 * @param len
 *   Number of bytes to read
 * @return
 *   - 0 on success
 *   - (<0): Error
 */
int uk_console_in_direct_all(struct uk_console *dev, char *buf, __sz len);

/**
 * Tries to write all bytes to the given console device in emergency mode and
 * blocks until it achieves to do so.
 *
 * @param dev
 *   Console device to write to
 * @param buf
 *   Source of bytes to write
 * @param len
 *   Number of bytes to write starting at buf
 * @return
 *   - 0 on success
 *   - (<0): Error
 */
__isr int uk_console_emerg_out_direct_all(struct uk_console *dev,
					  const char *buf, __sz len);

typedef void (*uk_console_async_handler_func)(struct uk_console *dev,
					      void *cookie);

/* Event raised on RX IRQ of console device */
#define UK_CONSOLE_ASYNC_EVENT_IN		UK_BIT(0)
/* Event raised on TX IRQ of console device */
#define UK_CONSOLE_ASYNC_EVENT_OUT		UK_BIT(1)

/**
 * Register an interrupt-safe callback with a console device. The console
 * device will raise this callback if the corresponding interrupt is triggered.
 *
 * @param dev
 *   The async-capable console device the callback will be called by
 * @param handler
 *   The callback function pointer that will be called on event
 * @param cookie
 *   The opaque cookie that will be passed to the handler as argument
 * @param event
 *   The event mask to call this callback on (UK_CONSOLE_ASYNC_EVENT_IN/OUT)
 * @return
 *   - 0 on success
 *   - (<0): Error
 */
int uk_console_async_register_callback(struct uk_console *dev,
				       uk_console_async_handler_func handler,
				       void *cookie, __u32 event);

/**
 * Unregister a previously registered async event callback from a console
 * device. The callback is identified by the exact (handler, cookie, event)
 * triple used during registration.
 *
 * @param dev
 *   The async-capable console device the callback was registered with
 * @param handler
 *   The callback function pointer that was registered
 * @param cookie
 *   The opaque cookie that was registered
 * @param event
 *   The event mask that was registered (UK_CONSOLE_ASYNC_EVENT_IN/OUT)
 * @return
 *   - 0 on success
 *   - (<0): Error
 */
int uk_console_async_unregister_callback(struct uk_console *dev,
					 uk_console_async_handler_func handler,
					 void *cookie, __u32 event);
#ifdef __cplusplus
}
#endif

#endif /* __UK_CONSOLE_H__ */
