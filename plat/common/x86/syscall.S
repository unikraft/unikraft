/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Simon Kuenzer <simon.kuenzer@neclab.eu>
 *          Sergiu Moga <sergiu@unikraft.io>
 *
 * Copyright (c) 2019, NEC Laboratories Europe GmbH, NEC Corporation.
 *                     All rights reserved.
 * Copyright (c) 2024, Unikraft GmbH. All rights reserved.
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

#include <kvm-x86/traps.h>
#include <uk/arch/lcpu.h>
#include <uk/asm.h>
#include <uk/asm/cfi.h>
#include <uk/plat/common/lcpu.h>
#include <uk/arch/ctx.h>

ENTRY(_ukplat_syscall)
	.cfi_startproc simple
	.cfi_def_cfa rsp, 0
	.cfi_register rip, rcx
	cli

	/* Switch to Unikraft's gs_base, which contains pointer to the current
	 * LCPU's `struct lcpu`.
	 */
	swapgs

	/* We can now use the scratch register %r11 (SYSv ABI) to temporarily
	 * store the current stack pointer and switch to the auxiliary stack
	 * of the current thread, which is also stored in `struct lcpu`'s
	 * `auxsp` field.
	 * We thus achieve a complete switch to another stack while preserving
	 * the context of the application.
	 */
	/* Temporarily store current stack pointer in scratch register */
	movq	%rsp, %r11

	/* Switch to the auxiliary stack so that we do not contaminate the
	 * application's stack, as this could either be too small and result
	 * in corrupted memory or we could unwantedly modify variables stored
	 * in the Red Zone.
	 */
	movq	%gs:LCPU_AUXSP_OFFSET, %rsp

	/* Describing the rsp relative to GS would make it necessary to emit
	 * raw CFI. Instead of doing so, mark rsp as undefined temporarily
	 */
	.cfi_undefined rsp

	subq	$(UKARCH_AUXSPCB_SIZE), %rsp
	movq	UKARCH_AUXSPCB_OFFSETOF_CURR_FP(%rsp), %rsp

	/* We subtract 8 bytes less here so that we have room for the pushed
	 * auxsp.
	 * Afterwards, the current stack pointer is pointing to the current
	 * frame within the auxiliary stack. Subtract UKARCH_EXECENV_END_ALIGN
	 * to make room for the 8-byte auxsp pointer, since the layout is
	 * struct uk_syscall_ctx {
	 *	struct ukarch_execenv execenv;
	 *	__uptr auxsp;  <-- make room for these
	 *     ..... space left unused because of alignment subtraction ....
	 * };
	 * We cannot just simply push or subtract 8 bytes because we break
	 * the alignment required by EXECENV, so we must subtract more.
	 * This leads to some wasted bytes but it's fine because they are not
	 * permanent and it is mandatory that we maintain alignment. This
	 * is an optimization so that we do not have to fetch `auxsp` in the
	 * syscall C entry as well (which usually involves reading some
	 * system register). The final stack layout for entering the syscall
	 * C handler should look like the following:
	 *
	 *               lcpu->auxsp (AUXSP aligned)
	 *                  +-------------+ ^
	 *                  |             | | EXECENV_END_ALIGN
	 *                ^ |<----------->| |             ^
	 *         8 bytes| |pushed auxsp | |             |
	 *                v |-------------| v             |
	 *                ^ |  struct     | ^             |
	 *                | |  __regs     | |__REGS_SIZEOF|
	 *                | |-------------| v             |
	 *                | |  struct     | ^             |uk_syscall_ctx
	 *      struct    | |ukarch_sysctx| |SYSCTX_SIZE  |
	 *  ukarch_execenv| |-------------| v             |
	 *                | |             | ^             |
	 *                | |  struct     | |             |
	 *                | | ukarch_ectx | |ECTX_SIZE    |
	 *                | |             | |             |
	 *                | |             | |             |
	 *                | |             | |             |
	 *                v +-------------+ v             v
	 *                       stack
	 *                       pointer
	 *
	 * Where ukarch_sysctx/ukarch_ectx/__regs is filled in the following,
	 * after making room for it.
	 */

	/* Create the room */
	/* The subtraction we need to make so that we can push the 8-byte auxsp.
	 * This is an inconvenient operation as it ends up wasting a few bytes,
	 * the subtraction being bigger than 8 bytes to comply with required
	 * alignment. But it is a trade-off worth doing to avoid having to
	 * read the GS_BASE MSR again in the syscall entry, by storing what
	 * we need now, while we are still swapgs'd.
	 *
	 * NOTE: It is 8 byte less than the actual required subtraction. This is
	 * so we can right afterwards push the auxsp's value, thus subtracting
	 * 8 bytes yet again from the stack pointer.
	 */
	subq	$(UKARCH_EXECENV_END_ALIGN - 8), %rsp
	.cfi_adjust_cfa_offset (UKARCH_EXECENV_END_ALIGN - 8)

	/* Push out auxsp for faster access later. */
	pushq	%gs:LCPU_AUXSP_OFFSET
	.cfi_adjust_cfa_offset 8

	/**
	 * We are done getting what we needed from KERNEL_GS_BASE, swap back.
	 * We do this immediately so that we avoid confusions like:
	 * - I am in an exception handler, was I in a syscall or application
	 * code when it happened? What is the value of KERNEL_GS_BASE/GS_BASE
	 * - I am returning from clone(), should I do swapgs? Was this an
	 * internal clone() call?
	 * ...
	 *
	 * By doing this we will always know:
	 * - Were we interrupted/trapped from Unikraft/syscall code? Then
	 * GS_BASE == lcpu
	 * - Were we interrupted/trapped from application code? Then we don't
	 * know GS_BASE so we need to get lcpu from KERNEL_GS_BASE.
	 */
	swapgs

	/* NOTE: We should normally align the stack before doing this
	 * subtraction because we must ensure that the `ectx` field
	 * is aligned to the corresponding ECTX alignment.
	 * However, this is guaranteed to already be the case for the
	 * auxiliary stack because it is allocated with this exact alignment
	 * in mind.
	 */
	subq	$(UKARCH_EXECENV_SIZE - __REGS_SIZEOF), %rsp
	.cfi_adjust_cfa_offset (UKARCH_EXECENV_SIZE - __REGS_SIZEOF)

	pushq_cfi	$(GDT_DESC_OFFSET(GDT_DESC_DATA))

	pushq_reg_cfi r11
	.cfi_rel_offset rsp, 0

	/*
	 * Push arguments in the order of 'struct __regs' to the stack.
	 * We are going to handover a reference to this stack area as
	 * `struct __regs *` argument to the system call handler.
	 */
	/* We now have %ss and %rsp on the frame, finish classic trap frame */
	/* Push EFLAGS register. Additionally, since we pushed it with IRQs
	 * disabled, it won't have the corresponding bit flag set, making it
	 * look like the caller of the syscall had IRQs off, which no sane
	 * application would do, therefore manually set the flag.
	 */
	pushfq			/* eflags */
	.cfi_adjust_cfa_offset 8
	orq	$X86_EFLAGS_IF, 0(%rsp)

	pushq_cfi	$(GDT_DESC_OFFSET(GDT_DESC_CODE))	/* cs */
	pushq_reg_cfi rcx	/* rcx contains the next rip on syscall exit */

	pushq_reg_cfi rax	/* orig_rax */
	pushq_reg_cfi rdi
	pushq_reg_cfi rsi
	pushq_reg_cfi rdx
	pushq_reg_cfi rcx
	.cfi_rel_offset rip, 0
	pushq_reg_cfi rax
	pushq_reg_cfi r8
	pushq_reg_cfi r9
	pushq_reg_cfi r10
	pushq_reg_cfi r11
	pushq_reg_cfi rbx
	pushq_reg_cfi rbp
	pushq_reg_cfi r12
	pushq_reg_cfi r13
	pushq_reg_cfi r14
	pushq_reg_cfi r15

	/* padding */
	subq  $(__REGS_PAD_SIZE), %rsp
	.cfi_adjust_cfa_offset __REGS_PAD_SIZE
	sti

	/*
	 * Handle call
	 * NOTE: Handler function is going to modify saved registers state
	 * NOTE: Stack pointer as "struct uk_syscall_ctx *" argument
	 *       (calling convention: 1st arg on %rdi)
	 */
	movq %rsp, %rdi

	/**
	 * Store execenv's stored ECTX which resides at offset:
	 * sizeof(struct __regs) + sizeof(struct ukarch_sysctx) from beginning
	 * of execenv.
	 */
	addq	$(__REGS_SIZEOF + UKARCH_SYSCTX_SIZE), %rdi
	call	ukarch_ectx_store

	/**
	 * After function calls, %rsp preserved value of execenv pointer so
	 * restore that into %rdi.
	 */
	movq	%rsp, %rdi

	/**
	 * Store execenv's system context which resides at offset:
	 * sizeof(struct __regs) from beginning of execenv.
	 */
	addq	$(__REGS_SIZEOF), %rdi
	call	ukarch_sysctx_store
	movq	%rsp, %rdi

	/*
	 * Make sure the stack is aligned to 16-bytes. We store the original
	 * stack pointer in the frame pointer (callee saved)
	 */
	movq %rsp, %rbp
	and $~15, %rsp
	.cfi_def_cfa_register rbp

	call ukplat_syscall_handler

	/* Restore original stack pointer */
	movq %rbp, %rsp
	.cfi_def_cfa_register rsp

	cli

	/**
	 * Assign pointer to execution environment to load (first argument).
	 * We do this because it will be easy to keep track of it as, unlike
	 * %rdi, we do not have to store/restore %rsp across function calls.
	 */
	movq	%rsp, %rdi

	/**
	 * Load execenv's stored ECTX which resides at offset:
	 * sizeof(struct __regs) + sizeof(struct ukarch_sysctx) from beginning
	 * of execenv.
	 */
	addq	$(__REGS_SIZEOF + UKARCH_SYSCTX_SIZE), %rdi
	call	ukarch_ectx_load

	/**
	 * As stated previously, after function calls, %rsp preserved value of
	 * execenv pointer so restore that into %rdi.
	 */
	movq	%rsp, %rdi

	/**
	 * Load execenv's stored system context which resides at offset:
	 * sizeof(struct __regs) from beginning of execenv.
	 */
	addq	$(__REGS_SIZEOF), %rdi
	call	ukarch_sysctx_load

	/* Load the updated state back to registers */
	addq $(__REGS_PAD_SIZE), %rsp
	.cfi_adjust_cfa_offset -__REGS_PAD_SIZE
	popq_reg_cfi r15
	popq_reg_cfi r14
	popq_reg_cfi r13
	popq_reg_cfi r12
	popq_reg_cfi rbp
	popq_reg_cfi rbx
	popq_reg_cfi r11
	popq_reg_cfi r10
	popq_reg_cfi r9
	popq_reg_cfi r8
	popq_reg_cfi rax
	popq_reg_cfi rcx
	.cfi_register rip, rcx
	popq_reg_cfi rdx
	popq_reg_cfi rsi
	popq_reg_cfi rdi

	movq	32(%rsp), %rsp
	.cfi_restore rsp
	.cfi_def_cfa rsp, 0

	sti

	/*
	 * Return from system call, inspired by HermiTux [1]
	 * NOTE: We can't use sysret because it changes protection mode [1]
	 *
	 * [1] Pierre et al., 2019, A binary-compatible Unikernel,
	 *     Proceedings of the 15th ACM SIGPLAN/SIGOPS International
	 *     Conference on Virtual Execution Environments (VEE 2019))
	 */
	jmp *%rcx
	.cfi_endproc
