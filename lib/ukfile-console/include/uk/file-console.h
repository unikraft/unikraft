/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_FILE_CONSOLE_H__
#define __UK_FILE_CONSOLE_H__

#include <uk/console.h>
#include <uk/file.h>

/**
 * Create console device-backed file based on a console device driver
 * registration.
 *
 * @param cons
 *   The console device driver instance to back the console file.
 * @return
 *   The generic file representation of the console file or NULL on error.
 */
const struct uk_file *uk_file_console_create(struct uk_console *cons);

/**
 * Create a file for the standard error stream of the console behind
 * console file @f: writes to it go through the console's err operation
 * (see uk_console_err_direct()), and it shares @f's line settings, as
 * it is the same terminal.  It holds a reference to @f.
 *
 * @param f
 *   A console file, from uk_file_console_create() or a devfs console node
 * @return
 *   The new file, or NULL if @f is not a console file or on error.
 */
const struct uk_file *uk_file_console_create_err(const struct uk_file *f);

#endif /* __UK_FILE_CONSOLE_H__ */
