/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2022, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */
#ifndef __UKARCH_CFI_X86_64_H__
#define __UKARCH_CFI_X86_64_H__

#ifdef __ASSEMBLY__

.macro pushq_cfi value
	pushq \value
	.cfi_adjust_cfa_offset 8
.endm

.macro pushq_reg_cfi reg
	pushq %\reg
	.cfi_adjust_cfa_offset 8
	.cfi_rel_offset \reg, 0
.endm

.macro popq_reg_cfi reg
	popq %\reg
	.cfi_adjust_cfa_offset -8
	.cfi_restore \reg
.endm

#else  /* !defined(__ASSEMBLY__) */

#define ukarch_cfi_unwind_end() __asm__ (".cfi_undefined rip\n")

#endif /* !defined(__ASSEMBLY__) */

#endif /* __UKARCH_CFI_X86_64_H__ */
