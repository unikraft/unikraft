/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <stddef.h>
#include <uk/syscall.h>

#include "signal.h"

UK_LLSYSCALL_R_DEFINE(int, tkill, int, tid, int, signum)
{
	return pprocess_signal_thread_do(tid, signum, NULL);
}
