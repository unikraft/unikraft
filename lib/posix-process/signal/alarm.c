/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/essentials.h>
#include <uk/syscall.h>

UK_SYSCALL_R_DEFINE(unsigned int, alarm,
		    unsigned int, seconds)
{
	UK_WARN_STUBBED();

	return 0;
}
