/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UKARCH_CRASH_H__
#error Do not include this header directly
#endif

extern char ukplat_explicit_crash;

#define ukarch_trigger_crash()						\
        do {								\
		__asm__ __volatile__ (					\
			"movb $1, %[indicator]\n"			\
			"ud2\n"						\
			: [indicator] "=m" (ukplat_explicit_crash)	\
		);							\
		__builtin_unreachable();				\
	} while(0)
