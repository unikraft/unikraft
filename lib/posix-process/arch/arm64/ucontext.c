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

	ucontext->uc_mcontext.regs[0] = uk_lcpu_regs_get(execenv->regs, X0);
	ucontext->uc_mcontext.regs[1] = uk_lcpu_regs_get(execenv->regs, X1);
	ucontext->uc_mcontext.regs[2] = uk_lcpu_regs_get(execenv->regs, X2);
	ucontext->uc_mcontext.regs[3] = uk_lcpu_regs_get(execenv->regs, X3);
	ucontext->uc_mcontext.regs[4] = uk_lcpu_regs_get(execenv->regs, X4);
	ucontext->uc_mcontext.regs[5] = uk_lcpu_regs_get(execenv->regs, X5);
	ucontext->uc_mcontext.regs[6] = uk_lcpu_regs_get(execenv->regs, X6);
	ucontext->uc_mcontext.regs[7] = uk_lcpu_regs_get(execenv->regs, X7);
	ucontext->uc_mcontext.regs[8] = uk_lcpu_regs_get(execenv->regs, X8);
	ucontext->uc_mcontext.regs[9] = uk_lcpu_regs_get(execenv->regs, X9);
	ucontext->uc_mcontext.regs[10] = uk_lcpu_regs_get(execenv->regs, X10);
	ucontext->uc_mcontext.regs[11] = uk_lcpu_regs_get(execenv->regs, X11);
	ucontext->uc_mcontext.regs[12] = uk_lcpu_regs_get(execenv->regs, X12);
	ucontext->uc_mcontext.regs[13] = uk_lcpu_regs_get(execenv->regs, X13);
	ucontext->uc_mcontext.regs[14] = uk_lcpu_regs_get(execenv->regs, X14);
	ucontext->uc_mcontext.regs[15] = uk_lcpu_regs_get(execenv->regs, X15);
	ucontext->uc_mcontext.regs[16] = uk_lcpu_regs_get(execenv->regs, X16);
	ucontext->uc_mcontext.regs[17] = uk_lcpu_regs_get(execenv->regs, X17);
	ucontext->uc_mcontext.regs[18] = uk_lcpu_regs_get(execenv->regs, X18);
	ucontext->uc_mcontext.regs[19] = uk_lcpu_regs_get(execenv->regs, X19);
	ucontext->uc_mcontext.regs[20] = uk_lcpu_regs_get(execenv->regs, X20);
	ucontext->uc_mcontext.regs[21] = uk_lcpu_regs_get(execenv->regs, X21);
	ucontext->uc_mcontext.regs[22] = uk_lcpu_regs_get(execenv->regs, X22);
	ucontext->uc_mcontext.regs[23] = uk_lcpu_regs_get(execenv->regs, X23);
	ucontext->uc_mcontext.regs[24] = uk_lcpu_regs_get(execenv->regs, X24);
	ucontext->uc_mcontext.regs[25] = uk_lcpu_regs_get(execenv->regs, X25);
	ucontext->uc_mcontext.regs[26] = uk_lcpu_regs_get(execenv->regs, X26);
	ucontext->uc_mcontext.regs[27] = uk_lcpu_regs_get(execenv->regs, X27);
	ucontext->uc_mcontext.regs[28] = uk_lcpu_regs_get(execenv->regs, X28);
	ucontext->uc_mcontext.regs[29] = uk_lcpu_regs_get(execenv->regs, X29);

	/* TODO Populate the rest of the context */
}

void pprocess_signal_arch_get_ucontext(ucontext_t *ucontext,
				       struct ukarch_execenv *execenv)
{
	UK_ASSERT(ucontext);
	UK_ASSERT(execenv);

	uk_lcpu_regs_set(execenv->regs, X0, ucontext->uc_mcontext.regs[0]);
	uk_lcpu_regs_set(execenv->regs, X1, ucontext->uc_mcontext.regs[1]);
	uk_lcpu_regs_set(execenv->regs, X2, ucontext->uc_mcontext.regs[2]);
	uk_lcpu_regs_set(execenv->regs, X3, ucontext->uc_mcontext.regs[3]);
	uk_lcpu_regs_set(execenv->regs, X4, ucontext->uc_mcontext.regs[4]);
	uk_lcpu_regs_set(execenv->regs, X5, ucontext->uc_mcontext.regs[5]);
	uk_lcpu_regs_set(execenv->regs, X6, ucontext->uc_mcontext.regs[6]);
	uk_lcpu_regs_set(execenv->regs, X7, ucontext->uc_mcontext.regs[7]);
	uk_lcpu_regs_set(execenv->regs, X8, ucontext->uc_mcontext.regs[8]);
	uk_lcpu_regs_set(execenv->regs, X9, ucontext->uc_mcontext.regs[9]);
	uk_lcpu_regs_set(execenv->regs, X10, ucontext->uc_mcontext.regs[10]);
	uk_lcpu_regs_set(execenv->regs, X11, ucontext->uc_mcontext.regs[11]);
	uk_lcpu_regs_set(execenv->regs, X12, ucontext->uc_mcontext.regs[12]);
	uk_lcpu_regs_set(execenv->regs, X13, ucontext->uc_mcontext.regs[13]);
	uk_lcpu_regs_set(execenv->regs, X14, ucontext->uc_mcontext.regs[14]);
	uk_lcpu_regs_set(execenv->regs, X15, ucontext->uc_mcontext.regs[15]);
	uk_lcpu_regs_set(execenv->regs, X16, ucontext->uc_mcontext.regs[16]);
	uk_lcpu_regs_set(execenv->regs, X17, ucontext->uc_mcontext.regs[17]);
	uk_lcpu_regs_set(execenv->regs, X18, ucontext->uc_mcontext.regs[18]);
	uk_lcpu_regs_set(execenv->regs, X19, ucontext->uc_mcontext.regs[19]);
	uk_lcpu_regs_set(execenv->regs, X20, ucontext->uc_mcontext.regs[20]);
	uk_lcpu_regs_set(execenv->regs, X21, ucontext->uc_mcontext.regs[21]);
	uk_lcpu_regs_set(execenv->regs, X22, ucontext->uc_mcontext.regs[22]);
	uk_lcpu_regs_set(execenv->regs, X23, ucontext->uc_mcontext.regs[23]);
	uk_lcpu_regs_set(execenv->regs, X24, ucontext->uc_mcontext.regs[24]);
	uk_lcpu_regs_set(execenv->regs, X25, ucontext->uc_mcontext.regs[25]);
	uk_lcpu_regs_set(execenv->regs, X26, ucontext->uc_mcontext.regs[26]);
	uk_lcpu_regs_set(execenv->regs, X27, ucontext->uc_mcontext.regs[27]);
	uk_lcpu_regs_set(execenv->regs, X28, ucontext->uc_mcontext.regs[28]);
	uk_lcpu_regs_set(execenv->regs, X29, ucontext->uc_mcontext.regs[29]);

	/* TODO Populate the rest of the context */
}
