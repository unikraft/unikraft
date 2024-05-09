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

#define usc_arg0		rdi
#define usc_arg1		rsi
#define usc_arg2		rdx
#define usc_arg3		r10
#define usc_arg4		r8
#define usc_arg5		r9
