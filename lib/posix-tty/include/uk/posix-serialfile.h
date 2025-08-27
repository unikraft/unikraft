/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_POSIX_SERIALFILE_H__
#define __UK_POSIX_SERIALFILE_H__

#if CONFIG_LIBPOSIX_TTY_SERIAL

#include <uk/file.h>

/* Public reference with static lifetime */

extern const struct uk_file uk_file_serial;

#endif /* CONFIG_LIBPOSIX_TTY_SERIAL */

#endif /* __UK_POSIX_SERIALFILE_H__ */
