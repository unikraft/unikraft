/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/* Eventfd support */

#ifndef __UK_POSIX_EVENTFD_H__
#define __UK_POSIX_EVENTFD_H__

#include <uk/file.h>

struct uk_file *uk_eventfile_create(unsigned int count, int flags);

#if CONFIG_LIBPOSIX_FDTAB
int uk_sys_eventfd(unsigned int count, int flags);
#endif /* CONFIG_LIBPOSIX_FDTAB */

#endif /* __UK_POSIX_EVENTFD_H__ */
