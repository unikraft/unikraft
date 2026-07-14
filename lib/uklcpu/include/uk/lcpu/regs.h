/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_LCPU_REGS_H__
#define __UK_LCPU_REGS_H__

#include <uk/arch/types.h>
#include <uk/plat/pal/regs.h>
#include <uk/pal/regs.h>
#include <uk/lcpu/arch/regs.h>

#define UK_LCPU_REGS_OFFSETOF_SP					\
	UK_PAL_REGS_OFFSETOF_SP
#define UK_LCPU_REGS_OFFSETOF_PC					\
	UK_PAL_REGS_OFFSETOF_PC

#define UK_LCPU_REGS_SIZE						\
	UK_PAL_REGS_SIZE

/* sanity check */
#if UK_LCPU_REGS_SIZE & 0xf
#error "uk_lcpu_regs structure size should be multiple of 16."
#endif

#if !__ASSEMBLY__
#ifdef __cplusplus
extern "C" {
#endif

struct uk_lcpu_regs;

#ifdef __cplusplus
}
#endif
#endif /* !__ASSEMBLY__ */
#endif /* __UK_LCPU_REGS_H__ */
