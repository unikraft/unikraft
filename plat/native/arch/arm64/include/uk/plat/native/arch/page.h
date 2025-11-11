/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2021, Karlsruhe Institute of Technology (KIT).
 *                     All rights reserved.
 * Copyright (c) 2022, OpenSynergy GmbH. All rights reserved.
 * Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_PLAT_NATIVE_ARCH_PAGE_H__
#define __UK_PLAT_NATIVE_ARCH_PAGE_H__

#ifdef __cplusplus
extern "C" {
#endif

#if CONFIG_LIBUKPLAT_NATIVE_PAGE

/* We use plain values here so we do not create dependencies on external helper
 * macros, which would forbid us to use the macros in functions defined further
 * down in this header.
 */
#define UK_PLAT_NATIVE_PAGE_LEVEL	0
#define UK_PLAT_NATIVE_PAGE_SHIFT	12
#define UK_PLAT_NATIVE_PAGE_SIZE	0x1000UL
#define UK_PLAT_NATIVE_PAGE_MASK	(~(UK_PLAT_NATIVE_PAGE_SIZE - 1))

#define UK_PLAT_NATIVE_PAGE_LARGE_LEVEL		1
#define UK_PLAT_NATIVE_PAGE_LARGE_SHIFT		21
#define UK_PLAT_NATIVE_PAGE_LARGE_SIZE		0x200000UL
#define UK_PLAT_NATIVE_PAGE_LARGE_MASK		\
	(~(UK_PLAT_NATIVE_PAGE_LARGE_SIZE - 1))

#define UK_PLAT_NATIVE_PAGE_HUGE_LEVEL		2
#define UK_PLAT_NATIVE_PAGE_HUGE_SHIFT		30
#define UK_PLAT_NATIVE_PAGE_HUGE_SIZE		0x40000000UL
#define UK_PLAT_NATIVE_PAGE_HUGE_MASK		\
	(~(UK_PLAT_NATIVE_PAGE_HUGE_SIZE - 1))

#define UK_PLAT_NATIVE_PAGE_Lx_SHIFT(lvl)		    \
	(UK_PLAT_NATIVE_PAGE_SHIFT +			    \
	 (UK_PLAT_NATIVE_PT_LEVEL_SHIFT * (lvl)))

#define UK_PLAT_NATIVE_PAGE_SHIFT_Lx(shift)		    \
	(((shift) - UK_PLAT_NATIVE_PAGE_SHIFT) / UK_PLAT_NATIVE_PT_LEVEL_SHIFT)

#define UK_PLAT_NATIVE_PAGE_ATTR_PROT_NONE		0x00
#define UK_PLAT_NATIVE_PAGE_ATTR_PROT_READ		0x01
#define UK_PLAT_NATIVE_PAGE_ATTR_PROT_WRITE		0x02
#define UK_PLAT_NATIVE_PAGE_ATTR_PROT_EXEC		0x04

#define UK_PLAT_NATIVE_PAGE_ATTR_TYPE_MASK		0x07
#define UK_PLAT_NATIVE_PAGE_ATTR_TYPE_SHIFT		5

#define UK_PLAT_NATIVE_PAGE_ATTR_TYPE_NORMAL_WB		\
	(0 << UK_PLAT_NATIVE_PAGE_ATTR_TYPE_SHIFT)
#define UK_PLAT_NATIVE_PAGE_ATTR_TYPE_NORMAL_WT		\
	(1 << UK_PLAT_NATIVE_PAGE_ATTR_TYPE_SHIFT)
#define UK_PLAT_NATIVE_PAGE_ATTR_TYPE_NORMAL_NC		\
	(2 << UK_PLAT_NATIVE_PAGE_ATTR_TYPE_SHIFT)
#define UK_PLAT_NATIVE_PAGE_ATTR_TYPE_DEVICE_nGnRnE	\
	(3 << UK_PLAT_NATIVE_PAGE_ATTR_TYPE_SHIFT)
#define UK_PLAT_NATIVE_PAGE_ATTR_TYPE_DEVICE_nGnRE	\
	(4 << UK_PLAT_NATIVE_PAGE_ATTR_TYPE_SHIFT)
#define UK_PLAT_NATIVE_PAGE_ATTR_TYPE_DEVICE_GRE	\
	(5 << UK_PLAT_NATIVE_PAGE_ATTR_TYPE_SHIFT)
#define UK_PLAT_NATIVE_PAGE_ATTR_TYPE_NORMAL_WB_TAGGED	\
	(6 << UK_PLAT_NATIVE_PAGE_ATTR_TYPE_SHIFT)

#if !__ASSEMBLY__

#define UK_PLAT_NATIVE_PAGE_Lx_HAS(lvl)			    \
	((lvl) <= __ARM64_PT_MAP_LEVEL_MAX)

#define UK_PLAT_NATIVE_PAGE_Lx_IS(pte, lvl)		    \
	(((lvl) == UK_PLAT_NATIVE_PAGE_LEVEL) ||	    \
	 ((pte) & PTE_TYPE_MASK) == PTE_TYPE_BLOCK)

#endif /* !__ASSEMBLY__ */

#endif /* CONFIG_LIBUKPLAT_NATIVE_PAGE */

#ifdef __cplusplus
}
#endif
#endif /* __UK_PLAT_NATIVE_ARCH_PAGE_H__ */
