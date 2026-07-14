/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_LCPU_REGS_H__
#error "Do not include this header directly"
#endif

#define UK_LCPU_X86_64_REGS_OFFSETOF_R15				\
	UK_PAL_X86_64_REGS_OFFSETOF_R15
#define UK_LCPU_X86_64_REGS_OFFSETOF_R14				\
	UK_PAL_X86_64_REGS_OFFSETOF_R14
#define UK_LCPU_X86_64_REGS_OFFSETOF_R13				\
	UK_PAL_X86_64_REGS_OFFSETOF_R13
#define UK_LCPU_X86_64_REGS_OFFSETOF_R12				\
	UK_PAL_X86_64_REGS_OFFSETOF_R12
#define UK_LCPU_X86_64_REGS_OFFSETOF_RBP				\
	UK_PAL_X86_64_REGS_OFFSETOF_RBP
#define UK_LCPU_X86_64_REGS_OFFSETOF_RBX				\
	UK_PAL_X86_64_REGS_OFFSETOF_RBX
#define UK_LCPU_X86_64_REGS_OFFSETOF_R11				\
	UK_PAL_X86_64_REGS_OFFSETOF_R11
#define UK_LCPU_X86_64_REGS_OFFSETOF_R10				\
	UK_PAL_X86_64_REGS_OFFSETOF_R10
#define UK_LCPU_X86_64_REGS_OFFSETOF_R9					\
	UK_PAL_X86_64_REGS_OFFSETOF_R9
#define UK_LCPU_X86_64_REGS_OFFSETOF_R8					\
	UK_PAL_X86_64_REGS_OFFSETOF_R8
#define UK_LCPU_X86_64_REGS_OFFSETOF_RAX				\
	UK_PAL_X86_64_REGS_OFFSETOF_RAX
#define UK_LCPU_X86_64_REGS_OFFSETOF_RCX				\
	UK_PAL_X86_64_REGS_OFFSETOF_RCX
#define UK_LCPU_X86_64_REGS_OFFSETOF_RDX				\
	UK_PAL_X86_64_REGS_OFFSETOF_RDX
#define UK_LCPU_X86_64_REGS_OFFSETOF_RSI				\
	UK_PAL_X86_64_REGS_OFFSETOF_RSI
#define UK_LCPU_X86_64_REGS_OFFSETOF_RDI				\
	UK_PAL_X86_64_REGS_OFFSETOF_RDI
#define UK_LCPU_X86_64_REGS_OFFSETOF_ORIG_RAX				\
	UK_PAL_X86_64_REGS_OFFSETOF_ORIG_RAX
#define UK_LCPU_X86_64_REGS_OFFSETOF_RIP				\
	UK_PAL_X86_64_REGS_OFFSETOF_RIP
#define UK_LCPU_X86_64_REGS_OFFSETOF_CS					\
	UK_PAL_X86_64_REGS_OFFSETOF_CS
#define UK_LCPU_X86_64_REGS_OFFSETOF_RFLAGS				\
	UK_PAL_X86_64_REGS_OFFSETOF_RFLAGS
#define UK_LCPU_X86_64_REGS_OFFSETOF_RSP				\
	UK_PAL_X86_64_REGS_OFFSETOF_RSP
#define UK_LCPU_X86_64_REGS_OFFSETOF_SS					\
	UK_PAL_X86_64_REGS_OFFSETOF_SS

/* X86_64 alias to work nicely with the below macro getter/setter */
#define UK_LCPU_X86_64_REGS_OFFSETOF_SP					\
	UK_PAL_REGS_OFFSETOF_SP
#define UK_LCPU_X86_64_REGS_OFFSETOF_PC					\
	UK_PAL_REGS_OFFSETOF_PC

#if !__ASSEMBLY__
#ifdef __cplusplus
extern "C" {
#endif

#define uk_lcpu_regs_get(_regs, _offset)				\
	(uk_pal_regs_get((struct uk_pal_regs *)(_regs),			\
			 UK_LCPU_X86_64_REGS_OFFSETOF_##_offset))

#define uk_lcpu_regs_set(_regs, _offset, _val)				\
	(uk_pal_regs_set((struct uk_pal_regs *)(_regs),			\
			  UK_LCPU_X86_64_REGS_OFFSETOF_##_offset, (_val)))

#ifdef __cplusplus
}
#endif
#endif /* !__ASSEMBLY__ */
