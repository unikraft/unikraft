/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_POSIX_TIME_H__
#define __UK_POSIX_TIME_H__

#include <time.h>
#include <sys/time.h>

int uk_sys_nanosleep(const struct timespec *req, struct timespec *rem);

time_t uk_sys_time(time_t *tloc);

int uk_sys_gettimeofday(struct timeval *restrict tv, void *tz);
int uk_sys_settimeofday(struct timeval *tv, void *tz);

int uk_sys_clock_getres(clockid_t clockid, struct timespec *res);

int uk_sys_clock_gettime(clockid_t clockid, struct timespec *tp);
int uk_sys_clock_settime(clockid_t clockid, const struct timespec *tp);

int uk_sys_clock_nanosleep(clockid_t clockid, int flags,
			   const struct timespec *request,
			   struct timespec *remain);

#endif /* __UK_POSIX_TIME_H__ */
