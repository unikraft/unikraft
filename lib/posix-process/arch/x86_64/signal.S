/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/asm.h>

/* Switch to application stack and call signal handler.
 * Switch back to UK stack before return.
 *
 * void pprocess_signal_call_handler_with_stack(int signum, siginfo_t *si,
 *                                              ucontext_t *ctx, void *handler,
 *                                              void *sp);
 *
 * Notice: Parameters to handler already saved to registers
 *         via this function's first four parameters.
 */
ENTRY(pprocess_signal_call_handler_with_stack)
	push	%rbp		/* save rbp before we corrupt */
	movq    %rsp, %rbp	/* save uk sp */
	movq    %r8, %rsp	/* switch to handler sp */
	call    *%rcx		/* call handler */
	movq    %rbp, %rsp	/* switch back to uk sp */
	pop	%rbp		/* restore rbp */
	ret
