/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_POSIX_TIMERFD_H__
#define __UK_POSIX_TIMERFD_H__

#include <sys/timerfd.h>

#include <uk/file.h>

/* File creation */

struct uk_file *uk_timerfile_create(clockid_t id);

/* Internal Syscalls */
#if CONFIG_LIBPOSIX_FDTAB
int uk_sys_timerfd_create(clockid_t id, int flags);
#endif /* CONFIG_LIBPOSIX_FDTAB */

int uk_sys_timerfd_settime(const struct uk_file *f, int flags,
			   const struct itimerspec *new_value,
			   struct itimerspec *old_value);

int uk_sys_timerfd_gettime(const struct uk_file *f,
			   struct itimerspec *curr_value);


#endif /* __UK_POSIX_TIMERFD_H__ */
