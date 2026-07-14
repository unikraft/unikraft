/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_PLAT_PAL_REGS_H__
#define __UK_PLAT_PAL_REGS_H__

#include <uk/arch/types.h>
#include <uk/plat/native/arch/regs.h>
#include <uk/plat/pal/arch/regs.h>

#define UK_PAL_REGS_OFFSETOF_SP						\
	UK_PLAT_NATIVE_REGS_OFFSETOF_SP
#define UK_PAL_REGS_OFFSETOF_PC						\
	UK_PLAT_NATIVE_REGS_OFFSETOF_PC
#define UK_PAL_REGS_SIZE						\
	UK_PLAT_NATIVE_REGS_SIZE

#if !__ASSEMBLY__
#ifdef __cplusplus
extern "C" {
#endif

struct uk_pal_regs;

__isr static inline
__u64 uk_pal_regs_get(const struct uk_pal_regs *regs, __sz offset)
{
	return uk_plat_native_regs_get((const struct uk_plat_native_regs *)regs,
				       offset);
}

__isr static inline
void uk_pal_regs_set(struct uk_pal_regs *regs, __sz offset, __u64 val)
{
	uk_plat_native_regs_set((struct uk_plat_native_regs *)regs,
				offset, val);
}

#ifdef __cplusplus
}
#endif
#endif /* !__ASSEMBLY__ */
#endif /* __UK_PLAT_PAL_REGS_H__ */
