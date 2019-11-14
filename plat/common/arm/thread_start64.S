/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Wei Chen <wei.chen@arm.com>
 *
 * Copyright (c) 2018, Arm Ltd. All rights reserved.
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
 *
 * THIS HEADER MAY NOT BE EXTRACTED OR MODIFIED IN ANY WAY.
 */
#include <uk/plat/common/sw_ctx.h>
#include <uk/arch/lcpu.h>
#include <uk/asm.h>

/*
 * Thread stack memory layout:
 *
 * |-----------------------------| --> SP
 * |    thread main function     | In init_sp, we have pushed thread main
 * |-----------------------------| function and arguments to stackfunction
 * |    arguments                | and arguments to stack.
 * |-----------------------------| --> ctx->sp = SP - function - arguments
 * |    Reversed room for Arm64  |
 * |    Callee saved registers:  |
 * |    x19 ~ x28                |
 * |-----------------------------|
 * |    x29 (frame pointer)      |
 * |-----------------------------|
 * |    ...                      |
 * |    ...                      |
 * |-----------------------------|
 * |    thread                   | --> used for uk_thread_current
 * |-----------------------------|
 */

/*
 * This function will jump to thread main function and set up
 * thread exit handler.
 */
ENTRY(asm_thread_starter)
	mov x2, sp
	ldp x0, x1, [x2]		/* Load func to x1, args to x0 */
	ldr x30, =uk_sched_thread_exit	/* Set thread exit handler */
	br  x1				/* Jump to thread main function */
ENDPROC(asm_thread_starter)

/*
 * x0 = ctx->sp
 * x1 = ctx->ip = asm_thread_starter
 *
 * This function will set the SP to thread's SP and jump to
 * ctx->ip: asm_thread_starter
 */
ENTRY(asm_ctx_start)
	mov sp, x0	/* set SP */
	br x1		/* jump to asm_thread_starter */
ENDPROC(asm_ctx_start)

/*
 * x0 = prectx
 * x1 = nextctx
 * This function will switch to the next thread context
 */
ENTRY(asm_sw_ctx_switch)
	/* Save callee-saved registers to prevctx's stack */
	sub x2, sp, #__CALLEE_SAVED_SIZE
	stp x19, x20, [x2, #16 * 0]
	stp x21, x22, [x2, #16 * 1]
	stp x23, x24, [x2, #16 * 2]
	stp x25, x26, [x2, #16 * 3]
	stp x27, x28, [x2, #16 * 4]
	stp x29, x30, [x2, #16 * 5]

	/*
	 * Record the restore point for switch out thread to restore
	 * its called-saved registers in next switch to time.
	 */
	ldr x30, =restore_point

	/* Save sp and restore point to previous context */
	stp x2, x30, [x0]

	/* Restire sp and restore point from next context */
	ldp x2, x30, [x1]
	mov sp, x2

	ret

restore_point:
	/* Restore the callee-saved registers */
	ldp x19, x20, [x2, #16 * 0]
	ldp x21, x22, [x2, #16 * 1]
	ldp x23, x24, [x2, #16 * 2]
	ldp x25, x26, [x2, #16 * 3]
	ldp x27, x28, [x2, #16 * 4]
	ldp x29, x30, [x2, #16 * 5]

	add sp, x2, #__CALLEE_SAVED_SIZE
	ret
ENDPROC(asm_sw_ctx_switch)
