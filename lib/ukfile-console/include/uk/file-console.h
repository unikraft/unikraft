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

#endif /* __UK_FILE_CONSOLE_H__ */
