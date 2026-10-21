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
 * Write standard-error bytes to the STDOUT default console(s).
 *
 * There is no STDERR console flag: write bytes to all devices with
 * UK_CONSOLE_FLAG_STDOUT set, through a device's `err`
 * operation when it has one and through `out` otherwise, so a device
 * that can tell standard error from standard output keeps them apart.
 *
 * @param buf
 *   Source of bytes to write
 * @param len
 *   Number of bytes to write starting at buf
 * @return
 *   - (>=0): Number of bytes written
 *   - (<0): Error
 */
__ssz uk_console_err(const char *buf, __sz len);

/**
 * uk_console_err(), but only to the STDOUT devices without a `log`
 * operation, or whose `log` declined the last message: a kernel
 * message's text, for the devices that did not take the message itself
 * from uk_console_log().
 *
 * @param buf
 *   Source of bytes to write
 * @param len
 *   Number of bytes to write starting at buf
 * @return
 *   - (>=0): Number of bytes written
 *   - (<0): Error
 */
__ssz uk_console_err_unlogged(const char *buf, __sz len);

struct uk_console_log_msg;

/**
 * Hand a kernel message to the STDOUT devices with a `log` operation,
 * which keep it apart from a program's output (in a host's log, say).
 * The others, and those that decline it, get its text from
 * uk_console_err_unlogged().
 *
 * @param msg
 *   The message
 */
void uk_console_log(const struct uk_console_log_msg *msg);

/**
 * uk_console_out(), writing each newline as CR-LF if
 * CONFIG_LIBUKCONSOLE_ONLCR is set and as written otherwise.
 *
 * @param buf
 *   Source of bytes to write
 * @param len
 *   Number of bytes to write starting at buf
 * @return
 *   - (>=0): Number of bytes written
 *   - (<0): Error
 */
__ssz uk_console_out_onlcr(const char *buf, __sz len);

/**
 * uk_console_err(), writing each newline as CR-LF if
 * CONFIG_LIBUKCONSOLE_ONLCR is set and as written otherwise.
 *
 * @param buf
 *   Source of bytes to write
 * @param len
 *   Number of bytes to write starting at buf
 * @return
 *   - (>=0): Number of bytes written
 *   - (<0): Error
 */
__ssz uk_console_err_onlcr(const char *buf, __sz len);

/**
 * uk_console_err_unlogged(), writing each newline as CR-LF if
 * CONFIG_LIBUKCONSOLE_ONLCR is set and as written otherwise.
 *
 * @param buf
 *   Source of bytes to write
 * @param len
 *   Number of bytes to write starting at buf
 * @return
 *   - (>=0): Number of bytes written
 *   - (<0): Error
 */
__ssz uk_console_err_unlogged_onlcr(const char *buf, __sz len);

/* The console's streams, as uk_console_is_terminal() tells them apart */
#define UK_CONSOLE_STREAM_INOUT		0	/* input and output */
#define UK_CONSOLE_STREAM_ERR		1	/* standard error */

/**
 * Whether a stream of the console reaches a terminal: unless a device
 * that serves it says it does not.
 *
 * @param stream
 *   UK_CONSOLE_STREAM_INOUT or UK_CONSOLE_STREAM_ERR
 * @return
 *   __true or __false
 */
__bool uk_console_is_terminal(int stream);

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
 * Write standard-error bytes to the given console device: through its
 * `err` operation, or through `out` when it has none.
 *
 * @param buf
 *   Source of bytes to write
 * @param len
 *   Number of bytes to write starting at buf
 * @return
 *   - (>=0): Number of bytes written
 *   - (<0): Error
 */
__ssz uk_console_err_direct(struct uk_console *dev, const char *buf, __sz len);

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
