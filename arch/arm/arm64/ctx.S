/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Wei Chen <wei.chen@arm.com>
 *          Robert Kuban <robert.kuban@opensynergy.com>
 *
 * Copyright (c) 2018, Arm Ltd. All rights reserved.
 * Copyright (c) 2022, OpenSynergy GmbH. All rights reserved.
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

#include <uk/arch/ctx.h>
#include <uk/arch/lcpu.h>

/*
 * NOTE: If we use the SP for single registers, we need to decrease the
 *       SP 16 bytes for single registers.
 * https://community.arm.com/arm-community-blogs/b/
 *  architectures-and-processors-blog/posts/
 *  using-the-stack-in-aarch64-implementing-push-and-pop
 *
 * WARNING: Changes here need also be reflected in uk/asm/ctx.h
 */

ENTRY(_ctx_arm_clearregs)
	/* wipe argument and return registers */
	mov x0, xzr
	mov x1, xzr
	mov x2, xzr
	mov x3, xzr
	mov x4, xzr
	mov x5, xzr
	mov x6, xzr
	mov x7, xzr

	/* wipe the platform-specific x18 register */
	mov x18, xzr

	/* wipe callee-saved registers */
	mov x19, xzr
	mov x20, xzr
	mov x21, xzr
	mov x22, xzr
	mov x23, xzr
	mov x24, xzr
	mov x25, xzr
	mov x26, xzr
	mov x27, xzr
	mov x28, xzr
#if __OMIT_FRAMEPOINTER__
	mov x29, xzr /* don't wipe if x29 is used as frame pointer */
#endif /* __OMIT_FRAMEPOINTER__ */

	/* jump to entrance function left on stack */
	ldr x30, [sp], #16
	ret

ENTRY(_ctx_arm_call0)
#if !__OMIT_FRAMEPOINTER__
	mov x29, xzr /* reset frame pointer */
#endif /* !__OMIT_FRAMEPOINTER__ */
	/* jump to entrance function left on stack */
	ldr x30, [sp], #16
	ret

ENTRY(_ctx_arm_call1)
#if !__OMIT_FRAMEPOINTER__
	mov x29, xzr /* reset frame pointer */
#endif /* !__OMIT_FRAMEPOINTER__ */
	ldr x0, [sp], #16
	/* jump to entrance function left on stack */
	ldr x30, [sp], #16
	ret

ENTRY(_ctx_arm_call2)
#if !__OMIT_FRAMEPOINTER__
	mov x29, xzr /* reset frame pointer */
#endif /* !__OMIT_FRAMEPOINTER__ */
	ldr x1, [sp], #16
	ldr x0, [sp], #16
	/* jump to entrance function left on stack */
	ldr x30, [sp], #16
	ret

/*
 * Switch context on the current LCPU.
 */
ENTRY(ukarch_ctx_switch)

	/*
	 * Store the x18 register on the stack.
	 *
	 * The role of register x18 is platform specific,
	 * but saving and restoring it later is definitely
	 * safer than leaving random values in it.
	 */
	str x18, [sp, #-16]!

	/* Save callee-saved registers to prevctx's stack */
	sub sp, sp, #__CALLEE_SAVED_SIZE
	stp x19, x20, [sp, #16 * 0]
	stp x21, x22, [sp, #16 * 1]
	stp x23, x24, [sp, #16 * 2]
	stp x25, x26, [sp, #16 * 3]
	stp x27, x28, [sp, #16 * 4]
	stp x29, x30, [sp, #16 * 5]

	/*
	 * Record the restore point for switch out thread to restore
	 * its callee-saved registers in next switch to time.
	 */
	adrp x30, restore_point
	add x30, x30, :lo12:restore_point

	/* Save sp and restore point to previous context */
	mov x2, sp
	stp x30, x2, [x0]

	/* Restore sp and restore point from next context */
	ldp x30, x2, [x1]
	mov sp, x2

	ret

restore_point:
	/* Restore the callee-saved registers */
	ldp x19, x20, [sp, #16 * 0]
	ldp x21, x22, [sp, #16 * 1]
	ldp x23, x24, [sp, #16 * 2]
	ldp x25, x26, [sp, #16 * 3]
	ldp x27, x28, [sp, #16 * 4]
	ldp x29, x30, [sp, #16 * 5]

	add sp, sp, #__CALLEE_SAVED_SIZE

	/* restore x18 */
	ldr x18, [sp], #16

	ret

