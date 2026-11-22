/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/essentials.h>
#include <uk/syscall.h>
#if CONFIG_LIBPOSIX_PROCESS_ITIMER
#include <uk/process/itimer.h>
#endif

UK_SYSCALL_R_DEFINE(unsigned int, alarm,
		    unsigned int, seconds)
{
#if CONFIG_LIBPOSIX_PROCESS_ITIMER
	return uk_process_alarm(seconds);
#else
	UK_WARN_STUBBED();
	return 0;
#endif
}
