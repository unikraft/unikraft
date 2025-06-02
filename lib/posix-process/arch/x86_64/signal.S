/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/asm.h>
#include <uk/arch/lcpu.h>
#include <uk/arch/ctx.h>
#include <uk/plat/common/lcpu.h>

#include "../../signal/signal.h"

/* Switch to application's context and call signal handler.
 *
 * Before executing the handler, align the auxsp and update
 * auxspcb->current_frame so that any kernel entry that happens
 * while the signal handler executes, starts off a clean state.
 *
 * @param handler_ctx    ptr to struct pprocess_signal_handler_ctx (rdi)
 * @param execenv        ptr to application's execenv (rsi)
 */
ENTRY(pprocess_signal_jmp_handler)
	pushq	%rbp
	pushq	%rbx
	pushq	%r12
	pushq	%r13
	movq	%rsp, %rbp /* save pre-aligned rsp */

	movq	%rdi, %r12 /* handler_ctx */
	movq	%rsi, %r13 /* execenv */

	/*───────────────────────────────────────────────────────────────────*
	 * Save uk sysctx                                                    *
	 *───────────────────────────────────────────────────────────────────*/
	subq	$(UKARCH_SYSCTX_SIZE), %rsp /* allocate space for struct */
	andq	$~0xf, %rsp		    /* align rsp */
	movq	%rsp, %rdi		    /* prepare args */
	call	ukarch_sysctx_store	    /* save uk sysctx */

	/*───────────────────────────────────────────────────────────────────*
	 * Align auxspcb->curr_fp.
	 *───────────────────────────────────────────────────────────────────*/
	movq	%gs:LCPU_AUXSP_OFFSET, %r10	/* get auxpcb */
	subq	$(UKARCH_AUXSPCB_SIZE - UKARCH_AUXSPCB_OFFSETOF_CURR_FP), %r10
	pushq	(%r10)				/* save curr_fp */
	pushq	%r10				/* also save its addr */

	/* Save uk rsp *before* we align auxsp for nested calls.
	 * We will be restoring the saved value upon handler return.
	 */
	movq	%rsp, %rbx

	movq	%rsp, %r11		/* now proceed to align auxsp */
	andq	$~(UKARCH_AUXSP_ALIGN - 1), %r11
	movq	%r11, (%r10)		/* write aligned address */

	/*───────────────────────────────────────────────────────────────────*
	 * Switch to the application's sysctx                                *
	 *───────────────────────────────────────────────────────────────────*/
	leaq	UKARCH_EXECENV_OFFSETOF_SYSCTX(%r13), %rdi  /* &ee->sysctx in rdi */
	call	ukarch_sysctx_load			    /* load app sysctx */

	/*───────────────────────────────────────────────────────────────────*
	 * Call signal handler                                               *
	 *───────────────────────────────────────────────────────────────────*/
	movq	PPROCESS_SIGHNDL_CTX_FN_OFFS(%r12), %r10	/* handler_fn */
	movq	PPROCESS_SIGHNDL_CTX_SP_OFFS(%r12), %r11	/* handler_sp */
	movl	PPROCESS_SIGHNDL_CTX_SN_OFFS(%r12), %edi	/* signum */
	movq	PPROCESS_SIGHNDL_CTX_SI_OFFS(%r12), %rsi	/* siginfo_t */
	movq	PPROCESS_SIGHNDL_CTX_UC_OFFS(%r12), %rdx	/* ucontext_t */

	movq	%r11, %rsp	/* switch to handler stack */
	call	*%r10		/* call handler */
	movq	%rbx, %rsp	/* switch back to uk sp */

	/*───────────────────────────────────────────────────────────────────*
	 * Restore auxspcb->curr_fp                                          *
	 *───────────────────────────────────────────────────────────────────*/
	popq	%r10		/* pop &auxpcb->curr_fp */
	popq	(%r10)		/* pop &auxpcb->curr_fp */

	/*───────────────────────────────────────────────────────────────────*
	 * Restore uk sysctx                                                 *
	 *───────────────────────────────────────────────────────────────────*/
	movq	%rsp, %rdi		    /* prepare args */
	call	ukarch_sysctx_load	    /* restore saved sysctx */

	movq	%rbp, %rsp  /* restore pre-aligned rsp */
	popq	%r13
	popq	%r12
	popq	%rbx
	popq	%rbp
	ret
