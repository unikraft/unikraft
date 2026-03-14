/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2019, NEC Laboratories Europe GmbH. All rights reserved.
 * Copyright (c) 2022, OpenSynergy GmbH All rights reserved.
 * Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __PLAT_CMN_ARM64_TLS_H__
#define __PLAT_CMN_ARM64_TLS_H__

#include <uk/arch/types.h>
#include <uk/arch/util.h>

#define get_tls_pointer() UK_ARCH_ARM64_SYSREG_READ(tpidr_el0)

/* like UK_ARCH_ARM64_SYSREG_WRITE, but with compiler barrier */
#define set_tls_pointer(ptr) \
	__asm__ __volatile__("msr tpidr_el0, %0" \
			: : "r" ((__u64)(ptr)) : "memory")

#endif /* __PLAT_CMN_ARM64_TLS_H__ */
