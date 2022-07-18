/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Michalis Pappas <michalis.pappas@opensynergy.com>
 *
 * Copyright (c) 2021, OpenSynergy GmbH. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <arm/smccc.h>
#include <uk/asm.h>

#ifdef CONFIG_ARCH_ARM_64
.macro smccc conduit

#if CONFIG_ARM64_FEAT_PAUTH
	/* Also covers the case when both PAuth and BTI
	 * are enalbled (ARM DDI 0487H.a Sect. D5.4.4)
	 */
	paciasp
#elif CONFIG_ARM64_FEAT_BTI
	bti jc
#endif
	str x19, [sp, #-16]!

	mov x19, x0

	ldp x0, x1,   [x19]
	ldp x2, x3,   [x19, #16]
	ldp x4, x5,   [x19, #32]
	ldp x6, x7,   [x19, #48]
	ldp x8, x9,   [x19, #64]
	ldp x10, x11, [x19, #80]
	ldp x12, x13, [x19, #96]
	ldp x14, x15, [x19, #112]
	ldp x16, x17, [x19, #128]

	\conduit #0

	stp x0, x1,   [x19]
	stp x2, x3,   [x19, #16]
	stp x4, x5,   [x19, #32]
	stp x6, x7,   [x19, #48]
	stp x8, x9,   [x19, #64]
	stp x10, x11, [x19, #80]
	stp x12, x13, [x19, #96]
	stp x14, x15, [x19, #112]
	stp x16, x17, [x19, #128]

	ldr x19, [sp], #16

	ret
.endm
#else
.macro smccc conduit

	push {r4-r8}

	mov r8, r0

	ldm r8, {r0, r1, r2, r3, r4, r5, r6, r7}

	\conduit #0

	stm r8, {r0, r1, r2, r3, r4, r5, r6, r7}

	pop {r4-r8}

	bx lr
.endm
#endif

.section .text
ENTRY(smccc_smc)
	smccc smc
END(smccc_smc)

ENTRY(smccc_hvc)
	smccc hvc
END(smccc_hvc)
