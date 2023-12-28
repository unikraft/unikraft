/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <signal.h>

#include <uk/essentials.h>
#include <uk/syscall.h>

void pprocess_signal_arch_set_ucontext(struct uk_syscall_ctx *usc,
				       ucontext_t *ucontext)
{
	memcpy(&ucontext->uc_mcontext.regs, &usc->regs.x,
	       ARRAY_SIZE(ucontext->uc_mcontext.regs));

	/* TODO Populate the rest of the context */
}
