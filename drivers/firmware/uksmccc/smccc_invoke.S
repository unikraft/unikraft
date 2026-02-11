/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2021, OpenSynergy GmbH. All rights reserved.
 * Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/smccc.h>
#include <uk/arch.h>

.macro uk_smccc conduit
#if CONFIG_ARM64_FEAT_PAUTH
	/* Also covers the case when both PAuth and BTI
	 * are enabled (ARM DDI 0487H.a Sect. D5.4.4)
	 */
	paciasp
#elif CONFIG_ARM64_FEAT_BTI
	bti	jc
#endif /* CONFIG_ARM64_FEAT_BTI */
	str	x19, [sp, #-16]!

	mov	x19, x0

	ldp	x0, x1, [x19]
	ldp	x2, x3, [x19, #16]
	ldp	x4, x5, [x19, #32]
	ldp	x6, x7, [x19, #48]
	ldp	x8, x9, [x19, #64]
	ldp	x10, x11, [x19, #80]
	ldp	x12, x13, [x19, #96]
	ldp	x14, x15, [x19, #112]
	ldp	x16, x17, [x19, #128]

	\conduit	#0

	stp	x0, x1, [x19]
	stp	x2, x3, [x19, #16]
	stp	x4, x5, [x19, #32]
	stp	x6, x7, [x19, #48]
	stp	x8, x9, [x19, #64]
	stp	x10, x11, [x19, #80]
	stp	x12, x13, [x19, #96]
	stp	x14, x15, [x19, #112]
	stp	x16, x17, [x19, #128]

	ldr	x19, [sp], #16

	ret
.endm

.section .text
ENTRY(uk_smccc_smc)
	uk_smccc smc
END(uk_smccc_smc)

ENTRY(uk_smccc_hvc)
	uk_smccc hvc
END(uk_smccc_hvc)
