/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <signal.h>
#include <string.h>

#include <uk/essentials.h>
#include <uk/syscall.h>

void pprocess_signal_arch_set_ucontext(struct ukarch_execenv *execenv,
				       ucontext_t *ucontext)
{
	UK_ASSERT(execenv);
	UK_ASSERT(ucontext);

	memcpy(&ucontext->uc_mcontext.regs, &execenv->regs.x,
	       ARRAY_SIZE(ucontext->uc_mcontext.regs));

	/* TODO Populate the rest of the context */
}

void pprocess_signal_arch_get_ucontext(ucontext_t *ucontext,
				       struct ukarch_execenv *execenv)
{
	UK_ASSERT(ucontext);
	UK_ASSERT(execenv);

	memcpy(&execenv->regs.x, &ucontext->uc_mcontext.regs,
	       ARRAY_SIZE(ucontext->uc_mcontext.regs));

	/* TODO Populate the rest of the context */
}
