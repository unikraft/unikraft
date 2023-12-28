/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <stddef.h>
#include <sys/types.h>
#include <uk/syscall.h>

#include "signal.h"

UK_SYSCALL_R_DEFINE(int, kill, pid_t, pid, int, signum)
{
	return pprocess_signal_process_do(pid, signum, NULL);
}
