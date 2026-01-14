/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/asm.h>
#include <uk/lcpu.h>
#include <uk/arch/ctx.h>

#include "../../signal/signal.h"

/* Switch to application's context and call signal handler.
 *
 * Before executing the handler, align the auxsp and update
 * auxspcb->current_frame so that any kernel entry that happens
 * while the signal handler executes, starts off a clean state.
 *
 * @param handler_ctx    ptr to struct pprocess_signal_handler_ctx (x0)
 * @param execenv        ptr to application's execenv (x1)
 */
ENTRY(pprocess_signal_jmp_handler)
	stp	x29, x30, [sp, #-16]!

	stp	x19, x20, [sp, #-16]!
	stp	x21, x22, [sp, #-16]!
	stp	x23, x24, [sp, #-16]!
	mov	x29, sp /* save pre-aligned sp */

	mov	x19, x0 /* handler_ctx */
	mov	x20, x1 /* execenv */

	/*───────────────────────────────────────────────────────────────────*
	 * Save uk sysctx                                                    *
	 *───────────────────────────────────────────────────────────────────*/
	sub sp, sp, #UK_LCPU_SYSCTX_SIZE  /* reserve space for ukarch sysctx */
	and sp, sp, #~0xf                         /* mare sure we're aligned */
	mov x0, sp                                /* prepare args */
	bl	uk_lcpu_sysctx_store		/* save uk sysctx */

	/*───────────────────────────────────────────────────────────────────*
	 * Align auxspcb->curr_fp.
	 *───────────────────────────────────────────────────────────────────*/
	mrs	x10, tpidr_el1			/* get auxspcb */
	add	x10, x10, #(LCPU_AUXSP_OFFSET - (UKARCH_AUXSPCB_SIZE - UKARCH_AUXSPCB_OFFSETOF_CURR_FP))
	ldr	x11, [x10]			/* save curr_fp and its addr */
	stp	x11, x10, [sp, #-16]!

	/* Save uk sp *before* we align auxsp for nested calls.
	 * We will be restoring the saved value upon handler return.
	 */
	mov	x21, sp

	/* Align auxsp */
	and	x11, sp, #~(UKARCH_AUXSP_ALIGN - 1)
	str	x11, [x10]

	/*───────────────────────────────────────────────────────────────────*
	 * Switch to the application's sysctx                                *
	 *───────────────────────────────────────────────────────────────────*/
	add	x0, x20, #UKARCH_EXECENV_OFFSETOF_SYSCTX
	bl	uk_lcpu_sysctx_load			/* load app sysctx */

	/*───────────────────────────────────────────────────────────────────*
	 * Call signal handler                                               *
	 *───────────────────────────────────────────────────────────────────*/
	ldr	x22, [x19, #PPROCESS_SIGHNDL_CTX_FN_OFFS]	/* handler_fn */
	ldr	x23, [x19, #PPROCESS_SIGHNDL_CTX_SP_OFFS]	/* handler_sp */
	ldr	w0, [x19, #PPROCESS_SIGHNDL_CTX_SN_OFFS]	/* signum */
	ldr	x1, [x19, #PPROCESS_SIGHNDL_CTX_SI_OFFS]	/* siginfo_t */
	ldr	x2, [x19, #PPROCESS_SIGHNDL_CTX_UC_OFFS]	/* ucontext_t */

	mov	sp, x23			/* switch to handler stack */
	blr	x22			/* call handler */
	mov	sp, x21			/* switch back to uk sp */

	/*───────────────────────────────────────────────────────────────────*
	 * Restore auxspcb->curr_fp                                          *
	 *───────────────────────────────────────────────────────────────────*/
	ldp	x11, x10, [sp], #16	/* pop auxpcb->curr_fp and &auxpcb->curr_fp */
	str	x11, [x10]		/* restore old value */

	/*───────────────────────────────────────────────────────────────────*
	 * Restore uk sysctx                                                 *
	 *───────────────────────────────────────────────────────────────────*/
	mov	x0, sp				/* prepare args */
	bl	uk_lcpu_sysctx_load		/* restore saved sysctx */

	mov	sp, x29				/* restore pre-aligned sp */
	ldp	x23, x24, [sp], #16
	ldp	x21, x22, [sp], #16
	ldp	x19, x20, [sp], #16
	ldp	x29, x30, [sp], #16
	ret
