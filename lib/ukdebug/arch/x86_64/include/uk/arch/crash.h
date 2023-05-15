/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_DEBUG_ARCH_CRASH_H__
#define __UK_DEBUG_ARCH_CRASH_H__

#include <uk/essentials.h>

#ifdef __cplusplus
extern "C" {
#endif

extern __bool _uk_crash_explicit;

#define uk_crash_trigger()						\
	do {								\
		__asm__ __volatile__ (					\
			"movb $1, %[indicator]\n"			\
			"ud2\n"						\
			: [indicator] "=m" (_uk_crash_explicit)		\
		);							\
		__builtin_unreachable();				\
	} while (0)

#ifdef __cplusplus
}
#endif

#endif /* __UK_DEBUG_ARCH_CRASH_H__ */
