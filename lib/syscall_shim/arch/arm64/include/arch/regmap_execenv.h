/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_SYSCALL_H__
#error Do not include this header directly
#endif

/* Taken from regmap_linuxabi.h, but defined differently so as not to
 * needlessly contaminate the namespace of source files including this header
 */

#define execenv_arg0		x[0]
#define execenv_arg1		x[1]
#define execenv_arg2		x[2]
#define execenv_arg3		x[3]
#define execenv_arg4		x[4]
#define execenv_arg5		x[5]
