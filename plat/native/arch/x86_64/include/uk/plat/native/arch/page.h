/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2021, Karlsruhe Institute of Technology (KIT).
 *                     All rights reserved.
 * Copyright (c) 2021, University Politehnica of Bucharest.
 *                     All rights reserved.
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

#define UK_PLAT_NATIVE_PAGE_LARGE_LEVEL	1
#define UK_PLAT_NATIVE_PAGE_LARGE_SHIFT	21
#define UK_PLAT_NATIVE_PAGE_LARGE_SIZE	0x200000UL
#define UK_PLAT_NATIVE_PAGE_LARGE_MASK	(~(UK_PLAT_NATIVE_PAGE_LARGE_SIZE - 1))

#define UK_PLAT_NATIVE_PAGE_HUGE_LEVEL	2
#define UK_PLAT_NATIVE_PAGE_HUGE_SHIFT	30
#define UK_PLAT_NATIVE_PAGE_HUGE_SIZE	0x40000000UL
#define UK_PLAT_NATIVE_PAGE_HUGE_MASK	(~(UK_PLAT_NATIVE_PAGE_HUGE_SIZE - 1))

#define UK_PLAT_NATIVE_PAGE_Lx_SHIFT(lvl)	__X86_64_PT_Lx_SHIFT(lvl)
#define UK_PLAT_NATIVE_PAGE_SHIFT_Lx(shift)	__X86_64_PT_SHIFT_Lx(shift)

/* Page attributes */
#define __X86_64_PAT_ENTRY(i, val)                ((unsigned long)(val) << ((i) * 8UL))

/* Default PAT value (see SDM Vol 3, 11.12.4 Programming the PAT) */
#define __X86_64_PAT_DEFAULT						\
	(__X86_64_PAT_ENTRY(0, UK_ARCH_PAT_WB) |			\
	 __X86_64_PAT_ENTRY(1, UK_ARCH_PAT_WT) |			\
	 __X86_64_PAT_ENTRY(2, UK_ARCH_PAT_UCM) |			\
	 __X86_64_PAT_ENTRY(3, UK_ARCH_PAT_UC) |			\
	 __X86_64_PAT_ENTRY(4, UK_ARCH_PAT_WB) |			\
	 __X86_64_PAT_ENTRY(5, UK_ARCH_PAT_WT) |			\
	 __X86_64_PAT_ENTRY(6, UK_ARCH_PAT_UCM) |			\
	 __X86_64_PAT_ENTRY(7, UK_ARCH_PAT_UC))

#define UK_PLAT_NATIVE_PAGE_ATTR_PROT_NONE	0x00 /* Page is not accessible */
#define UK_PLAT_NATIVE_PAGE_ATTR_PROT_READ	0x01 /* Page is readable */
#define UK_PLAT_NATIVE_PAGE_ATTR_PROT_WRITE	0x02 /* Page is writeable */
#define UK_PLAT_NATIVE_PAGE_ATTR_PROT_EXEC	0x04 /* Page is executable */

#if !__ASSEMBLY__

#define UK_PLAT_NATIVE_PAGE_Lx_HAS(lvl)			\
	((lvl) <= UK_PLAT_NATIVE_PAGE_HUGE_LEVEL)

/* For lvl > UK_PLAT_NATIVE_PAGE_HUGE_LEVEL the
 * UK_ARCH_PTE_PSE bit must always be 0 (resv.)
 */
#define UK_PLAT_NATIVE_PAGE_Lx_IS(pte, lvl)		\
	(((lvl) == UK_PLAT_NATIVE_PAGE_LEVEL) || ((pte) & UK_ARCH_PTE_PSE))

#endif /* !__ASSEMBLY__ */

#endif /* CONFIG_LIBUKPLAT_NATIVE_PAGE */

#ifdef __cplusplus
}
#endif
#endif /* __UK_PLAT_NATIVE_ARCH_PAGE_H__ */
