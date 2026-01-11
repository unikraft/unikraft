/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_LCPU_H__
#error "Do not include this header directly"
#endif

#define UK_LCPU_X86_64_SYSCTX_OFFSETOF_GSBASE				\
	UK_PAL_X86_64_SYSCTX_OFFSETOF_GSBASE
#define UK_LCPU_X86_64_SYSCTX_OFFSETOF_FSBASE				\
	UK_PAL_X86_64_SYSCTX_OFFSETOF_FSBASE

/* X86_64 alias to work nicely with the below macro getter/setter */
#define UK_LCPU_X86_64_SYSCTX_OFFSETOF_TLSP				\
	UK_PAL_SYSCTX_OFFSETOF_TLSP

#if !__ASSEMBLY__
#ifdef __cplusplus
extern "C" {
#endif

#define uk_lcpu_sysctx_get(_sc, _offset)				\
	(uk_pal_sysctx_get((struct uk_pal_sysctx *)(_sc),		\
			    UK_LCPU_X86_64_SYSCTX_OFFSETOF_##_offset))

#define uk_lcpu_sysctx_set(_sc, _offset, _val)				\
	(uk_pal_sysctx_set((struct uk_pal_sysctx *)(_sc),		\
			    UK_LCPU_X86_64_SYSCTX_OFFSETOF_##_offset, (_val)))

#ifdef __cplusplus
}
#endif
#endif /* !__ASSEMBLY__ */
