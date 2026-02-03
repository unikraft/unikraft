/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2019, Arm Ltd. All rights reserved.
 * Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef UK_PLAT_COMMON_ARM64_TIME_H
#define UK_PLAT_COMMON_ARM64_TIME_H

#include <uk/arch/util.h>
#include <uk/lcpu.h>

#define	get_el0(x)	UK_ARCH_ARM64_SYSREG_READ64(x ##_el0)
#define	get_el1(x)	UK_ARCH_ARM64_SYSREG_READ64(x ##_el1)
#define	set_el0(x, val)	UK_ARCH_ARM64_SYSREG_WRITE64(x ##_el0, val)
#define	set_el1(x, val)	UK_ARCH_ARM64_SYSREG_WRITE64(x ##_el1, val)

#endif /* UK_PLAT_COMMON_ARM64_TIME_H */
