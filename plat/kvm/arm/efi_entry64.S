/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */
#include <uk/config.h>
#include <uk/asm.h>
#include <uk/arch/arm64.h>

.section .text
ENTRY(uk_efi_entry64)
#if __ARM_FEATURE_ATOMICS
	/* Built w/ LSE atomics: stop here if the CPU lacks FEAT_LSE */
	mrs	x9, ID_AA64ISAR0_EL1
	lsr	x9, x9, #UK_ARCH_ARM64_ID_AA64ISAR0_EL1_ATOMIC_SHIFT
	and	x9, x9, #UK_ARCH_ARM64_ID_AA64ISAR0_EL1_ATOMIC_MASK
	cmp	x9, #2	/* 0b0010: FEAT_LSE */
	b.lo	lse_unsupported
#endif /* __ARM_FEATURE_ATOMICS */

	stp	x0, x1, [sp, #16]

	mov	x0, xzr
	mov	x1, xzr
	bl	do_uk_reloc
	ldp	x0, x1, [sp, #16]

	bl	uk_efi_main

uk_efi_fail:
	wfi
	b	uk_efi_fail
END(uk_efi_entry64)

#if __ARM_FEATURE_ATOMICS
lse_unsupported:
	wfi
	b	lse_unsupported
#endif /* __ARM_FEATURE_ATOMICS */
