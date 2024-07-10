/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_CONSOLE_H__
#define __UK_CONSOLE_H__

#include <uk/arch/types.h>
#include <uk/assert.h>

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
__ssz uk_console_out_direct(struct uk_console *dev, const char *buf, __sz len);

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
__ssz uk_console_in_direct(struct uk_console *dev, char *buf, __sz len);

#ifdef __cplusplus
}
#endif

#endif /* __UK_CONSOLE_H__ */
