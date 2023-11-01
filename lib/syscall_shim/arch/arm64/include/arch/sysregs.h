/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_SYSCALL_H__
#error Do not include this header directly
#endif

#define UKARCH_SYSREGS_SIZE			16

#if !__ASSEMBLY__

#include <uk/essentials.h>

/* Architecture specific userland context */
struct ukarch_sysregs {
	__uptr tpidr_el0;

	__u8 pad[8];	/* Make sure we are a multiple of 16 bytes */
};

UK_CTASSERT(sizeof(struct ukarch_sysregs) == UKARCH_SYSREGS_SIZE);

#endif /* !__ASSEMBLY__ */
