/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Wei Chen <wei.chen@arm.com>
 *          Robert Kuban <robert.kuban@opensynergy.com>
 *	    Eduard Vintilă <eduard.vintila47@gmail.com>
 *
 * Copyright (c) 2018, Arm Ltd. All rights reserved.
 * Copyright (c) 2022, OpenSynergy GmbH. All rights reserved.
 * Copyright (c) 2022, University of Bucharest. All rights reserved.
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
#include <uk/asm.h>

/*
 * NOTE: If we use the SP for single registers, we need to decrease the
 *       SP 16 bytes for single registers.
 * WARNING: Changes here need also be reflected in uk/asm/ctx.h
 */

ENTRY(_ctx_riscv_clearregs)
	/* wipe argument and return registers */
	mv x10, x0
	mv x11, x0
	mv x12, x0
	mv x13, x0
	mv x14, x0
	mv x15, x0
	mv x16, x0
	mv x17, x0

	/* wipe temp registers  */
	mv x5, x0
	mv x6, x0
	mv x7, x0
	mv x28, x0
	mv x29, x0
	mv x30, x0
	mv x31, x0
#if __OMIT_FRAMEPOINTER__
	mv x8, x0 /* don't wipe if x8 is used as frame pointer */
#endif /* __OMIT_FRAMEPOINTER__ */

	/* jump to entrance function left on stack */
	ld ra, 0(sp)
	addi sp, sp, 16
	ret

ENTRY(_ctx_riscv_call0)
#if !__OMIT_FRAMEPOINTER__
	mv fp, x0 /* reset frame pointer */
#endif /* !__OMIT_FRAMEPOINTER__ */
	/* jump to entrance function left on stack */
	ld ra, 0(sp)
	addi sp, sp, 16
	ret

ENTRY(_ctx_riscv_call1)
#if !__OMIT_FRAMEPOINTER__
	mv fp, x0 /* reset frame pointer */
#endif /* !__OMIT_FRAMEPOINTER__ */
	ld a0, 0(sp)
	/* jump to entrance function left on stack */
	ld ra, 16(sp)
	addi sp, sp, 32
	ret

ENTRY(_ctx_riscv_call2)
#if !__OMIT_FRAMEPOINTER__
	mv fp, x0 /* reset frame pointer */
#endif /* !__OMIT_FRAMEPOINTER__ */
	ld a0, 0(sp)
	ld a1, 16(sp)
	/* jump to entrance function left on stack */
	ld ra, 32(sp)
	addi sp, sp, 48
	ret

/*
 * Switch context on the current LCPU.
 */
ENTRY(ukarch_ctx_switch)
	/* Save callee-saved registers to prevctx's stack.
	 *
	 * Even though we save only 13 registers, we reserve space for
	 * 16 of them such that we can keep sp's 16-byte alignment.
	 */
	addi sp, sp, -__CALLEE_SAVED_SIZE
	sd ra, 12*REGBYTES(sp)
	sd s11, 11*REGBYTES(sp)
	sd s10, 10*REGBYTES(sp)
	sd s9, 9*REGBYTES(sp)
	sd s8, 8*REGBYTES(sp)
	sd s7, 7*REGBYTES(sp)
	sd s6, 6*REGBYTES(sp)
	sd s5, 5*REGBYTES(sp)
	sd s4, 4*REGBYTES(sp)
	sd s3, 3*REGBYTES(sp)
	sd s2, 2*REGBYTES(sp)
	sd s1, 1*REGBYTES(sp)
	sd s0, 0*REGBYTES(sp)

	/* Save sp and restore point to previous context */
	la t0, restore_point
	sd t0, UKARCH_CTX_OFFSETOF_IP(a0)
	sd sp, UKARCH_CTX_OFFSETOF_SP(a0)

	/* Restore sp and restore point from next context */
	ld ra, UKARCH_CTX_OFFSETOF_IP(a1)
	ld sp, UKARCH_CTX_OFFSETOF_SP(a1)

	ret

restore_point:
	/* Restore the callee-saved registers */
	ld ra, 12*REGBYTES(sp)
	ld s11, 11*REGBYTES(sp)
	ld s10, 10*REGBYTES(sp)
	ld s9, 9*REGBYTES(sp)
	ld s8, 8*REGBYTES(sp)
	ld s7, 7*REGBYTES(sp)
	ld s6, 6*REGBYTES(sp)
	ld s5, 5*REGBYTES(sp)
	ld s4, 4*REGBYTES(sp)
	ld s3, 3*REGBYTES(sp)
	ld s2, 2*REGBYTES(sp)
	ld s1, 1*REGBYTES(sp)
	ld s0, 0*REGBYTES(sp)
	addi sp, sp, __CALLEE_SAVED_SIZE

	ret

